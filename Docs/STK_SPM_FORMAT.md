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

## Current limitations

- `SPMS` space-partitioned meshes are rejected.
- `SPMA` skinned meshes have their base geometry decoded and are marked `requires_skinning`; joint animation and skinning are not evaluated yet.
- The reader does not load image files, apply STK material scripts, or upload mesh data to the GPU.
- The Xbox 360 renderer currently has a smaller index-count limit per draw than the reader's format limit. The bridge must split large submeshes into safe draw batches.
- Only a synthetic format fixture is currently tested. Real official kart/track files still need to be fetched and checked before this reader is treated as production-ready.

Run the portable test with:

```bash
g++ -std=c++03 -Wall -Wextra -Werror -pedantic \
  STK/Source/SPMReader.cpp STK/Tests/spm_reader_test.cpp \
  -o spm-reader-test
./spm-reader-test
```
