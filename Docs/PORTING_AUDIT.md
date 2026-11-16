# SuperTuxKart source and Xbox 360 porting audit

**Audit date:** 2026-11-16  
**Upstream inspected:** `supertuxkart/stk-code`, moving `master` branch  
**Status:** source reconnaissance only; no upstream source or assets have been copied into this repository.

## Important version rule

Do not build the port against an unpinned `master` checkout. Before importing code, record an exact upstream commit SHA and matching asset revision, then keep those revisions together. The upstream branch can change independently of this project.

## What the current upstream build tells us

The current upstream `CMakeLists.txt` shows that STK is not a small standalone game loop. Its build includes or links several substantial systems:

| Area | Upstream component(s) | Porting consequence |
| --- | --- | --- |
| Physics | Bullet dynamics, collision, math | Preserve the existing physics first; measure CPU cost before changing behavior |
| Network transport | ENet | Check socket and platform-network compatibility; protocol compatibility is a separate test |
| Rendering | STK graphics engine and graphics utilities | This is the largest Redraw integration area; desktop graphics assumptions cannot be carried over unchanged |
| Input | SDL2 gamepad support | Replace or adapt the input backend for Xbox 360 controller APIs |
| Audio | OpenAL or bundled MojoAL, Ogg/Vorbis | Provide a console audio backend and validate codec/runtime support |
| Scripting | AngelScript and mcpp | Keep script compatibility; verify compiler/runtime assumptions |
| Text and localization | FreeType, HarfBuzz, tinygettext | Check font memory, glyph coverage, and text layout performance |
| Images and textures | JPEG, PNG, Squish, zlib and related utilities | Audit actual asset formats and texture memory; add Xbox-friendly conversion only where needed |
| Online services/security | libcurl and OpenSSL or MbedTLS | Audit service calls and TLS support; do not assume existing services will accept a console build |
| Optional systems | SQLite, recorder, WiiUse, shaderc and others | Disable or replace only after tracing whether game code depends on them |

This is a dependency inventory from the upstream build configuration, not a statement that every dependency is required in the Xbox build. Some are optional, desktop-only, or replaceable.

## Porting strategy

1. **Pin the source and assets.** Record the STK commit, asset revision, licenses, and notices before importing either.
2. **Map platform boundaries.** Locate window/lifecycle, input, filesystem, audio, graphics, networking, timing, and threading calls in the pinned source.
3. **Bring up Redraw independently.** The repository now has a pinned xsdk-based XEX build path and an early Xenos renderer implementation. Its native XEX still needs hardware verification and it is not yet complete enough for STK's full materials/scene system.
4. **Preserve gameplay systems.** Keep STK race rules, kart behavior, AI, items, tracks, and Bullet physics intact wherever possible. Adapt interfaces instead of rewriting game logic prematurely.
5. **Integrate one subsystem at a time.** Input and filesystem first, then renderer and audio, then network services.
6. **Profile on hardware.** Set budgets for frame time, memory, texture residency, draw calls, and audio before selecting quality settings.

## Redraw integration boundary

Redraw should own GPU-facing resources and draw submission. STK-facing code should translate scene/material/camera data into Redraw calls. The game layer should not include Xbox-specific headers.

The initial interface should stay deliberately small:
- device initialization and shutdown
- frame begin, clear, and present
- view/projection matrices
- vertex/index mesh creation and release
- texture creation and release
- material state and indexed draw submission

Advanced effects, dynamic shadows, post-processing, and shader variants are deferred until a basic scene is stable. Static/baked lighting and conservative shadow options are preferred initially.

## Current platform bring-up

- The STK source baseline is pinned in `Docs/UPSTREAM.md`; the matching historical runtime asset revision and per-file license inventory remain unverified.
- `xenosworks/xsdk` is pinned in `Build/Build-Xbox360.sh` as the current community homebrew SDK/toolchain. `Docs/XBOX360_TOOLCHAIN.md` records its scope and build steps.
- `Redraw/Xbox360/XsdkRenderDevice.cpp` implements an initial Xenos GPU backend for frame presentation, indexed meshes, a basic shader pair, one 32-bit texture format, depth/culling/blending state, and basic controller input in its renderer test title.
- `STK/Source/SPMReader.cpp` is a portable little-endian reader for the official STK SPM mesh format. It has passed both a synthetic fixture and real upstream Adiumy/Black Forest SPM files at SVN revision 18627. Adiumy is skinned; Black Forest's track mesh is static.
- `STK/Source/SPMRedrawBridge.cpp` converts SPM LODs into Redraw vertices and splits large index streams to the current 65,535-index Xenos draw limit.
- `STK/Source/TrackDataReader.cpp` parses track quads, graph edges, and basic track metadata. It has parsed Black Forest's 1,807 quads and main loop (0-1054). `STK/Source/KartDynamics.cpp` adds a first-pass arcade movement model with on-road checks; this is not STK's Bullet physics.
- The Xbox title entry point has been updated to load the official Black Forest track mesh, Adiumy kart mesh, and track quads/graph, then use a first-pass arcade controller model. Its integrated XEX build is still being verified; it is not yet hardware-confirmed as playable. This is a playable-slice target, not the complete STK game. It still lacks STK's Bullet physics, textured materials, skeletal animation, menus, music/SFX, AI, items, and multiplayer.

## Not decided yet

- Which historical STK SVN asset revision matches the pinned source commit
- How the full STK platform, graphics, input, storage, audio, and network dependencies will be ported or replaced
- Whether a given upstream dependency can be reused, needs a compatibility layer, or must be replaced
- Whether existing STK multiplayer services are compatible

## Explicitly out of scope for this milestone

There is no in-game updater, update client, update server, or automatic update workflow in the project plan. Revisit that only if a later release strategy requires multiple supported versions.
