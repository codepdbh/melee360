# Source-port fidelity

The target is the original Melee behavior, assets and presentation. This is
a source port. A playable approximation is an intermediate state, not the
finished game.

The current game-source checkout is `upstream/melee-pc`, revision
`6c2f5c527d0484b4d20d9d31ac6e0a30fa22eeac`. Game data comes from the user's
original `GALE01` image. Neither the image nor extracted Nintendo assets
belong in Git.

## Implementation criteria

- Compile original game functions and tables when their dependencies are
  supported. Platform adapters supply Xbox services while preserving the
  original function's inputs, outputs and side effects.
- Retain original frame timing, inputs, random-number behavior, encounter
  conditions, collision rules, animations, voices, music and sound selection.
  Do not change these to help an automated test pass.
- Derive stage coordinates, collision, markers and materials from the original
  archives and source. Missing hazards, cameras, menus and cinematics remain
  missing features until their original behavior is ported.
- Label native substitutes and unfinished paths in the remaining-work report.
  Host tests establish specific rules; native tests establish the observed
  route. Neither establishes visual/audio parity without comparison.
- Keep test pilots, deterministic seed settings and diagnostic scene shortcuts
  out of the normal live-input executable. Test pilots operate through PAD
  inputs; they do not change fighter position, damage, lives or goal placement.

## Current limits

The full original collision engine and stage callbacks are not running.
The native campaign flow still omits original presentation and several course
systems. Classic is a five-normal-fight preview, with its special encounters,
bonus stages and bosses unfinished. Adventure's mountain climb, race traffic,
maze hazards and transition animations remain unfinished. Rendering and console
behavior also require comparison and validation.

See [remaining work](PLAYABLE_PORT_GAPS.md) and [test evidence](STATUS.md).
