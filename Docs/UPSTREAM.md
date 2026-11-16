# Upstream source pin

**Project:** SuperTuxKart360  
**Upstream repository:** https://github.com/supertuxkart/stk-code  
**Branch inspected:** `master`  
**Pinned source commit:** `b2a9f223326422aa2dd4eea28319b0c5115fde03`  
**Commit date:** 2026-11-03  
**Commit subject:** Minor physics cleanup  
**Commit verification:** GitHub reports the commit signature as verified.

## Scope of this pin

This records an exact source-code baseline for repeatable investigation. It does **not** mean upstream source has been copied into this repository or that the Xbox 360 port builds against it.

Use this immutable source URL when inspecting the pinned code:

https://github.com/supertuxkart/stk-code/tree/b2a9f223326422aa2dd4eea28319b0c5115fde03

## Runtime asset revision

The official runtime assets are stored separately in the SuperTuxKart SourceForge SVN repository:

`https://svn.code.sf.net/p/supertuxkart/code/stk-assets`

The full asset-fetch workflow has successfully checked out and packaged the runtime asset tree; its recorded SVN revision was **18627** in the successful 2026-11-16 run. The initial Xbox drive-slice build now explicitly pins **18627** for the selected Adiumy kart and Black Forest track folders. That is a reproducible asset input for this prototype, not proof that revision 18627 is the historically matching asset revision for the source commit above. The full asset-fetch script still defaults to the repository's `HEAD`.

Do not describe source commit `b2a9f223326422aa2dd4eea28319b0c5115fde03` plus asset revision 18627 as a historically matched upstream pair until the upstream history and asset compatibility have been checked. The current SPM and track-XML parsers have been tested against the selected revision's Adiumy and Black Forest files, which proves those formats can be decoded, not that every asset matches the pinned game source.

## Next verification tasks

1. Determine which SVN asset revision corresponds to the selected STK source baseline.
2. Pin that revision in `Build/Fetch-STK-Assets.sh` once compatibility is established.
3. Inventory asset and bundled-component licenses before importing or redistributing files.
4. Re-run the dependency audit against the pinned source commit rather than a moving branch.

## Upstream references

- Source repository: https://github.com/supertuxkart/stk-code
- Pinned commit: https://github.com/supertuxkart/stk-code/commit/b2a9f223326422aa2dd4eea28319b0c5115fde03
- Official source-control documentation: https://supertuxkart.github.io/stk-website/Source_control
