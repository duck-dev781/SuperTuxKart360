#include "SPMRedrawBridge.h"

namespace stk360
{
    namespace
    {
        void setError(std::string* error, const char* message)
        {
            if (error)
                *error = message;
        }
    }

    SPMDrawBatch::SPMDrawBatch()
        : lod_index(0), material_index(0), static_skinned_pose(false)
    {
    }

    SPMBridgeResult SPMRedrawBridge::convert(
        const SPMModel& model,
        unsigned int lod_index,
        bool allow_skinned_base_pose,
        std::vector<SPMDrawBatch>* output,
        std::string* error_message,
        unsigned int max_indices_per_batch)
    {
        if (!output)
        {
            setError(error_message, "null output batch list");
            return SPM_BRIDGE_INVALID_ARGUMENT;
        }
        output->clear();

        if (model.submeshes.empty())
        {
            setError(error_message, "SPM model has no submeshes");
            return SPM_BRIDGE_NO_SUBMESHES;
        }
        if (model.requires_skinning && !allow_skinned_base_pose)
        {
            setError(error_message, "SPM model requires skeletal skinning");
            return SPM_BRIDGE_SKINNING_REQUIRED;
        }

        if (max_indices_per_batch > 65535u)
            max_indices_per_batch = 65535u;
        max_indices_per_batch -= max_indices_per_batch % 3u;
        if (max_indices_per_batch < 3u)
        {
            setError(error_message, "batch index limit must allow at least one triangle");
            return SPM_BRIDGE_INVALID_BATCH_SIZE;
        }

        unsigned int matched_submeshes = 0;
        for (unsigned int i = 0; i < model.submeshes.size(); ++i)
        {
            const SPMSubMesh& source = model.submeshes[i];
            if (source.lod_index != lod_index)
                continue;

            ++matched_submeshes;
            if (source.vertices.empty() || source.indices.empty() ||
                source.vertices.size() > 65535u || source.indices.size() % 3u != 0 ||
                source.material_index >= model.materials.size())
            {
                output->clear();
                setError(error_message, "invalid SPM submesh data");
                return SPM_BRIDGE_INVALID_MESH_DATA;
            }

            std::vector<redraw::Vertex> vertices(source.vertices.size());
            for (unsigned int v = 0; v < source.vertices.size(); ++v)
            {
                const SPMVertex& input = source.vertices[v];
                redraw::Vertex& vertex = vertices[v];
                vertex.x = input.position[0];
                vertex.y = input.position[1];
                vertex.z = input.position[2];
                vertex.nx = input.normal[0];
                vertex.ny = input.normal[1];
                vertex.nz = input.normal[2];
                vertex.u = input.uv[0];
                vertex.v = input.uv[1];
            }

            for (unsigned int index = 0; index < source.indices.size(); ++index)
            {
                if (source.indices[index] >= source.vertices.size())
                {
                    output->clear();
                    setError(error_message, "SPM submesh index exceeds vertex count");
                    return SPM_BRIDGE_INVALID_MESH_DATA;
                }
            }

            for (unsigned int offset = 0; offset < source.indices.size();)
            {
                const unsigned int remaining =
                    static_cast<unsigned int>(source.indices.size()) - offset;
                const unsigned int count =
                    remaining < max_indices_per_batch ? remaining : max_indices_per_batch;

                SPMDrawBatch batch;
                batch.lod_index = source.lod_index;
                batch.material_index = source.material_index;
                batch.static_skinned_pose = model.requires_skinning;
                batch.vertices = vertices;
                batch.indices.assign(source.indices.begin() + offset,
                                     source.indices.begin() + offset + count);
                output->push_back(batch);
                offset += count;
            }
        }

        if (matched_submeshes == 0)
        {
            setError(error_message, "requested SPM LOD was not found");
            return SPM_BRIDGE_LOD_NOT_FOUND;
        }

        if (error_message)
            error_message->clear();
        return SPM_BRIDGE_OK;
    }
}
