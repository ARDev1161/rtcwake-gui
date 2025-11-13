# rtcwake-gui

**ru:** Графический планировщик для `rtcwake`, позволяющий выставлять разовые и недельные будильники, выбирать способ завершения работы и показывать предупреждающий баннер с таймером и кнопками "Применить" / "Отложить".

**en:** Friendly Qt5 desktop app that helps you arm `rtcwake`, pick the shutdown/suspend strategy, and review the next alarm from a Plasma widget.

## Highlights
- Dual time input: classic `QTimeEdit` field plus an analog clock preview.
- Single-shot and weekly schedules with automatic selection of the nearest enabled day.
- Dynamic power-state radio buttons (power off / suspend / hibernate / freeze) based on `/sys/power/state`.
- Optional warning banner with customizable message, countdown, and snooze interval.
- Activity log with full command lines so you can copy/paste them into the terminal.
- Optional Plasma widget (`org.kde.plasma.rtcwakeplanner`) that shows the next alarm and launches the planner.
- JSON status exported to `~/.local/share/rtcwake-gui/next-wake.json` for integrations.

## Requirements
- Qt 5.12+ (Widgets, Core, Gui, optionally Quick for the Plasma package).
- CMake 3.16+ and a C++17 compiler.
- `rtcwake` and sufficient privileges (usually root via `sudo` or `polkit`).
- Optional: KDE Plasma 5.24+ for the plasmoid, Doxygen for docs generation.

## Building
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Options:
- `-DBUILD_PLASMA_WIDGET=ON` — installable Plasma widget (requires KF5Plasma + QtQuick).
- `-DENABLE_DOXYGEN=ON` — generates API docs under `build/docs`.

Run the application from `build/src/rtcwake-gui` (must have permission to call `rtcwake`).

## Usage
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

## Notes & Caveats
- `rtcwake` needs elevated privileges on most systems; run the GUI under `sudo` or configure Polkit rules accordingly.
- The weekly view schedules only the closest next occurrence. Re-open the app (or rely on automation) to re-arm future alarms.
- Plasma integration is optional; non-Plasma users can ignore `BUILD_PLASMA_WIDGET`.

Enjoy and wake on time!
