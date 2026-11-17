# Application Store Publish Record

Keep this file in the application project root. Update it when package
metadata, listing copy, assets, privacy choices, or submission status changes.
The machine-readable contract is `app-builder.json`; the build/package contract
is `cmake/cm0-package.cmake`.

Portal: <https://dev.cardputer.cc/#/upload>

## Package

- Debian package: `dist/ZeroWatt_0.1.0_rev1_arm64.deb`
- Package name: `zero-watt`
- Version: `0.1.0`, revision `rev1`
- Architecture: `arm64`
- Maintainer: `hzy <573493999@qq.com>`
- APPLaunch desktop: `/usr/share/APPLaunch/applications/zero-watt.desktop`
- Executable: `/usr/bin/zero-watt`
- User collector unit: `/usr/lib/systemd/user/zero-watt-collector.service`
- Development upgrade: the package declares `Replaces`/`Conflicts: power-monitor`,
  so `dpkg -i` replaces a previous Power Monitor development install.

## Rename History

2026-11-17: renamed from Power Monitor (`power-monitor`) to Zero Watt (`zero-watt`)
before the first store upload. Store identity, binary, desktop entry, service,
icon, settings directory, and environment variable changed; the provisional
share code moved from `PWRM` to `ZWAT`. Nothing had been published, so there was
no existing store identity to migrate.

## Source and Ownership

- Public source repository: **TODO** — planned as `https://github.com/hzy/zero-watt`; that URL currently returns 404, so create/push the repository before upload.
- Expected GitHub uploader/owner: `hzy` (confirm this is the actual GitHub identity)
- License: MIT
- Store share code: `ZWAT` (provisional; confirm it is still unique in the store before first upload)

## Listing

- Application name: Zero Watt
- One-line summary (maximum 80 characters): ASCII battery power monitor with a 60-second history for Cardputer Zero
- Description: LVGL character interface displaying battery power, current, voltage, capacity, temperature and discharge ETA from the BQ27220 sysfs, with a 60-second live graph and full-screen history.
- Categories (maximum 2 from the store enum): System Tools, Hardware & IoT
- Permissions: background service only; camera, microphone, IMU, network, additional hardware, and external display are false.

## Assets

- 256 x 256 source icon: `applaunch/share/images/zero-watt.png`
- Packaged icon: `/usr/share/APPLaunch/share/images/zero-watt.png`
- Screenshot 1 (320 x 170): `preview/device-compact-home.png`
- Screenshot 2 (320 x 170): `preview/device-h-help.png`
- Screenshot 3 (320 x 170): `preview/device-history.png`
- Screenshot 4 (320 x 170): `preview/device-settings.png`

The four store screenshots predate the rename and still show the `POWER MONITOR`
header. Replace them with fresh device captures (or the current 320 x 170
captures) before uploading.

## Preflight

- [x] `cmake --workflow --preset cp0-cross-package` succeeds.
- [x] `.deb` exists and can be parsed with `dpkg-deb -I` and `dpkg-deb -c`.
- [x] Control fields contain Package, Version, Architecture, Maintainer, and Depends.
- [x] Architecture is `arm64`.
- [x] APPLaunch `.desktop`, executable, PNG icon, and user collector unit exist inside the package.
- [x] `cmake -DPACKAGE_FILE=dist/ZeroWatt_0.1.0_rev1_arm64.deb -P .agents/skills/cardputerzero-package-release/scripts/validate_app_builder.cmake` passes.
- [ ] `app-builder.json` source repo exists publicly, author identity is confirmed, and `share_code` uniqueness is confirmed.
- [ ] Listing copy and categories have been reviewed.
- [ ] Four clean 320 x 170 baseline screenshots describe the current UI (current
      device captures still show the old `POWER MONITOR` header).
- [x] No credentials, tokens, or private server details are embedded.
- [ ] The user explicitly approved external submission.

## Submission Result

- Status: `not submitted`
- Submitted at: TODO
- Portal message: TODO
- Actions URL: TODO
- Tracking or pull-request URL: TODO
