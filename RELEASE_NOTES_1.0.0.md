# Game Compressor 1.0.0

Game Compressor 1.0.0 is focused on stability, safer storage operations, and
better handling of real PS5 setups with USB/external storage and duplicate title
locations.

Compared against `v0.9.6`.

## Highlights

- Fixed crashes and UI stalls caused by scanning large USB-hosted game folders.
- Fixed operations on duplicate title IDs by targeting the selected source path,
  not only the title ID.
- Fixed ShadowMountPlus refresh/remount handling for duplicate or moved game
  instances.
- Improved USB/external compression and uncompression by safely pipelining
  read/write work only when source and destination are on different physical
  devices.
- Added persistent background size estimates so large folder scans no longer
  block normal library refreshes.
- Added safer cancellation behavior for destructive compression and delete
  operations.
- Added a dedicated Delete Game Data flow with confirmation, cache cleanup, and
  ShadowMountPlus rescan support.
- Improved operation history with source/output paths, target roots, saved
  space, scan summaries, repair details, and read-test metrics.
- Improved PFSC compression/decompression internals, including parallel folder
  scanning and windowed decompression with safe fallbacks.
- Polished the UI with clearer loading states, current-output indicators,
  multiple-location labels, better USB target labels, and clearer progress
  phases.

## Upgrade Notes

- Runtime data still lives under:

  ```text
  /data/GameCompressor
  ```

- Size estimates are stored in:

  ```text
  /data/GameCompressor/size-cache.jsonl
  ```

- No rebuilt ShadowMountPlus binary is required. Game Compressor uses supported
  ShadowMountPlus config and manual-list files.

See `CHANGELOG.md` for the full detailed change list.

