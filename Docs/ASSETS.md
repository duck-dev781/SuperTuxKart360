# Official SuperTuxKart runtime assets

This project uses the **actual runtime data** maintained by the SuperTuxKart team. The official project keeps source code in GitHub and runtime data in a separate Subversion repository; the artist media repository is not the runtime data package. The runtime data includes the shipped karts, tracks, textures, libraries, music, and sound effects.

## Fetch the real source and data

From the project root, run:

```bash
bash Build/Fetch-STK-Assets.sh
```

This creates:

```text
External/
  stk-code/       official upstream source checkout
  stk-assets/     official runtime assets checkout
  stk-upstream-lock.txt
```

The script checks out the source at the recorded baseline commit and checks out the runtime assets from the official SVN repository. It verifies the main runtime asset directories and records the exact source commit and SVN revision actually fetched. Set `STK_SOURCE_REF` and `STK_ASSETS_REV` to override those defaults.

**Revision note:** the source baseline is pinned, but the matching historical SVN asset revision has not yet been independently established. Until that pairing is verified, the script records the actual SVN revision it fetched rather than claiming the two revisions are a tested compatible pair.

## Why the data is fetched separately

The official project deliberately separates its large binary runtime data from the Git source repository. We therefore fetch the upstream assets into the build workspace rather than duplicating hundreds of megabytes of third-party binary data in this port repository. The asset files remain the upstream files; this project does not replace them with placeholders.

## Asset use and redistribution

Keep the upstream directory structure and all notices/metadata intact. The source-code license does not automatically determine the license of every model, texture, font, music track, or sound effect. Before redistributing a packaged build, inventory the relevant per-file notices and comply with each asset's terms. Do not remove attribution or upstream license files.

## Layout expected by upstream

The upstream build expects `stk-code` and `stk-assets` to be sibling directories. The fetch script preserves that layout under `External/`.
