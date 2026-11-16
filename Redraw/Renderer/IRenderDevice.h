#ifndef REDRAW_RENDERER_DEVICE_H
#define REDRAW_RENDERER_DEVICE_H

/*
 * Redraw: minimal renderer contract.
 *
 * This header intentionally uses a small C++03-compatible interface and
 * contains no Xbox SDK or desktop graphics headers. Platform backends
 * implement this contract; the Xbox 360 Xenos backend is in ../Xbox360.
 * That backend is an early bring-up implementation, not yet feature-complete
 * for SuperTuxKart's full material and scene requirements.
 */

namespace redraw
{
    struct Color
    {
        float r;
        float g;
        float b;
        float a;
    };

    struct Matrix4
    {
        // Column-major 4x4 matrix.
        float m[16];
    };

    struct Vertex
    {
        float x;
        float y;
        float z;
        float nx;
        float ny;
        float nz;
        float u;
        float v;
    };

    typedef unsigned int MeshHandle;
    typedef unsigned int TextureHandle;

    struct MeshDesc
    {
        const Vertex* vertices;
        unsigned int vertex_count;
        const unsigned short* indices;
        unsigned int index_count;
    };

    struct TextureDesc
    {
        const void* pixels;
        unsigned int width;
        unsigned int height;
        unsigned int bytes_per_pixel;
        unsigned int row_pitch;
    };

    struct Material
    {
        TextureHandle texture;
        Color diffuse;
        bool alpha_blend;
        bool depth_test;
        bool depth_write;
        bool backface_cull;
    };

    class IRenderDevice
    {
    public:
        virtual ~IRenderDevice() {}

        virtual bool initialize(unsigned int width, unsigned int height) = 0;
        virtual void shutdown() = 0;

        virtual bool beginFrame() = 0;
        virtual void clear(const Color& color, bool clear_depth) = 0;
        virtual void setViewProjection(const Matrix4& view,
                                       const Matrix4& projection) = 0;

        virtual MeshHandle createMesh(const MeshDesc& desc) = 0;
        virtual void releaseMesh(MeshHandle mesh) = 0;

        virtual TextureHandle createTexture(const TextureDesc& desc) = 0;
        virtual void releaseTexture(TextureHandle texture) = 0;

        virtual void drawIndexed(MeshHandle mesh,
                                 const Material& material) = 0;

        virtual void endFrame() = 0;
        virtual void present() = 0;
    };
}

#endif
