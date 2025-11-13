# rtcwake-gui

Friendly Qt5 desktop app that helps you arm `rtcwake`, pick the shutdown/suspend strategy, and review the next alarm from a Plasma widget.

## Highlights
- Dual time input with analog clock preview, now covering both shutdown and wake times.
- Single-shot and weekly schedules stored in a shared config that the daemon reads automatically.
- Dynamic power-state radio buttons (power off / suspend / hibernate / freeze) based on `/sys/power/state`.
- Warning banner with customizable message/countdown/snooze, shown by a dedicated helper app the daemon launches.
- Activity log plus JSON status (`~/.local/share/rtcwake-gui/next-wake.json`) for integrations such as the Plasma widget.
- Background daemon re-arms the RTC via `rtcwake -m no`, shows the banner, and executes the selected power action without launching the GUI.

## Requirements
- Qt 5.12+ (Widgets, Core, Gui, optionally Quick for the Plasma package).
- CMake 3.16+ and a C++17 compiler.
- `rtcwake` and sufficient privileges (usually root via `sudo` or `polkit`).
- Optional: KDE Plasma 5.24+ for the plasmoid, Doxygen for docs generation.

## Usage
The fastest way to install and start the daemon is via the helper script:
```bash
sudo ./scripts/install.sh
```
It configures the project in Release mode (asking whether to include the optional Plasma widget), builds the GUI + daemon + warning helper + tests, runs `ctest`, installs binaries into `/usr/local/bin`, installs a launcher entry + icon automatically, optionally adds a desktop shortcut for the invoking user, installs a system-wide `rtcwake-daemon` service (preconfigured with the user config path and warning helper), and offers to enable that service immediately. Remove everything later via:
```bash
sudo ./scripts/uninstall.sh
```

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
1. **Single schedule tab** — pick shutdown date/time plus the wake date/time, verify on the analog clock, and click *Save single schedule*.
2. **Weekly schedule tab** — enable any subset of weekdays, set shutdown and wake times per day, and click *Save weekly schedule*.
3. **Settings tab** — select the post-shutdown action and configure the optional warning banner (message, countdown seconds, snooze minutes).
4. The daemon consumes the saved config and runs unattended; the GUI just edits the JSON.

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

Install it somewhere on your `$PATH` (for example `/usr/local/bin/rtcwake-daemon`) and copy `systemd/rtcwake-daemon.service` to `/etc/systemd/system/`. Update `ExecStart` so it passes the desired `--config`, `--user`, `--home`, and `--warning-app` parameters, then enable it:

```bash
sudo systemctl enable --now rtcwake-daemon.service
```

The daemon watches `~/.config/rtcwake-gui/config.json`, re-arms the upcoming wake using `rtcwake -m no`, launches the warning helper inside the user's session, and only then executes the selected power action (suspend/poweroff/etc.). The GUI no longer runs `rtcwake` itself.

## Notes & Caveats
- `rtcwake` needs elevated privileges on most systems; run the GUI under `sudo` or configure Polkit rules accordingly.
- The weekly view schedules only the closest next occurrence. Re-open the app (or rely on automation) to re-arm future alarms.
- Plasma integration is optional; non-Plasma users can ignore `BUILD_PLASMA_WIDGET`.
- All scheduling preferences are stored in `~/.config/rtcwake-gui/config.json`. Remove or edit that file if you need to reset the UI.

Enjoy and wake on time!
