#!/bin/bash
# Builds a distributable macOS release of Goxvi (PDR D10):
#   universal (Apple Silicon + Intel) Goxvi.app, signed with Developer ID +
#   hardened runtime, notarized by Apple, stapled, packed as
#   dist/goxvi-<version>-macos.dmg (+ .sha256). Opening Goxvi.app from the DMG
#   installs it into ~/Library/Input Methods (mac/src/self-install.mm).
#
# One-time setup (same as nostyra/scripts/release-macos.sh):
#   - "Developer ID Application" certificate of $GOXVI_TEAM_ID in the login keychain.
#   - notarytool credentials: xcrun notarytool store-credentials "<profile>" \
#       --apple-id <email> --team-id <team> --password <app-specific password>
#
# Env overrides: GOXVI_TEAM_ID, GOXVI_NOTARY_PROFILE, SKIP_NOTARIZE=1 (sign only).
set -euo pipefail

cd "$(dirname "$0")/../.."
ROOT=$(pwd)

TEAM="${GOXVI_TEAM_ID:-LVJ9374KGW}"
PROFILE="${GOXVI_NOTARY_PROFILE:-macxa}"
VERSION=$(tr -d '[:space:]' < VERSION)
BUILD="$ROOT/build/mac-dist"
APP="$BUILD/mac/Goxvi.app"
DIST="$ROOT/dist"
DMG="$DIST/goxvi-$VERSION-macos.dmg"

echo "▸ Checking prerequisites (version $VERSION)"
# Pick the Developer ID of exactly this team: a machine may hold several.
DEV_ID=$(security find-identity -v -p codesigning | grep "Developer ID Application" \
         | grep -F "($TEAM)" | awk '{print $2}' | head -1 || true)
[ -n "$DEV_ID" ] || { echo "  ✗ no 'Developer ID Application' certificate of team $TEAM"; exit 1; }
echo "  certificate: $(security find-identity -v -p codesigning | grep "$DEV_ID" | head -1 | cut -d'"' -f2)"
if [ "${SKIP_NOTARIZE:-0}" != "1" ]; then
    xcrun notarytool history --keychain-profile "$PROFILE" >/dev/null 2>&1 \
        || { echo "  ✗ notarytool profile '$PROFILE' missing (see header)"; exit 1; }
    echo "  notary profile: $PROFILE"
fi

echo "▸ Building universal Goxvi.app"
cmake -S . -B "$BUILD" -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" >/dev/null
rm -rf "$APP"
cmake --build "$BUILD" --target goxvi-mac -j "$(sysctl -n hw.ncpu)" >/dev/null
[ -d "$APP" ] || { echo "  ✗ $APP not found after build"; exit 1; }
lipo -info "$APP/Contents/MacOS/Goxvi" | sed 's/^/  /'

echo "▸ Signing (Developer ID + hardened runtime)"
codesign --force --timestamp --options runtime --sign "$DEV_ID" "$APP"
codesign --verify --strict --deep --verbose=2 "$APP" 2>&1 | sed 's/^/  /'

notarize() { # $1 = file to submit
    local log; log=$(mktemp)
    if ! xcrun notarytool submit "$1" --keychain-profile "$PROFILE" --wait 2>&1 | tee "$log" | sed 's/^/  /' \
       || ! grep -q "status: Accepted" "$log"; then
        local id; id=$(awk '/^  id:/{print $2; exit}' "$log")
        echo "  ✗ notarization failed. Details: xcrun notarytool log $id --keychain-profile $PROFILE"
        exit 1
    fi
}

mkdir -p "$DIST"
if [ "${SKIP_NOTARIZE:-0}" != "1" ]; then
    echo "▸ Notarizing the app"
    ZIP="$DIST/goxvi-notarize.zip"
    ditto -c -k --keepParent "$APP" "$ZIP"   # ditto keeps the signature intact
    notarize "$ZIP"
    rm -f "$ZIP"
    xcrun stapler staple "$APP" | sed 's/^/  /'
fi

echo "▸ Packing DMG"
STAGE=$(mktemp -d)
cp -R "$APP" "$STAGE/"
rm -f "$DMG"
hdiutil create -volname "Goxvi $VERSION" -srcfolder "$STAGE" -ov -format UDZO "$DMG" >/dev/null
rm -rf "$STAGE"
codesign --force --timestamp --sign "$DEV_ID" "$DMG"

if [ "${SKIP_NOTARIZE:-0}" != "1" ]; then
    echo "▸ Notarizing the DMG"
    notarize "$DMG"
    xcrun stapler staple "$DMG" | sed 's/^/  /'
    spctl --assess --type open --context context:primary-signature -v "$DMG" 2>&1 | sed 's/^/  /'
fi
(cd "$DIST" && shasum -a 256 "$(basename "$DMG")" > "$(basename "$DMG").sha256")
echo "✓ $DMG"
