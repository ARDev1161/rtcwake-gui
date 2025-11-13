# Agent Notes

## Code Layout
- `src/` contains the Qt Widgets application entry point and implementation files.
- `include/` mirrors the public headers; keep UI strings in English and prefer Qt-friendly types.
- `plasma-widget/` is an optional plasmoid package installed via `BUILD_PLASMA_WIDGET`.
- `systemd/rtcwake-daemon.service` is the system-wide service template that launches `rtcwake-daemon`.
- `~/.config/rtcwake-gui/config.json` (managed by `ConfigRepository`) keeps the persisted schedules.
- `tests/` contains Qt Test suites (run with `ctest`) for config IO and scheduling logic.

## Build & Test
1. Configure: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`
2. Build: `cmake --build build`
3. (Optional) Docs: reconfigure with `-DENABLE_DOXYGEN=ON`, then run `cmake --build build --target doxygen_docs`.
4. Tests: `cmake --build build --target rtcwake-configrepo-test rtcwake-scheduleplanner-test` then `ctest --output-on-failure`.

## Guidelines
- Target C++17, Qt5 widgets; avoid introducing other GUI toolkits.
- Never remove the analog clock/time edit dual input requirement.
- When touching scheduling logic, update the JSON summary writer so the Plasma widget stays in sync.
- Keep documentation (README.md + Doxygen comments) in sync with UI labels.
- The daemon (`rtcwake-daemon`) must stay headless and rely only on QtCore; the warning helper (`rtcwake-warning`) is the only GUI it spawns.
- Keep the warning helper CLI stable (exit codes: 0 apply, 1 snooze, 2 cancel).
- Always run `ctest` after touching persistence or scheduling code.

## Known Gaps
- Environment capture for the warning helper currently relies on the GUI saving DISPLAY/XDG variables; ensure the GUI runs at least once per user session.
- Multiple users are not yet supported simultaneously by the system daemon.
