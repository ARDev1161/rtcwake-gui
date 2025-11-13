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
DESKTOP_USER_DIR=$(sudo -u "${TARGET_USER}" xdg-user-dir DESKTOP 2>/dev/null || echo "${TARGET_HOME}/Desktop")
DESKTOP_USER_FILE=${DESKTOP_USER_DIR}/rtcwake-gui.desktop

PREFIX=/usr/local/bin
SERVICE_DST=/etc/systemd/system/rtcwake-daemon.service
DESKTOP_DST=/usr/share/applications/rtcwake-gui.desktop
ICON_DST=/usr/share/icons/hicolor/scalable/apps/rtcwake-gui.svg

read -r -p "Disable system rtcwake-daemon service? [Y/n] " reply
reply=${reply:-Y}
if [[ ${reply} =~ ^[Yy]$ ]]; then
  systemctl disable --now rtcwake-daemon.service || true
fi

rm -f "${PREFIX}/rtcwake-gui" "${PREFIX}/rtcwake-daemon"
rm -f "${SERVICE_DST}" "${DESKTOP_DST}" "${ICON_DST}" "${DESKTOP_USER_FILE}"

systemctl daemon-reload || true

echo "Uninstall complete."
