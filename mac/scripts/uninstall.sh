#!/bin/bash
# Disable the Goxvi input source, stop the IMK server and remove the bundle.
# Only touches ~/Library/Input Methods/Goxvi.app.
set -euo pipefail

DEST_APP="$HOME/Library/Input Methods/Goxvi.app"

if [[ -x "$DEST_APP/Contents/MacOS/Goxvi" ]]; then
  "$DEST_APP/Contents/MacOS/Goxvi" --uninstall
else
  echo "Goxvi.app not installed, nothing to disable"
fi

killall Goxvi 2>/dev/null || true
rm -rf "$DEST_APP"
echo "removed $DEST_APP"
