# Changelog

## 0.9.6 - 2026-06-12

Compared against `main` because this repository does not currently have a
local or remote `develop` branch.

### Added

- Reworked the web UI with a polished responsive layout, richer game state
  badges, improved progress details, smoother action menus, and clearer
  compression/uncompression dialogs.
- Added compression destination choices for keeping output on the current
  storage, writing to internal SSD, or writing directly to detected external
  storage targets.
- Added external storage discovery for `/mnt/ext0`, `/mnt/ext1`, and
  `/mnt/usb0` through `/mnt/usb7`, including free-space checks and target
  labels in the UI.
- Added move and copy operations between internal storage and external storage,
  including API routes for move/copy to USB and move/copy back to internal SSD.
- Added compression profiles: `space` for smaller output and `fast` for faster
  compression.
- Added destructive stream compression with resumable journal support and a
  minimum-free-space guard for low-space workflows.
- Added validate-only, read-speed-test, read-EOF-test, delete-game-data,
  original-restore, and uncompress-plan API flows.
- Added persistent size caching and background folder-size measurement so large
  title listings do not block the UI.
- Added ShadowMountPlus hint/config management for compressed PFS/exFAT images,
  source scan requests, forced remounts, and restart of a running ShadowMountPlus
  payload when required.
- Added runtime handoff/status state used to resume or report work after a
  browser refresh or payload restart.

### Changed

- Compression now supports optimized worker scheduling, raw-block handling, and
  zlib/miniz/runtime encoder configuration through the Makefile.
- Progress reporting now includes elapsed time, estimated remaining time, speed,
  block counts, repair counters, stream-budget counters, and richer phase names.
- History entries now preserve operation details such as format, profile,
  destination, target root, preserved originals, repair summaries, and read-test
  metrics.
- Repair and validation paths now use `/data/GameCompressor/logs/repair`, with
  runtime data under `/data/GameCompressor`.
- Payload linking now includes notification and IPMI libraries for completion
  notifications and power/idle guard behavior.
- README runtime paths were corrected to match the actual deployed
  `/data/GameCompressor` layout.

### Fixed

- Fixed source deletion ordering so ShadowMountPlus does not see duplicate
  image/source entries during final commit.
- Fixed USB/external-storage compression and transfer handling.
- Fixed parallel writer ordering for USB output cases.
- Improved recovery behavior for interrupted compression, validation, repair,
  remount, and move/copy operations.

## 0.9.5 - 2026-06-07

- Initial public release metadata for the standalone PS5 Game Compressor
  payload.
