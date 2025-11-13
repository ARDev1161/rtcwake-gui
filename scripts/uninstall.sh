#!/usr/bin/env bash
set -euo pipefail

PREFIX=${HOME}/.local/bin
SERVICE_DST=${HOME}/.config/systemd/user/rtcwake-daemon.service
DESKTOP_DST=${HOME}/.local/share/applications/rtcwake-gui.desktop
ICON_DST=${HOME}/.local/share/icons/hicolor/scalable/apps/rtcwake-gui.svg

read -r -p "Disable rtcwake-daemon systemd user service? [Y/n] " reply
reply=${reply:-Y}
if [[ ${reply} =~ ^[Yy]$ ]]; then
  systemctl --user disable --now rtcwake-daemon.service || true
fi

rm -f "${PREFIX}/rtcwake-gui" "${PREFIX}/rtcwake-daemon"
rm -f "${SERVICE_DST}" "${DESKTOP_DST}" "${ICON_DST}"

echo "Uninstall complete."
