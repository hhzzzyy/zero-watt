#include "ascii_view.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
namespace power {
static std::string num(Value n,int precision,const char *unit) {
    if(!n) return std::string("-- ")+unit;
    std::ostringstream o; o<<std::fixed<<std::setprecision(precision)<<*n<<' '<<unit; return o.str();
}
Frame render(const Sample &s,const History &h,int64_t now,bool demo,bool help) {
    Frame f{};
    f[0]=demo?" POWER MONITOR 0.1            [DEMO]":" POWER MONITOR 0.1            [LIVE]";
    if(help) {
        f[2]=" KEYS / ABOUT";
        f[4]=" Enter       Toggle this help";
        f[5]=" Esc tap     Return to monitor";
        f[6]=" Esc hold    Exit after 1 second";
        f[8]=" Battery-side power = V * abs(I)";
        f[9]=" +I charging / -I discharging";
        f[10]=" ETA shown only while discharging";
        f[12]=" Graph: 60s; peak per 2-second bin";
        f[13]=" ? = missing/partial bin; . = low";
        f[14]=" -- = unavailable; never fake zero";
        f[15]=" No per-process watts in v0.1";
    } else {
        f[2]=" POWER    "+num(s.watts,3,"W");
        f[3]=" CURRENT  "+num(s.amps?Value(*s.amps*1000):Value{},0,"mA");
        f[4]=" VOLTAGE  "+num(s.volts,3,"V");
        std::string bar(20,'-');
        if(s.capacity) std::fill_n(bar.begin(),std::clamp(int(*s.capacity/5),0,20),'#');
        else bar=std::string(20,'?');
        f[5]=" BAT ["+bar+"] "+num(s.capacity,0,"%");
        std::string eta="--";
        if(s.eta_seconds) eta=std::to_string(*s.eta_seconds/3600)+"h"+std::to_string((*s.eta_seconds%3600)/60)+"m";
        f[6]=" TEMP "+num(s.temperature,1,"C")+"   ETA "+eta;
        f[7]=" STATE    "+s.status;
        auto window=h.window(now);
        double top=0.5,sum=0; size_t count=0;
        for(auto v:window) if(v) { top=std::max(top,*v); sum+=*v; ++count; }
        top=std::ceil(top*2)/2;
        f[9]=" 60s POWER [W]       top "+num(top,1,"W");
        for(int r=0;r<4;++r) f[10+r]="    |"+std::string(30,' ')+"|";
        for(int x=0;x<30;++x) {
            auto a=window[2*x],b=window[2*x+1];
            if(!a || !b) {f[13][5+x]='?'; continue;}
            double peak=std::max(*a,*b);
            int level=std::clamp(int(std::ceil(peak/top*4)),0,4);
            if(level==0) f[13][5+x]='.';
            for(int r=0;r<level;++r) f[13-r][5+x]='#';
        }
        f[14]="    +------------------------------+";
        f[15]=" -60s  avg "+num(count?Value(sum/count):Value{},2,"W")+"      now";
    }
    f[17]=" Enter:help Esc:back Hold Esc:exit";
    for(auto &line:f) { if(line.size()>columns) line.resize(columns); line.resize(columns,' '); }
    return f;
}
}
