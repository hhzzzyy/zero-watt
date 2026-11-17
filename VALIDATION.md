# v0.1 validation

Validated locally on macOS with AppleClang 21 and LVGL v9.5.0.

- Core test: real temporary sysfs files, supplied 3.722 V / -384 mA = 1.429248 W, capacity/temperature/ETA units, charging ETA suppression, real zero, missing values, malformed numbers, overflow, invalid capacity, ETA sentinels, rolling eviction, missed seconds, same-second updates, stale expiration, Esc threshold/release/tick rollover, fixed-width printable ASCII frames.
- SDL lifecycle: a quick Esc press does not exit; physical Esc hold exits without repeat; SDL quit exits without double-free after LVGL teardown. SDL dummy video backend runs these without opening a desktop window.
- Actual LVGL renders checked at 320×170: demo history, supplied battery snapshot, missing sensor, help. PNGs are conversions of application's PPM captures, not a mockup or browser reimplementation.
- Device input now reads evdev key events directly for B/T/G, digits, Backspace, arrows, Enter, and Esc. SDL physical key events feed the same commands. The standard LVGL translator omits alphabetic and numeric input.
- Launcher metadata uses an absolute `/usr/bin/power-monitor` executable and the APPLaunch-relative icon `share/images/power-monitor.png`; `Terminal=false`; the icon is a 256×256 RGB PNG.

Historical note: Linux AArch64 linkage, real framebuffer, and packaging were later validated in the dated sections below; the final official package is recorded at the end. Physical keyboard, full launcher navigation, and power overhead remain unvalidated.

Device follow-up: user reports Debian 13/aarch64 and `/proc/fb` entry `0 panel-mipi-dbid`. Discovery now accepts that exact driver name as well as `fb_st7789v`; the existing 320×170 ioctl check remains. This change was later executed on the device in the dated sections below.

## Continuous power curve update — 2026-11-17

The history display now draws a green polyline through each of the 60 one-second samples. Missing samples break the line, including partially populated history; isolated readings remain visible. Scaling is shared with the displayed upper bound, and the plot is hidden on the help page.

- Desktop build and all three existing CTest checks passed.
- Built with GCC on the connected Debian 13 AArch64 device and installed under `/usr/share/APPLaunch`.
- Captured and visually checked the actual 320×170 RGB565 framebuffer, including curve and help views. `preview/device-curve-demo.png` uses explicitly labelled demo data.
- Injected Enter and Esc events through the device's evdev node: help opens, short Esc returns without exiting, one-second hold exits successfully.
- Restored the user's `APPLaunch.service` after the test. This is a user unit, not a system unit.

These checks supersede the earlier unvalidated AArch64/framebuffer notes. Physical key presses, Home handling, full launcher navigation, and power overhead remain unmeasured.

## Background collection and full-screen history — 2026-11-17

- Five CTest checks passed: core, collector lifecycle, interaction, Esc lifecycle, window close. Collector tests use temporary sysfs, a separate process, no frontend during continued sampling, duplicate startup, missing readings, persisted settings, and explicit shutdown. Interaction checks cover invalid input, save/cancel, residency, navigation, zoom, and live following.
- Device GCC build and targeted installation succeeded, including `/usr/lib/systemd/user/power-monitor-collector.service`; user daemon configuration reloaded. No release package created.
- Injected real evdev B/T/G, numeric input, Enter, arrow keys, and a 1.2-second Esc hold. Set retention to two minutes, enabled residency, exited frontend, and resumed APPLaunch. Collector status advanced from 10 samples (ticks 1950–1959) to 13 (1950–1962) while the launcher was active.
- Reopened the frontend and captured live history, retaining the original start tick. Disabled residency, restored 30-minute retention, held Esc, and verified `active=0 background=0 minutes=30`. UI logs were empty.
- Real framebuffer screenshots: `preview/device-settings.png`, `preview/device-history.png`; the latter contains real battery readings. Desktop full-history demo is `preview/history.png`. Grid labels use two decimals to accurately label quarter divisions.
- Launcher restored after test; collector stopped. Physical key presses, complete launcher navigation, Home behavior, 24-hour soak, actual suspend/resume, and monitoring power overhead remain untested. Normal history is volatile runtime data; settings persist. An explicit stop followed by a new monitoring session clears old history.
- During pre-deployment backup, copying the existing build cache filled `/tmp`; that newly created duplicate cache was removed immediately. The source backup remains at `/tmp/power-monitor-before-background`, and `/tmp` had 176 MiB free afterward. Original device files were not removed.

## Signed battery power and explicit shortcut labels — 2026-11-17

- Power now preserves the current sign (`V * I`): charging positive/green, discharging negative/red. Numeric current/power and net average retain signs. Negative history is no longer rejected.
- Both graph views scale around zero using shared bounds. Single-direction windows use only the relevant half-axis. A grey zero line is always visible; segments crossing zero split color at the crossing. Per-pixel extrema retain signed peaks, and missing readings still break runs.
- Footer uses `B(ackground) T(ime) G(raph)` plus a separate help/hold-Esc instruction. Battery power is labelled `BAT POWER`.
- Five CTest checks passed; core tests now explicitly cover negative samples, positive charging, negative-only/mixed/positive-only/all-zero bounds. Collector process test confirms negative watts survive shared storage.
- Desktop LVGL demo and full-history captures were visually checked and automatically checked for both red and green pixels inside graph bounds. Existing discharge/missing/help previews regenerated.
- Runtime format version increased to invalidate old unsigned history, which cannot reliably recover direction. Persistent settings are retained. Demo input intentionally exercises both directions.

Signed-power device verification: GCC build and installation succeeded; real RGB565 framebuffer checked in `preview/device-signed-home.png` and `preview/device-signed-history.png` (explicit DEMO data). G opens history and long Esc exits. Live collector startup also verified. Launcher restored; collector stopped; persisted settings remain BG:OFF / 30 minutes.

## Single-line footer and H help — 2026-11-17

All five CTest checks passed. Desktop and device builds succeeded. The footer fits on a single 40-column line: `B(ackground) T(ime) G(raph) H(elp) Esc`. H is now an independent help command on SDL and evdev; Enter remains settings-confirm/live-follow only. Injected device KEY_H to open and close help, then held Esc to exit successfully. Actual framebuffer screenshots checked: `preview/device-signed-home.png`, `preview/device-h-help.png`. Launcher restored, UI log empty. Esc hold timing and residency behavior are unchanged.

## Compact electrical readout — 2026-11-17

Merged watts, amps, and volts on BAT POWER using compact units; removed CURRENT/VOLTAGE/STATE rows. Battery percentage now ends with + (positive current), - (negative), = (zero), or ? (missing). Help explains symbols. Increased home graph height from 36 to 54 pixels; full-screen history unchanged. Desktop charging demo, discharge fixture, and missing-data captures checked. All five existing tests and diff whitespace check passed. Device build/install succeeded; actual framebuffer verified in `preview/device-compact-home.png` (DEMO). H and held Esc verified with evdev injection; launcher restored and UI log empty.

## Official Template build/package alignment — 2026-11-17

The project now uses the official CardputerZero Template release flow instead of the
previous ad-hoc helper scripts:

- `CMakePresets.json` provides `darwin-arm64`, `linux-x86-64`, `cp0-native`, `cp0-cross`,
  the `cp0-cross-rel` build preset, the `cp0-cross-deb` package preset, and the
  `cp0-cross-package` workflow.
- `cmake/toolchains/cp0-aarch64-linux-gnu.cmake` downloads and uses the CM0 BSP
  (`sdk_bsp.tar.gz` v0.0.4, about 435 MB compressed) as `CMAKE_SYSROOT`.
- `cmake/cm0-package.cmake` installs the official layout (`/usr/bin/power-monitor`,
  APPLaunch desktop/icon, user collector unit, docs) and builds the DEB with CPack.
- `app-builder.json` now carries the official store contract; the Template validator
  is under `.agents/skills/cardputerzero-package-release/`.

Validation performed:

- `cmake --preset darwin-arm64`, `cmake --build --preset darwin-arm64-rel`, and
  `ctest --test-dir build/darwin-arm64 -C Release` passed all five tests.
- `cmake --workflow --preset cp0-cross-package` completed configure, Release build, and
  DEB generation on macOS after installing Homebrew `dpkg`. CPack only enables its DEB
  generator on macOS when `dpkg` is in `PATH`.
- The BSP build used GCC 15.2 with the BSP's glibc 2.41 / libstdc++ 3.4.33. The binary
  requires `libstdc++.so.6`, `libm.so.6`, `libgcc_s.so.1`, and `libc.so.6`; its highest
  version references are `GLIBC_2.38` and `GLIBCXX_3.4.32`, both available on the device.
- `dist/PowerMonitor_0.1.0_rev1_arm64.deb` was parsed with `dpkg-deb -I` and
  `dpkg-deb -c`; the official validator passed with the package path.
- The package was installed with `dpkg -i` on the Debian 13 device. The installed
  `/usr/bin/power-monitor` ran `--demo --capture` successfully, the per-user
  `power-monitor-collector.service` started from `/usr/bin/power-monitor --collect`,
  and `--status` reported active collection.
- With `APPLaunch.service` temporarily stopped, `timeout 5 /usr/bin/power-monitor --demo`
  ran until timeout (exit 124), confirming the cross-built fbdev/evdev path opened the
  real display and keyboard. The launcher was restored and is active.

Not validated: physical key presses, full launcher navigation/selection, Home handling,
24-hour soak, actual suspend/resume, and power overhead. The store source repository,
author GitHub identity, and `share_code` in `app-builder.json` are provisional and must be
confirmed before an upload.
