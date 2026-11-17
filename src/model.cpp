#include "model.hpp"
#include <cmath>
#include <fstream>
#include <sstream>
#include <utility>

namespace power {
  static std::optional<int64_t> integer(const std::string &path) {
    std::ifstream file(path);
    int64_t value;
    if(!(file >> value)) return {};
    file >> std::ws;
    if(!file.eof()) return {};
    return value;
  }
  Sample BatterySource::read() const {
    Sample s;
    const auto v=integer(root_+"/voltage_now"), i=integer(root_+"/current_now");
    const auto c=integer(root_+"/capacity"), t=integer(root_+"/temp");
    const auto eta=integer(root_+"/time_to_empty_now");
    if(v && *v>0) s.volts=*v/1e6;
    if(i) s.amps=*i/1e6;
    if(s.volts && s.amps) s.watts=*s.volts * *s.amps;
    if(c && *c>=0 && *c<=100) s.capacity=double(*c);
    if(t) s.temperature=*t/10.0;
    std::ifstream f(root_+"/status");
    std::string status;
    std::getline(f,status);
    if(!status.empty() && status.back()=='\r') status.pop_back();
    if(status=="Charging" || status=="Discharging" || status=="Full" ||
       status=="Not charging" || status=="Unknown") s.status=status;
    // Linux time_to_empty is not a charging ETA. Reject unknown/sentinel values.
    if(s.status=="Discharging" && eta && *eta>=0 && *eta!=65535)
      s.eta_seconds=*eta;
    return s;
  }
  void History::add(int64_t second, Value watts) {
    if(second<0) return;
    if(watts && !std::isfinite(*watts)) watts.reset();
    points_[size_t(second)%size]={second,watts};
  }
  std::array<Value, History::size> History::window(int64_t now) const {
    std::array<Value,size> out{};
    for(size_t j=0;j<size;++j) {
      auto second=now-int64_t(size-1-j);
      if(second<0) continue;
      const auto &p=points_[size_t(second)%size];
      if(p.second==second) out[j]=p.watts;
    }
    return out;
  }
  Sample demo_sample(int64_t second) {
    Sample s;
    s.volts=3.722; s.amps=0.38*std::sin(second*0.12);
    s.watts=*s.volts * *s.amps; s.capacity=56; s.temperature=39.7;
    if(*s.amps<0)s.eta_seconds=6540;
    s.status=*s.amps<0?"Discharging":"Charging"; return s;
  }
}
