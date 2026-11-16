#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
BINARY=${1:-"$ROOT/build-device/power-monitor"}
: "${MAINTAINER:?Set MAINTAINER to your name and email before creating a release package}"
python3 - "$BINARY" <<'PY'
import pathlib,struct,sys
p=pathlib.Path(sys.argv[1]); data=p.read_bytes()[:20]
if len(data)<20 or data[:4]!=b'\x7fELF' or data[4:6]!=b'\x02\x01' or struct.unpack_from('<H',data,18)[0]!=183:
    raise SystemExit('Refusing to package: expected a Linux AArch64 ELF binary')
PY
python3 "$ROOT/scripts/make_applauncher_package.py" \
  --app-name 'Power Monitor' --package power-monitor --version 0.1.0 \
  --binary "$BINARY" --icon "$ROOT/applaunch/share/images/power-monitor.png" \
  --description 'ASCII battery power monitor for Cardputer Zero' \
  --maintainer "$MAINTAINER" --out "$ROOT/dist" --deb
