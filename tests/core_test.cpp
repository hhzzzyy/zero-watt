#include "ascii_view.hpp"
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace power;
void check(bool ok,const char *message) { if(!ok) throw std::runtime_error(message); }
int main() {
    auto dir=std::filesystem::temp_directory_path()/ ("power-test-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directory(dir);
    struct Cleanup { std::filesystem::path p; ~Cleanup(){std::filesystem::remove_all(p);} } cleanup{dir};
    auto put=[&](const char *key,const char *value){std::ofstream(dir/key)<<value<<'\n';};
    BatterySource source(dir.string());
    check(!source.read().watts,"Missing sensor must not read as zero");
    put("voltage_now","3722000"); put("current_now","-384000"); put("capacity","56");
    put("temp","397"); put("time_to_empty_now","6540"); put("status","Discharging");
    auto s=source.read();
    check(s.watts && std::abs(*s.watts+1.429248)<1e-9,"Unit conversion/power");
    check(s.temperature==39.7 && s.capacity==56 && s.eta_seconds==6540,"Other units");
    put("status","Charging"); put("current_now","384000");
    check(!source.read().eta_seconds && source.read().watts==-*s.watts,"Charge direction and ETA");
    put("current_now","0"); check(source.read().watts==0.0,"True zero supported");
    put("current_now","12bad"); check(!source.read().watts,"Trailing garbage rejected");
    put("current_now","999999999999999999999999999999"); check(!source.read().amps,"Overflow rejected");
    put("capacity","101"); check(!source.read().capacity,"Invalid percent");
    put("status","Discharging"); put("time_to_empty_now","-1"); check(!source.read().eta_seconds,"Negative ETA");
    put("time_to_empty_now","65535"); check(!source.read().eta_seconds,"Unknown ETA sentinel");
    History h;
    for(int i=0;i<75;++i) h.add(i,double(i));
    auto w=h.window(74); check(w.front()==15 && w.back()==74,"60-second eviction");
    h.add(77,2.0); w=h.window(77); check(!w[57] && !w[58] && w[59]==2.0,"Time gap not filled");
    h.add(77,3.0); check(h.window(77)[59]==3.0,"Same-second replace");
    h.add(78,{}); check(!h.window(78)[59],"Missing stays missing");
    check(!h.window(200)[59],"Stale values expire");
    h.add(201,-2.0);check(h.window(201).back()==-2.0,"Negative history retained");
    auto range=graph_range(h.window(201));check(range.bottom==-2.0 && range.top==0,"Discharge-only axis");
    h.add(202,1.2);range=graph_range(h.window(202));check(range.bottom==-2.0 && range.top==1.5,"Mixed signed axis");
    range=graph_range(0,1.2);check(range.bottom==0 && range.top==1.5,"Charge-only axis");
    range=graph_range(0,0);check(range.bottom==0 && range.top>0,"Nondegenerate zero axis");
    EscapeHold esc; esc.update(true,100); check(!esc.should_exit(1099) && esc.should_exit(1100),"Long hold without repeat");
    esc.update(false,1101); check(!esc.should_exit(2000),"Release cancels hold");
    esc.update(true,UINT32_MAX-500); check(esc.should_exit(500),"Tick rollover");
    for(bool help:{false,true}) for(bool demo:{false,true}) {
        auto f=render(s,h,78,demo,help);
        for(const auto &line:f) {check(line.size()==40,"Fixed width"); for(unsigned char c:line) check(c>=32 && c<=126,"ASCII only");}
    }
    auto home=render(s,h,78,false,false);
    check(home[2].find("BAT POWER")!=std::string::npos && home[3].find("BAT [")!=std::string::npos,"Battery rows adjacent");
    check(home[3].find("DISCHARGING")!=std::string::npos,"Battery state word");
    check(home[4].find("TEMP")!=std::string::npos && home[5].find_first_not_of(' ')==std::string::npos,"Blank line before graph header");
    check(home[7].compare(1,1,"+")==0 && home[7].compare(38,1,"+")==0,"Graph frame top border");
    check(home[15].compare(1,4,"-60s")==0 && home[15].compare(36,3,"now")==0,"Graph time labels align with frame");
    check(home[6].find("[W]")!=std::string::npos && home[6].find("avg")!=std::string::npos,"Graph stats on top");
    std::cout<<"All core tests passed\n";
}
