#include "ascii_view.hpp"
#include "lvgl.h"
#include <algorithm>
#include <atomic>
#if USE_DESKTOP
#include <SDL.h>
#endif
#include <chrono>
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
    bool help=false, dirty=true, down=false;
    uint32_t key=0;
    power::EscapeHold escape;
    std::atomic<bool> physical_escape{false};
};
#if USE_DESKTOP
int physical_keys(void *context,SDL_Event *event) {
    auto &state=*static_cast<InputState *>(context);
    if((event->type==SDL_KEYDOWN || event->type==SDL_KEYUP) && event->key.keysym.sym==SDLK_ESCAPE)
        state.physical_escape=event->type==SDL_KEYDOWN;
    if(event->type==SDL_WINDOWEVENT && event->window.event==SDL_WINDOWEVENT_FOCUS_LOST)
        state.physical_escape=false;
    return 1;
}
#endif
void key_event(lv_event_t *event) {
    auto &state=*static_cast<InputState *>(lv_event_get_user_data(event));
    auto *input=static_cast<lv_indev_t *>(lv_event_get_target(event));
    const bool down=lv_indev_get_state(input)==LV_INDEV_STATE_PRESSED;
    const auto key=lv_indev_get_key(input);
#if !USE_DESKTOP
    state.escape.update(down && key==LV_KEY_ESC,ticks());
#endif
    if(down && (!state.down || key!=state.key)) {
        if(key==LV_KEY_ESC) {state.help=false; state.dirty=true;}
        if(key=='h' || key=='H' || key==LV_KEY_ENTER) {state.help=!state.help; state.dirty=true;}
    }
    state.down=down; state.key=key;
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
        bool demo=false,help=false;
        std::string root="/sys/class/power_supply/bq27220-0",capture;
        for(int i=1;i<argc;++i) {
            std::string arg=argv[i];
            if(arg=="--demo") demo=true;
            else if(arg=="--sysfs-root" && i+1<argc) root=argv[++i];
            else if(arg=="--capture" && i+1<argc) capture=argv[++i];
            else if(arg=="--help-screen") help=true;
            else if(arg=="--help") { std::cout<<"power-monitor [--demo] [--sysfs-root DIR] [--capture FILE.ppm] [--help-screen]\n"; return 0; }
            else throw std::runtime_error("Unknown or incomplete option: "+arg);
        }
        std::signal(SIGINT,stop); std::signal(SIGTERM,stop);
        lv_init(); lv_tick_set_cb(ticks);
        lv_display_t *display=nullptr;
        lv_indev_t *keyboard=nullptr;
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
            keyboard=lv_evdev_create(LV_INDEV_TYPE_KEYPAD,key);
#endif
            if(!keyboard) throw std::runtime_error("Cannot open keyboard input device");
        }
        if(!display) throw std::runtime_error("Cannot initialize display");
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
            uint32_t color=r>=10 && r<=13?0x66dc8a:0xc3d4d8;
            if(r==2) color=0xffd166;
            if(r==9 || r==15) color=0x68c4ce;
            lv_obj_set_style_text_color(label,lv_color_hex(color),0);
            if(r==0 || r==17) {
                lv_obj_set_style_bg_color(label,lv_color_hex(0x17545b),0);
                lv_obj_set_style_bg_opa(label,LV_OPA_COVER,0);
                lv_obj_set_style_text_color(label,lv_color_hex(0xffffff),0);
            }
        }
        auto *group=lv_group_create();
        lv_group_add_obj(group,labels[0]);
        InputState input; input.help=help;
        if(keyboard) {
            lv_indev_set_group(keyboard,group);
            lv_indev_add_event_cb(keyboard,key_event,LV_EVENT_KEY,&input);
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
        while(running && lv_is_initialized() && lv_display_get_next(nullptr)) {
            auto now=seconds()+(demo?59:0);
            if(now!=previous) {
                sample=demo?power::demo_sample(now):source.read();
                history.add(now,sample.watts); previous=now; input.dirty=true;
            }
#if USE_DESKTOP
            input.escape.update(input.physical_escape.load(),ticks());
#endif
            if(input.escape.should_exit(ticks())) running=0;
            if(input.dirty) {
                auto frame=power::render(sample,history,now,demo,input.help);
                for(int r=0;r<power::rows;++r) lv_label_set_text(labels[r],frame[r].c_str());
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
