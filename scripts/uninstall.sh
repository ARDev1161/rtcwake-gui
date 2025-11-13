#!/usr/bin/env bash
set -euo pipefail

PREFIX=${HOME}/.local/bin
SERVICE_DST=${HOME}/.config/systemd/user/rtcwake-daemon.service
DESKTOP_DST=${HOME}/.local/share/applications/rtcwake-gui.desktop
ICON_DST=${HOME}/.local/share/icons/hicolor/scalable/apps/rtcwake-gui.svg
DESKTOP_USER_DIR=$(xdg-user-dir DESKTOP 2>/dev/null || echo "${HOME}/Desktop")
DESKTOP_USER_FILE=${DESKTOP_USER_DIR}/rtcwake-gui.desktop

read -r -p "Disable rtcwake-daemon systemd user service? [Y/n] " reply
reply=${reply:-Y}
if [[ ${reply} =~ ^[Yy]$ ]]; then
  systemctl --user disable --now rtcwake-daemon.service || true
fi

rm -f "${PREFIX}/rtcwake-gui" "${PREFIX}/rtcwake-daemon"
rm -f "${SERVICE_DST}" "${DESKTOP_DST}" "${ICON_DST}" "${DESKTOP_USER_FILE}"

echo "Uninstall complete."
