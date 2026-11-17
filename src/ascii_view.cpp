#include "ascii_view.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
namespace power {
  static std::string num(Value n,int precision,const char *unit,bool signed_value=false) {
    if(!n) return std::string("-- ")+unit;
    std::ostringstream o; if(signed_value && *n>0)o<<std::showpos; o<<std::fixed<<std::setprecision(precision)<<*n<<' '<<unit; return o.str();
  }
  static std::string compact(Value n,int precision,const char *unit,bool signed_value=false) {
    auto text=num(n,precision,unit,signed_value);
    text.erase(std::remove(text.begin(),text.end(),' '),text.end());
    return text;
  }
  GraphRange graph_range(double minimum,double maximum) {
    GraphRange range{std::floor(std::min(0.0,minimum)*2)/2,std::ceil(std::max(0.0,maximum)*2)/2};
    if(range.bottom==range.top)range.top=0.5;
    return range;
  }
  GraphRange graph_range(const std::array<Value, History::size> &values) {
    double minimum=0,maximum=0;
    for(auto v:values)if(v && std::isfinite(*v)){minimum=std::min(minimum,*v);maximum=std::max(maximum,*v);}
    return graph_range(minimum,maximum);
  }
  Frame render(const Sample &s,const History &h,int64_t now,bool demo,bool help) {
    Frame f{};
    f[0]=demo?" POWER MONITOR 0.1            [DEMO]":" POWER MONITOR 0.1            [LIVE]";
    if(help) {
      f[2]=" KEYS";
      f[4]=" B        Background sampling";
      f[5]=" T        History length";
      f[6]=" G        Full-screen history";
      f[7]=" H        Toggle help";
      f[9]=" Esc      Back";
      f[10]=" Hold Esc Exit or hide";
    } else {
      f[2]=" BAT POWER "+compact(s.watts,3,"W",true)+" ("+compact(s.amps,3,"A",true)+" x "+compact(s.volts,3,"V")+")";
      std::string bar(16,'-');
      if(s.capacity) std::fill_n(bar.begin(),std::clamp(int(std::lround(*s.capacity*bar.size()/100.0)),0,int(bar.size())),'#');
      else bar=std::string(bar.size(),'?');
      std::string state="UNKNOWN";
      if(s.amps) state=*s.amps>0?"CHARGING":*s.amps<0?"DISCHARGING":"IDLE";
      f[3]=" BAT ["+bar+"] "+compact(s.capacity,0,"%")+" "+state;
      std::string eta="--";
      if(s.eta_seconds) eta=std::to_string(*s.eta_seconds/3600)+"h"+std::to_string((*s.eta_seconds%3600)/60)+"m";
      f[4]=" TEMP "+num(s.temperature,1,"C")+"   ETA "+eta;
      f[5]="";
      auto window=h.window(now);
      auto range=graph_range(window);double sum=0; size_t count=0;
      for(auto v:window) if(v) { sum+=*v; ++count; }
      std::string limits="[W] "+num(range.bottom,1,"")+"to "+num(range.top,1,"");
      while(!limits.empty() && limits.back()==' ') limits.pop_back();
      std::string mean="avg "+num(count?Value(sum/count):Value{},2,"W",true);
      f[6]=std::string(columns,' ');
      f[6].replace(graph_box_left,limits.size(),limits);
      f[6].replace(graph_box_right+1-mean.size(),mean.size(),mean);
      std::string border=std::string(graph_box_left,' ')+"+"+std::string(graph_box_inner,'-')+"+";
      f[7]=border;
      for(int r=0;r<6;++r) f[8+r]=std::string(graph_box_left,' ')+"|"+std::string(graph_box_inner,' ')+"|";
      f[14]=border;
      f[15]=std::string(columns,' ');
      f[15].replace(graph_box_left,4,"-60s");
      f[15].replace(graph_box_right+1-3,3,"now");
    }
    f[17]="B(ackground) T(ime) G(raph) H(elp) Esc";
    for(auto &line:f) { if(line.size()>columns) line.resize(columns); line.resize(columns,' '); }
    return f;
  }
}
