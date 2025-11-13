#!/usr/bin/env bash
set -euo pipefail

if [[ $EUID -ne 0 ]]; then
  echo "Please run this script with sudo."
  exit 1
fi

TARGET_USER=${SUDO_USER:-$(logname 2>/dev/null || root)}
TARGET_HOME=$(eval echo "~${TARGET_USER}")
if [[ ! -d ${TARGET_HOME} ]]; then
  TARGET_HOME="/home/${TARGET_USER}"
fi

PROJECT_ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
BUILD_DIR=${PROJECT_ROOT}/build
PREFIX=/usr/local/bin
SERVICE_SRC=${PROJECT_ROOT}/systemd/rtcwake-daemon.service
SERVICE_DST=/etc/systemd/system/rtcwake-daemon.service
DESKTOP_SRC=${PROJECT_ROOT}/resources/rtcwake-gui.desktop
DESKTOP_DST=/usr/share/applications/rtcwake-gui.desktop
ICON_SRC=${PROJECT_ROOT}/resources/icons/clock.svg
ICON_DST=/usr/share/icons/hicolor/scalable/apps/rtcwake-gui.svg
DESKTOP_USER_DIR=$(sudo -u "${TARGET_USER}" xdg-user-dir DESKTOP 2>/dev/null || echo "${TARGET_HOME}/Desktop")

read -r -p "Build Plasma widget? [y/N] " build_plasma
CMAKE_ARGS="-DCMAKE_BUILD_TYPE=Release"
if [[ ${build_plasma:-N} =~ ^[Yy]$ ]]; then
  CMAKE_ARGS+=" -DBUILD_PLASMA_WIDGET=ON"
else
  CMAKE_ARGS+=" -DBUILD_PLASMA_WIDGET=OFF"
fi

if ! command -v rtcwake >/dev/null 2>&1; then
  echo "rtcwake not found in PATH. Please install util-linux package."
  exit 1
fi

if ! sudo -u "${TARGET_USER}" rtcwake -m no -s 60 >/dev/null 2>&1; then
  echo "Warning: user ${TARGET_USER} cannot run rtcwake without elevated privileges."
  RTCWAKE_BIN=$(command -v rtcwake)
  read -r -p "Grant rtcwake capability via 'setcap cap_sys_time+ep ${RTCWAKE_BIN}'? [Y/n] " cap_reply
  if [[ ${cap_reply:-Y} =~ ^[Yy]$ ]]; then
    setcap cap_sys_time+ep "${RTCWAKE_BIN}" || true
  else
    echo "Cannot proceed without rtcwake permissions. Aborting."
    exit 1
  fi

  if ! sudo -u "${TARGET_USER}" rtcwake -m no -s 60 >/dev/null 2>&1; then
    echo "rtcwake is still not usable without permissions. Aborting."
    exit 1
  fi
fi

cmake -S "${PROJECT_ROOT}" -B "${BUILD_DIR}" ${CMAKE_ARGS}
cmake --build "${BUILD_DIR}"
cmake --build "${BUILD_DIR}" --target rtcwake-configrepo-test rtcwake-scheduleplanner-test
(cd "${BUILD_DIR}" && ctest --output-on-failure)

install -m 755 "${BUILD_DIR}/src/rtcwake-gui" "${PREFIX}/rtcwake-gui"
if [[ -f "${BUILD_DIR}/src/rtcwake-daemon" ]]; then
  install -m 755 "${BUILD_DIR}/src/rtcwake-daemon" "${PREFIX}/rtcwake-daemon"
fi

if [[ -f "${SERVICE_SRC}" ]]; then
  sed "s|ExecStart=.*|ExecStart=${PREFIX}/rtcwake-daemon|" "${SERVICE_SRC}" > "${SERVICE_DST}"
fi

if [[ -f "${DESKTOP_SRC}" ]]; then
  install -m 644 "${DESKTOP_SRC}" "${DESKTOP_DST}"
  read -r -p "Create desktop shortcut for ${TARGET_USER} in ${DESKTOP_USER_DIR}? [y/N] " answer
  if [[ ${answer:-N} =~ ^[Yy]$ ]]; then
    mkdir -p "${DESKTOP_USER_DIR}"
    install -m 744 "${DESKTOP_SRC}" "${DESKTOP_USER_DIR}/rtcwake-gui.desktop"
    chown "${TARGET_USER}:${TARGET_USER}" "${DESKTOP_USER_DIR}/rtcwake-gui.desktop"
  fi
fi

if [[ -f "${ICON_SRC}" ]]; then
  install -m 644 "${ICON_SRC}" "${ICON_DST}"
fi

echo "Binaries installed to ${PREFIX}."
if [[ -x "${PREFIX}/rtcwake-daemon" ]]; then
  read -r -p "Enable system rtcwake-daemon service now? [Y/n] " reply
  reply=${reply:-Y}
  if [[ ${reply} =~ ^[Yy]$ ]]; then
    systemctl daemon-reload
    systemctl enable --now rtcwake-daemon.service
  fi
fi
