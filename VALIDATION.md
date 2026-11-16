# v0.1 validation

Validated locally on macOS with AppleClang 21 and LVGL v9.5.0.

- Core test: real temporary sysfs files, supplied 3.722 V / -384 mA = 1.429248 W, capacity/temperature/ETA units, charging ETA suppression, real zero, missing values, malformed numbers, overflow, invalid capacity, ETA sentinels, rolling eviction, missed seconds, same-second updates, stale expiration, Esc threshold/release/tick rollover, fixed-width printable ASCII frames.
- SDL lifecycle: a quick Esc press does not exit; physical Esc hold exits without repeat; SDL quit exits without double-free after LVGL teardown. SDL dummy video backend runs these without opening a desktop window.
- Actual LVGL renders checked at 320×170: demo history, supplied battery snapshot, missing sensor, help. PNGs are conversions of application's PPM captures, not a mockup or browser reimplementation.
- Device input uses evdev's standard Enter/Esc mappings. LVGL's default evdev translator does not map alphabetic keys; H is only a desktop convenience. SDL synthesizes key releases, so its physical Esc state is observed separately for hold timing.
- Launcher metadata has a relative executable and icon path with Terminal=false; wrapper is executable; icon is a 256×256 RGB PNG.
- Packaging guard rejects the desktop Mach-O binary as an arm64 release input.

Not validated: Linux AArch64 compilation/linkage, real framebuffer pixel format, physical keyboard, launcher reload/icon/return/Home handling, battery read permissions and actual power overhead. No arm64 package has been built or deployed. PUBLISH.md tracks remaining release inputs.

Device follow-up: user reports Debian 13/aarch64 and `/proc/fb` entry `0 panel-mipi-dbid`. Discovery now accepts that exact driver name as well as `fb_st7789v`; the existing 320×170 ioctl check remains. This Linux-only change has not yet been executed on the device.
