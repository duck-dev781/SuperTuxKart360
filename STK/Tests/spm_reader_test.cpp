#include "../Source/SPMReader.h"

#include <cmath>
#include <cstring>
#include <iostream>
#include <vector>

namespace
{
    void appendU8(std::vector<unsigned char>& bytes, unsigned char value)
    {
        bytes.push_back(value);
    }

    void appendU16(std::vector<unsigned char>& bytes, unsigned short value)
    {
        bytes.push_back(static_cast<unsigned char>(value & 255u));
        bytes.push_back(static_cast<unsigned char>((value >> 8) & 255u));
    }

    void appendU32(std::vector<unsigned char>& bytes, unsigned int value)
    {
        bytes.push_back(static_cast<unsigned char>(value & 255u));
        bytes.push_back(static_cast<unsigned char>((value >> 8) & 255u));
        bytes.push_back(static_cast<unsigned char>((value >> 16) & 255u));
        bytes.push_back(static_cast<unsigned char>((value >> 24) & 255u));
    }

    void appendFloat(std::vector<unsigned char>& bytes, float value)
    {
        unsigned int bits;
        std::memcpy(&bits, &value, sizeof(bits));
        appendU32(bytes, bits);
    }

    void appendString(std::vector<unsigned char>& bytes, const char* value)
    {
        const unsigned int length = static_cast<unsigned int>(std::strlen(value));
        appendU8(bytes, static_cast<unsigned char>(length));
        bytes.insert(bytes.end(), value, value + length);
    }

    void appendVertex(std::vector<unsigned char>& bytes,
                      float x, float y, float z,
                      unsigned short u, unsigned short v)
    {
        appendFloat(bytes, x);
        appendFloat(bytes, y);
        appendFloat(bytes, z);
        appendU32(bytes, 0x1FF00000u); // packed +Z normal
        appendU16(bytes, u);
        appendU16(bytes, v);
    }

    std::vector<unsigned char> makeStaticTriangle()
    {
        std::vector<unsigned char> bytes;
        appendU8(bytes, 'S');
        appendU8(bytes, 'P');
        appendU8(bytes, 0x0Au); // version 1, SPMN variant
        appendU8(bytes, 0x01u); // normals present

        appendFloat(bytes, -1.0f);
        appendFloat(bytes, -1.0f);
        appendFloat(bytes, -1.0f);
        appendFloat(bytes, 1.0f);
        appendFloat(bytes, 1.0f);
        appendFloat(bytes, 1.0f);

        appendU16(bytes, 1); // one material
        appendString(bytes, "kart.png");
        appendString(bytes, "");

        appendU16(bytes, 1); // one LOD
        appendU16(bytes, 1); // one submesh
        appendU32(bytes, 3); // vertices
        appendU32(bytes, 3); // indices
        appendU16(bytes, 0); // material id

        appendVertex(bytes, 0.0f, 0.5f, 0.0f, 0x0000u, 0x0000u);
        appendVertex(bytes, -0.5f, -0.5f, 0.0f, 0x3C00u, 0x0000u);
        appendVertex(bytes, 0.5f, -0.5f, 0.0f, 0x0000u, 0x3C00u);

        appendU8(bytes, 0);
        appendU8(bytes, 1);
        appendU8(bytes, 2);
        return bytes;
    }

    bool closeEnough(float a, float b)
    {
        return std::fabs(a - b) < 0.0001f;
    }
}

int main()
{
    std::vector<unsigned char> bytes = makeStaticTriangle();
    stk360::SPMModel model;
    std::string error;

    if (stk360::SPMReader::read(&bytes[0], static_cast<unsigned int>(bytes.size()),
                                &model, &error) != stk360::SPM_READ_OK)
    {
        std::cerr << "SPM reader rejected valid fixture: " << error << "\n";
        return 1;
    }

    if (model.materials.size() != 1 ||
        model.materials[0].texture1 != "kart.png" ||
        model.submeshes.size() != 1 ||
        model.submeshes[0].vertices.size() != 3 ||
        model.submeshes[0].indices.size() != 3)
    {
        std::cerr << "SPM reader returned incorrect mesh structure\n";
        return 1;
    }

    if (!closeEnough(model.submeshes[0].vertices[1].position[0], -0.5f) ||
        !closeEnough(model.submeshes[0].vertices[1].uv[0], 1.0f) ||
        model.submeshes[0].vertices[0].normal[2] < 0.99f)
    {
        std::cerr << "SPM reader decoded vertex data incorrectly\n";
        return 1;
    }

    bytes.resize(bytes.size() - 2);
    if (stk360::SPMReader::read(&bytes[0], static_cast<unsigned int>(bytes.size()),
                                &model, &error) != stk360::SPM_READ_TRUNCATED)
    {
        std::cerr << "SPM reader did not reject a truncated index stream\n";
        return 1;
    }
    if (!model.submeshes.empty())
    {
        std::cerr << "SPM reader left partial data after failure\n";
        return 1;
    }

    std::cout << "SPM reader tests passed\n";
    return 0;
}
