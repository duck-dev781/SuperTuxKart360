#ifndef REDRAW_XBOX360_XSDK_RENDER_DEVICE_H
#define REDRAW_XBOX360_XSDK_RENDER_DEVICE_H

#include "../Renderer/IRenderDevice.h"

#include <vector>

#include "xsdk/gpu.h"
#include "xsdk/present.h"
#include "xsdk/shader.h"
#include "xsdk/texture.h"

namespace redraw
{
    class XsdkRenderDevice : public IRenderDevice
    {
    public:
        XsdkRenderDevice();
        virtual ~XsdkRenderDevice();

        virtual bool initialize(unsigned int width, unsigned int height);
        virtual void shutdown();

        virtual bool beginFrame();
        virtual void clear(const Color& color, bool clear_depth);
        virtual void setViewProjection(const Matrix4& view,
                                       const Matrix4& projection);
        virtual void setWorld(const Matrix4& world);

        virtual MeshHandle createMesh(const MeshDesc& desc);
        virtual void releaseMesh(MeshHandle mesh);

        virtual TextureHandle createTexture(const TextureDesc& desc);
        virtual void releaseTexture(TextureHandle texture);

        virtual void drawIndexed(MeshHandle mesh, const Material& material);

        virtual void endFrame();
        virtual void present();

    private:
        struct MeshSlot
        {
            volatile uint32_t* vertices;
            volatile uint32_t* indices;
            uint32_t vertex_bytes;
            uint32_t index_count;
            bool valid;
            MeshSlot();
        };

        struct TextureSlot
        {
            XsdkTexture texture;
            bool valid;
            TextureSlot();
        };

        bool m_initialized;
        bool m_present_open;
        bool m_frame_active;
        unsigned int m_width;
        unsigned int m_height;
        uint32_t m_frame_address;
        uint32_t m_clear_argb;
        Matrix4 m_view_projection;
        Matrix4 m_world;
        XsdkShader m_vertex_shader;
        XsdkShader m_pixel_shader;
        XsdkTexture m_white_texture;
        XsdkSamplerState m_sampler_state;
        XsdkDepthStencilState m_depth_state;
        XsdkRasterState m_raster_state;
        XsdkBlendState m_opaque_blend;
        XsdkBlendState m_alpha_blend;
        std::vector<MeshSlot> m_meshes;
        std::vector<TextureSlot> m_textures;

        static uint32_t packColor(const Color& color);
        static Matrix4 multiply(const Matrix4& a, const Matrix4& b);
        static void writeGpuMemory(volatile uint32_t* destination,
                                   const void* source,
                                   uint32_t size_bytes);
        MeshSlot* findMesh(MeshHandle handle);
        TextureSlot* findTexture(TextureHandle handle);
    };
}

#endif
