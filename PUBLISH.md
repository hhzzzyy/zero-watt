# Application Store Publish Record

Keep this file in the application project root. Update it when package
metadata, listing copy, assets, privacy choices, or submission status changes.
The machine-readable contract is `app-builder.json`; the build/package contract
is `cmake/cm0-package.cmake`.

Portal: <https://dev.cardputer.cc/#/upload>

## Package

- Debian package: `dist/PowerMonitor_0.1.0_rev1_arm64.deb`
- Package name: `power-monitor`
- Version: `0.1.0`, revision `rev1`
- Architecture: `arm64`
- Maintainer: `hzy <573493999@qq.com>`
- APPLaunch desktop: `/usr/share/APPLaunch/applications/power-monitor.desktop`
- Executable: `/usr/bin/power-monitor`
- User collector unit: `/usr/lib/systemd/user/power-monitor-collector.service`

## Source and Ownership

- Public source repository: **TODO** — planned as `https://github.com/hzy/cardputer-power-monitor`; that URL currently returns 404, so create/push the repository before upload.
- Expected GitHub uploader/owner: `hzy` (confirm this is the actual GitHub identity)
- License: MIT
- Store share code: `PWRM` (provisional; confirm it is still unique in the store before first upload)

## Listing

- Application name: Power Monitor
- One-line summary (maximum 80 characters): ASCII battery power monitor with a 60-second history for Cardputer Zero
- Description: LVGL character interface displaying battery power, current, voltage, capacity, temperature and discharge ETA from the BQ27220 sysfs, with a 60-second live graph and full-screen history.
- Categories (maximum 2 from the store enum): System Tools, Hardware & IoT
- Permissions: background service only; camera, microphone, IMU, network, additional hardware, and external display are false.

## Assets

- 256 x 256 source icon: `applaunch/share/images/power-monitor.png`
- Packaged icon: `/usr/share/APPLaunch/share/images/power-monitor.png`
- Screenshot 1 (320 x 170): `preview/device-compact-home.png`
- Screenshot 2 (320 x 170): `preview/device-h-help.png`
- Screenshot 3 (320 x 170): `preview/device-history.png`
- Screenshot 4 (320 x 170): `preview/device-settings.png`

## Preflight

- [x] `cmake --workflow --preset cp0-cross-package` succeeds.
- [x] `.deb` exists and can be parsed with `dpkg-deb -I` and `dpkg-deb -c`.
- [x] Control fields contain Package, Version, Architecture, Maintainer, and Depends.
- [x] Architecture is `arm64`.
- [x] APPLaunch `.desktop`, executable, PNG icon, and user collector unit exist inside the package.
- [x] `cmake -DPACKAGE_FILE=dist/PowerMonitor_0.1.0_rev1_arm64.deb -P .agents/skills/cardputerzero-package-release/scripts/validate_app_builder.cmake` passes.
- [ ] `app-builder.json` source repo exists publicly, author identity is confirmed, and `share_code` uniqueness is confirmed.
- [ ] Listing copy and categories have been reviewed.
- [ ] Four clean 320 x 170 baseline screenshots describe the current UI.
- [x] No credentials, tokens, or private server details are embedded.
- [ ] The user explicitly approved external submission.

## Submission Result

- Status: `not submitted`
- Submitted at: TODO
- Portal message: TODO
- Actions URL: TODO
- Tracking or pull-request URL: TODO
