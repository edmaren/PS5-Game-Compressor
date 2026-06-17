# Game Compressor 1.0.0

Game Compressor 1.0.0 adds built-in APR Emu indexing for compressed games.

Compared against `v0.9.9`.

## Highlights

- Added APR Emu support before compression. The app now performs the same
  `build_ampr_index.py` work directly on the PS5 before compressing any title
  that uses APR Emu.
- Generates a fresh `ampr_emu.index` during the pre-compress scan, then includes
  that app-generated index inside the compressed PFS or exFAT image.
- Replaces stale existing `ampr_emu.index` files for APR Emu titles so the
  compressed output is built from a clean index.
- Shows an `APR indexed` tag in the game list, selected-game details, and
  operation history when Game Compressor generated the index.
- Cleans common macOS metadata files during pre-compress folder scans, including
  `.DS_Store`, `._*`, `.Spotlight-V100`, and `__MACOSX`.

## Compatibility Notes

- This is handled by Game Compressor on the PS5. Users no longer need to run the
  manual `build_ampr_index.py` script before compressing APR Emu titles.
- APR index generation only runs for titles that need it. Non-APR titles keep
  the existing compression behavior.
- No rebuilt ShadowMountPlus or kstuff binary is required.

See `CHANGELOG.md` for the full detailed change list.
