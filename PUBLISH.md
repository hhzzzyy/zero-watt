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

- Public source repository: `https://github.com/hhzzzyy/zero-watt` (created and pushed 2026-11-17)
- GitHub owner: `hhzzzyy`
- License: MIT
- Store share code: `ZWAT` (not used by any of the 73 apps in the 2026-11-17 registry; confirm again right before upload)

## Listing

- Application name: Zero Watt
- One-line summary (maximum 80 characters): Battery power monitor for Cardputer Zero with a live watt curve and history
- Description: Reads BQ27220 voltage, current, capacity and temperature every second, showing signed charge/discharge watts, net average, a 60-second live curve and a zoomable full-screen history.
- Categories (maximum 2 from the store enum): System Tools, Hardware & IoT
- Category note: the official definition of `Hardware & IoT` covers external sensors, actuators, USB and Grove/EXT modules. Zero Watt only reads the built-in battery, so `System Tools` alone is the safer listing; two categories still pass automated validation.
- Permissions: background service only; camera, microphone, IMU, network, additional hardware, and external display are false.

## Assets

- 256 x 256 source icon: `applaunch/share/images/zero-watt.png`
- Packaged icon: `/usr/share/APPLaunch/share/images/zero-watt.png`
- Screenshot 1 (320 x 170): `preview/device-compact-home.png`
- Screenshot 2 (320 x 170): `preview/device-h-help.png`
- Screenshot 3 (320 x 170): `preview/device-history.png`
- Screenshot 4 (320 x 170): `preview/device-settings.png`

All four store screenshots were recaptured on the device after the rename and
show the current `ZERO WATT` header. Each is a real 320 x 170 RGB565 framebuffer
read, using demo data where the UI is marked `[DEMO]`.

## Preflight

- [x] `cmake --workflow --preset cp0-cross-package` succeeds.
- [x] `.deb` exists and can be parsed with `dpkg-deb -I` and `dpkg-deb -c`.
- [x] Control fields contain Package, Version, Architecture, Maintainer, and Depends.
- [x] Architecture is `arm64`.
- [x] APPLaunch `.desktop`, executable, PNG icon, and user collector unit exist inside the package.
- [x] The renamed package was installed on the device with `dpkg -i`; it replaced
      the `power-monitor` development package and the old collector was stopped.
- [x] `cmake -DPACKAGE_FILE=dist/ZeroWatt_0.1.0_rev1_arm64.deb -P .agents/skills/cardputerzero-package-release/scripts/validate_app_builder.cmake` passes.
- [x] `app-builder.json` source repo and author point at `https://github.com/hhzzzyy/zero-watt`.
- [x] The public repository contains the pushed `main` history.
- [x] `share_code` uniqueness: `ZWAT` is absent from the 2026-11-17 store registry (73 apps).
- [x] The official `store_meta_policy.py` reports no errors and no warnings for the staged metadata.
- [ ] Listing copy reviewed; category choice still open (see the category note above).
- [x] Four clean 320 x 170 baseline screenshots describe the current UI.
- [x] No credentials, tokens, or private server details are embedded.
- [ ] Demo video recorded on the real device (launch, main features, exit) and attached to the submission PR.
- [ ] The user explicitly approved external submission.

## Submission Result

- Status: `not submitted`
- Demo video: TODO
- Submitted at: TODO
- Portal message: TODO
- Actions URL: TODO
- Tracking or pull-request URL: TODO
