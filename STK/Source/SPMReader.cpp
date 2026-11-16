#include "SPMReader.h"

#include <cmath>
#include <cstring>

namespace stk360
{
    namespace
    {
        const unsigned int kMaxMaterials = 1024;
        const unsigned int kMaxLods = 64;
        const unsigned int kMaxSubMeshes = 4096;
        const unsigned int kMaxVerticesPerMesh = 65535;
        const unsigned int kMaxIndicesPerMesh = 3000000;

        class Reader
        {
        public:
            Reader(const unsigned char* bytes, unsigned int size)
                : m_bytes(bytes), m_size(size), m_offset(0)
            {
            }

            bool readU8(unsigned char* value)
            {
                if (!value || m_offset >= m_size)
                    return false;
                *value = m_bytes[m_offset++];
                return true;
            }

            bool readU16(unsigned short* value)
            {
                if (!value || remaining() < 2)
                    return false;
                *value = static_cast<unsigned short>(
                    static_cast<unsigned short>(m_bytes[m_offset]) |
                    (static_cast<unsigned short>(m_bytes[m_offset + 1]) << 8));
                m_offset += 2;
                return true;
            }

            bool readU32(unsigned int* value)
            {
                if (!value || remaining() < 4)
                    return false;
                *value = static_cast<unsigned int>(m_bytes[m_offset]) |
                         (static_cast<unsigned int>(m_bytes[m_offset + 1]) << 8) |
                         (static_cast<unsigned int>(m_bytes[m_offset + 2]) << 16) |
                         (static_cast<unsigned int>(m_bytes[m_offset + 3]) << 24);
                m_offset += 4;
                return true;
            }

            bool readFloat(float* value)
            {
                unsigned int bits;
                if (!value || !readU32(&bits))
                    return false;
                std::memcpy(value, &bits, sizeof(bits));
                return true;
            }

            bool readBytes(void* destination, unsigned int count)
            {
                if (count > remaining() || (!destination && count != 0))
                    return false;
                if (count != 0)
                    std::memcpy(destination, m_bytes + m_offset, count);
                m_offset += count;
                return true;
            }

            bool readString(std::string* value)
            {
                unsigned char length;
                if (!value || !readU8(&length) || length > remaining())
                    return false;
                value->assign(reinterpret_cast<const char*>(m_bytes + m_offset),
                              static_cast<std::string::size_type>(length));
                m_offset += length;
                return true;
            }

            unsigned int remaining() const
            {
                return m_size - m_offset;
            }

        private:
            const unsigned char* m_bytes;
            unsigned int m_size;
            unsigned int m_offset;
        };

        float halfToFloat(unsigned short half)
        {
            const unsigned int sign = (half >> 15) & 1u;
            const unsigned int exponent = (half >> 10) & 31u;
            const unsigned int mantissa = half & 1023u;
            float value;

            if (exponent == 0)
                value = mantissa == 0 ? 0.0f :
                    std::ldexp(static_cast<float>(mantissa) / 1024.0f, -14);
            else if (exponent == 31)
                value = mantissa == 0 ? 65504.0f : 0.0f;
            else
                value = std::ldexp(1.0f + static_cast<float>(mantissa) / 1024.0f,
                                  static_cast<int>(exponent) - 15);

            return sign ? -value : value;
        }

        float signed10(unsigned int value)
        {
            value &= 1023u;
            if (value & 512u)
                return -static_cast<float>(1024u - value) / 512.0f;
            return static_cast<float>(value) / 511.0f;
        }

        void normalize(float normal[3])
        {
            const float length_squared = normal[0] * normal[0] +
                                         normal[1] * normal[1] +
                                         normal[2] * normal[2];
            if (length_squared <= 0.000001f)
            {
                normal[0] = 0.0f;
                normal[1] = 1.0f;
                normal[2] = 0.0f;
                return;
            }
            const float inverse_length = 1.0f / std::sqrt(length_squared);
            normal[0] *= inverse_length;
            normal[1] *= inverse_length;
            normal[2] *= inverse_length;
        }

        void decodeNormal(unsigned int packed, float normal[3])
        {
            normal[0] = signed10(packed);
            normal[1] = signed10(packed >> 10);
            normal[2] = signed10(packed >> 20);
            normalize(normal);
        }

        void computeNormals(SPMSubMesh* mesh)
        {
            unsigned int i;
            for (i = 0; i < mesh->vertices.size(); ++i)
            {
                mesh->vertices[i].normal[0] = 0.0f;
                mesh->vertices[i].normal[1] = 0.0f;
                mesh->vertices[i].normal[2] = 0.0f;
            }

            for (i = 0; i + 2 < mesh->indices.size(); i += 3)
            {
                const SPMVertex& a = mesh->vertices[mesh->indices[i]];
                const SPMVertex& b = mesh->vertices[mesh->indices[i + 1]];
                const SPMVertex& c = mesh->vertices[mesh->indices[i + 2]];
                const float ab[3] = {
                    b.position[0] - a.position[0],
                    b.position[1] - a.position[1],
                    b.position[2] - a.position[2]
                };
                const float ac[3] = {
                    c.position[0] - a.position[0],
                    c.position[1] - a.position[1],
                    c.position[2] - a.position[2]
                };
                const float face[3] = {
                    ab[1] * ac[2] - ab[2] * ac[1],
                    ab[2] * ac[0] - ab[0] * ac[2],
                    ab[0] * ac[1] - ab[1] * ac[0]
                };
                unsigned int corner;
                for (corner = 0; corner < 3; ++corner)
                {
                    SPMVertex& vertex = mesh->vertices[mesh->indices[i + corner]];
                    vertex.normal[0] += face[0];
                    vertex.normal[1] += face[1];
                    vertex.normal[2] += face[2];
                }
            }

            for (i = 0; i < mesh->vertices.size(); ++i)
                normalize(mesh->vertices[i].normal);
        }

        void setError(std::string* error, const char* message)
        {
            if (error)
                *error = message;
        }

        bool readVertex(Reader* reader, SPMVertex* vertex,
                        bool read_normal, bool read_color,
                        bool read_tangent, bool uv_one, bool uv_two,
                        bool skinned)
        {
            unsigned int packed;
            unsigned char color_id;
            unsigned short half;
            unsigned char rgb[3];
            unsigned char skin_data[16];

            if (!reader->readFloat(&vertex->position[0]) ||
                !reader->readFloat(&vertex->position[1]) ||
                !reader->readFloat(&vertex->position[2]))
                return false;

            if (read_normal)
            {
                if (!reader->readU32(&packed))
                    return false;
                decodeNormal(packed, vertex->normal);
            }
            else
            {
                vertex->normal[0] = 0.0f;
                vertex->normal[1] = 1.0f;
                vertex->normal[2] = 0.0f;
            }

            if (read_color)
            {
                if (!reader->readU8(&color_id))
                    return false;
                if (color_id == 128)
                {
                    vertex->color[0] = 255;
                    vertex->color[1] = 255;
                    vertex->color[2] = 255;
                }
                else
                {
                    if (!reader->readBytes(rgb, 3))
                        return false;
                    vertex->color[0] = rgb[0];
                    vertex->color[1] = rgb[1];
                    vertex->color[2] = rgb[2];
                }
            }
            else
            {
                vertex->color[0] = 255;
                vertex->color[1] = 255;
                vertex->color[2] = 255;
            }
            vertex->color[3] = 255;

            vertex->uv[0] = 0.0f;
            vertex->uv[1] = 0.0f;
            if (uv_one)
            {
                if (!reader->readU16(&half))
                    return false;
                vertex->uv[0] = halfToFloat(half);
                if (!reader->readU16(&half))
                    return false;
                vertex->uv[1] = halfToFloat(half);

                if (uv_two && !reader->readBytes(skin_data, 4))
                    return false;
                if (read_tangent && !reader->readU32(&packed))
                    return false;
            }

            if (skinned && !reader->readBytes(skin_data, sizeof(skin_data)))
                return false;

            return true;
        }
    }

    SPMModel::SPMModel()
        : has_normals(false), has_vertex_colors(false), requires_skinning(false)
    {
        clear();
    }

    void SPMModel::clear()
    {
        unsigned int i;
        for (i = 0; i < 6; ++i)
            bounds[i] = 0.0f;
        has_normals = false;
        has_vertex_colors = false;
        requires_skinning = false;
        materials.clear();
        submeshes.clear();
    }

    SPMReadResult SPMReader::read(const unsigned char* bytes,
                                  unsigned int size,
                                  SPMModel* output,
                                  std::string* error_message)
    {
        if (!bytes || !output)
        {
            setError(error_message, "null input or output");
            return SPM_READ_INVALID_ARGUMENT;
        }

        output->clear();
        Reader reader(bytes, size);
        unsigned char magic[2];
        unsigned char version_and_type;
        unsigned char flags;
        unsigned short material_count;
        unsigned short lod_count;
        unsigned int i;

        if (!reader.readBytes(magic, 2))
        {
            setError(error_message, "truncated SPM header");
            return SPM_READ_TRUNCATED;
        }
        if (magic[0] != 'S' || magic[1] != 'P')
        {
            setError(error_message, "invalid SPM magic");
            return SPM_READ_BAD_HEADER;
        }
        if (!reader.readU8(&version_and_type))
        {
            setError(error_message, "truncated SPM version");
            return SPM_READ_TRUNCATED;
        }
        if ((version_and_type >> 3) != 1)
        {
            setError(error_message, "unsupported SPM version");
            return SPM_READ_UNSUPPORTED_VERSION;
        }

        const unsigned int variant = version_and_type & 7u;
        if (variant == 0u || variant > 2u)
        {
            setError(error_message, "unsupported SPM variant");
            return SPM_READ_UNSUPPORTED_VARIANT;
        }

        if (!reader.readU8(&flags))
        {
            setError(error_message, "truncated SPM flags");
            return SPM_READ_TRUNCATED;
        }

        SPMModel parsed;
        parsed.has_normals = (flags & 1u) != 0;
        parsed.has_vertex_colors = (flags & 2u) != 0;
        parsed.requires_skinning = variant == 1u;

        for (i = 0; i < 6; ++i)
        {
            if (!reader.readFloat(&parsed.bounds[i]))
            {
                setError(error_message, "truncated SPM bounds");
                return SPM_READ_TRUNCATED;
            }
        }

        if (!reader.readU16(&material_count))
        {
            setError(error_message, "truncated SPM material count");
            return SPM_READ_TRUNCATED;
        }
        if (material_count == 0 || material_count > kMaxMaterials)
        {
            setError(error_message, "invalid SPM material count");
            return SPM_READ_LIMIT_EXCEEDED;
        }

        parsed.materials.resize(material_count);
        for (i = 0; i < material_count; ++i)
        {
            if (!reader.readString(&parsed.materials[i].texture1) ||
                !reader.readString(&parsed.materials[i].texture2))
            {
                setError(error_message, "truncated SPM material names");
                return SPM_READ_TRUNCATED;
            }
        }

        if (!reader.readU16(&lod_count))
        {
            setError(error_message, "truncated SPM LOD count");
            return SPM_READ_TRUNCATED;
        }
        if (lod_count == 0 || lod_count > kMaxLods)
        {
            setError(error_message, "invalid SPM LOD count");
            return SPM_READ_LIMIT_EXCEEDED;
        }

        unsigned int total_submeshes = 0;
        for (i = 0; i < lod_count; ++i)
        {
            unsigned short submesh_count;
            unsigned int j;
            if (!reader.readU16(&submesh_count))
            {
                setError(error_message, "truncated SPM submesh count");
                return SPM_READ_TRUNCATED;
            }
            if (submesh_count > kMaxSubMeshes - total_submeshes)
            {
                setError(error_message, "too many SPM submeshes");
                return SPM_READ_LIMIT_EXCEEDED;
            }
            total_submeshes += submesh_count;

            for (j = 0; j < submesh_count; ++j)
            {
                unsigned int vertex_count;
                unsigned int index_count;
                unsigned short material_id;
                unsigned int k;
                SPMSubMesh submesh;

                if (!reader.readU32(&vertex_count) ||
                    !reader.readU32(&index_count) ||
                    !reader.readU16(&material_id))
                {
                    setError(error_message, "truncated SPM submesh header");
                    return SPM_READ_TRUNCATED;
                }
                if (vertex_count == 0 || vertex_count > kMaxVerticesPerMesh ||
                    index_count == 0 || index_count > kMaxIndicesPerMesh ||
                    index_count % 3u != 0 || material_id >= material_count)
                {
                    setError(error_message, "invalid SPM submesh dimensions or material");
                    return SPM_READ_INVALID_DATA;
                }

                submesh.lod_index = i;
                submesh.material_index = material_id;
                submesh.vertices.resize(vertex_count);

                const SPMMaterial& material = parsed.materials[material_id];
                const bool uv_one = !material.texture1.empty();
                const bool uv_two = !material.texture2.empty();
                const bool read_tangent = (flags & 4u) != 0;

                for (k = 0; k < vertex_count; ++k)
                {
                    if (!readVertex(&reader, &submesh.vertices[k],
                                    parsed.has_normals, parsed.has_vertex_colors,
                                    read_tangent, uv_one, uv_two,
                                    parsed.requires_skinning))
                    {
                        setError(error_message, "truncated SPM vertex data");
                        return SPM_READ_TRUNCATED;
                    }
                }

                submesh.indices.resize(index_count);
                if (vertex_count > 255u)
                {
                    for (k = 0; k < index_count; ++k)
                    {
                        if (!reader.readU16(&submesh.indices[k]))
                        {
                            setError(error_message, "truncated SPM 16-bit indices");
                            return SPM_READ_TRUNCATED;
                        }
                    }
                }
                else
                {
                    unsigned char index;
                    for (k = 0; k < index_count; ++k)
                    {
                        if (!reader.readU8(&index))
                        {
                            setError(error_message, "truncated SPM 8-bit indices");
                            return SPM_READ_TRUNCATED;
                        }
                        submesh.indices[k] = index;
                    }
                }

                for (k = 0; k < index_count; ++k)
                {
                    if (submesh.indices[k] >= vertex_count)
                    {
                        setError(error_message, "SPM index is outside its vertex array");
                        return SPM_READ_INVALID_DATA;
                    }
                }

                if (!parsed.has_normals)
                    computeNormals(&submesh);

                parsed.submeshes.push_back(submesh);
            }
        }

        *output = parsed;
        if (error_message)
            error_message->clear();
        return SPM_READ_OK;
    }
}
