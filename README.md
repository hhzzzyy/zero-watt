# Power Monitor v0.1.0

Cardputer Zero 电池功率监视器。LVGL 9.5 在 320×170 屏幕上绘制 **40 列 × 18 行纯 ASCII**，使用内置 UNSCII 8 等宽字体，每行 9 像素。界面只有字符行，没有按钮、进度条或图表控件。APPLaunch 的 `Terminal=false`：这是字符风格的图形程序。

![LVGL demo capture](preview/demo.png)

截图带 `[DEMO]`，为模拟数据；实际启动默认读真实 sysfs，缺少设备时不会自动切换到模拟数据。

## 数据与操作

每秒读取 `/sys/class/power_supply/bq27220-0/` 下的六个文件：

| 文件 | 换算及显示 |
|---|---|
| `voltage_now` | µV → V |
| `current_now` | µA → mA，保留正负方向 |
| `capacity` | 0–100% |
| `temp` | 0.1°C → °C，ASCII 使用 C |
| `time_to_empty_now` | 秒 → 时分，仅放电时显示 |
| `status` | 充电、放电等原始状态 |

功率 = `voltage_V * abs(current_A)`，不使用 `power_avg`。这是电池端充/放电功率；接入外部供电时不等同于整机输入功率。sysfs 各文件独立读取，不能保证是硬件同一瞬间的原子快照。

- Enter：帮助页（桌面预览也支持 `H`）。
- 短按 Esc：返回监视页，在主页保持主页。
- 按住 Esc 1 秒：退出，不依赖按键重复。
- SIGINT / SIGTERM：正常退出，供 APPLaunch 长按 Home 使用。
- 60 秒环形历史以单调时钟计时；30 个曲线格，每格取两秒峰值。平均值使用窗口内有效采样。
- `--` 表示读失败/无效，`?` 表示曲线格数据缺失或不完整；不会用 0 或旧数据冒充新读数。

## 桌面编译和预览

需要 CMake ≥3.24、C/C++17 编译器、Git、SDL2。首次配置从 GitHub 获取 LVGL v9.5.0。

```sh
cmake -S . -B build -DUSE_DESKTOP=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
ctest --test-dir build --output-on-failure
./build/power-monitor --demo
```

如果 SDL2 不在默认搜索路径，增加 `-DCMAKE_PREFIX_PATH=/path/to/SDL2/prefix`。已有 LVGL v9.5.0 源码可增加 `-DFETCHCONTENT_SOURCE_DIR_LVGL=/absolute/path/to/lvgl`，避免再次下载。

```sh
# 指定测试目录，目录内放六个同名文本文件
./build/power-monitor --sysfs-root /path/to/fixture
# 使用真实 LVGL 软件渲染，无需窗口，导出 320×170 PPM
./build/power-monitor --demo --capture preview/demo.ppm
./build/power-monitor --demo --help-screen --capture preview/help.ppm
# 无需 LVGL/SDL，独立测试采样、历史、格式化
cmake -S . -B build-core -DBUILD_GUI=OFF
cmake --build build-core
ctest --test-dir build-core --output-on-failure
```

## 真机编译和安装

本项目沿用当前官方 Template 的 CMake + LVGL v9.5.0 路线，缩减了本应用无需的资源、控件和依赖；不是 VibAPP 的预编译模板，不能直接套用该模板的 `make APP_SLUG=...` 命令。

在 Cardputer Zero 本机，需要 `cmake`、`git`、`g++`（C++17）和 Linux 开发头文件；不需要 SDL2：

```sh
cmake -S . -B build-device -DUSE_DESKTOP=OFF -DBUILD_TESTING=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-device -j1
sudo cmake --install build-device --prefix /usr
sudo systemctl restart APPLaunch.service
```

安装包含程序、wrapper、`.desktop` 和 PNG 图标，全部落在 `/usr/share/APPLaunch`。重启 launcher 会中断当前运行的 launcher 应用，应在安装完成后执行。

Linux 开发机交叉编译（先安装 `aarch64-linux-gnu-gcc/g++`；使用与设备兼容的 sysroot/libc）：

```sh
cmake -S . -B build-device -DUSE_DESKTOP=OFF -DBUILD_TESTING=OFF \
  -DCMAKE_TOOLCHAIN_FILE=cmake/aarch64-linux.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build build-device -j4
# 可选：先生成安装目录，检查后拷贝到设备
DESTDIR="$PWD/dist/stage" cmake --install build-device --prefix /usr
# 可选：需要 Linux dpkg-deb；拒绝将 macOS/SDL 二进制装成 arm64 包
MAINTAINER='Your Name <your-email>' ./scripts/package.sh build-device/power-monitor
```

打包脚本使用官方 skill 的 helper。包内 launcher 直接执行 `bin/power-monitor`；CMake 安装使用 `bin/run-power-monitor` wrapper，两者均无命令参数、均符合 APPLaunch。发布前还需要填写 `PUBLISH.md` 中的维护者、源码链接和商店素材；本版本未提交商店。

设备默认扫描 `/proc/fb` 中的 `fb_st7789v` 或 `panel-mipi-dbid`，按条目编号选择设备，不固定使用 `/dev/fb0`。可用环境变量覆盖：

```sh
LV_LINUX_FBDEV_DEVICE=/dev/fb1 \
POWER_MONITOR_INPUT_DEVICE=/dev/input/by-path/platform-3f804000.i2c-event \
/usr/share/APPLaunch/bin/run-power-monitor
```

只在确认节点后设置覆盖值；程序会检查分辨率为 320×170。设备输入使用官方约定的 evdev 默认路径，可通过上面的变量更换。

## 结构和后续 /proc 扩展

- `src/model.*`：独立于 LVGL 的 sysfs provider、快照、按秒历史、Esc 保持状态。
- `src/ascii_view.*`：快照 → 40×18 ASCII 帧，保持界面纯函数。
- `src/main.cpp`：LVGL 字符行、显示/输入后端、1 Hz 调度、退出和截图。
- `tests/`：临时 sysfs 测试，以及通过真实 SDL 事件验证短按/长按 Esc 和窗口退出。
- `applaunch/`：launcher 入口、wrapper、应用图标。
- `scripts/`：图标生成及官方打包 helper。

后续增加独立 `ProcessSource`，读取 `/proc/stat`、`/proc/[pid]/stat` 和 `/proc/[pid]/io`，使用同一单调时间计算差值，并增加进程视图。不要把 `/proc` 文件访问塞进字符渲染器；CPU/IO 活动不能直接转成精确的逐进程瓦数。

## 已验证与待验证

已在 macOS 编译 LVGL/SDL 版本；核心测试、SDL 生命周期测试和 320×170 LVGL 截图检查详见 `VALIDATION.md`。

尚未在真机上验证：AArch64 编译、framebuffer 显示、实体键盘映射、launcher 图标和返回、长按 Home、程序自身的功耗开销。当前交付为源码工程及桌面预览，不包含已验证的 arm64 可执行文件或 `.deb`。

## 上游约定

- [CardputerZero skill](https://github.com/CardputerZero/skill/blob/main/cardputer-zero-application/SKILL.md)，核对提交 `2a061c0908c40676837d7d40e40b23ac43a2f888`。
- [CardputerZero Template](https://github.com/CardputerZero/Template)，核对提交 `477fa6d217b683e3084c361a757f03dc8ca9f753`。参考其 CMake/后端组织，未复制传统 widget UI。
- [LVGL v9.5.0](https://github.com/lvgl/lvgl/tree/v9.5.0)，提交 `85aa60d18b3d5e5588d7b247abf90198f07c8a63`，包含等宽字体。

`make_applauncher_package.py` 与 `generate_app_icon.py` 原样来自上述 skill 提交；`PUBLISH.md` 由 skill 模板填写。上游代码和依赖仍遵守各自许可证；发布前确定新增应用代码的许可。
