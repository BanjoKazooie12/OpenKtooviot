#!/bin/bash
# Build OpenKtooviot from this source tree and install it for the current user
# on Ubuntu or another Debian-based distribution.
#
# usage: tools/install-ubuntu.sh [--skip-deps]
#   --skip-deps   don't install build dependencies with apt (e.g. on a re-run)
#
# Installs to ~/.local by default; set PREFIX to install elsewhere.

set -euo pipefail

cd "$(dirname "$(readlink -f "$0")")/.."
PREFIX="${PREFIX:-$HOME/.local}"
BUILD_DIR=build-release

if [ "${1:-}" != "--skip-deps" ]; then
	echo "==> Installing build dependencies (this asks for your password)"
	sudo apt-get update
	sudo apt-get install -y build-essential pkg-config meson ninja-build gettext intltool \
		libfontconfig1-dev libass-dev libboost-chrono-dev libboost-locale-dev \
		libboost-regex-dev libboost-system-dev libboost-thread-dev zlib1g-dev \
		wx3.2-headers libwxgtk3.2-dev icu-devtools libicu-dev libpulse-dev \
		libasound2-dev libopenal-dev libffms2-dev libfftw3-dev libhunspell-dev \
		libuchardet-dev libcurl4-gnutls-dev libgl1-mesa-dev libgtest-dev \
		libgmock-dev libportal-gtk3-dev hunspell-he
fi

echo "==> Configuring"
if [ -d "$BUILD_DIR" ]; then
	meson setup --reconfigure "$BUILD_DIR" --buildtype=release --prefix="$PREFIX"
else
	meson setup "$BUILD_DIR" --buildtype=release --prefix="$PREFIX"
fi

echo "==> Building (this takes a few minutes the first time)"
meson compile -C "$BUILD_DIR"

echo "==> Installing to $PREFIX"
meson install -C "$BUILD_DIR" --quiet

# The app menu may not have ~/.local/bin on its PATH until the next login, so
# point the menu entry at the program directly
desktop="$PREFIX/share/applications/org.aegisub.Aegisub.desktop"
if [ -f "$desktop" ]; then
	sed -i -E -e "s|^Exec=aegisub|Exec=$PREFIX/bin/aegisub|" -e 's/^Name(\[[^]]*\])?=.*/Name\1=OpenKtooviot/' "$desktop"
	command -v update-desktop-database >/dev/null && update-desktop-database "$PREFIX/share/applications" || true
fi

echo
echo "OpenKtooviot is installed. Open it from the app menu, or run: $PREFIX/bin/aegisub"
