# rtcwake-gui

Friendly Qt5 desktop app that helps you arm `rtcwake`, pick the shutdown/suspend strategy, and review the next alarm from a Plasma widget.

## Highlights
- Dual time input: classic `QTimeEdit` field plus an analog clock preview.
- Single-shot and weekly schedules with automatic selection of the nearest enabled day.
- Dynamic power-state radio buttons (power off / suspend / hibernate / freeze) based on `/sys/power/state`.
- Optional warning banner with customizable message, countdown, and snooze interval.
- Activity log with full command lines so you can copy/paste them into the terminal.
- Optional Plasma widget (`org.kde.plasma.rtcwakeplanner`) that shows the next alarm and launches the planner.
- JSON status exported to `~/.local/share/rtcwake-gui/next-wake.json` for integrations, plus a persistent config at `~/.config/rtcwake-gui/config.json`.
- Optional background daemon that keeps the next wake alarm armed even if the GUI is not opened.

## Requirements
- Qt 5.12+ (Widgets, Core, Gui, optionally Quick for the Plasma package).
- CMake 3.16+ and a C++17 compiler.
- `rtcwake` and sufficient privileges (usually root via `sudo` or `polkit`).
- Optional: KDE Plasma 5.24+ for the plasmoid, Doxygen for docs generation.

## Usage
The fastest way to install and start the daemon is via the helper script:
```bash
./scripts/install.sh
```
It configures the project in Release mode (asking whether to include the optional Plasma widget), checks/patches your `rtcwake` permissions (offering to call `sudo setcap` if needed), builds the GUI + daemon + tests, runs `ctest`, installs binaries into `~/.local/bin`, installs a launcher entry + icon automatically, optionally adds a desktop shortcut (prompted), copies the systemd user unit, and offers to enable the daemon right away. To remove everything later, run `./scripts/uninstall.sh`.

## Manual build
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Options:
- `-DBUILD_PLASMA_WIDGET=ON` — installable Plasma widget (requires KF5Plasma + QtQuick).
- `-DBUILD_DAEMON=OFF` — disable building the daemon if you don't need it.
- `-DENABLE_DOXYGEN=ON` — generates API docs under `build/docs`.

Run the application from `build/src/rtcwake-gui` (must have permission to call `rtcwake`).

## Planner usage
1. **Single Wake tab** — pick date + time, verify on the analog clock, and hit *Schedule wake*.
2. **Weekly Schedule tab** — enable any subset of weekdays and assign times. Press *Schedule next weekly wake* to arm the closest upcoming entry.
3. **Settings tab** — select the power action that should run after programming the RTC and configure the optional warning banner (message, countdown seconds, snooze minutes).
4. The status label and log show the exact `rtcwake` command that was executed.

> Tip: the app writes the next-alarm summary to `~/.local/share/rtcwake-gui/next-wake.json`. The included Plasma widget reads that file and offers quick refresh + a button to launch the planner.

## Plasma widget
```bash
cmake -S . -B build -DBUILD_PLASMA_WIDGET=ON
cmake --build build --target install  # or copy plasma-widget/package manually
```
After installation, add *rtcwake Planner* from the Plasma widgets list. The plasmoid periodically parses the JSON summary and exposes quick *Refresh* / *Open planner* controls.

## Documentation
Enable Doxygen while configuring or run `cmake --build build --target doxygen_docs` once `ENABLE_DOXYGEN=ON`. The generated HTML ends up in `build/docs/html/index.html`.

## Tests
Qt-based unit tests live under `tests/` and cover the configuration serializer plus the schedule planner helper. Enable testing (on by default) and execute:

```bash
cmake --build build --target rtcwake-configrepo-test rtcwake-scheduleplanner-test
ctest --output-on-failure
```

## Daemon & systemd service
The repository ships a lightweight daemon (`rtcwake-daemon`) that re-arms the next wake alarm using your saved config. Build it with the default options or explicitly via:

```bash
cmake --build build --target rtcwake-daemon
```

Install it somewhere on your `$PATH` (for example `~/.local/bin/rtcwake-daemon`) and copy `systemd/rtcwake-daemon.service` to `~/.config/systemd/user/`. Update `ExecStart` if you chose a different install path, then enable it:

```bash
systemctl --user enable --now rtcwake-daemon.service
```

The daemon watches `~/.config/rtcwake-gui/config.json`, re-arms the upcoming wake using `rtcwake -m no`, and refreshes the JSON summary read by the Plasma widget. It does not power down the machine automatically; use the GUI or your own workflow for that part.

## Notes & Caveats
- `rtcwake` needs elevated privileges on most systems; run the GUI under `sudo` or configure Polkit rules accordingly.
- The weekly view schedules only the closest next occurrence. Re-open the app (or rely on automation) to re-arm future alarms.
- Plasma integration is optional; non-Plasma users can ignore `BUILD_PLASMA_WIDGET`.
- All scheduling preferences are stored in `~/.config/rtcwake-gui/config.json`. Remove or edit that file if you need to reset the UI.

Enjoy and wake on time!
