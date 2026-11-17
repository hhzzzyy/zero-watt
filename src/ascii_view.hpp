#pragma once
#include "model.hpp"
namespace power {
  constexpr int columns=40, rows=18, char_pixel_width=8;
  constexpr int graph_box_left=1, graph_box_right=38;
  constexpr int graph_box_inner=graph_box_right-graph_box_left-1;
  constexpr int graph_plot_x=(graph_box_left+1)*char_pixel_width;
  constexpr int graph_plot_width=graph_box_inner*char_pixel_width;
  using Frame=std::array<std::string,rows>;
  struct GraphRange { double bottom=0, top=0.5; };
  GraphRange graph_range(double minimum,double maximum);
  GraphRange graph_range(const std::array<Value, History::size> &values);
  Frame render(const Sample &,const History &,int64_t now,bool demo=false,bool help=false);
}
