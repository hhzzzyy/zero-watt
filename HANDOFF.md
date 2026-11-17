# Zero Watt 交接记录

更新：2026-11-17（Asia/Shanghai）

## 最新更新：修复商店安装路径

- 商店拒绝了 `usr/lib/systemd/user/zero-watt-collector.service`。单元改为模板 `usr/share/zero-watt/zero-watt-collector.service`；应用启动时复制到 `~/.config/systemd/user/`，执行 `systemctl --user daemon-reload` 后再启动。
- 包内移除 postinst，不再安装任何 `usr/lib/systemd/user` 或系统级 service。
- 官方 `install_path_policy.py` 对 `dist/ZeroWatt_0.1.0_rev1_arm64.deb` 返回 clean。
- 设备当时未连接，运行时安装单元这一步尚未真机验证。

## 最新更新：更名为 Zero Watt

- 显示名 `Zero Watt`，包名与二进制 `zero-watt`；源码目录仍是 `cardputer-power-monitor`，没有改目录名。
- 同步改名：`/usr/bin/zero-watt`、`zero-watt-collector.service`、`zero-watt.desktop`、`zero-watt.png`、运行时目录 `$XDG_RUNTIME_DIR/zero-watt`、设置 `~/.config/zero-watt/settings`、环境变量 `ZERO_WATT_INPUT_DEVICE`，共享内存格式版本改为 `0x5a570003`。
- 商店字段：`app_name=Zero Watt`、`package_name=zero-watt`、`share_code=ZWAT`（仍待商店确认唯一）。
- 重新构建：桌面五项测试通过；`cp0-cross-package` 生成 `dist/ZeroWatt_0.1.0_rev1_arm64.deb`，validator 通过。DEB 声明 `Replaces`/`Conflicts: power-monitor`，设备上可直接替换旧开发包。
- 桌面截图 demo/help/sample/missing 已按新标题重渲；真机已安装 `zero-watt` 并用实际 framebuffer 重拍四张商店截图（home/help/history/settings），标题均为 `ZERO WATT 0.1`。
- `dpkg -i` 已用 `zero-watt` 替换旧的 `power-monitor`；旧的用户级采集进程已停止，新采集服务由应用启动。

## 最新更新：完全对齐官方 Template 构建/发布标准

- 删除旧的 `scripts/package.sh`、`scripts/make_applauncher_package.py`、`cmake/aarch64-linux.cmake`、旧 launcher wrapper 和旧 desktop 文件。
- 新增 `CMakePresets.json`（`darwin-arm64`、`linux-x86-64`、`cp0-cross`、`cp0-cross-deb`、`cp0-cross-package`）、`cmake/toolchains/cp0-aarch64-linux-gnu.cmake`（BSP sysroot）、`cmake/cm0-package.cmake` 及模板、`deploy.sh`、`.agents` 发布 skill、MIT `LICENSE`。
- 安装布局改为 `/usr/bin/power-monitor`；用户级采集服务保留在 `/usr/lib/systemd/user`，因为应用需要按用户启动采集器；官方模板的 system service 模板不适用于该架构。
- 官方 workflow 已在 macOS 跑通：首次 BSP 约 435 MB，交叉使用 GCC 15.2 + BSP glibc 2.41；生成 `dist/PowerMonitor_0.1.0_rev1_arm64.deb`；官方 `validate_app_builder.cmake` 通过；已用 `dpkg -i` 安装到设备，采集服务和 fbdev/evdev 路径已验证。
- 待确认：`app-builder.json` 的 source repo、author、`share_code`；商店上传尚未执行。

## 最新更新：合并电气读数和状态符号

- 主屏 `BAT POWER ±x.xxxW (±x.xxxA x x.xxxV)`，移除 CURRENT、VOLTAGE、STATE 独立行。
- 电量放在第 4 行（0-based），末尾根据电流显示 + / - / = / ?（充电 / 放电 / 零电流 / 缺失）。温度与 ETA 在第 5 行；帮助页增加图例。
- 主页曲线改为 (40,76)、240×54，标题第 7 行，外框第 8～14 行；全屏历史保持原布局。

## 最新更新：单行底部与 H 帮助

主页底部仅一行 `B(ackground) T(ime) G(raph) H(elp) Esc`；不再显示 Hold 字样，但 Esc 仍需长按 1 秒才退出/转入后台。H 独立切换帮助，设备 KEY_H 和桌面 SDL 均已映射；Enter 只用于设置保存及历史页跟随。帮助页面与 README 已同步。

## 最新更新：带符号功率与快捷键含义

- 功率改为 `V * I`：正数充电（绿），负数放电（红），零线灰色。主页数值和平均值保留符号；历史允许负数。
- 主页和全屏历史共用范围逻辑，始终包含零；仅充电/放电时只显示相应半轴。过零线段在零点分色，缺失数据仍断线，像素汇总保留两侧峰值。
- 底部 `B(ackground) T(ime) G(raph)`；下一行 `Enter:help Hold Esc:background/quit`。帮助页解释颜色。
- 共享存储版本升级为 `0x504d0003`，旧无符号历史无法恢复方向，因此清空旧历史，持久化设置保留。
- DEMO 数据覆盖充电与放电，截图明确标记 DEMO。

## 最新更新：后台常驻和历史页（优先于下方旧记录）

用户批准实现后台监控，并将 F1/F2/F3 改为字母快捷键：

- B 切换常驻；T 输入历史保留分钟数（1～1440，默认 30）；G 打开全屏历史图。
- 常驻开时长按 Esc 关闭前台但继续采集；常驻关时停止采集并退出。短 Esc 返回，主页不退出。
- 历史页有时间／W 坐标、网格，左右平移、上下缩放、Enter 跟随。主屏仍显示 60 秒。
- 设置持久化；历史保存在用户运行时共享内存，只在采集服务存活期间延续，重启服务清空。缩短时长立即裁剪，放大不补回旧记录。
- 新增 `src/collector.*`，使用单实例进程锁。设备按需启动 `power-monitor-collector.service` 用户服务，异常退出自动重启，不设开机自启。服务不访问屏幕和输入。
- 设备输入改为直接读取 evdev，因为 LVGL 默认映射没有字母和数字；桌面 SDL 使用同一命令处理逻辑。
- 新增两个测试（采集生命周期、交互），全部五项测试通过。真机编译安装完成，按键注入验证后台持续采样、重开保留历史、关闭常驻后真正停止。详见 VALIDATION.md。
- 测试后启动器恢复，采集已停止，设置为 BG:OFF / 30m。没有 Git 提交、推送或发布。
- 新截图 `preview/history.png`、`preview/device-history.png`、`preview/device-settings.png`。
- `scripts/package.sh` 曾是用户服务打包脚本；现已移除，改由官方 CPack 生成 `.deb`。
- 设备旧源码备份 `/tmp/power-monitor-before-background`；本次测试脚本 `/tmp/test-power-background.sh`。备份时误复制的大型 build-device 缓存已删除，原工程未删除。

## 工作位置与用户要求

唯一主工程：`/Users/hzy/Programming/cardputer-power-monitor/`。
不要继续修改旧 ChatGPT 项目镜像中的同名目录或旧 ZIP。

目标设备为 Linux/AArch64 Cardputer Zero，屏幕 320×170。
用户最初要求 htop 风格等宽 ASCII 界面，通过 LVGL 绘制；最新要求将功率图中的 `#` 改为连续曲线，其余字符风格保持。
用户已授权通过 USB ADB 编译、安装和调试本应用，测试期间可暂时占用屏幕、停止并恢复启动器。没有授权发布商店或推送 Git 远端。

## 当前成果

- C++17 + CMake + LVGL v9.5.0，桌面 SDL，设备 fbdev + evdev。
- 每秒读取 `/sys/class/power_supply/bq27220-0/{voltage_now,current_now,capacity,temp,time_to_empty_now,status}`。
- 功率为 `voltage_V * abs(current_A)`，保留电流正负号；不用 `power_avg`。
- ETA 仅放电时显示；缺失/无效值显示 `--`，不会伪装为零。
- 单调时钟 60 秒历史环形缓冲；缺采样保留空白。
- 最新实现：绿色连续折线，每秒一个点，缺失处断开，孤立有效点仍可见。图形范围 240×36，坐标 (40,94)。纵轴从零开始，上限与字符显示共用 `graph_top()`。
- Enter 打开/关闭帮助；短按 Esc 返回；长按 1 秒退出。桌面额外支持 H。
- 新版已在真机编译并安装；测试后启动器已恢复。

## Git 状态

分支 `main`。提交：

- `06c3c20 Initial Power Monitor v0.1 source`（基线，保留当时的工程名）
- `0aa885a feat: background collector, history page, and official packaging`
- 本次改名提交：`refactor: rename project to Zero Watt`

没有设置远端或推送；不要自动推断商店发布许可。

## 本机编译

本机为 macOS（CMake 4.4.5、Ninja、Homebrew `dpkg`）。BSP 在 `.cache/sdk_bsp-src`；本次测试为指向 `/tmp/cp0-bsp/root` 的符号链接，正式首次 configure 会自动下载约 435 MB 的 BSP。交叉编译器在 `/tmp/cross-toolchain/aarch64-unknown-linux-gnu`，正式环境可 `brew install aarch64-unknown-linux-gnu`。

```sh
cd /Users/hzy/Programming/cardputer-power-monitor
cmake --preset darwin-arm64
cmake --build --preset darwin-arm64-rel
ctest --test-dir build/darwin-arm64 -C Release --output-on-failure
./build/darwin-arm64/Release/zero-watt --demo
./build/darwin-arm64/Release/zero-watt --demo --capture /tmp/zero-watt-demo.ppm

# 官方交叉构建/打包（需要 PATH 中有 aarch64-linux-gnu-gcc/g++）
export PATH=/tmp/cross-toolchain/aarch64-unknown-linux-gnu/bin:$PATH
cmake --workflow --preset cp0-cross-package
```

五项测试最近全部通过；`--capture` 为真实 LVGL 离屏渲染，不打开 framebuffer；PPM 可通过 macOS `sips` 转 PNG。CPack 在 macOS 上只有在 PATH 中存在 `dpkg` 时才启用 DEB 生成器。

## ADB 与设备

ADB：`/tmp/cardputer-adb/platform-tools/adb`，来自 Google 官方 Platform Tools，未全局安装。
最近设备 serial：`CardputerZero-c7f0bb67`；先运行 `adb devices -l` 确认。
ADB shell 是 root。先前在沙箱内启动的 ADB 服务无法识别 USB，重启为可访问 USB 的服务后成功；工具操作可能需要沙箱权限。不要把一次空列表直接判定成设备不支持 ADB。

设备：Debian GNU/Linux 13 (trixie)，aarch64，约 352 MiB RAM + swap。使用 `-j1` 编译。

- 程序：`/usr/bin/zero-watt`
- 采集器单元模板：`/usr/share/zero-watt/zero-watt-collector.service`；运行时安装到 `~/.config/systemd/user/zero-watt-collector.service`
- desktop：`/usr/share/APPLaunch/applications/zero-watt.desktop`
- 图标：`/usr/share/APPLaunch/share/images/zero-watt.png`
- 文档：`/usr/share/doc/zero-watt/`

设备上已安装 `zero-watt` 0.1.0-rev1；`dpkg -i` 通过 `Replaces`/`Conflicts: power-monitor` 移除了旧的 `power-monitor` 开发包，旧的采集服务进程也已停止。旧的手动安装文件（`/usr/share/APPLaunch/bin/power-monitor`、`run-power-monitor` wrapper）不是官方包安装的，仍留在设备上；启动器已指向 `/usr/bin/zero-watt`。设备不再需要本地源码或 `build-device`。

官方包在 Mac 上构建，设备上直接安装：

```sh
ADB=/tmp/cardputer-adb/platform-tools/adb
"$ADB" -s CardputerZero-c7f0bb67 push dist/ZeroWatt_0.1.0_rev1_arm64.deb /data/local/tmp/
"$ADB" -s CardputerZero-c7f0bb67 shell dpkg -i /data/local/tmp/ZeroWatt_0.1.0_rev1_arm64.deb
# postinst 已对活动用户执行 daemon-reload；需要时手动再执行
"$ADB" -s CardputerZero-c7f0bb67 shell 'runuser -u hzy -- env XDG_RUNTIME_DIR=/run/user/1000 systemctl --user daemon-reload'
"$ADB" -s CardputerZero-c7f0bb67 shell 'runuser -u hzy -- env XDG_RUNTIME_DIR=/run/user/1000 systemctl --user restart APPLaunch.service'
```

官方 `deploy.sh` 走 scp + 设备端 `apt install` 的路径；本机开发使用上面的 ADB 方式。

## 显示、键盘和启动器

`/proc/fb` 为 `0 panel-mipi-dbid`；实际 `/dev/fb0`：320×170、RGB565、stride 640。
程序按驱动名 `panel-mipi-dbid` 或 `fb_st7789v` 探测，支持 `LV_LINUX_FBDEV_DEVICE` 覆盖，检查尺寸；不盲选 fb0。

键盘 `/dev/input/by-path/platform-3f804000.i2c-event` → `/dev/input/event3`，可由 `POWER_MONITOR_INPUT_DEVICE` 覆盖。
默认 LVGL evdev 仅转换控制键，因此真机帮助使用 Enter。SDL 键盘会合成释放事件，所以桌面 Esc 长按通过 SDL event watch 追踪物理状态。

**启动器是用户 hzy (UID 1000) 的 systemd 用户服务，不是 system service。**

```sh
# ADB root shell 内
runuser -u hzy -- env XDG_RUNTIME_DIR=/run/user/1000 systemctl --user status APPLaunch.service
runuser -u hzy -- env XDG_RUNTIME_DIR=/run/user/1000 systemctl --user restart APPLaunch.service
```

直接运行应用前先暂停/停止启动器，防止争用 framebuffer；测试脚本必须 trap 恢复服务。避免使用旧说明的 `sudo systemctl restart APPLaunch.service`。
设备可能自动熄屏，亮度 `/sys/class/backlight/backlight/brightness` 为 0。曾通过短 Esc 唤醒，锁屏显示 Press TAB to unlock，Tab 为无密码解锁。

## 最新验证证据

- 桌面编译及三个 CTest 通过；`git diff --check` 通过。
- 真机 GCC 增量编译成功，新程序安装成功。
- 暂停用户级启动器，以 `--demo` 启动新程序，读取实际 framebuffer，曲线清晰、坐标无越界。
- evdev 注入 Enter 打开帮助，帮助页没有残留曲线；短 Esc 返回不退出；长 Esc 1.2 秒后程序正常返回 0。
- 测试结束恢复启动器，`is-active` 返回 active，应用运行日志为空。
- 之前真实电池读取和离屏渲染已成功；接 USB 时为 Charging，显示电池充电功率和空 ETA。不要把它解释为整机总输入功率。

最近辅助脚本在 Mac 和设备 `/tmp/`，可能重启后消失：

- `cardputer-fb-capture.py`：ioctl 读 framebuffer 格式，导出 PPM。
- `cardputer-key.py`：向 event3 写 EV_KEY + SYN，参数 Linux keycode 和按住秒数。Enter=28、Esc=1、Tab=15。
- `test-power-curve.sh`：停止用户级 launcher、运行 DEMO、截图、注入按键、确保退出、trap 恢复 launcher。

## 后续边界和待办

用户最新要求“曲线而不是 #”已经完成，并已部署。当前请求是交接，不要自行开启新功能。
尚未完成：用户实际物理按键确认、启动器选择入口/图标的完整导航测试、Home 强制退出、功耗开销测量和商店发布。`.deb` 已生成并在设备安装。
图标/desktop 已存在，但不要将独立运行测试写成完整 launcher 导航已验证。

若继续优化，可先确认用户真机观感；按需处理测试记录中旧的未验证说明。后续 /proc profiling 仍只是架构预留：独立 provider 和视图，不估造逐进程瓦数。

技能参考：`https://github.com/CardputerZero/skill/blob/main/cardputer-zero-application/SKILL.md`，已按其 APPLaunch/LVGL 约定执行。本地副本 `/tmp/cardputer-skill`（临时）；核对提交 `2a061c0908c40676837d7d40e40b23ac43a2f888`。

Signed-power device verification: GCC build and installation succeeded; real RGB565 framebuffer checked in `preview/device-signed-home.png` and `preview/device-signed-history.png` (explicit DEMO data). G opens history and long Esc exits. Live collector startup also verified. Launcher restored; collector stopped; persisted settings remain BG:OFF / 30 minutes.
