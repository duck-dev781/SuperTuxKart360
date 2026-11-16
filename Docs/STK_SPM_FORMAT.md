# SuperTuxKart SPM mesh reader

The upstream source uses a custom binary `.spm` mesh format. The implementation in `STK/Source/SPMReader.cpp` reads the file's little-endian fields explicitly, so it does not depend on the host CPU's byte order. This matters for Xbox 360's big-endian PowerPC CPU: upstream's current `SPMeshLoader` explicitly refuses non-little-endian hosts.

## Source basis

The format details were checked against the pinned STK source:

- `src/graphics/sp_mesh_loader.cpp`: file header, material names, LOD/submesh records, vertex/index decoding.
- `lib/graphics_engine/include/mini_glm.hpp`: packed 10:10:10 normal decoding and half-float conversion.
- `lib/irrlicht/include/S3DVertex.h`: packed SPM vertex fields.

The reader is a separate implementation; it does not copy the upstream loader's Irrlicht or graphics-engine dependencies.

## Decoded fields

- Version 1 SPM header and variant.
- Six model bounds.
- Material texture-name pairs.
- LOD and submesh records.
- Little-endian 32-bit positions.
- Packed 10:10:10 signed normals.
- Optional RGB vertex colors.
- Optional half-float UV coordinates.
- Optional second UV set and tangent data (the second set/tangent are consumed but not yet exposed).
- 8-bit or 16-bit triangle indices.
- Generated normals when the file has no normal channel.

The output keeps each submesh's LOD index and material index. A later STK-to-Redraw bridge must preserve the material grouping and select one LOD at a time; drawing every LOD together would be incorrect.

## Official asset validation

At SVN revision **18627**, the portable reader successfully parsed these real upstream assets:

| Asset | Materials | Submeshes | Vertices | Indices | Skinning |
| --- | ---: | ---: | ---: | ---: | --- |
| `karts/adiumy/adiumy.spm` | 4 | 4 | 4,710 | 19,746 | Required |
| `tracks/black_forest/black_forest_track.spm` | 64 | 64 | 61,227 | 145,743 | Not required |

SHA-256:

- `adiumy.spm`: `30518d2f8e03ab56b5e042c58f73b96f1630c6aa1c65a08c35dd4ec4b280f2c2`
- `black_forest_track.spm`: `a6cb15db7eb0841cae8160cb50220d1bdfec1eccdc24d21e6e02c8785393c8a1`

The CI validation fetches only those two SPM files and their per-asset `licenses.txt` files; it does not redistribute the asset files.

## SPM-to-Redraw bridge

`STK/Source/SPMRedrawBridge.cpp` converts one selected LOD into Redraw's vertex layout and splits index streams into batches of at most 65,535 indices, matching the current Xenos backend's per-draw limit.

- Static meshes can be converted directly.
- Skinned meshes are rejected unless the caller explicitly opts into a static, unevaluated base pose.
- The bridge preserves material and LOD IDs but does not yet load the referenced textures or interpret STK's full material definitions.
- Vertex colors and second UV sets are currently not passed through to Redraw.

## Current limitations

- `SPMS` space-partitioned meshes are rejected.
- `SPMA` skinned meshes have their base geometry decoded and are marked `requires_skinning`; joint animation and skinning are not evaluated yet.
- The reader does not load image files, apply STK material scripts, or upload mesh data to the GPU.
- The bridge and renderer have not yet been connected to STK's live scene submission.
- Only the selected kart and track files above have been validated against real assets; the rest of the official asset repository still needs coverage.

Run the portable tests with:

```bash
g++ -std=c++03 -Wall -Wextra -Werror -pedantic \
  STK/Source/SPMReader.cpp STK/Tests/spm_reader_test.cpp \
  -o spm-reader-test
./spm-reader-test

g++ -std=c++03 -Wall -Wextra -Werror -pedantic \
  STK/Source/SPMReader.cpp STK/Source/SPMRedrawBridge.cpp \
  STK/Tests/spm_redraw_bridge_test.cpp -o spm-redraw-bridge-test
./spm-redraw-bridge-test
```
