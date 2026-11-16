#include "XsdkRenderDevice.h"

#include "redraw_xbox360_shaders.h"

#include <cstring>
#include <vector>

namespace redraw
{
    namespace
    {
        const unsigned int kFrameTimeoutMs = 5000;
        const unsigned int kPresentBuffers = 2;
        const unsigned int kSwapInterval = 1;
        const uint32_t kVertexStride = sizeof(Vertex);

        const XsdkVertexElement kVertexLayout[] =
        {
            { XSDK_USAGE_POSITION, 0, XSDK_VERTEX_FLOAT3, 0, 0,
              { XSDK_COMPONENT_OWN, XSDK_COMPONENT_OWN, XSDK_COMPONENT_OWN, XSDK_COMPONENT_OWN }, false },
            { XSDK_USAGE_NORMAL, 0, XSDK_VERTEX_FLOAT3, 0, 12,
              { XSDK_COMPONENT_OWN, XSDK_COMPONENT_OWN, XSDK_COMPONENT_OWN, XSDK_COMPONENT_OWN }, false },
            { XSDK_USAGE_TEXCOORD, 0, XSDK_VERTEX_FLOAT2, 0, 24,
              { XSDK_COMPONENT_OWN, XSDK_COMPONENT_OWN, XSDK_COMPONENT_OWN, XSDK_COMPONENT_OWN }, false }
        };
    }

    XsdkRenderDevice::MeshSlot::MeshSlot()
        : vertices(0), indices(0), vertex_bytes(0), index_count(0), valid(false)
    {
    }

    XsdkRenderDevice::TextureSlot::TextureSlot()
        : valid(false)
    {
        std::memset(&texture, 0, sizeof(texture));
    }

    XsdkRenderDevice::XsdkRenderDevice()
        : m_initialized(false), m_present_open(false), m_frame_active(false),
          m_width(0), m_height(0), m_frame_address(0), m_clear_argb(0xFF202020)
    {
        std::memset(&m_vertex_shader, 0, sizeof(m_vertex_shader));
        std::memset(&m_pixel_shader, 0, sizeof(m_pixel_shader));
        std::memset(&m_white_texture, 0, sizeof(m_white_texture));
        std::memset(&m_sampler_state, 0, sizeof(m_sampler_state));
        std::memset(&m_depth_state, 0, sizeof(m_depth_state));
        std::memset(&m_raster_state, 0, sizeof(m_raster_state));
        std::memset(&m_opaque_blend, 0, sizeof(m_opaque_blend));
        std::memset(&m_alpha_blend, 0, sizeof(m_alpha_blend));

        for (unsigned int i = 0; i < 16; ++i)
        {
            m_view_projection.m[i] = (i % 5 == 0) ? 1.0f : 0.0f;
            m_world.m[i] = (i % 5 == 0) ? 1.0f : 0.0f;
        }
    }

    XsdkRenderDevice::~XsdkRenderDevice()
    {
        shutdown();
    }

    uint32_t XsdkRenderDevice::packColor(const Color& color)
    {
        const float values[4] = { color.r, color.g, color.b, color.a };
        uint32_t channels[4];
        for (unsigned int i = 0; i < 4; ++i)
        {
            float value = values[i];
            if (value < 0.0f) value = 0.0f;
            if (value > 1.0f) value = 1.0f;
            channels[i] = static_cast<uint32_t>(value * 255.0f + 0.5f);
        }
        return (channels[3] << 24) | (channels[0] << 16) |
               (channels[1] << 8) | channels[2];
    }

    Matrix4 XsdkRenderDevice::multiply(const Matrix4& a, const Matrix4& b)
    {
        Matrix4 result;
        for (unsigned int column = 0; column < 4; ++column)
        {
            for (unsigned int row = 0; row < 4; ++row)
            {
                float sum = 0.0f;
                for (unsigned int k = 0; k < 4; ++k)
                    sum += a.m[k * 4 + row] * b.m[column * 4 + k];
                result.m[column * 4 + row] = sum;
            }
        }
        return result;
    }

    void XsdkRenderDevice::writeGpuMemory(volatile uint32_t* destination,
                                           const void* source,
                                           uint32_t size_bytes)
    {
        const uint32_t words = (size_bytes + 3u) / 4u;
        std::vector<uint32_t> copy(words, 0u);
        std::memcpy(&copy[0], source, size_bytes);
        for (uint32_t i = 0; i < words; ++i)
            destination[i] = copy[i];
    }

    XsdkRenderDevice::MeshSlot* XsdkRenderDevice::findMesh(MeshHandle handle)
    {
        if (handle == 0 || handle > m_meshes.size())
            return 0;
        MeshSlot& slot = m_meshes[handle - 1];
        return slot.valid ? &slot : 0;
    }

    XsdkRenderDevice::TextureSlot* XsdkRenderDevice::findTexture(TextureHandle handle)
    {
        if (handle == 0 || handle > m_textures.size())
            return 0;
        TextureSlot& slot = m_textures[handle - 1];
        return slot.valid ? &slot : 0;
    }

    bool XsdkRenderDevice::initialize(unsigned int width, unsigned int height)
    {
        if (m_initialized || width != 1280 || height != 720)
            return false;

        NTSTATUS status = xsdk_gpu_open();
        if (!NT_SUCCESS(status))
            return false;

        status = xsdk_shader_load(&m_vertex_shader, redraw_vert.container,
                                  redraw_vert.container_bytes);
        if (NT_SUCCESS(status))
            status = xsdk_shader_load(&m_pixel_shader, redraw_frag.container,
                                      redraw_frag.container_bytes);
        if (NT_SUCCESS(status))
            status = xsdk_shader_set_vertex_layout(
                &m_vertex_shader, kVertexLayout,
                sizeof(kVertexLayout) / sizeof(kVertexLayout[0]),
                &kVertexStride);

        if (!NT_SUCCESS(status))
        {
            xsdk_shader_free(&m_pixel_shader);
            xsdk_shader_free(&m_vertex_shader);
            xsdk_gpu_close();
            return false;
        }

        XsdkDepthStencil depth = {};
        depth.depth_test = true;
        depth.depth_write = true;
        depth.depth_compare = XSDK_COMPARE_LESS_EQUAL;
        xsdk_gpu_depth_stencil_state(&m_depth_state, &depth);

        XsdkRaster raster = {};
        raster.cull = XSDK_CULL_BACK;
        raster.front_clockwise = false;
        raster.fill = XSDK_FILL_SOLID;
        xsdk_gpu_raster_state(&m_raster_state, &raster);

        XsdkBlend opaque = {};
        opaque.color_source = XSDK_BLEND_ONE;
        opaque.color_dest = XSDK_BLEND_ZERO;
        opaque.color_op = XSDK_BLEND_OP_ADD;
        opaque.alpha_source = XSDK_BLEND_ONE;
        opaque.alpha_dest = XSDK_BLEND_ZERO;
        opaque.alpha_op = XSDK_BLEND_OP_ADD;
        opaque.write_mask = XSDK_WRITE_ALL;
        xsdk_gpu_blend_state(&m_opaque_blend, &opaque);

        XsdkBlend alpha = opaque;
        alpha.color_source = XSDK_BLEND_SRC_ALPHA;
        alpha.color_dest = XSDK_BLEND_ONE_MINUS_SRC_ALPHA;
        alpha.alpha_source = XSDK_BLEND_ONE;
        alpha.alpha_dest = XSDK_BLEND_ONE_MINUS_SRC_ALPHA;
        xsdk_gpu_blend_state(&m_alpha_blend, &alpha);

        XsdkSampler sampler = {};
        sampler.mag_filter = XSDK_FILTER_LINEAR;
        sampler.min_filter = XSDK_FILTER_LINEAR;
        sampler.mip_filter = XSDK_FILTER_POINT;
        sampler.address_u = XSDK_ADDRESS_WRAP;
        sampler.address_v = XSDK_ADDRESS_WRAP;
        sampler.level_count = 1;
        xsdk_texture_sampler_state(&m_sampler_state, &sampler);

        status = xsdk_texture_create(&m_white_texture, XSDK_FORMAT_A8R8G8B8,
                                     1, 1, 1, true);
        if (!NT_SUCCESS(status))
        {
            xsdk_shader_free(&m_pixel_shader);
            xsdk_shader_free(&m_vertex_shader);
            xsdk_gpu_close();
            return false;
        }
        const uint32_t white = 0xFFFFFFFFu;
        xsdk_texture_upload(&m_white_texture, 0, &white, sizeof(white));
        xsdk_gpu_invalidate(m_white_texture.address, m_white_texture.size_bytes);

        status = xsdk_present_open(XSDK_DISPLAY_720P, kPresentBuffers,
                                   kSwapInterval, &m_width, &m_height);
        if (!NT_SUCCESS(status))
        {
            xsdk_texture_free(&m_white_texture);
            xsdk_shader_free(&m_pixel_shader);
            xsdk_shader_free(&m_vertex_shader);
            xsdk_gpu_close();
            return false;
        }

        m_present_open = true;
        m_initialized = true;
        m_clear_argb = 0xFF202020;
        return true;
    }

    void XsdkRenderDevice::shutdown()
    {
        if (!m_initialized && !m_present_open)
            return;

        if (m_frame_active)
        {
            xsdk_gpu_resolve(m_frame_address, m_width, m_height, m_clear_argb);
            xsdk_present_end();
            m_frame_active = false;
        }

        for (unsigned int i = 0; i < m_meshes.size(); ++i)
        {
            if (m_meshes[i].valid)
            {
                xsdk_gpu_free(m_meshes[i].indices);
                xsdk_gpu_free(m_meshes[i].vertices);
                m_meshes[i].valid = false;
            }
        }
        for (unsigned int i = 0; i < m_textures.size(); ++i)
        {
            if (m_textures[i].valid)
            {
                xsdk_texture_free(&m_textures[i].texture);
                m_textures[i].valid = false;
            }
        }

        if (m_present_open)
        {
            xsdk_present_close();
            m_present_open = false;
        }
        xsdk_texture_free(&m_white_texture);
        xsdk_shader_free(&m_pixel_shader);
        xsdk_shader_free(&m_vertex_shader);
        xsdk_gpu_close();
        m_initialized = false;
    }

    bool XsdkRenderDevice::beginFrame()
    {
        if (!m_initialized || m_frame_active)
            return false;

        const NTSTATUS status = xsdk_present_begin(&m_frame_address, kFrameTimeoutMs);
        if (!NT_SUCCESS(status))
            return false;

        xsdk_gpu_set_target(m_width, m_height);
        xsdk_gpu_set_depth_clear(1.0f, 0);
        xsdk_gpu_set_depth_stencil(&m_depth_state);
        xsdk_gpu_set_raster(&m_raster_state);
        m_frame_active = true;
        return true;
    }

    void XsdkRenderDevice::clear(const Color& color, bool clear_depth)
    {
        if (!m_frame_active)
            return;

        m_clear_argb = packColor(color);
        if (clear_depth)
            xsdk_gpu_set_depth_clear(1.0f, 0);

        xsdk_gpu_set_resolve_clears(true, clear_depth);
        xsdk_gpu_resolve(m_frame_address, m_width, m_height, m_clear_argb);
        xsdk_gpu_set_resolve_clears(true, true);
        xsdk_gpu_set_target(m_width, m_height);
        xsdk_gpu_set_depth_stencil(&m_depth_state);
        xsdk_gpu_set_raster(&m_raster_state);
    }

    void XsdkRenderDevice::setViewProjection(const Matrix4& view,
                                             const Matrix4& projection)
    {
        m_view_projection = multiply(projection, view);
    }

    void XsdkRenderDevice::setWorld(const Matrix4& world)
    {
        m_world = world;
    }

    MeshHandle XsdkRenderDevice::createMesh(const MeshDesc& desc)
    {
        if (!m_initialized || !desc.vertices || !desc.indices ||
            desc.vertex_count == 0 || desc.index_count == 0 ||
            desc.index_count > 65535u)
            return 0;

        const uint32_t vertex_bytes = desc.vertex_count * sizeof(Vertex);
        const uint32_t index_bytes = desc.index_count * sizeof(unsigned short);
        const uint32_t padded_index_bytes = (index_bytes + 3u) & ~3u;

        MeshSlot slot;
        slot.vertices = xsdk_gpu_alloc(vertex_bytes);
        slot.indices = xsdk_gpu_alloc(padded_index_bytes);
        if (!slot.vertices || !slot.indices)
        {
            if (slot.indices) xsdk_gpu_free(slot.indices);
            if (slot.vertices) xsdk_gpu_free(slot.vertices);
            return 0;
        }

        writeGpuMemory(slot.vertices, desc.vertices, vertex_bytes);
        writeGpuMemory(slot.indices, desc.indices, index_bytes);
        xsdk_gpu_invalidate(xsdk_gpu_address(slot.vertices), vertex_bytes);
        xsdk_gpu_invalidate(xsdk_gpu_address(slot.indices), padded_index_bytes);

        slot.vertex_bytes = vertex_bytes;
        slot.index_count = desc.index_count;
        slot.valid = true;
        m_meshes.push_back(slot);
        return static_cast<MeshHandle>(m_meshes.size());
    }

    void XsdkRenderDevice::releaseMesh(MeshHandle handle)
    {
        MeshSlot* slot = findMesh(handle);
        if (!slot)
            return;
        xsdk_gpu_free(slot->indices);
        xsdk_gpu_free(slot->vertices);
        slot->valid = false;
    }

    TextureHandle XsdkRenderDevice::createTexture(const TextureDesc& desc)
    {
        if (!m_initialized || !desc.pixels || desc.width == 0 ||
            desc.height == 0 || desc.bytes_per_pixel != 4)
            return 0;

        const uint32_t minimum_pitch = desc.width * 4u;
        if (desc.row_pitch != 0 && desc.row_pitch < minimum_pitch)
            return 0;

        TextureSlot slot;
        NTSTATUS status = xsdk_texture_create(&slot.texture, XSDK_FORMAT_A8R8G8B8,
                                              desc.width, desc.height, 1, true);
        if (!NT_SUCCESS(status))
            return 0;

        const uint32_t row_pitch = desc.row_pitch ? desc.row_pitch : desc.width * 4u;
        xsdk_texture_upload(&slot.texture, 0, desc.pixels, row_pitch);
        xsdk_gpu_invalidate(slot.texture.address, slot.texture.size_bytes);
        slot.valid = true;
        m_textures.push_back(slot);
        return static_cast<TextureHandle>(m_textures.size());
    }

    void XsdkRenderDevice::releaseTexture(TextureHandle handle)
    {
        TextureSlot* slot = findTexture(handle);
        if (!slot)
            return;
        xsdk_texture_free(&slot->texture);
        slot->valid = false;
    }

    void XsdkRenderDevice::drawIndexed(MeshHandle handle, const Material& material)
    {
        if (!m_frame_active)
            return;

        MeshSlot* mesh = findMesh(handle);
        if (!mesh)
            return;

        TextureSlot* texture = findTexture(material.texture);
        const XsdkTexture* bound_texture = texture ? &texture->texture : &m_white_texture;

        xsdk_gpu_set_shaders(&m_vertex_shader, &m_pixel_shader);
        xsdk_built_shader_set_literals(&redraw_vert);
        xsdk_built_shader_set_literals(&redraw_frag);
        const Matrix4 mvp = multiply(m_view_projection, m_world);
        xsdk_gpu_set_vertex_constants(0, mvp.m, 4);

        const float diffuse[4] =
        {
            material.diffuse.r, material.diffuse.g,
            material.diffuse.b, material.diffuse.a
        };
        xsdk_gpu_set_pixel_constants(0, diffuse, 1);

        xsdk_gpu_set_texture(0, bound_texture, &m_sampler_state);
        XsdkDepthStencil depth = {};
        depth.depth_test = material.depth_test;
        depth.depth_write = material.depth_write;
        depth.depth_compare = XSDK_COMPARE_LESS_EQUAL;
        XsdkDepthStencilState depth_state;
        xsdk_gpu_depth_stencil_state(&depth_state, &depth);
        xsdk_gpu_set_depth_stencil(&depth_state);

        XsdkRaster raster = {};
        raster.cull = material.backface_cull ? XSDK_CULL_BACK : XSDK_CULL_NONE;
        raster.front_clockwise = false;
        raster.fill = XSDK_FILL_SOLID;
        XsdkRasterState raster_state;
        xsdk_gpu_raster_state(&raster_state, &raster);
        xsdk_gpu_set_raster(&raster_state);
        xsdk_gpu_set_blend(material.alpha_blend ? &m_alpha_blend : &m_opaque_blend);
        xsdk_gpu_set_vertices(0, xsdk_gpu_address(mesh->vertices), mesh->vertex_bytes);
        xsdk_gpu_draw_indexed(XSDK_PRIMITIVE_TRIANGLE_LIST,
                              xsdk_gpu_address(mesh->indices),
                              mesh->index_count, false);
    }

    void XsdkRenderDevice::endFrame()
    {
        if (!m_frame_active)
            return;
        xsdk_gpu_resolve(m_frame_address, m_width, m_height, m_clear_argb);
        xsdk_present_end();
        m_frame_active = false;
    }

    void XsdkRenderDevice::present()
    {
        // xsdk_present_end queues the resolved frame for the vblank callback.
    }
}
