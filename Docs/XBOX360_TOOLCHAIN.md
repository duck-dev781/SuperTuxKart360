# Xbox 360 homebrew toolchain

## Selected toolchain

The first reproducible target build uses [xenosworks/xsdk](https://github.com/xenosworks/xsdk), pinned to:

- Repository: `https://github.com/xenosworks/xsdk`
- Commit: `b08bb27350b67518f986bc60e72f7e4e6ebe6094`
- License: MIT (check the pinned upstream `LICENSE` and `THIRD_PARTY_NOTICES.md` before redistribution)

This is a community-built Xbox 360 homebrew SDK. It provides a PowerPC/Xenon toolchain, XEX packaging, controller and system APIs, audio, storage, and a low-level Xenos GPU API. It is not Microsoft's XDK and does not contain leaked Microsoft SDK material.

The build also pins the shader compiler fork used by xsdk:

- Repository: `https://github.com/xenosworks/glslang-xenon`
- Commit: `d2f00185cf5feaa449f6e15c1894fb232b2b6bcd`

Both repositories are fetched by `Build/Build-Xbox360.sh`; neither is copied into this repository. Their exact revisions are kept in that script so CI builds are reproducible.

## Requirements and target

- Linux build host (GitHub Actions uses Ubuntu).
- A console configured to run unsigned homebrew XEX titles. xsdk documents RGH/JTAG as its supported retail-kernel environment; a standard unmodified retail console is not a target for these unsigned builds.
- An FTP or USB method to transfer the resulting XEX to the console.
- Actual console testing is still required. A successful cross-compile does not prove a title runs correctly on hardware.

No proprietary SDK, console firmware, signing keys, or credentials are included or requested.

## Build

From the repository root on Debian/Ubuntu:

```bash
bash Build/Build-Xbox360.sh
```

The script:
1. Checks out the pinned xsdk and glslang-xenon commits.
2. Builds the PowerPC/Xenon compiler and xsdk runtime.
3. Builds the Redraw XEX renderer test using the real Xenos GPU API.
4. Places the resulting `RedrawRendererTest.xex` in `Build/Xbox360/Artifacts/`.

GitHub Actions runs the same script and uploads the XEX as the `redraw-xbox360-xex` artifact.

## What this milestone proves

The Redraw renderer test is a native Xbox 360 XEX. It opens the display and Xenos GPU, compiles and loads shaders, allocates vertex/index buffers, submits indexed geometry, resolves frames, and reads the left stick and Back button through the console input API.

It is a renderer/platform bring-up test, **not the SuperTuxKart game**. It does not yet load STK kart or track assets, implement STK physics, audio, menus, race flow, AI, or networking. Those remain integration work. The test intentionally does not introduce replacement game art or claim to be a playable STK build.

## Next integration steps

1. Verify this XEX on the target console and record its display/input results.
2. Extend Redraw's GPU resource and material support to cover STK's material/mesh requirements.
3. Port the official STK SPM mesh path and texture formats, preserving upstream notices.
4. Connect STK's scene submission, camera, and race loop to Redraw.
5. Port the remaining platform services (filesystem, audio, timing, threading, and any required networking).
6. Profile memory and frame time on actual Xbox 360 hardware before enabling expensive effects.

The source and runtime assets remain pinned separately in `Docs/UPSTREAM.md` and `Docs/ASSETS.md`. The full official asset checkout is not committed into this repository.
