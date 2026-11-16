# SuperTuxKart360

An experimental Xbox 360 homebrew porting project built around the original **SuperTuxKart (STK)** game, with a purpose-built lightweight rendering/platform layer called **Redraw**.

> **Status: early porting foundation.** The repository now contains an upstream dependency audit and a platform-neutral Redraw renderer interface. It does not yet contain a working port, compiled Xbox 360 executable, or verified multiplayer connection.

## Project goals

- Port the original SuperTuxKart game and preserve its identity, gameplay, tracks, karts, characters, items, physics, audio, menus, and AI wherever technically and legally possible.
- Develop **Redraw**, an Xbox 360-oriented rendering layer designed around the console's hardware limits.
- Add Xbox 360 controller support and a console-friendly interface.
- Investigate LAN multiplayer, custom servers, and compatibility with existing STK online services. Compatibility is a research goal, not a confirmed feature.

## Repository layout

```text
Redraw/                 Rendering and engine support
  Core/                 Platform-independent engine utilities
  Renderer/             Rendering backend and scene submission
  Shadows/              Shadow techniques and configuration
  Textures/             Texture loading and format handling
  Xbox360/              Xbox 360-specific renderer/platform code
STK/                    SuperTuxKart integration
  Source/               Game source integration
  Assets/               Asset manifests and integration notes
  Physics/              Physics integration and tuning
  Multiplayer/          Network and multiplayer integration
Platform/
  Xbox360/               Console input, storage, lifecycle, and system glue
Build/
  Xbox360/               Build scripts, configuration, and packaging
LICENSES/                Third-party license and attribution records
```

## Content preservation and Xbox-specific customization

The target is the complete original SuperTuxKart experience, not a recreation with replacement content.

- **Original STK content:** use the official upstream karts, characters, tracks, models, textures, menus, music, sound effects, and other game content wherever technically possible and permitted by each item's license.
- **Original game behavior:** preserve the upstream race modes, kart physics, items, AI, progression, and menu flows wherever possible.
- **Redraw renderer:** replace/adapt the rendering backend for Xbox 360. Redraw changes the rendering implementation; it is not intended to replace STK's art direction or content.
- **Controller support:** provide native Xbox 360 controller handling, sensible default mappings, and configurable bindings.
- **Xbox-specific settings:** add a separate settings area for Redraw and console options, such as resolution/display behavior where supported, graphics quality, shadow quality, effects, performance options, and controller configuration. Keep original STK gameplay settings and menu behavior intact where possible.

### Asset and audio licensing

"Official assets" means the original assets distributed by the SuperTuxKart project, not automatically unrestricted assets. Track the exact source revision, asset package revision, per-file license, copyright notice, and attribution requirements before copying or redistributing content. Preserve upstream notices and do not assume the source-code license applies identically to every sound, image, model, font, or other bundled component.

## Redraw design principles

Redraw is intended to be lightweight and scalable rather than imitate a desktop renderer feature-for-feature.

- Prefer efficient scene submission, batching, and conservative memory use.
- Use baked or simplified lighting and shadows where dynamic effects are too expensive.
- Support compressed textures and sensible texture/mipmap budgets.
- Scale particles, post-processing, and shadow quality to measured performance.
- Keep the game-facing interfaces separate from Xbox 360-specific implementation details.

The renderer and platform layer must be validated on actual target hardware. No specific frame rate or visual feature is guaranteed at this stage.

## Controller plan

Initial Xbox controller mapping:

| Control | Planned action |
| --- | --- |
| Left stick | Steer |
| RT | Accelerate |
| LT | Brake / reverse |
| A | Jump / drift action, subject to STK input design |
| X | Use item |
| B | Look behind |
| Start | Pause |

The mapping will be configurable where practical.

## Multiplayer investigation

The project will first identify the exact STK networking version, protocol, authentication flow, master-server dependencies, and service availability. Then it can assess:

1. LAN play between compatible builds.
2. A self-hosted/custom server.
3. Interoperability with existing STK multiplayer services.

A console port may require changes to sockets, TLS, certificates, DNS, serialization, timing, and platform networking. Existing-server compatibility must not be advertised until it is tested end-to-end.

## Build and hardware assumptions

The target is Xbox 360 homebrew. A usable build depends on a compatible C++ toolchain, platform libraries, and lawful access to the required development environment. The exact build path will be documented once the available toolchain and target console environment are confirmed.

Do not assume that a standard desktop STK build can simply be recompiled for Xbox 360. Dependencies, filesystem access, threading, audio, input, graphics, and networking will need platform-specific work.

## Third-party code and assets

SuperTuxKart is a separate upstream project. Its source code is distributed under GPL-3.0-or-later; individual assets and bundled components may have different licenses and attribution requirements. Before copying or redistributing any source or asset, record its exact upstream origin, license, copyright notice, and required attribution in `LICENSES/`.

This repository does not claim ownership of SuperTuxKart or its assets. Redraw's own license will be selected and documented before substantial original code is published. Do not remove upstream notices or assume every asset shares the source-code license.

## Development milestones

- [x] Create the repository structure and technical plan.
- [x] Audit the current upstream build configuration and identify major dependency groups. Exact source and asset revisions still need to be pinned before importing code.
- [ ] Record the pinned STK source revision and matching asset revision, with a complete asset-license inventory.
- [ ] Establish a reproducible Xbox 360 toolchain/build environment.
- [x] Define the initial platform-neutral Redraw renderer interface.
- [ ] Implement a backend and minimal renderer test on a confirmed toolchain.
- [ ] Port the windowless/platform lifecycle, input, filesystem, audio, and graphics layers.
- [ ] Bring up a minimal STK scene on target hardware.
- [ ] Add race flow, controller mapping, assets, and performance profiling.
- [ ] Test LAN multiplayer, then investigate server interoperability.

## Release scope

This project targets the complete SuperTuxKart game. It is not a demo build. No in-game updater, update client, update server, or automatic update workflow is currently planned; update support can be reconsidered only if a future multi-version release strategy needs it.

## Technical notes

See [the porting audit](Docs/PORTING_AUDIT.md) for the upstream dependency review and integration strategy. The initial renderer contract is in `Redraw/Renderer/IRenderDevice.h`; it is an interface scaffold, not a completed renderer.

## Contributing

Keep changes small and testable. Document the hardware, toolchain, build command, and test result for platform-specific changes. Never report a feature as working unless it has been tested on the stated target.
