#!/bin/bash
# Build eqviz.app and eqviz.dmg (into dist/), optionally installing to ~/Applications.
#   scripts/package_mac.sh            build + dmg
#   scripts/package_mac.sh --install  also copy eqviz.app to ~/Applications
set -euo pipefail
cd "$(dirname "$0")/.."
ROOT=$(pwd)

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release >/dev/null
cmake --build build -j 8 --target eqviz

# Icon: rendered by the app itself, then turned into an .icns
if [ ! -f packaging/eqviz.icns ]; then
  echo "Rendering icon..."
  ICONSET=$(mktemp -d)/eqviz.iconset
  mkdir -p "$ICONSET"
  ./build/eqviz.app/Contents/MacOS/eqviz --render-icon "$ICONSET/master.png"
  for s in 16 32 128 256 512; do
    sips -z $s $s "$ICONSET/master.png" --out "$ICONSET/icon_${s}x${s}.png" >/dev/null
    sips -z $((s*2)) $((s*2)) "$ICONSET/master.png" --out "$ICONSET/icon_${s}x${s}@2x.png" >/dev/null
  done
  rm "$ICONSET/master.png"
  iconutil -c icns "$ICONSET" -o packaging/eqviz.icns
  cmake -S . -B build >/dev/null           # pick up the icon
  cmake --build build -j 8 --target eqviz
fi

APP=build/eqviz.app
codesign --force --deep --sign - "$APP" 2>/dev/null  # ad-hoc signature (local use)

mkdir -p dist
rm -rf dist/eqviz.app dist/eqviz.dmg
cp -R "$APP" dist/
STAGE=$(mktemp -d)
cp -R "$APP" "$STAGE/"
ln -s /Applications "$STAGE/Applications"
hdiutil create -volname eqviz -srcfolder "$STAGE" -ov -format UDZO dist/eqviz.dmg >/dev/null
echo "Built $ROOT/dist/eqviz.app and $ROOT/dist/eqviz.dmg"

if [ "${1:-}" = "--install" ]; then
  mkdir -p "$HOME/Applications"
  rm -rf "$HOME/Applications/eqviz.app"
  cp -R "$APP" "$HOME/Applications/"
  echo "Installed $HOME/Applications/eqviz.app"
fi
