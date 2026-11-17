#pragma once
#include "model.hpp"
#include <vector>
#include <string>
namespace power {
  struct Record { int64_t tick=0, wall=0; double watts=0; };
  struct Settings { bool background=false; int minutes=30; };
  struct Snapshot { Sample sample; std::vector<Record> records; int64_t now=0; bool alive=false; };
  class Collector {
  public:
    Collector(); ~Collector();
    Collector(const Collector&)=delete;
    Settings settings() const;
    void configure(Settings);
    Snapshot snapshot();
    void request_stop();
    bool active();
    int run(const std::string &root);
  private:
    int fd_=-1; void *data_=nullptr; std::string directory_,config_;
  };
  int64_t monotonic_second();
}
