#include "../Renderer/IRenderDevice.h"

class TestRenderDevice : public redraw::IRenderDevice
{
public:
    bool initialize(unsigned int, unsigned int) { return true; }
    void shutdown() {}
    bool beginFrame() { return true; }
    void clear(const redraw::Color&, bool) {}
    void setViewProjection(const redraw::Matrix4&, const redraw::Matrix4&) {}
    void setWorld(const redraw::Matrix4&) {}
    redraw::MeshHandle createMesh(const redraw::MeshDesc&) { return 1; }
    void releaseMesh(redraw::MeshHandle) {}
    redraw::TextureHandle createTexture(const redraw::TextureDesc&) { return 1; }
    void releaseTexture(redraw::TextureHandle) {}
    void drawIndexed(redraw::MeshHandle, const redraw::Material&) {}
    void endFrame() {}
    void present() {}
};

int main()
{
    TestRenderDevice device;
    redraw::Color clear_color = { 0.0f, 0.0f, 0.0f, 1.0f };
    if (!device.initialize(1280, 720))
        return 1;
    if (!device.beginFrame())
        return 2;
    device.clear(clear_color, true);
    device.endFrame();
    device.present();
    device.shutdown();
    return 0;
}
