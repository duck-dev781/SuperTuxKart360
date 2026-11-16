#ifndef STK360_SPM_REDRAW_BRIDGE_H
#define STK360_SPM_REDRAW_BRIDGE_H

#include "SPMReader.h"
#include "../../Redraw/Renderer/IRenderDevice.h"

#include <string>
#include <vector>

namespace stk360
{
    struct SPMDrawBatch
    {
        unsigned int lod_index;
        unsigned int material_index;
        bool static_skinned_pose;
        std::vector<redraw::Vertex> vertices;
        std::vector<unsigned short> indices;

        SPMDrawBatch();
    };

    enum SPMBridgeResult
    {
        SPM_BRIDGE_OK,
        SPM_BRIDGE_INVALID_ARGUMENT,
        SPM_BRIDGE_NO_SUBMESHES,
        SPM_BRIDGE_LOD_NOT_FOUND,
        SPM_BRIDGE_SKINNING_REQUIRED,
        SPM_BRIDGE_INVALID_BATCH_SIZE,
        SPM_BRIDGE_INVALID_MESH_DATA
    };

    class SPMRedrawBridge
    {
    public:
        // Converts one LOD into Redraw batches. The Xenos backend currently
        // accepts at most 65535 indices per draw, so larger submeshes are split.
        // Skinned base geometry is opt-in and remains a static, unevaluated pose.
        static SPMBridgeResult convert(const SPMModel& model,
                                       unsigned int lod_index,
                                       bool allow_skinned_base_pose,
                                       std::vector<SPMDrawBatch>* output,
                                       std::string* error_message = 0,
                                       unsigned int max_indices_per_batch = 65535u);
    };
}

#endif
