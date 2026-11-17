#include "ascii_view.hpp"
#include "collector.hpp"
#include <memory>
#include <mutex>
#include <deque>
#include <iomanip>
#include <ctime>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include "lvgl.h"
#include <algorithm>
#include <atomic>
#if USE_DESKTOP
#include <SDL.h>
#endif
#include <chrono>
#include <cmath>
#include <csignal>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>
#include <vector>
#if !USE_DESKTOP
#include <fcntl.h>
#include <linux/fb.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif

namespace {
  using Clock=std::chrono::steady_clock;
  const auto epoch=Clock::now();
  volatile std::sig_atomic_t running=1;
  void stop(int) { running=0; }
  uint32_t ticks() { return uint32_t(std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now()-epoch).count()); }
  int64_t seconds() { return std::chrono::duration_cast<std::chrono::seconds>(Clock::now()-epoch).count(); }
  struct InputState {
    bool help=false, dirty=true;
    bool editing=false, history_page=false, follow=true, replace_entry=false;
    power::Settings settings;
    std::string entry,error;
    int64_t view_end=0, span=1800;
    std::mutex mutex; std::deque<int> commands;
    power::EscapeHold escape;
    std::atomic<bool> physical_escape{false};
  };
  enum { ToggleBackground=1001,SetTime,ShowGraph,ShowHelp,Left,Right,Up,Down,Enter,Escape,Backspace };
  void command(InputState &s,int key) { std::lock_guard<std::mutex> lock(s.mutex);s.commands.push_back(key); }
#if USE_DESKTOP
  int physical_keys(void *context,SDL_Event *event) {
    auto &state=*static_cast<InputState *>(context);
    if((event->type==SDL_KEYDOWN || event->type==SDL_KEYUP) && event->key.keysym.sym==SDLK_ESCAPE)
      state.physical_escape=event->type==SDL_KEYDOWN;
    if(event->type==SDL_KEYDOWN && !event->key.repeat) {
      int k=0; auto sym=event->key.keysym.sym;
      switch(sym) {case SDLK_b:k=ToggleBackground;break;case SDLK_t:k=SetTime;break;case SDLK_g:k=ShowGraph;break;
      case SDLK_LEFT:k=Left;break;case SDLK_RIGHT:k=Right;break;case SDLK_UP:k=Up;break;case SDLK_DOWN:k=Down;break;
      case SDLK_RETURN:k=Enter;break;case SDLK_ESCAPE:k=Escape;break;case SDLK_BACKSPACE:k=Backspace;break;
      case SDLK_h:k=ShowHelp;break;default:if(sym>=SDLK_0 && sym<=SDLK_9)k=int(sym);break;}
      if(k)command(state,k);
    }
    if(event->type==SDL_WINDOWEVENT && event->window.event==SDL_WINDOWEVENT_FOCUS_LOST)
      state.physical_escape=false;
    return 1;
  }
#endif
  void process_commands(InputState &s,power::Collector *collector,int64_t now,int64_t oldest) {
    std::deque<int> keys;{std::lock_guard<std::mutex> lock(s.mutex);keys.swap(s.commands);}
    for(int key:keys){
      s.dirty=true;
      try {
	if(s.editing){
	  if(key==Escape){s.editing=false;s.error.clear();}
	  else if(key==Backspace){s.replace_entry=false;if(!s.entry.empty())s.entry.pop_back();s.error.clear();}
	  else if(key>='0' && key<='9'){if(s.replace_entry){s.entry.clear();s.replace_entry=false;}if(s.entry.size()<4)s.entry+=char(key);s.error.clear();}
	  else if(key==Enter){
	    int minutes=s.entry.empty()?0:std::stoi(s.entry);
	    if(minutes<1 || minutes>1440)s.error="Enter 1 to 1440 minutes";
	    else {auto settings=s.settings;settings.minutes=minutes;if(collector)collector->configure(settings);s.settings=settings;s.span=minutes*60;s.editing=false;s.error.clear();}
	  }
	  continue;
	}
	if(key==ToggleBackground){auto settings=s.settings;settings.background=!settings.background;if(collector)collector->configure(settings);s.settings=settings;}
	else if(key==SetTime){s.editing=true;s.replace_entry=true;s.entry=std::to_string(s.settings.minutes);s.error.clear();}
	else if(key==ShowGraph){s.history_page=!s.history_page;s.help=false;s.follow=true;s.span=s.settings.minutes*60;}
	else if(key==ShowHelp){s.help=!s.help;s.history_page=false;}
	else if(key==Escape){s.help=false;s.history_page=false;}
	else if(s.history_page){
	  if(key==Enter)s.follow=true;
	  if(key==Up || key==Down){s.span=std::clamp<int64_t>(key==Up?s.span/2:s.span*2,60,s.settings.minutes*60);s.follow=false;}
	  if(key==Left || key==Right){s.follow=false;s.view_end+= (key==Left?-1:1)*std::max<int64_t>(1,s.span/4);}
	  s.view_end=std::clamp(s.view_end,std::min(now,oldest+s.span-1),now);
	}
      }catch(const std::exception &e){s.error=e.what();}
    }
  }
  struct GraphState {
    std::array<power::Value,power::History::size> values{};
    double bottom=0,top=0.5;
    bool full=false;
    int64_t start=0,end=0;
    std::vector<power::Record> records;
  };
  void draw_graph(lv_event_t *event) {
    const auto &graph=*static_cast<GraphState *>(lv_event_get_user_data(event));
    auto *object=static_cast<lv_obj_t *>(lv_event_get_target(event));
    lv_area_t area; lv_obj_get_coords(object,&area);
    auto *layer=lv_event_get_layer(event);
    lv_draw_line_dsc_t line; lv_draw_line_dsc_init(&line);
    line.color=lv_color_hex(0x66dc8a); line.width=1;
    const double left=area.x1+1,right=area.x2-1,upper=area.y1+1,lower=area.y2-1;
    auto y=[&](double watts){return lower-std::clamp((watts-graph.bottom)/(graph.top-graph.bottom),0.0,1.0)*(lower-upper);};
    auto stroke=[&](double x1,double y1,double x2,double y2,uint32_t color){
      line.color=lv_color_hex(color);line.p1={lv_value_precise_t(x1),lv_value_precise_t(y1)};line.p2={lv_value_precise_t(x2),lv_value_precise_t(y2)};lv_draw_line(layer,&line);
    };
    auto color=[](double watts){return watts>0?0x66dc8au:watts<0?0xff6666u:0x8cabb8u;};
    auto segment=[&](double x1,double w1,double x2,double w2){
      if((w1<0 && w2>0)||(w1>0 && w2<0)){
	double zero_x=x1+(x2-x1)*(-w1)/(w2-w1);
	stroke(x1,y(w1),zero_x,y(0),color(w1));stroke(zero_x,y(0),x2,y(w2),color(w2));
      }else stroke(x1,y(w1),x2,y(w2),color(w1!=0?w1:w2));
    };
    if(graph.full){
      for(int i=0;i<=4;++i){
	double x=left+i*(right-left)/4,gy=upper+i*(lower-upper)/4;
	stroke(x,upper,x,lower,0x263a42);stroke(left,gy,right,gy,0x263a42);
      }
    }
    stroke(left,y(0),right,y(0),0x8cabb8);
    if(graph.full){
      // Aggregate each continuous run per pixel, retaining both signed extrema.
      bool have=false;int column=0;double low=0,high=0,last=0;int64_t previous=0;
      auto flush=[&](){if(have)segment(left+column,low,left+column+(low==high?1:0),high);};
      for(const auto &r:graph.records){
	if(r.tick<graph.start || r.tick>graph.end)continue;
	if(!std::isfinite(r.watts)){flush();have=false;continue;}
	int x=int((r.tick-graph.start)*(right-left)/std::max<int64_t>(1,graph.end-graph.start));
	if(!have || r.tick!=previous+1){flush();have=true;column=x;low=high=last=r.watts;}
	else if(x==column){low=std::min(low,r.watts);high=std::max(high,r.watts);last=r.watts;}
	else {flush();segment(left+column,last,left+x,r.watts);column=x;low=high=last=r.watts;}
	previous=r.tick;
      }
      flush();return;
    }
    auto x=[&](size_t i){return left+i*(right-left)/(graph.values.size()-1);};
    for(size_t i=0;i<graph.values.size();++i){
      if(!graph.values[i])continue;
      if(i+1<graph.values.size() && graph.values[i+1])segment(x(i),*graph.values[i],x(i+1),*graph.values[i+1]);
      else segment(x(i),*graph.values[i],x(i)+1,*graph.values[i]);
    }
  }
  std::vector<unsigned char> pixels(320*170*3);
  void capture_flush(lv_display_t *d,const lv_area_t *a,uint8_t *data) {
    const int stride=lv_draw_buf_width_to_stride(lv_area_get_width(a),LV_COLOR_FORMAT_RGB888);
    for(int y=a->y1;y<=a->y2;++y)
      for(int x=a->x1;x<=a->x2;++x) {
	auto *src=data+(y-a->y1)*stride+(x-a->x1)*3;
	auto *dst=pixels.data()+(y*320+x)*3;
	dst[0]=src[2]; dst[1]=src[1]; dst[2]=src[0];
      }
    lv_display_flush_ready(d);
  }
  // Footer hints are muted gray; only B(ackground) is tinted to show residency.
  std::string background_footer(bool on) {
    return std::string(on?"#66dc8a B(ackground)#":"#8cabb8 B(ackground)#")+" #8cabb8 T(ime) G(raph) H(elp) Esc    ";
  }
#if !USE_DESKTOP
  std::string framebuffer() {
    if(auto *env=std::getenv("LV_LINUX_FBDEV_DEVICE")) if(*env) return env;
    std::ifstream f("/proc/fb"); std::string line;
    while(std::getline(f,line)) {
      std::istringstream entry(line);
      std::string id, name;
      if(entry >> id >> name) {
	const bool supported = name=="fb_st7789v" || name=="panel-mipi-dbid";
	if(supported && !id.empty() && id.find_first_not_of("0123456789")==std::string::npos)
	  return "/dev/fb"+id;
      }
    }
    throw std::runtime_error("Supported Cardputer framebuffer not found; set LV_LINUX_FBDEV_DEVICE explicitly");
  }
#endif
}
int main(int argc,char **argv) {
  try {
    bool demo=false,help=false,collect=false,status_only=false,history_screen=false;
    std::string root="/sys/class/power_supply/bq27220-0",capture;
    for(int i=1;i<argc;++i) {
      std::string arg=argv[i];
      if(arg=="--demo") demo=true;
      else if(arg=="--collect") collect=true;
      else if(arg=="--status") status_only=true;
      else if(arg=="--history-screen") history_screen=true;
      else if(arg=="--sysfs-root" && i+1<argc) root=argv[++i];
      else if(arg=="--capture" && i+1<argc) capture=argv[++i];
      else if(arg=="--help-screen") help=true;
      else if(arg=="--help") { std::cout<<"power-monitor [--demo] [--sysfs-root DIR] [--capture FILE.ppm] [--help-screen] [--history-screen] [--collect]\n"; return 0; }
      else throw std::runtime_error("Unknown or incomplete option: "+arg);
    }
    if(status_only){power::Collector c;auto s=c.snapshot();auto settings=c.settings();
      std::cout<<"active="<<s.alive<<" background="<<settings.background<<" minutes="<<settings.minutes<<" samples="<<s.records.size();
      if(!s.records.empty())std::cout<<" first="<<s.records.front().tick<<" last="<<s.records.back().tick;
      std::cout<<'\n';return 0;}
    if(collect){power::Collector collector;return collector.run(root);}
    std::unique_ptr<power::Collector> collector;
    if(!demo && capture.empty()) {
      collector=std::make_unique<power::Collector>();
      if(!collector->active()) {
#if USE_DESKTOP
	pid_t child=fork();
	if(child<0)throw std::runtime_error("Cannot start collector");
	if(child==0){setsid();pid_t daemon=fork();if(daemon<0)_exit(1);if(daemon>0)_exit(0);
	  int nullfd=open("/dev/null",O_RDWR);for(int i=0;i<3;++i)dup2(nullfd,i);if(nullfd>2)close(nullfd);
	  execl(argv[0],argv[0],"--collect","--sysfs-root",root.c_str(),nullptr);_exit(1);}
	int status=0;waitpid(child,&status,0);
#else
	if(std::system("systemctl --user start power-monitor-collector.service")!=0)throw std::runtime_error("Cannot start collector user service");
#endif
	for(int i=0;i<30 && !collector->active();++i)std::this_thread::sleep_for(std::chrono::milliseconds(100));
	if(!collector->active())throw std::runtime_error("Collector did not start");
      }
    }
    struct SessionCleanup {
      power::Collector *collector;
      ~SessionCleanup(){try{if(collector && !collector->settings().background){collector->request_stop();for(int i=0;i<20 && collector->active();++i)std::this_thread::sleep_for(std::chrono::milliseconds(100));}}catch(...){}}
    } cleanup{collector.get()};
    std::signal(SIGINT,stop); std::signal(SIGTERM,stop);
    lv_init(); lv_tick_set_cb(ticks);
    lv_display_t *display=nullptr;
    lv_indev_t *keyboard=nullptr;
#if !USE_DESKTOP
    int input_fd=-1;
#endif
    std::vector<uint8_t> capture_buffer;
    if(!capture.empty()) {
      display=lv_display_create(320,170);
      lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB888);
      capture_buffer.resize(320*170*3);
      lv_display_set_buffers(display,capture_buffer.data(),nullptr,capture_buffer.size(),LV_DISPLAY_RENDER_MODE_FULL);
      lv_display_set_flush_cb(display,capture_flush);
    } else {
#if USE_DESKTOP
      display=lv_sdl_window_create(320,170);
      lv_sdl_window_set_title(display,"Power Monitor");
      keyboard=lv_sdl_keyboard_create();
#else
      const auto fb=framebuffer();
      int fd=open(fb.c_str(),O_RDWR);
      if(fd<0) throw std::runtime_error("Cannot open framebuffer: "+fb);
      fb_var_screeninfo info{};
      int ok=ioctl(fd,FBIOGET_VSCREENINFO,&info); close(fd);
      if(ok<0 || info.xres!=320 || info.yres!=170)
	throw std::runtime_error("Expected a 320x170 framebuffer: "+fb);
      display=lv_linux_fbdev_create();
      lv_linux_fbdev_set_file(display,fb.c_str());
      const char *key=std::getenv("POWER_MONITOR_INPUT_DEVICE");
      if(!key || !*key) key="/dev/input/by-path/platform-3f804000.i2c-event";
      input_fd=open(key,O_RDONLY|O_NONBLOCK|O_CLOEXEC);
      if(input_fd<0)throw std::runtime_error("Cannot open keyboard input device");
#endif
#if USE_DESKTOP
      if(!keyboard) throw std::runtime_error("Cannot open keyboard input device");
#endif
    }
    if(!display) throw std::runtime_error("Cannot initialize display");
    constexpr int history_box_left=5, history_box_right=38;
    constexpr int history_box_inner=history_box_right-history_box_left-1;
    constexpr int history_plot_x=(history_box_left+1)*power::char_pixel_width;
    constexpr int history_plot_width=history_box_inner*power::char_pixel_width;
    constexpr int history_plot_y=22, history_plot_height=117;
    auto *screen=lv_screen_active();
    lv_obj_remove_style_all(screen);
    lv_obj_set_style_bg_color(screen,lv_color_hex(0x071013),0);
    lv_obj_set_style_bg_opa(screen,LV_OPA_COVER,0);
    lv_obj_remove_flag(screen,LV_OBJ_FLAG_SCROLLABLE);
    std::array<lv_obj_t *,power::rows> labels{};
    for(int r=0;r<power::rows;++r) {
      auto *label=labels[r]=lv_label_create(screen);
      lv_obj_remove_style_all(label);
      lv_obj_set_style_text_font(label,&lv_font_unscii_8,0);
      lv_obj_set_style_text_letter_space(label,0,0);
      lv_obj_set_pos(label,0,4+r*9);
      lv_obj_set_size(label,320,9);
      lv_label_set_long_mode(label,LV_LABEL_LONG_CLIP);
      uint32_t color=r>=8 && r<=13?0x66dc8a:0xc3d4d8;
      if(r==2) color=0xffd166;
      if(r==7 || r==15) color=0x68c4ce;
      lv_obj_set_style_text_color(label,lv_color_hex(color),0);
      if(r==0 || r==17) {
	lv_obj_set_style_bg_color(label,lv_color_hex(0x17545b),0);
	lv_obj_set_style_bg_opa(label,LV_OPA_COVER,0);
	lv_obj_set_style_text_color(label,lv_color_hex(r==17?0x8cabb8:0xffffff),0);
      }
      if(r==17) lv_label_set_recolor(label,true);
    }
    // Battery state word is a separate label so the '#' capacity bar is not
    // mistaken for recolor markup.
    auto *status_text=lv_label_create(screen);
    lv_obj_remove_style_all(status_text);
    lv_obj_set_style_text_font(status_text,&lv_font_unscii_8,0);
    lv_obj_set_style_text_letter_space(status_text,0,0);
    lv_obj_set_pos(status_text,0,4+3*9);
    lv_obj_set_size(status_text,320,9);
    lv_label_set_long_mode(status_text,LV_LABEL_LONG_CLIP);
    lv_obj_add_flag(status_text,LV_OBJ_FLAG_HIDDEN);
    GraphState graph;
    auto *plot=lv_obj_create(screen);
    lv_obj_remove_style_all(plot);
    lv_obj_remove_flag(plot,LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(plot,LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_pos(plot,power::graph_plot_x,76);
    lv_obj_set_size(plot,power::graph_plot_width,54);
    lv_obj_add_event_cb(plot,draw_graph,LV_EVENT_DRAW_MAIN,&graph);
    auto *group=lv_group_create();
    lv_group_add_obj(group,labels[0]);
    InputState input; input.help=help;input.history_page=history_screen;
    if(collector)input.settings=collector->settings();
    input.span=input.settings.minutes*60;
    std::array<lv_obj_t*,10> axes{};
    for(auto &axis:axes){axis=lv_label_create(screen);lv_obj_remove_style_all(axis);lv_obj_set_style_text_font(axis,&lv_font_unscii_8,0);lv_obj_set_style_text_color(axis,lv_color_hex(0x68c4ce),0);lv_obj_add_flag(axis,LV_OBJ_FLAG_HIDDEN);}
    if(keyboard) {
      lv_indev_set_group(keyboard,group);

    }
#if USE_DESKTOP
    // LVGL SDL sends synthetic releases; observe physical Esc for hold timing.
    if(keyboard) SDL_AddEventWatch(physical_keys,&input);
#endif
    power::BatterySource source(root);
    power::History history;
    power::Sample sample;
    if(demo) for(int64_t s=0;s<60;++s) history.add(s,power::demo_sample(s).watts);
    int64_t previous=-1;
    power::Snapshot snapshot;
    if(demo){auto now=seconds()+59;for(int64_t t=now-input.settings.minutes*60+1;t<=now;++t)snapshot.records.push_back({t,int64_t(time(nullptr))+t-now,*power::demo_sample(t).watts});}

    while(running && lv_is_initialized() && lv_display_get_next(nullptr)) {
      auto now=collector?power::monotonic_second():seconds()+(demo?59:0);
#if !USE_DESKTOP
      input_event event{};
      while(input_fd>=0 && read(input_fd,&event,sizeof(event))==sizeof(event)){
	if(event.type!=EV_KEY)continue;
	if(event.code==KEY_ESC)input.escape.update(event.value!=0,ticks());
	if(event.value!=1)continue;
	int k=0;
	switch(event.code){case KEY_B:k=ToggleBackground;break;case KEY_T:k=SetTime;break;case KEY_G:k=ShowGraph;break;
	case KEY_LEFT:k=Left;break;case KEY_RIGHT:k=Right;break;case KEY_UP:k=Up;break;case KEY_DOWN:k=Down;break;
	case KEY_H:k=ShowHelp;break;case KEY_ENTER:k=Enter;break;case KEY_ESC:k=Escape;break;case KEY_BACKSPACE:k=Backspace;break;
	case KEY_0:k='0';break;default:if(event.code>=KEY_1 && event.code<=KEY_9)k='1'+event.code-KEY_1;break;}
	if(k)command(input,k);
      }
#endif
      process_commands(input,collector.get(),now,snapshot.records.empty()?now:snapshot.records.front().tick);
      if(now!=previous) {
	if(collector){snapshot=collector->snapshot();sample=snapshot.sample;for(auto r:snapshot.records)if(r.tick>now-60)history.add(r.tick,std::isfinite(r.watts)?power::Value(r.watts):power::Value{});}
	else {sample=demo?power::demo_sample(now):source.read();if(demo && (snapshot.records.empty() || snapshot.records.back().tick!=now))snapshot.records.push_back({now,int64_t(time(nullptr)),*sample.watts});}
	if(input.follow)input.view_end=now;
	if(!snapshot.records.empty())input.view_end=std::clamp(input.view_end,std::min(now,snapshot.records.front().tick+input.span-1),now);
	if(!collector)history.add(now,sample.watts);
	previous=now; input.dirty=true;
      }
#if USE_DESKTOP
      input.escape.update(input.physical_escape.load(),ticks());
#endif
      if(input.escape.should_exit(ticks())) {running=0;}
      if(input.dirty) {
	auto frame=power::render(sample,history,now,demo,input.help);
	bool full=input.history_page && !input.editing && !input.help;
	for(auto axis:axes)lv_obj_add_flag(axis,LV_OBJ_FLAG_HIDDEN);
	graph.full=full;
	lv_obj_set_pos(plot,full?history_plot_x:power::graph_plot_x,full?history_plot_y:76);lv_obj_set_size(plot,full?history_plot_width:power::graph_plot_width,full?history_plot_height:54);
	if(input.editing){
	  frame={};frame[0]=" HISTORY RETENTION";frame[3]=" Minutes (1-1440):";frame[5]=" > "+input.entry+"_";
	  frame[8]=" Backspace:delete  Enter:save";frame[10]=" Esc:cancel";frame[12]=" Applies to continuous recording";frame[14]=input.error;
	} else if(full){
	  frame={}; graph.records=snapshot.records;graph.end=input.view_end;
	  graph.start=std::max(graph.end-input.span+1,snapshot.records.empty()?graph.end-59:snapshot.records.front().tick);
	  if(graph.start>=graph.end)graph.start=graph.end-59;
	  double minimum=0,maximum=0;
	  for(auto r:graph.records)if(r.tick>=graph.start && r.tick<=graph.end && std::isfinite(r.watts)){minimum=std::min(minimum,r.watts);maximum=std::max(maximum,r.watts);}
	  auto range=power::graph_range(minimum,maximum);graph.bottom=range.bottom;graph.top=range.top;
	  auto wall=[&](int64_t tick)->time_t{for(const auto &r:graph.records)if(r.tick>=tick)return time_t(r.wall-(r.tick-tick));return time(nullptr)+tick-now;};
	  char date[32];auto endwall=wall(graph.end);strftime(date,sizeof(date),"%m-%d %H:%M:%S",localtime(&endwall));
	  frame[0]=std::string(" ")+date+(demo?" DEMO":(input.follow?" LIVE":" VIEW"));
	  frame[1]=std::string(history_box_left,' ')+"+"+std::string(history_box_inner,'-')+"+";
	  for(int r=2;r<=14;++r)frame[r]=std::string(history_box_left,' ')+"|"+std::string(history_box_inner,' ')+"|";
	  frame[15]=frame[1];
	  frame[17]=" <>:pan ^v:zoom Enter:live Esc:back";
	  const int grid_top=history_plot_y+1,grid_bottom=history_plot_y+history_plot_height-2;
	  for(int i=0;i<5;++i){
	    std::ostringstream text;text<<std::fixed<<std::setprecision(2)<<(graph.top-(graph.top-graph.bottom)*i/4);
	    lv_label_set_text(axes[i],text.str().c_str());lv_obj_set_pos(axes[i],0,grid_top+i*(grid_bottom-grid_top)/4-4);lv_obj_remove_flag(axes[i],LV_OBJ_FLAG_HIDDEN);
	  }
	  const int time_width=8*power::char_pixel_width;
	  const int time_x[3]={history_box_left*power::char_pixel_width,history_plot_x+(history_plot_width-time_width)/2,(history_box_right+1)*power::char_pixel_width-time_width};
	  for(int i=0;i<3;++i){char time[16];auto stamp=wall(graph.start+(graph.end-graph.start)*i/2);strftime(time,sizeof(time),"%H:%M:%S",localtime(&stamp));lv_label_set_text(axes[5+i],time);lv_obj_set_pos(axes[5+i],time_x[i],4+16*9);lv_obj_remove_flag(axes[5+i],LV_OBJ_FLAG_HIDDEN);}
	  lv_label_set_text(axes[8],"W");lv_obj_set_pos(axes[8],8,12);lv_obj_remove_flag(axes[8],LV_OBJ_FLAG_HIDDEN);
	} else {
	  graph.values=history.window(now);auto range=power::graph_range(graph.values);graph.bottom=range.bottom;graph.top=range.top;
	  frame[16]="";
	  frame[17]="B(ackground) T(ime) G(raph) H(elp) Esc";
	  if(!input.error.empty())frame[16]=input.error;
	}
	auto graph_label_color=[&](int r)->uint32_t {
	  if(input.help)return 0xc3d4d8;
	  if(input.editing)return r>=8 && r<=13?0x66dc8a:0xc3d4d8;
	  if(full)return r>=1 && r<=15?0xffffff:0xc3d4d8;
	  if(r==2)return 0xffd166;
	  if(r==6 || r==15)return 0x68c4ce;
	  if(r>=7 && r<=14)return 0xffffff;
	  return 0xc3d4d8;
	};
	for(int r=1;r<=15;++r)lv_obj_set_style_text_color(labels[r],lv_color_hex(graph_label_color(r)),0);
	lv_obj_add_flag(status_text,LV_OBJ_FLAG_HIDDEN);
	for(int r=0;r<power::rows;++r){
	  if(r==17 && frame[17].rfind("B(ackground)",0)==0) {
	    lv_label_set_text(labels[r],background_footer(input.settings.background).c_str());
	    continue;
	  }
	  if(r==3 && !input.help && !input.editing && !full && frame[3].rfind(" BAT [",0)==0) {
	    std::string text=frame[3];
	    while(!text.empty() && text.back()==' ')text.pop_back();
	    auto split=text.rfind(' ');
	    if(split!=std::string::npos) {
	      auto state=text.substr(split+1);
	      uint32_t color=state=="CHARGING"?0x66dc8a:state=="DISCHARGING"?0xff6666:0x8cabb8;
	      lv_label_set_text(labels[r],text.substr(0,split+1).c_str());
	      lv_label_set_text(status_text,state.c_str());
	      lv_obj_set_style_text_color(status_text,lv_color_hex(color),0);
	      lv_obj_set_pos(status_text,int((split+1)*power::char_pixel_width),4+3*9);
	      lv_obj_remove_flag(status_text,LV_OBJ_FLAG_HIDDEN);
	      continue;
	    }
	  }
	  if(frame[r].size()>40)frame[r].resize(40);
	  lv_label_set_text(labels[r],frame[r].c_str());
	}
	if(input.help || input.editing)lv_obj_add_flag(plot,LV_OBJ_FLAG_HIDDEN);else lv_obj_remove_flag(plot,LV_OBJ_FLAG_HIDDEN);
	lv_obj_invalidate(plot);
	input.dirty=false;
      }
      auto delay=lv_timer_handler();
      if(!capture.empty()) {
	lv_refr_now(display);
	std::ofstream out(capture,std::ios::binary);
	out<<"P6\n320 170\n255\n";
	out.write(reinterpret_cast<const char *>(pixels.data()),pixels.size());
	if(!out) throw std::runtime_error("Cannot write capture: "+capture);
	break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(std::clamp(delay,1u,20u)));
    }
#if USE_DESKTOP
    if(keyboard) SDL_DelEventWatch(physical_keys,&input);
#endif
#if !USE_DESKTOP
    if(input_fd>=0)close(input_fd);
#endif
    // SDL_QUIT can already have deinitialized LVGL inside its event handler.
    if(lv_is_initialized()) {
      if(keyboard) lv_indev_delete(keyboard);
      lv_group_delete(group);
      if(lv_display_get_next(nullptr)) lv_display_delete(display);
      lv_deinit();
    }
    return 0;
  } catch(const std::exception &e) { std::cerr<<"Power Monitor: "<<e.what()<<'\n'; return 1; }
}
