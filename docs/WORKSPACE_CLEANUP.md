# Workspace cleanup — 2026-10-13

The active development target remains the native XDK/XEX port. Cleanup
preserves the root Git repository, all tracked game source, the primary
`upstream/melee-pc` checkout, SDK installation, the current normal XEX and
paired map, Xenia executable/configuration, required generated include overlay,
and the latest cache diagnostic XEX/map. Personal console dump files were
left intact.

The inventory distinguished NTFS hardlinks by file identity. Before cleanup
the folder contained approximately 7.51 GB of logical file lengths but only
4.59 GB of unique files. Afterwards it contains approximately 3.26 GB logical
and 1.80 GB unique. These figures use decimal GB and exclude filesystem
metadata/cluster overhead. Actual drive free space also changes with other
Windows activity.

There is one physical 1,459,978,240-byte Melee ISO. Its remaining names are
`iso/MeleeUSAv1.02.iso` (source/tools) and `dist/melee.iso` (runtime). They have
the same NTFS file identity; they do not consume two ISO allocations. The
temporary `dist/soak0` link and staging directory were removed. The soak
harness recreates its staging hardlink when needed.

Removed 133 obsolete directories: historical diagnostic staging/builds,
old emulator builds and dependencies, Xenia caches, generated host tests and
scratch objects. The optional `tools/xenon-emulator` checkout and clean
`upstream/doldecomp-melee` reference checkout were removed; the native XEX
build does not require them. The legacy emulator had no local branch commits
outside its remotes. Its six modified source files were saved as the complete
diff [local-workspace-20261013.patch](../patches/xenon-emulator/local-workspace-20261013.patch).
A reverse-apply check against the working tree succeeded before deletion.
The base commit is `0284bbe6c8125935d97bf54ab3132089c6c65c8b` from
`https://github.com/xenon-emu/xenon.git`; apply this complete diff to that clean
commit, rather than applying both this diff and the earlier compatibility patch.
The removed clean decomp reference was
`6f6e1ade7c8f723d78c3cdb8f2ca96efd6885dc6` from
`https://github.com/doldecomp/melee.git`.

Historical traces, maps, result tables, logs and scratch source were compressed
into the local ignored `build-x360/evidence-archive-20261013.zip` (about 54 MiB).
All 8,222 entries were read back and checked with SHA-256 against their source
files before deletion. Paths cited in older status sections can now be found
inside this archive. The deletion manifest and inventory are also retained
locally under `build-x360/cleanup-*`.

The current `dist/default.xex` hash is unchanged:
`FF60CE6E11A3674B69A6C3657BB2087318C1FBBA359B00683903E690092D7876`.
`dist/assets/sprite_atlas.png` and the play guide remain beside it.

An automatic tool approval review rejected additional deletion of old XEX
files and root build logs, including a narrower attempt using thirteen
explicit old executable paths. The only stated reason was “blocked by policy”.
Those remaining files and the evidence archive were left in place. Cleanup
does not imply the remaining root build artifacts are all current.

The next full build regenerates scratch objects. Use the retained explicit
`build-x360/vertex-cache/diagnostic.xex` for existing diagnostic runs; cleanup
removed older scratch directories and their temporary incremental scripts.
