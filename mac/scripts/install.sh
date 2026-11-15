#!/bin/bash
# Build Goxvi.app, install it into ~/Library/Input Methods/, ad-hoc sign it and
# register/enable the input source (PDR D9). Usage: install.sh [release]
# Exit != 0 only if build, copy, codesign or TISRegisterInputSource fails;
# "--status" is informational (plan.md "Kiem thu tay").
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
PRESET="mac-debug"
if [[ "${1:-}" == "release" ]]; then
  PRESET="mac-release"
fi
SRC_APP="$REPO_ROOT/build/$PRESET/mac/Goxvi.app"
DEST_DIR="$HOME/Library/Input Methods"
DEST_APP="$DEST_DIR/Goxvi.app"

cd "$REPO_ROOT"
cmake --preset "$PRESET"
cmake --build --preset "$PRESET" --target goxvi-mac -j "$(sysctl -n hw.ncpu)"

# Kill the running IMK server so macOS launches the new binary next time.
killall Goxvi 2>/dev/null || true

mkdir -p "$DEST_DIR"
rm -rf "$DEST_APP"
cp -R "$SRC_APP" "$DEST_APP"
codesign --force --deep --sign - "$DEST_APP"

"$DEST_APP/Contents/MacOS/Goxvi" --install

STATUS_LINE="$("$DEST_APP/Contents/MacOS/Goxvi" --status || true)"
echo "$STATUS_LINE"
if [[ "$STATUS_LINE" == "registered=1 enabled=1" ]]; then
  echo "status: ok"
else
  echo "status: pending-human (add Goxvi in System Settings > Keyboard > Input Sources, or log out/in)"
fi
