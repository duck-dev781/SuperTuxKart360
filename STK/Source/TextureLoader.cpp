#include "TextureLoader.h"

#include <climits>
#include <cstring>

#include "stb_image.h"

namespace stk360
{
    TextureImage::TextureImage()
        : width(0), height(0), row_pitch(0), has_alpha(false)
    {
    }

    void TextureImage::clear()
    {
        width = 0;
        height = 0;
        row_pitch = 0;
        has_alpha = false;
        argb_pixels.clear();
    }

    bool TextureLoader::decode(const unsigned char* bytes,
                               unsigned int size,
                               unsigned int max_dimension,
                               TextureImage* output,
                               std::string* error_message)
    {
        if (!bytes || size == 0 || !output || size > static_cast<unsigned int>(INT_MAX))
        {
            if (error_message)
                *error_message = "invalid texture input";
            return false;
        }

        output->clear();
        int source_width = 0;
        int source_height = 0;
        int source_channels = 0;
        unsigned char* rgba = stbi_load_from_memory(
            bytes, static_cast<int>(size), &source_width,
            &source_height, &source_channels, 4);
        if (!rgba || source_width <= 0 || source_height <= 0)
        {
            if (rgba)
                stbi_image_free(rgba);
            if (error_message)
                *error_message = "unsupported or corrupt PNG/JPEG texture";
            return false;
        }

        unsigned int target_width = static_cast<unsigned int>(source_width);
        unsigned int target_height = static_cast<unsigned int>(source_height);
        if (max_dimension > 0 &&
            (target_width > max_dimension || target_height > max_dimension))
        {
            if (target_width >= target_height)
            {
                target_height = (target_height * max_dimension) / target_width;
                target_width = max_dimension;
            }
            else
            {
                target_width = (target_width * max_dimension) / target_height;
                target_height = max_dimension;
            }
            if (target_width == 0) target_width = 1;
            if (target_height == 0) target_height = 1;
        }

        output->width = target_width;
        output->height = target_height;
        output->row_pitch = target_width * 4;
        output->argb_pixels.resize(
            static_cast<std::vector<unsigned char>::size_type>(target_width) *
            static_cast<std::vector<unsigned char>::size_type>(target_height) * 4);
        output->has_alpha = false;

        for (unsigned int y = 0; y < target_height; ++y)
        {
            const unsigned int source_y =
                (y * static_cast<unsigned int>(source_height)) / target_height;
            for (unsigned int x = 0; x < target_width; ++x)
            {
                const unsigned int source_x =
                    (x * static_cast<unsigned int>(source_width)) / target_width;
                const unsigned int source_offset =
                    (source_y * static_cast<unsigned int>(source_width) + source_x) * 4;
                const unsigned int target_offset = (y * target_width + x) * 4;

                const unsigned char red = rgba[source_offset + 0];
                const unsigned char green = rgba[source_offset + 1];
                const unsigned char blue = rgba[source_offset + 2];
                const unsigned char alpha = rgba[source_offset + 3];

                output->argb_pixels[target_offset + 0] = alpha;
                output->argb_pixels[target_offset + 1] = red;
                output->argb_pixels[target_offset + 2] = green;
                output->argb_pixels[target_offset + 3] = blue;
                if (alpha != 255)
                    output->has_alpha = true;
            }
        }

        stbi_image_free(rgba);
        if (error_message)
            error_message->clear();
        return true;
    }
}
