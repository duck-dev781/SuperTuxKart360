# SuperTuxKart360

An experimental Xbox 360 homebrew porting project built around the original **SuperTuxKart (STK)** game, with a purpose-built lightweight rendering/platform layer called **Redraw**.

> **Status: early porting foundation.** The repository now has a pinned open Xbox 360 homebrew toolchain path and a Redraw-to-Xenos renderer test target. That target is a hardware bring-up test, not a playable SuperTuxKart build. The complete STK source/assets, game loop, physics, menus, audio, and race integration are not yet running on Xbox 360.

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
  Xbox360/              Xenos renderer backend and bring-up title
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

## Official runtime assets

The port fetches the **real upstream runtime asset repository** (not replacement content and not the artist-only media repository) alongside the official STK source. Run `bash Build/Fetch-STK-Assets.sh` to populate `External/stk-code` and `External/stk-assets`, verify the main asset groups, and record the exact upstream revisions. See [Docs/ASSETS.md](Docs/ASSETS.md). The large binary asset checkout is kept out of this Git repository and fetched into the build workspace from the official SuperTuxKart SVN host. The source commit is pinned; the compatible historical asset revision and complete per-file license inventory remain open tasks.

## Redraw design principles

Redraw is intended to be lightweight and scalable rather than imitate a desktop renderer feature-for-feature.

- Prefer efficient scene submission, batching, and conservative memory use.
- Use baked or simplified lighting and shadows where dynamic effects are too expensive.
- Support compressed textures and sensible texture/mipmap budgets.
- Scale particles, post-processing, and shadow quality to measured performance.
- Keep the game-facing interfaces separate from Xbox 360-specific implementation details.

The Xenos backend currently covers a narrow bring-up path: frame lifecycle, indexed mesh buffers, one 32-bit texture format, a basic shader pair, depth/culling/blending state, and display presentation. The current title entry point is being extended into a first drive slice that loads the official Black Forest track mesh and Adiumy kart mesh, then applies a small controller-driven movement model. Textures/material scripts, skeletal animation, and STK's full scene systems are still missing. The renderer and platform layer must be validated on actual target hardware; no specific frame rate or visual feature is guaranteed.

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

## Xbox 360 build path

The initial target build uses [xenosworks/xsdk](https://github.com/xenosworks/xsdk), pinned in `Build/Build-Xbox360.sh`, to produce native Xbox 360 XEX files without Microsoft's proprietary XDK. The pinned build path targets homebrew-enabled RGH/JTAG environments supported by xsdk; it does not produce a signed retail-console title. See [Docs/XBOX360_TOOLCHAIN.md](Docs/XBOX360_TOOLCHAIN.md).

Run:

```bash
bash Build/Build-Xbox360.sh
```

GitHub Actions runs the same build and uploads the Redraw renderer test XEX. A successful cross-compile still needs testing on the actual console. Do not store proprietary SDK files, signing material, or credentials in the repository.

Do not assume that a standard desktop STK build can simply be recompiled for Xbox 360. Dependencies, filesystem access, threading, audio, input, graphics, and networking need platform-specific work.

## Third-party code and assets

SuperTuxKart is a separate upstream project. Its source code is distributed under GPL-3.0-or-later; individual assets and bundled components may have different licenses and attribution requirements. Before copying or redistributing any source or asset, record its exact upstream origin, license, copyright notice, and required attribution in `LICENSES/`.

The Xbox 360 build path also fetches xsdk and glslang-xenon at immutable commits. Their upstream licenses and third-party notices must be preserved in any redistributed build. This repository does not claim ownership of SuperTuxKart or its assets. Redraw's own license will be selected and documented before substantial original code is published.

## Development milestones

- [x] Create the repository structure and technical plan.
- [x] Audit the pinned upstream source's major dependency groups.
- [x] Pin the STK source-code baseline.
- [ ] Verify a matching historical STK asset revision and complete the per-file asset-license inventory.
- [x] Implement a little-endian SPM reader and validate official Adiumy and Black Forest meshes.
- [x] Add SPM-to-Redraw mesh conversion with safe 65,535-index batching.
- [x] Parse Black Forest drive quads/graph and test a first-pass controller-driven kart motion model.
- [ ] Verify the native XEX build on an actual Xbox 360.
- [ ] Load and render the official Black Forest track and Adiumy kart from the XEX.
- [ ] Complete Redraw's texture, mesh, material, and scene support for STK.
- [ ] Port STK's windowless/platform lifecycle, input, filesystem, audio, and graphics layers.
- [ ] Bring up an official STK kart and track scene on target hardware.
- [ ] Connect the STK race loop, physics, menus, and original game content.
- [ ] Profile and test a complete playable race on the console.
- [ ] Test LAN multiplayer, then investigate server interoperability.

## Release scope

This project targets the complete SuperTuxKart game. It is not a demo build. No in-game updater, update client, update server, or automatic update workflow is currently planned; update support can be reconsidered only if a future multi-version release strategy needs it.

## Technical notes

See [the porting audit](Docs/PORTING_AUDIT.md), [the Xbox 360 toolchain notes](Docs/XBOX360_TOOLCHAIN.md), and [the renderer contract](Redraw/Renderer/IRenderDevice.h). The Xenos backend implementation is in `Redraw/Xbox360/`; the test title is only a renderer/input bring-up test.

## Continuous integration

GitHub Actions has three separate responsibilities:

- compile and run the portable C++03 renderer/controller contract tests;
- fetch and package the pinned STK source and official runtime assets;
- build the Xenos renderer test as a native Xbox 360 XEX with the pinned xsdk toolchain.

The XEX workflow now builds a first playable drive slice with the official Adiumy kart and Black Forest track asset folders packaged beside the XEX. It still does **not** build the complete STK game, and a successful cross-compile does not prove that the title runs on a physical console.

## Contributing

Keep changes small and testable. Document the hardware, toolchain, build command, and test result for platform-specific changes. Never report a feature as working unless it has been tested on the stated target.
