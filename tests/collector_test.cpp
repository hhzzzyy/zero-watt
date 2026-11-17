#include "collector.hpp"
#include <sys/wait.h>
#include <unistd.h>
#include <csignal>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <chrono>
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
int main(){
    char temp[]="/tmp/power-collector-test-XXXXXX";auto *path=mkdtemp(temp);if(!path)return 1;
    setenv("XDG_RUNTIME_DIR",path,1);setenv("XDG_CONFIG_HOME",path,1);
    std::string root=std::string(path)+"/battery";std::filesystem::create_directory(root);
    std::ofstream(root+"/voltage_now")<<4000000;std::ofstream(root+"/current_now")<<-250000;
    std::ofstream(root+"/capacity")<<80;std::ofstream(root+"/temp")<<250;std::ofstream(root+"/status")<<"Discharging";
    pid_t child=-1;
    try{
        {power::Collector c;check(!c.settings().background && c.settings().minutes==30,"defaults");c.configure({true,1});}
        child=fork();if(child==0){power::Collector worker;_exit(worker.run(root));}
        check(child>0,"fork");
        {power::Collector c;for(int i=0;i<40 && c.snapshot().records.empty();++i)std::this_thread::sleep_for(std::chrono::milliseconds(100));
         auto s=c.snapshot();check(s.alive && s.sample.watts && *s.sample.watts==-1,"live sample");check(c.settings().background,"saved residency");}
        // No frontend exists while the collector continues sampling.
        std::this_thread::sleep_for(std::chrono::milliseconds(2200));
        {power::Collector c;auto s=c.snapshot();check(s.records.size()>=3,"background history retained");
         pid_t duplicate=fork();if(duplicate==0){power::Collector other;_exit(other.run(root));}int status;waitpid(duplicate,&status,0);check(WIFEXITED(status)&&WEXITSTATUS(status)==0,"singleton");
         check(c.active(),"original survives duplicate");
         c.configure({false,2});check(c.settings().minutes==2,"settings update");
         bool rejected=false;try{c.configure({false,0});}catch(...){rejected=true;}check(rejected,"invalid retention rejected");
         std::filesystem::remove(root+"/voltage_now");std::this_thread::sleep_for(std::chrono::milliseconds(1200));check(!c.snapshot().sample.watts,"missing sample not stale");
         c.request_stop();}
        int status;waitpid(child,&status,0);child=-1;check(WIFEXITED(status)&&WEXITSTATUS(status)==0,"clean stop");
        {power::Collector c;check(!c.active(),"stopped");}
        std::filesystem::remove(std::string(path)+"/power-monitor/state");
        {power::Collector c;check(!c.settings().background && c.settings().minutes==2,"settings persist without runtime");}
        std::filesystem::remove_all(path);return 0;
    }catch(const std::exception &e){std::cerr<<e.what()<<'\n';if(child>0){kill(child,SIGTERM);waitpid(child,nullptr,0);}std::filesystem::remove_all(path);return 1;}
}
