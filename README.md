# MELEE360

Experimental native Xbox 360 porting workspace for the reconstructed Super
Smash Bros. Melee codebase. Proprietary game data is never stored in Git and
is read from the user's legally obtained disc image.

The current target is the libxenon platform test, not the complete game.

```sh
./tools/setup_xenon.sh --check
./tools/build_x360.sh
```

The resulting `dist/xenon.elf` can be placed at the root of a FAT32 USB drive
for XeLL Reloaded. The platform test initializes Xenos, draws a test triangle,
polls analog controller state, plays a short synthetic tone, and probes common
read-only locations for a `GALE01` disc image.

On Windows, run the scripts from WSL. The build script can use either a native
`DEVKITXENON` installation or the official `free60/libxenon` Docker image.
