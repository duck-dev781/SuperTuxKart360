#include "XsdkRenderDevice.h"

#include "xsdk/kernel.h"
#include "xsdk/log.h"

#include <cmath>

namespace
{
    float clampStick(short value)
    {
        if (value > 7000 || value < -7000)
            return static_cast<float>(value) / 32768.0f;
        return 0.0f;
    }
}

int main()
{
    xsdk_log_write("STK360 Redraw: renderer test started; move the left stick, press Back to exit");

    redraw::XsdkRenderDevice renderer;
    if (!renderer.initialize(1280, 720))
    {
        xsdk_log_write("STK360 Redraw: renderer initialization failed");
        return 1;
    }

    const redraw::Vertex vertices[] =
    {
        {  0.0f,  0.65f, 0.5f, 0.0f, 0.0f, 1.0f, 0.5f, 0.0f },
        { -0.55f, -0.45f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f },
        {  0.55f, -0.45f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f }
    };
    const unsigned short indices[] = { 0, 1, 2 };

    redraw::MeshDesc mesh_desc;
    mesh_desc.vertices = vertices;
    mesh_desc.vertex_count = 3;
    mesh_desc.indices = indices;
    mesh_desc.index_count = 3;

    const redraw::MeshHandle mesh = renderer.createMesh(mesh_desc);
    if (mesh == 0)
    {
        xsdk_log_write("STK360 Redraw: mesh allocation failed");
        renderer.shutdown();
        return 1;
    }

    redraw::Material material;
    material.texture = 0;
    material.diffuse.r = 0.15f;
    material.diffuse.g = 0.85f;
    material.diffuse.b = 1.0f;
    material.diffuse.a = 1.0f;
    material.alpha_blend = false;
    material.depth_test = true;
    material.depth_write = true;
    material.backface_cull = false;

    float angle = 0.0f;
    for (;;)
    {
        INPUT_STATE input = {};
        if (XamInputGetState(0, 0, &input) == ERROR_SUCCESS)
        {
            if (input.gamepad.buttons & INPUT_GAMEPAD_BACK)
                break;
            angle += clampStick(input.gamepad.thumb_lx) * 0.035f;
        }

        redraw::Matrix4 view;
        redraw::Matrix4 projection;
        for (unsigned int i = 0; i < 16; ++i)
        {
            view.m[i] = (i % 5 == 0) ? 1.0f : 0.0f;
            projection.m[i] = (i % 5 == 0) ? 1.0f : 0.0f;
        }
        const float c = std::cos(angle);
        const float s = std::sin(angle);
        projection.m[0] = c;
        projection.m[1] = s;
        projection.m[4] = -s;
        projection.m[5] = c;

        if (!renderer.beginFrame())
            continue;
        redraw::Color clear;
        clear.r = 0.025f;
        clear.g = 0.035f;
        clear.b = 0.075f;
        clear.a = 1.0f;
        renderer.clear(clear, true);
        renderer.setViewProjection(view, projection);
        renderer.drawIndexed(mesh, material);
        renderer.endFrame();
    }

    renderer.releaseMesh(mesh);
    renderer.shutdown();
    xsdk_log_write("STK360 Redraw: renderer test ended");
    return 0;
}
