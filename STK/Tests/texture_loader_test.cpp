#include "../Source/TextureLoader.h"

#include <iostream>
#include <string>

int main()
{
    // 2x2 RGBA PNG: red, green, half-alpha blue, white.
    const unsigned char png[] =
    {
        0x89,0x50,0x4e,0x47,0x0d,0x0a,0x1a,0x0a,
        0x00,0x00,0x00,0x0d,0x49,0x48,0x44,0x52,
        0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x02,
        0x08,0x06,0x00,0x00,0x00,0x72,0xb6,0x0d,0x24,
        0x00,0x00,0x00,0x18,0x49,0x44,0x41,0x54,
        0x78,0x9c,0x05,0xc1,0x81,0x01,0x00,0x00,
        0x04,0xc0,0xa0,0xf8,0xdc,0xe5,0x53,0x22,
        0x69,0x47,0x97,0x3c,0x43,0x70,0x07,0x7e,
        0x0d,0x8c,0x70,0x38,0x00,0x00,0x00,0x00,
        0x49,0x45,0x4e,0x44,0xae,0x42,0x60,0x82
    };
    stk360::TextureImage image;
    std::string error;
    if (!stk360::TextureLoader::decode(
            png, sizeof(png), 512, &image, &error))
    {
        std::cerr << "PNG decode failed: " << error << "\n";
        return 1;
    }

    if (image.width != 2 || image.height != 2 || image.row_pitch != 8 ||
        image.argb_pixels.size() != 16 || !image.has_alpha)
    {
        std::cerr << "Decoded texture dimensions/alpha are incorrect\n";
        return 1;
    }

    // First source pixel is opaque red, represented as A,R,G,B.
    if (image.argb_pixels[0] != 255 || image.argb_pixels[1] != 255 ||
        image.argb_pixels[2] != 0 || image.argb_pixels[3] != 0)
    {
        std::cerr << "A8R8G8B8 channel conversion is incorrect\n";
        return 1;
    }

    stk360::TextureImage reduced;
    if (!stk360::TextureLoader::decode(
            png, sizeof(png), 1, &reduced, &error) ||
        reduced.width != 1 || reduced.height != 1)
    {
        std::cerr << "Texture downscale failed\n";
        return 1;
    }

    const unsigned char invalid[] = { 0, 1, 2, 3 };
    if (stk360::TextureLoader::decode(
            invalid, sizeof(invalid), 512, &reduced, &error))
    {
        std::cerr << "Invalid texture data was accepted\n";
        return 1;
    }

    std::cout << "Texture loader tests passed\n";
    return 0;
}
