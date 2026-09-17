# Dolphin DVD compatibility

MELEE360 provides a small Dolphin DVD API compatibility layer backed by its
portable raw GCM reader. The goal is to keep reconstructed game code calling
the original public API while the Xbox 360 implementation reads from a legal
disc image on FAT storage.

## Implemented

- `DVDInit`
- `DVDConvertPathToEntrynum`
- `DVDFastOpen`
- `DVDOpen`
- `DVDClose`
- `DVDReadPrio`
- `DVDReadAsyncPrio` (currently completes synchronously before invoking the callback)
- `DVDGetFileInfoStatus`
- `DVDGetTransferredSize`
- `DVDGetCurrentDiskID`
- `DVDCheckDisk`

The compatibility layer accepts the original 32-byte-rounded reads used by
Melee, including up to 31 bytes of disc padding beyond a file's logical size.

## Validation

Run:

```bash
./tools/test_gcm.sh
```

The test mounts the user's ignored `MeleeUSAv1.02.iso`, resolves
`/opening.bnr`, opens it by FST entry number, reads it through `DVDReadPrio`
and validates the `BNR1` signature. No copyrighted content is copied into the
repository or test output.

## Next work

Directory traversal, cancellation, seeking, streaming and a true asynchronous
worker queue remain unimplemented. The immediate consumer work is adapting
Melee's `lbfile`/`lbdvd` request path to this first synchronous backend.
