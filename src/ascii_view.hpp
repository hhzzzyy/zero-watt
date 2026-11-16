#pragma once
#include "model.hpp"
namespace power {
constexpr int columns=40, rows=18;
using Frame=std::array<std::string,rows>;
Frame render(const Sample &,const History &,int64_t now,bool demo=false,bool help=false);
}
