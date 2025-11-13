#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
BUILD_DIR=${PROJECT_ROOT}/build
PREFIX=${HOME}/.local/bin
SERVICE_SRC=${PROJECT_ROOT}/systemd/rtcwake-daemon.service
SERVICE_DST=${HOME}/.config/systemd/user/rtcwake-daemon.service
DESKTOP_SRC=${PROJECT_ROOT}/resources/rtcwake-gui.desktop
DESKTOP_DST=${HOME}/.local/share/applications/rtcwake-gui.desktop
ICON_SRC=${PROJECT_ROOT}/resources/icons/clock.svg
ICON_DST=${HOME}/.local/share/icons/hicolor/scalable/apps/rtcwake-gui.svg
DESKTOP_USER_DIR=$(xdg-user-dir DESKTOP 2>/dev/null || echo "${HOME}/Desktop")

read -r -p "Build Plasma widget? [y/N] " build_plasma
CMAKE_ARGS="-DCMAKE_BUILD_TYPE=Release"
if [[ ${build_plasma:-N} =~ ^[Yy]$ ]]; then
  CMAKE_ARGS="${CMAKE_ARGS} -DBUILD_PLASMA_WIDGET=ON"
else
  CMAKE_ARGS="${CMAKE_ARGS} -DBUILD_PLASMA_WIDGET=OFF"
fi

# Check rtcwake permission
if ! rtcwake --version >/dev/null 2>&1; then
  echo "rtcwake not found in PATH. Please install util-linux package."
  exit 1
fi

if ! rtcwake -m no -s 60 >/dev/null 2>&1; then
  echo "Warning: current user cannot run rtcwake without elevated privileges."
  read -r -p "Re-run this installer with sudo? [y/N] " sudo_retry
  if [[ ${sudo_retry:-N} =~ ^[Yy]$ ]]; then
    exec sudo bash "$0"
  fi
fi

cmake -S "${PROJECT_ROOT}" -B "${BUILD_DIR}" ${CMAKE_ARGS}
cmake --build "${BUILD_DIR}"
cmake --build "${BUILD_DIR}" --target rtcwake-configrepo-test rtcwake-scheduleplanner-test
(cd "${BUILD_DIR}" && ctest --output-on-failure)

mkdir -p "${PREFIX}"
install -m 755 "${BUILD_DIR}/src/rtcwake-gui" "${PREFIX}/rtcwake-gui"
if [ -f "${BUILD_DIR}/src/rtcwake-daemon" ]; then
  install -m 755 "${BUILD_DIR}/src/rtcwake-daemon" "${PREFIX}/rtcwake-daemon"
fi

mkdir -p "$(dirname "${SERVICE_DST}")"
if [ -f "${SERVICE_SRC}" ]; then
  sed "s|ExecStart=.*|ExecStart=${PREFIX}/rtcwake-daemon|" "${SERVICE_SRC}" > "${SERVICE_DST}"
fi

if [ -f "${DESKTOP_SRC}" ]; then
  mkdir -p "$(dirname "${DESKTOP_DST}")"
  install -m 644 "${DESKTOP_SRC}" "${DESKTOP_DST}"
  read -r -p "Create desktop shortcut in ${DESKTOP_USER_DIR}? [y/N] " answer
  if [[ ${answer:-N} =~ ^[Yy]$ ]]; then
    mkdir -p "${DESKTOP_USER_DIR}"
    install -m 744 "${DESKTOP_SRC}" "${DESKTOP_USER_DIR}/rtcwake-gui.desktop"
  fi
fi

if [ -f "${ICON_SRC}" ]; then
  mkdir -p "$(dirname "${ICON_DST}")"
  install -m 644 "${ICON_SRC}" "${ICON_DST}"
fi

echo "Binaries installed to ${PREFIX}."
if [ -x "${PREFIX}/rtcwake-daemon" ]; then
  read -r -p "Enable rtcwake-daemon systemd user service now? [Y/n] " reply
  reply=${reply:-Y}
  if [[ ${reply} =~ ^[Yy]$ ]]; then
    systemctl --user daemon-reload || true
    systemctl --user enable --now rtcwake-daemon.service || true
  fi
fi
