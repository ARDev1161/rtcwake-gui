#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
BUILD_DIR=${PROJECT_ROOT}/build
PREFIX=${HOME}/.local/bin
SERVICE_SRC=${PROJECT_ROOT}/systemd/rtcwake-daemon.service
SERVICE_DST=${HOME}/.config/systemd/user/rtcwake-daemon.service

cmake -S "${PROJECT_ROOT}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release
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

echo "Binaries installed to ${PREFIX}."
if [ -x "${PREFIX}/rtcwake-daemon" ]; then
  read -r -p "Enable rtcwake-daemon systemd user service now? [Y/n] " reply
  reply=${reply:-Y}
  if [[ ${reply} =~ ^[Yy]$ ]]; then
    systemctl --user daemon-reload || true
    systemctl --user enable --now rtcwake-daemon.service || true
  fi
fi
