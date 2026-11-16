#ifndef STK360_TEXTURE_LOADER_H
#define STK360_TEXTURE_LOADER_H

#include <string>
#include <vector>

namespace stk360
{
    struct TextureImage
    {
        unsigned int width;
        unsigned int height;
        unsigned int row_pitch;
        bool has_alpha;
        std::vector<unsigned char> argb_pixels;

        TextureImage();
        void clear();
    };

    class TextureLoader
    {
    public:
        // Decodes official PNG/JPEG bytes to Xbox A8R8G8B8 byte order (A,R,G,B).
        // Large images are reduced proportionally to max_dimension.
        static bool decode(const unsigned char* bytes,
                           unsigned int size,
                           unsigned int max_dimension,
                           TextureImage* output,
                           std::string* error_message = 0);
    };
}

#endif
