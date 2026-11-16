#include "XsdkRenderDevice.h"

#include "SPMReader.h"
#include "SPMRedrawBridge.h"
#include "TrackDataReader.h"
#include "KartDynamics.h"

#include "xsdk/kernel.h"
#include "xsdk/log.h"

#include <cmath>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace
{
    struct DrawItem
    {
        redraw::MeshHandle mesh;
        redraw::Material material;
    };

    struct Vec3
    {
        float x;
        float y;
        float z;
    };

    bool loadBinary(const char* path, std::vector<unsigned char>* bytes)
    {
        if (!path || !bytes)
            return false;
        std::ifstream input(path, std::ios::binary);
        if (!input)
            return false;
        std::vector<unsigned char> loaded(
            (std::istreambuf_iterator<char>(input)),
            std::istreambuf_iterator<char>());
        if (loaded.empty())
            return false;
        bytes->swap(loaded);
        return true;
    }

    bool loadText(const char* path, std::string* text)
    {
        std::vector<unsigned char> bytes;
        if (!text || !loadBinary(path, &bytes))
            return false;
        text->assign(bytes.begin(), bytes.end());
        return true;
    }

    float controllerAxis(short value)
    {
        if (value > 7000)
            return static_cast<float>(value - 7000) / 25767.0f;
        if (value < -7000)
            return static_cast<float>(value + 7000) / 25768.0f;
        return 0.0f;
    }

    Vec3 subtract(const Vec3& a, const Vec3& b)
    {
        Vec3 result = { a.x - b.x, a.y - b.y, a.z - b.z };
        return result;
    }

    Vec3 cross(const Vec3& a, const Vec3& b)
    {
        Vec3 result =
        {
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        };
        return result;
    }

    float dot(const Vec3& a, const Vec3& b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    Vec3 normalize(const Vec3& value)
    {
        const float length_squared = dot(value, value);
        if (length_squared < 0.000001f)
        {
            Vec3 zero = { 0.0f, 0.0f, 0.0f };
            return zero;
        }
        const float inverse_length = 1.0f / std::sqrt(length_squared);
        Vec3 result = { value.x * inverse_length,
                        value.y * inverse_length,
                        value.z * inverse_length };
        return result;
    }

    redraw::Matrix4 identityMatrix()
    {
        redraw::Matrix4 result;
        for (unsigned int i = 0; i < 16; ++i)
            result.m[i] = (i % 5 == 0) ? 1.0f : 0.0f;
        return result;
    }

    redraw::Matrix4 makeWorld(const stk360::KartState& kart)
    {
        redraw::Matrix4 result = identityMatrix();
        const float c = std::cos(kart.yaw);
        const float s = std::sin(kart.yaw);
        result.m[0] = c;
        result.m[2] = -s;
        result.m[8] = s;
        result.m[10] = c;
        result.m[12] = kart.position.x;
        result.m[13] = kart.position.y;
        result.m[14] = kart.position.z;
        return result;
    }

    redraw::Matrix4 makePerspective(float vertical_fov_radians,
                                    float aspect, float near_plane,
                                    float far_plane)
    {
        redraw::Matrix4 result = {};
        const float f = 1.0f / std::tan(vertical_fov_radians * 0.5f);
        result.m[0] = f / aspect;
        result.m[5] = f;
        result.m[10] = (far_plane + near_plane) / (near_plane - far_plane);
        result.m[11] = -1.0f;
        result.m[14] = (2.0f * far_plane * near_plane) /
                       (near_plane - far_plane);
        return result;
    }

    redraw::Matrix4 makeLookAt(const Vec3& eye, const Vec3& target,
                               const Vec3& world_up)
    {
        const Vec3 forward = normalize(subtract(target, eye));
        const Vec3 side = normalize(cross(forward, world_up));
        const Vec3 up = cross(side, forward);

        redraw::Matrix4 result = identityMatrix();
        result.m[0] = side.x;
        result.m[1] = up.x;
        result.m[2] = -forward.x;
        result.m[4] = side.y;
        result.m[5] = up.y;
        result.m[6] = -forward.y;
        result.m[8] = side.z;
        result.m[9] = up.z;
        result.m[10] = -forward.z;
        result.m[12] = -dot(side, eye);
        result.m[13] = -dot(up, eye);
        result.m[14] = dot(forward, eye);
        return result;
    }

    bool createDrawItems(redraw::XsdkRenderDevice* renderer,
                         const stk360::SPMModel& model,
                         unsigned int lod,
                         bool allow_static_skinned_pose,
                         bool center_model,
                         std::vector<DrawItem>* output)
    {
        if (!renderer || !output)
            return false;

        std::vector<stk360::SPMDrawBatch> batches;
        std::string error;
        if (stk360::SPMRedrawBridge::convert(
                model, lod, allow_static_skinned_pose, &batches, &error) !=
            stk360::SPM_BRIDGE_OK)
        {
            xsdk_log_write(error.c_str());
            return false;
        }

        const float center_x = (model.bounds[0] + model.bounds[3]) * 0.5f;
        const float base_y = model.bounds[1];
        const float center_z = (model.bounds[2] + model.bounds[5]) * 0.5f;

        for (unsigned int i = 0; i < batches.size(); ++i)
        {
            stk360::SPMDrawBatch& batch = batches[i];
            if (center_model)
            {
                for (unsigned int v = 0; v < batch.vertices.size(); ++v)
                {
                    batch.vertices[v].x -= center_x;
                    batch.vertices[v].y -= base_y;
                    batch.vertices[v].z -= center_z;
                }
            }

            redraw::MeshDesc description;
            description.vertices = &batch.vertices[0];
            description.vertex_count =
                static_cast<unsigned int>(batch.vertices.size());
            description.indices = &batch.indices[0];
            description.index_count =
                static_cast<unsigned int>(batch.indices.size());

            DrawItem item;
            item.mesh = renderer->createMesh(description);
            if (item.mesh == 0)
            {
                xsdk_log_write("STK360: Xenos mesh allocation failed");
                return false;
            }

            item.material.texture = 0;
            item.material.diffuse.r = 0.82f;
            item.material.diffuse.g = 0.86f;
            item.material.diffuse.b = 0.92f;
            item.material.diffuse.a = 1.0f;
            item.material.alpha_blend = false;
            item.material.depth_test = true;
            item.material.depth_write = true;
            item.material.backface_cull = false;
            output->push_back(item);
        }
        return !output->empty();
    }

    void releaseDrawItems(redraw::XsdkRenderDevice* renderer,
                          std::vector<DrawItem>* items)
    {
        if (!renderer || !items)
            return;
        for (unsigned int i = 0; i < items->size(); ++i)
            renderer->releaseMesh((*items)[i].mesh);
        items->clear();
    }

    void drawItems(redraw::XsdkRenderDevice* renderer,
                   const std::vector<DrawItem>& items)
    {
        for (unsigned int i = 0; i < items.size(); ++i)
            renderer->drawIndexed(items[i].mesh, items[i].material);
    }
}

int main()
{
    xsdk_log_write("SuperTuxKart360: loading official Black Forest and Adiumy assets");

    std::vector<unsigned char> track_bytes;
    std::vector<unsigned char> kart_bytes;
    std::string quads_xml;
    std::string graph_xml;
    std::string track_xml;

    if (!loadBinary("game:/assets/black_forest/black_forest_track.spm",
                    &track_bytes) ||
        !loadBinary("game:/assets/adiumy/adiumy.spm", &kart_bytes) ||
        !loadText("game:/assets/black_forest/quads.xml", &quads_xml) ||
        !loadText("game:/assets/black_forest/graph.xml", &graph_xml) ||
        !loadText("game:/assets/black_forest/track.xml", &track_xml))
    {
        xsdk_log_write("SuperTuxKart360: missing official asset files beside the XEX");
        return 1;
    }

    stk360::SPMModel track_model;
    stk360::SPMModel kart_model;
    std::string error;

    if (stk360::SPMReader::read(&track_bytes[0],
            static_cast<unsigned int>(track_bytes.size()),
            &track_model, &error) != stk360::SPM_READ_OK)
    {
        xsdk_log_write(error.c_str());
        return 1;
    }
    if (stk360::SPMReader::read(&kart_bytes[0],
            static_cast<unsigned int>(kart_bytes.size()),
            &kart_model, &error) != stk360::SPM_READ_OK)
    {
        xsdk_log_write(error.c_str());
        return 1;
    }

    stk360::TrackData track;
    if (stk360::TrackDataReader::readQuads(quads_xml.data(),
            static_cast<unsigned int>(quads_xml.size()), &track, &error) !=
        stk360::TRACK_READ_OK ||
        stk360::TrackDataReader::readGraph(graph_xml.data(),
            static_cast<unsigned int>(graph_xml.size()), &track, &error) !=
        stk360::TRACK_READ_OK ||
        stk360::TrackDataReader::readTrackInfo(track_xml.data(),
            static_cast<unsigned int>(track_xml.size()), &track, &error) !=
        stk360::TRACK_READ_OK)
    {
        xsdk_log_write(error.c_str());
        return 1;
    }

    stk360::KartState kart;
    if (!stk360::KartDynamics::initialize(track, &kart))
    {
        xsdk_log_write("SuperTuxKart360: could not initialize the track start");
        return 1;
    }

    redraw::XsdkRenderDevice renderer;
    if (!renderer.initialize(1280, 720))
    {
        xsdk_log_write("SuperTuxKart360: Xenos renderer initialization failed");
        return 1;
    }

    std::vector<DrawItem> track_items;
    std::vector<DrawItem> kart_items;
    if (!createDrawItems(&renderer, track_model, 0, false, false, &track_items) ||
        !createDrawItems(&renderer, kart_model, 0, true, true, &kart_items))
    {
        releaseDrawItems(&renderer, &track_items);
        releaseDrawItems(&renderer, &kart_items);
        renderer.shutdown();
        return 1;
    }

    xsdk_log_write("SuperTuxKart360: official track and kart loaded; RT accelerates, LT brakes, left stick steers, Back exits");

    const redraw::Matrix4 projection = makePerspective(
        1.04719755f, 1280.0f / 720.0f, 0.1f, 2000.0f);
    const Vec3 world_up = { 0.0f, 1.0f, 0.0f };

    for (;;)
    {
        INPUT_STATE controller = {};
        stk360::KartInput kart_input;
        if (XamInputGetState(0, 0, &controller) == ERROR_SUCCESS)
        {
            if (controller.gamepad.buttons & INPUT_GAMEPAD_BACK)
                break;
            if (controller.gamepad.buttons & INPUT_GAMEPAD_START)
                stk360::KartDynamics::initialize(track, &kart);

            kart_input.steering = controllerAxis(controller.gamepad.thumb_lx);
            kart_input.throttle =
                static_cast<float>(controller.gamepad.right_trigger) / 255.0f;
            kart_input.brake =
                static_cast<float>(controller.gamepad.left_trigger) / 255.0f;
        }

        stk360::KartDynamics::update(track, kart_input, 1.0f / 60.0f, &kart);

        const float forward_x = std::sin(kart.yaw);
        const float forward_z = std::cos(kart.yaw);
        const Vec3 eye =
        {
            kart.position.x - forward_x * 10.0f,
            kart.position.y + 5.0f,
            kart.position.z - forward_z * 10.0f
        };
        const Vec3 target =
        {
            kart.position.x + forward_x * 2.0f,
            kart.position.y + 1.0f,
            kart.position.z + forward_z * 2.0f
        };
        const redraw::Matrix4 view = makeLookAt(eye, target, world_up);
        const redraw::Matrix4 world = makeWorld(kart);

        if (!renderer.beginFrame())
            continue;

        redraw::Color clear;
        clear.r = 0.035f;
        clear.g = 0.055f;
        clear.b = 0.10f;
        clear.a = 1.0f;
        renderer.clear(clear, true);
        renderer.setViewProjection(view, projection);
        renderer.setWorld(identityMatrix());
        drawItems(&renderer, track_items);
        renderer.setWorld(world);
        drawItems(&renderer, kart_items);
        renderer.endFrame();
    }

    releaseDrawItems(&renderer, &track_items);
    releaseDrawItems(&renderer, &kart_items);
    renderer.shutdown();
    xsdk_log_write("SuperTuxKart360: title ended");
    return 0;
}
