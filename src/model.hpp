#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <string>

namespace power {
using Value = std::optional<double>;
struct Sample {
    Value volts, amps, watts, capacity, temperature;
    std::optional<int64_t> eta_seconds;
    std::string status = "Unavailable";
};
// Future /proc collectors belong beside this provider, never inside the view.
class BatterySource {
public:
    explicit BatterySource(std::string root) : root_(std::move(root)) {}
    Sample read() const;
private:
    std::string root_;
};
struct Point { int64_t second = -1; Value watts; };
class History {
public:
    static constexpr size_t size = 60;
    void add(int64_t second, Value watts);
    std::array<Value, size> window(int64_t now) const;
private:
    std::array<Point, size> points_{};
};
struct EscapeHold {
    std::optional<uint32_t> pressed;
    void update(bool down, uint32_t now) { if(down && !pressed) pressed=now; if(!down) pressed.reset(); }
    bool should_exit(uint32_t now) const { return pressed && uint32_t(now-*pressed)>=1000; }
};
Sample demo_sample(int64_t second);
}
