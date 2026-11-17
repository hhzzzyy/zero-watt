# Zero Watt v0.1.0

Zero Watt 是 Cardputer Zero 的电池功率监视器。它每秒读取 BQ27220 的电压、电流、电量、温度和状态，显示电池端充/放电功率、净平均值、最近 60 秒的实时曲线，以及可平移缩放的历史页。界面用 LVGL 9.5 绘制在 320×170 屏幕上，等宽字符排版，没有按钮，用 B / T / G / H / Esc 操作。APPLaunch 的 `Terminal=false`：这是图形程序。

![LVGL demo capture](preview/demo.png)

截图带 `[DEMO]`，为模拟数据；实际启动默认读真实 sysfs，缺少设备时不会自动切换到模拟数据。

## 数据与操作

每秒读取 `/sys/class/power_supply/bq27220-0/` 下的六个文件：

| 文件 | 换算及显示 |
|---|---|
| `voltage_now` | µV → V |
| `current_now` | µA → mA，保留正负方向 |
| `capacity` | 0–100% |
| `temp` | 0.1°C → °C，度数符号显示为 `C` |
| `time_to_empty_now` | 秒 → 时分，仅放电时显示 |
| `status` | 充电、放电等原始状态 |

功率 = `voltage_V * current_A`，保留正负号，不使用 `power_avg`。正值表示充电（绿色），负值表示放电（红色），零线为灰色；主页和历史页均按可见数据自动选择正／负坐标范围。平均值是带符号的净平均功率。这是电池端充/放电功率；接入外部供电时不等同于整机输入功率。sysfs 各文件独立读取，不能保证是硬件同一瞬间的原子快照。

- **B**：切换后台常驻；首次默认关闭，设置自动保存。
- **T**：设置历史保留时长，输入 1～1440 分钟（默认 30）；直接输入替换原值，Backspace 删除，Enter 保存，Esc 取消。
- **G**：全屏历史页。横轴实际时间、纵轴带符号 W，白色字符边框内有网格与红绿曲线。左右翻看，上下缩放，Enter 跟随最新，短 Esc/G 返回主页。
- **H**：主页帮助；短 Esc 从帮助返回，主页短 Esc 不退出。
- **长按 Esc 1 秒**：常驻开时关闭界面、继续采样；常驻关时关闭界面并停止采集。
- SIGINT / SIGTERM 和窗口关闭也遵循常驻设置。SIGKILL 无法运行界面清理逻辑。
- 主屏幕始终显示最近 60 秒。历史时长决定保留多少数据，不是监控结束时间。历史页缩放不会删除数据。
- 缩短保留时长立即丢弃更旧数据；增加后逐渐积累。历史保存在运行时内存中，重启采集服务／设备后清空，设置保存在 `$XDG_CONFIG_HOME/zero-watt/settings`（默认 `~/.config`）。
- 缺失样本保留空白；长时间图按像素汇总连续有效样本的最低／最高值，保留峰值，不跨缺口连线。

### 后台采集

设备使用独立的 `zero-watt-collector.service` **systemd 用户服务**，首次打开应用时按需启动，无开机自启。服务只读取电池 sysfs，不打开显示和键盘；前台通过用户运行时目录中的共享内存取得样本。单实例锁避免重复采集，服务异常退出后由 systemd 重启。实际 suspend 时采样暂停。

安装新版本后执行 `systemctl --user daemon-reload`。诊断：

```sh
systemctl --user status zero-watt-collector.service
/usr/bin/zero-watt --status
# 手动停止采集
systemctl --user stop zero-watt-collector.service
```

后台监控仍测量电池端功率，不是 USB 总输入功率。桌面预览 `--demo` 不启动服务、不保存设置。

主页底部提示为 单行 `B(ackground) T(ime) G(raph) H(elp) Esc`，其中 `B(ackground)` 在常驻后台开启时为绿色、关闭时为灰色，其余提示统一使用同款灰色（`#8cabb8`）。H 打开／关闭帮助；Esc 仍默认长按退出或后台运行。旧版无符号历史在升级时清空，设置保留。

主页将功率、电流和电压合并为一行，例如 `BAT POWER -1.429W (-0.384A x 3.722V)`。电量行末尾显示状态词：绿色 `CHARGING`、红色 `DISCHARGING`、灰色 `IDLE`（零电流）或 `UNKNOWN`（读取不到电流）。移除独立 CURRENT/VOLTAGE/STATE 行，主页曲线高度由 36 增至 54 像素。曲线使用白色字符外框，量程与平均功率位于框顶，`-60s`／`now` 与框左右边缘对齐。

## 桌面编译和预览

需要 CMake ≥3.31、Ninja、C/C++17 编译器、Git、SDL2。首次配置从 GitHub 获取 LVGL v9.5.0。

macOS（Apple Silicon）：

```sh
cmake --preset darwin-arm64
cmake --build --preset darwin-arm64-rel
ctest --test-dir build/darwin-arm64 -C Release --output-on-failure
./build/darwin-arm64/Release/zero-watt --demo
```

Linux 桌面：

```sh
cmake --preset linux-x86-64
cmake --build --preset linux-x86-64-rel
ctest --test-dir build/linux-x86-64 -C Release --output-on-failure
./build/linux-x86-64/Release/zero-watt --demo
```

如果 SDL2 不在默认搜索路径，增加 `-DCMAKE_PREFIX_PATH=/path/to/SDL2/prefix`。已有 LVGL v9.5.0 源码可增加 `-DFETCHCONTENT_SOURCE_DIR_LVGL=/absolute/path/to/lvgl`，避免再次下载。

```sh
# 指定测试目录，目录内放六个同名文本文件
./build/darwin-arm64/Release/zero-watt --sysfs-root /path/to/fixture
# 使用真实 LVGL 软件渲染，无需窗口，导出 320×170 PPM
./build/darwin-arm64/Release/zero-watt --demo --capture preview/demo.ppm
./build/darwin-arm64/Release/zero-watt --demo --help-screen --capture preview/help.ppm
./build/darwin-arm64/Release/zero-watt --demo --history-screen --capture preview/history.ppm
# 无需 LVGL/SDL，独立测试采样、历史、格式化
cmake -S . -B build-core -DBUILD_GUI=OFF
cmake --build build-core
ctest --test-dir build-core --output-on-failure
```

## 真机编译和安装

`cp0-cross` 完全使用官方 Template 的流程：CMake preset + BSP sysroot + CPack DEB。首次 configure 会自动下载约 435 MB 的 BSP（`.cache/sdk_bsp-src`），并需要主机上的 `aarch64-linux-gnu-gcc/g++`。

> 官方工具链的下载超时是 300 秒；慢网下 435 MB 的 BSP 可能超时。可以先把 `sdk_bsp.tar.gz` 解压到 `.cache/sdk_bsp-src`，或调大 `cmake/toolchains/cp0-aarch64-linux-gnu.cmake` 中的 `TIMEOUT`。

macOS：

```sh
brew install aarch64-unknown-linux-gnu ninja dpkg
cmake --preset cp0-cross
cmake --build --preset cp0-cross-rel
cpack --preset cp0-cross-deb
```

Debian 主机：

```sh
sudo apt install crossbuild-essential-arm64
```

也可以一条命令完成 configure、Release 构建和打包：

```sh
cmake --workflow --preset cp0-cross-package
```

产物为 `dist/ZeroWatt_0.1.0_rev1_arm64.deb`，布局：

- `/usr/bin/zero-watt`
- `/usr/share/APPLaunch/applications/zero-watt.desktop`
- `/usr/share/APPLaunch/share/images/zero-watt.png`
- `/usr/lib/systemd/user/zero-watt-collector.service`
- `/usr/share/doc/zero-watt/{README.md,LICENSE}`

官方部署方式是通过 `deploy.sh` 复制到设备，然后在设备上安装：

```sh
REMOTE_USER=<user> REMOTE_HOST=<device-ip> ./deploy.sh
sudo apt install --no-install-recommends ./ZeroWatt_0.1.0_rev1_arm64.deb
```

`deploy.sh` 只复制包，不执行安装。安装后 postinst 会对活动用户执行 `systemctl --user daemon-reload`；随后重启一次启动器刷新条目：

```sh
runuser -u hzy -- env XDG_RUNTIME_DIR=/run/user/1000 systemctl --user restart APPLaunch.service
```

如果设备通过 ADB 连接（本机开发便利，非官方 deploy.sh 流程），可以直接：

```sh
ADB=/tmp/cardputer-adb/platform-tools/adb
$ADB -s CardputerZero-c7f0bb67 push dist/ZeroWatt_0.1.0_rev1_arm64.deb /data/local/tmp/
$ADB -s CardputerZero-c7f0bb67 shell dpkg -i /data/local/tmp/ZeroWatt_0.1.0_rev1_arm64.deb
$ADB -s CardputerZero-c7f0bb67 shell 'runuser -u hzy -- env XDG_RUNTIME_DIR=/run/user/1000 systemctl --user daemon-reload'
$ADB -s CardputerZero-c7f0bb67 shell 'runuser -u hzy -- env XDG_RUNTIME_DIR=/run/user/1000 systemctl --user restart APPLaunch.service'
```

项目早期名为 Power Monitor；`zero-watt` 包声明了 `Replaces`/`Conflicts: power-monitor`，`dpkg -i` 会替换旧的开发安装。旧设置目录 `~/.config/power-monitor` 不会自动迁移。

旧版本如果曾用 `cmake --install` 安装到 `/usr/share/APPLaunch/bin`，升级后请确认启动器已指向 `/usr/bin/zero-watt`。CPack 的 DEB 生成在 Debian 主机上最可靠；macOS 上需要 Homebrew `dpkg`（它同时提供 `dpkg-deb`），CPack 只有在 PATH 中找到 `dpkg` 时才启用 DEB 生成器。

设备默认扫描 `/proc/fb` 中的 `fb_st7789v` 或 `panel-mipi-dbid`，按条目编号选择设备，不固定使用 `/dev/fb0`。可用环境变量覆盖：

```sh
LV_LINUX_FBDEV_DEVICE=/dev/fb1 \
ZERO_WATT_INPUT_DEVICE=/dev/input/by-path/platform-3f804000.i2c-event \
/usr/bin/zero-watt
```

只在确认节点后设置覆盖值；程序会检查分辨率为 320×170。设备输入使用官方约定的 evdev 默认路径，可通过上面的变量更换。

## 结构和后续 /proc 扩展

- `src/model.*`：独立于 LVGL 的 sysfs provider、快照、按秒历史、Esc 保持状态。
- `src/ascii_view.*`：快照 → 40×18 ASCII 帧，保持界面纯函数。
- `src/collector.*`：独立采样服务、运行时环形记录、设置持久化和进程锁。
- `src/main.cpp`：LVGL 字符行和功率折线绘制、显示/输入后端、1 Hz 调度、退出和截图。
- `tests/`：临时 sysfs 测试，以及通过真实 SDL 事件验证短按/长按 Esc 和窗口退出。
- `applaunch/`：APPLaunch 图标和用户级采集服务单元。
- `cmake/`：官方 BSP 交叉工具链、CPack 包契约和模板。
- `scripts/`：图标生成辅助脚本。

后续增加独立 `ProcessSource`，读取 `/proc/stat`、`/proc/[pid]/stat` 和 `/proc/[pid]/io`，使用同一单调时间计算差值，并增加进程视图。不要把 `/proc` 文件访问塞进字符渲染器；CPU/IO 活动不能直接转成精确的逐进程瓦数。

## 已验证与待验证

macOS 桌面 preset 构建和五项 CTest 已通过；官方 `cp0-cross-package` workflow 在 macOS 上完成 BSP 交叉构建并生成 `dist/ZeroWatt_0.1.0_rev1_arm64.deb`，`dpkg-deb` 和 `app-builder` validator 均通过。详见 `VALIDATION.md`。

改名后的 `zero-watt` 包已在 Debian 13 / AArch64 真机用 `dpkg -i` 安装，并替换掉旧的 `power-monitor` 开发包。`--demo --capture`、用户级采集服务启动和 `--status` 均正常；注入 evdev H/G/T/Esc 验证了帮助、历史、设置和长按退出，并用实际 framebuffer 重拍四张商店截图。启动器已恢复。仍待验证实体按键、从启动器选中应用的完整流程、长按 Home、24 小时 soak 和程序自身功耗。商店上传尚未执行；`app-builder.json` 的 source repo、author 和 share code 仍是待确认值。

## 上游约定

- [CardputerZero skill](https://github.com/CardputerZero/skill/blob/main/cardputer-zero-application/SKILL.md)，核对提交 `2a061c0908c40676837d7d40e40b23ac43a2f888`。
- [CardputerZero Template](https://github.com/CardputerZero/Template)，核对提交 `477fa6d217b683e3084c361a757f03dc8ca9f753`。交叉构建、BSP sysroot、CPack 包契约和 `cp0-cross-package` workflow 均按该模板对齐；未复制其 widget UI。
- [LVGL v9.5.0](https://github.com/lvgl/lvgl/tree/v9.5.0)，提交 `85aa60d18b3d5e5588d7b247abf90198f07c8a63`，包含等宽字体。

`generate_app_icon.py` 原样来自上述 skill 提交；发布元数据以 `app-builder.json` 为准，应用代码采用 MIT 许可证。上游代码和依赖仍遵守各自许可证。
