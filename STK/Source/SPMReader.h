#ifndef STK360_SPM_READER_H
#define STK360_SPM_READER_H

#include <string>
#include <vector>

namespace stk360
{
    struct SPMMaterial
    {
        std::string texture1;
        std::string texture2;
    };

    struct SPMVertex
    {
        float position[3];
        float normal[3];
        float uv[2];
        unsigned char color[4];
    };

    struct SPMSubMesh
    {
        unsigned int lod_index;
        unsigned int material_index;
        std::vector<SPMVertex> vertices;
        std::vector<unsigned short> indices;
    };

    struct SPMModel
    {
        float bounds[6];
        bool has_normals;
        bool has_vertex_colors;
        bool requires_skinning;
        std::vector<SPMMaterial> materials;
        std::vector<SPMSubMesh> submeshes;

        SPMModel();
        void clear();
    };

    enum SPMReadResult
    {
        SPM_READ_OK,
        SPM_READ_INVALID_ARGUMENT,
        SPM_READ_TRUNCATED,
        SPM_READ_BAD_HEADER,
        SPM_READ_UNSUPPORTED_VERSION,
        SPM_READ_UNSUPPORTED_VARIANT,
        SPM_READ_INVALID_DATA,
        SPM_READ_LIMIT_EXCEEDED
    };

    class SPMReader
    {
    public:
        // Reads the little-endian STK SPM format. SPMA meshes are decoded as
        // geometry but marked requires_skinning; animation is not evaluated.
        // SPMS space-partitioned meshes are not supported by this reader.
        static SPMReadResult read(const unsigned char* bytes,
                                  unsigned int size,
                                  SPMModel* output,
                                  std::string* error_message = 0);
    };
}

#endif
