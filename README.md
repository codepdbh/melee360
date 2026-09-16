# MELEE360

Experimental native Xbox 360 porting workspace for the reconstructed Super
Smash Bros. Melee codebase. Proprietary game data is never stored in Git and
is read from the user's legally obtained disc image.

The current target is the libxenon platform test, not the complete game.

```sh
./tools/setup_xenon.sh --check
./tools/build_x360.sh
```

On Windows, run the scripts from WSL. The build script can use either a native
`DEVKITXENON` installation or the official `free60/libxenon` Docker image.

