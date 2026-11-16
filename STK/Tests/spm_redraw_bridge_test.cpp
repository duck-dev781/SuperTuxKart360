#include "../Source/SPMRedrawBridge.h"

#include <iostream>
#include <vector>

namespace
{
    stk360::SPMModel makeModel(unsigned int index_count)
    {
        stk360::SPMModel model;
        model.materials.resize(1);
        stk360::SPMSubMesh mesh;
        mesh.lod_index = 0;
        mesh.material_index = 0;

        stk360::SPMVertex a = {};
        a.position[0] = -1.0f;
        a.normal[2] = 1.0f;
        stk360::SPMVertex b = {};
        b.position[0] = 1.0f;
        b.normal[2] = 1.0f;
        stk360::SPMVertex c = {};
        c.position[1] = 1.0f;
        c.normal[2] = 1.0f;
        mesh.vertices.push_back(a);
        mesh.vertices.push_back(b);
        mesh.vertices.push_back(c);

        for (unsigned int i = 0; i < index_count; i += 3)
        {
            mesh.indices.push_back(0);
            mesh.indices.push_back(1);
            mesh.indices.push_back(2);
        }
        model.submeshes.push_back(mesh);
        return model;
    }
}

int main()
{
    stk360::SPMModel model = makeModel(3);
    std::vector<stk360::SPMDrawBatch> batches;
    std::string error;

    if (stk360::SPMRedrawBridge::convert(model, 0, false, &batches, &error) !=
        stk360::SPM_BRIDGE_OK)
    {
        std::cerr << "Static SPM conversion failed: " << error << "\n";
        return 1;
    }
    if (batches.size() != 1 || batches[0].vertices.size() != 3 ||
        batches[0].indices.size() != 3 || batches[0].material_index != 0)
    {
        std::cerr << "Static SPM conversion returned incorrect batches\n";
        return 1;
    }

    model.requires_skinning = true;
    if (stk360::SPMRedrawBridge::convert(model, 0, false, &batches, &error) !=
        stk360::SPM_BRIDGE_SKINNING_REQUIRED || !batches.empty())
    {
        std::cerr << "Bridge did not guard unimplemented skinning\n";
        return 1;
    }

    if (stk360::SPMRedrawBridge::convert(model, 0, true, &batches, &error) !=
        stk360::SPM_BRIDGE_OK || batches.size() != 1 ||
        !batches[0].static_skinned_pose)
    {
        std::cerr << "Bridge did not expose the explicit static-pose option\n";
        return 1;
    }

    model = makeModel(65538);
    if (stk360::SPMRedrawBridge::convert(model, 0, false, &batches, &error) !=
        stk360::SPM_BRIDGE_OK || batches.size() != 2 ||
        batches[0].indices.size() != 65535 ||
        batches[1].indices.size() != 3)
    {
        std::cerr << "Bridge did not split an oversized submesh into safe batches\n";
        return 1;
    }

    if (stk360::SPMRedrawBridge::convert(model, 1, false, &batches, &error) !=
        stk360::SPM_BRIDGE_LOD_NOT_FOUND || !batches.empty())
    {
        std::cerr << "Bridge did not reject a missing LOD\n";
        return 1;
    }

    std::cout << "SPM Redraw bridge tests passed\n";
    return 0;
}
