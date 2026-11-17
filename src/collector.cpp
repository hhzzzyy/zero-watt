#include "collector.hpp"
#include <sys/mman.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <cmath>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <stdexcept>
#include <csignal>
#include <ctime>
#include <limits>
#include <new>
namespace power {
  namespace {
    constexpr int capacity=86400;
    struct Shared { uint32_t magic; int stop; int64_t heartbeat; Settings settings; double volts,amps,watts,battery,temp; int64_t eta; char status[32]; Record records[capacity]; };
    constexpr uint32_t magic=0x504d0003;
    volatile sig_atomic_t collecting=1;
    void stop_signal(int) { collecting=0; }
    struct Lock { int fd; explicit Lock(int f):fd(f) { if(flock(fd,LOCK_EX)) throw std::runtime_error("Cannot lock collector storage"); } ~Lock(){flock(fd,LOCK_UN);} };
    double raw(Value v) {return v.value_or(std::numeric_limits<double>::quiet_NaN());}
    Value value(double v) {return std::isfinite(v)?Value(v):Value{};}
  }
  int64_t monotonic_second(){return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();}
  Collector::Collector() {
    const char *runtime=getenv("XDG_RUNTIME_DIR");
    directory_=runtime?std::string(runtime)+"/power-monitor":"/tmp/power-monitor-"+std::to_string(getuid());
    mkdir(directory_.c_str(),0700);
    struct stat st{};
    if(lstat(directory_.c_str(),&st) || !S_ISDIR(st.st_mode) || st.st_uid!=getuid() || (st.st_mode&0077)) throw std::runtime_error("Unsafe collector runtime directory");
    const char *home=getenv("HOME"),*config=getenv("XDG_CONFIG_HOME");
    config_=config?config:(home?std::string(home)+"/.config":directory_);
    config_+="/power-monitor/settings";
    fd_=open((directory_+"/state").c_str(),O_RDWR|O_CREAT|O_NOFOLLOW,0600);
    if(fd_<0) throw std::runtime_error("Cannot open collector storage");
    Lock lock(fd_);
    if(ftruncate(fd_,sizeof(Shared))) throw std::runtime_error("Cannot size collector storage");
    data_=mmap(nullptr,sizeof(Shared),PROT_READ|PROT_WRITE,MAP_SHARED,fd_,0);
    if(data_==MAP_FAILED) {data_=nullptr;throw std::runtime_error("Cannot map collector storage");}
    auto &s=*static_cast<Shared*>(data_);
    if(s.magic!=magic) {
      new(data_) Shared{}; s.magic=magic; s.settings={false,30};
      std::ifstream in(config_); int bg=0,minutes=30;
      if(in>>bg>>minutes && minutes>=1 && minutes<=1440) s.settings={bg!=0,minutes};
    }
  }
  Collector::~Collector(){if(data_) munmap(data_,sizeof(Shared));if(fd_>=0) close(fd_);}
  Settings Collector::settings() const {Lock lock(fd_);return static_cast<Shared*>(data_)->settings;}
  void Collector::configure(Settings settings) {
    if(settings.minutes<1 || settings.minutes>1440) throw std::runtime_error("History must be 1-1440 minutes");
    std::filesystem::create_directories(std::filesystem::path(config_).parent_path());
    Lock lock(fd_);
    std::ofstream out(config_+".tmp"); out<<settings.background<<' '<<settings.minutes<<'\n';out.close();
    if(!out || rename((config_+".tmp").c_str(),config_.c_str())) throw std::runtime_error("Cannot save settings");
    auto &s=*static_cast<Shared*>(data_);s.settings=settings;
    const auto cutoff=monotonic_second()-settings.minutes*60;
    for(auto &r:s.records) if(r.tick<=cutoff) r={};
  }
  bool Collector::active(){
    int fd=open((directory_+"/owner").c_str(),O_RDWR|O_CREAT|O_NOFOLLOW,0600);
    if(fd<0) return false;
    bool active=flock(fd,LOCK_EX|LOCK_NB)!=0;close(fd);return active;
  }
  void Collector::request_stop(){Lock lock(fd_);static_cast<Shared*>(data_)->stop=1;}
  Snapshot Collector::snapshot(){
    Snapshot out;out.now=monotonic_second();out.alive=active();Lock lock(fd_);auto &s=*static_cast<Shared*>(data_);
    if(out.alive && out.now-s.heartbeat<=3){out.sample.volts=value(s.volts);out.sample.amps=value(s.amps);out.sample.watts=value(s.watts);out.sample.capacity=value(s.battery);out.sample.temperature=value(s.temp);if(s.eta>=0)out.sample.eta_seconds=s.eta;out.sample.status=s.status;}
    for(int64_t t=out.now-s.settings.minutes*60+1;t<=out.now;++t){auto r=s.records[(t%capacity+capacity)%capacity];if(r.tick==t && t>0)out.records.push_back(r);}
    return out;
  }
  int Collector::run(const std::string &root){
    int owner=open((directory_+"/owner").c_str(),O_RDWR|O_CREAT|O_NOFOLLOW,0600);
    if(owner<0) return 1;
    if(flock(owner,LOCK_EX|LOCK_NB)){close(owner);return 0;}
    collecting=1;signal(SIGTERM,stop_signal);signal(SIGINT,stop_signal);
    {Lock lock(fd_);auto &s=*static_cast<Shared*>(data_);s.stop=0;s.heartbeat=0;for(auto &r:s.records)r={};}
    BatterySource source(root);int64_t previous=-1;
    while(collecting){
      auto now=monotonic_second();
      {Lock lock(fd_);if(static_cast<Shared*>(data_)->stop)break;}
      if(now!=previous){
	auto sample=source.read();Lock lock(fd_);auto &s=*static_cast<Shared*>(data_);
	s.volts=raw(sample.volts);s.amps=raw(sample.amps);s.watts=raw(sample.watts);s.battery=raw(sample.capacity);s.temp=raw(sample.temperature);s.eta=sample.eta_seconds.value_or(-1);
	snprintf(s.status,sizeof(s.status),"%s",sample.status.c_str());s.heartbeat=now;
	s.records[now%capacity]={now,int64_t(time(nullptr)),raw(sample.watts)};
	// Remove expired slots, including gaps after a suspended system.
	for(auto &r:s.records)if(r.tick<=now-s.settings.minutes*60)r={};
	previous=now;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    close(owner);return 0;
  }
}
