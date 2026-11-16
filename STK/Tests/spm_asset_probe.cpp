#include "../Source/SPMReader.h"

#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace
{
    bool inspect(const std::string& path)
    {
        std::ifstream input(path.c_str(), std::ios::binary);
        if (!input)
        {
            std::cerr << "Cannot open official asset: " << path << "\n";
            return false;
        }

        std::vector<unsigned char> bytes(
            (std::istreambuf_iterator<char>(input)),
            std::istreambuf_iterator<char>());
        if (bytes.empty())
        {
            std::cerr << "Official asset is empty: " << path << "\n";
            return false;
        }

        stk360::SPMModel model;
        std::string error;
        const stk360::SPMReadResult result =
            stk360::SPMReader::read(&bytes[0],
                                    static_cast<unsigned int>(bytes.size()),
                                    &model, &error);
        if (result != stk360::SPM_READ_OK)
        {
            std::cerr << "SPM parse failed for " << path << ": " << error
                      << " (result " << static_cast<int>(result) << ")\n";
            return false;
        }

        unsigned int vertices = 0;
        unsigned int indices = 0;
        for (unsigned int i = 0; i < model.submeshes.size(); ++i)
        {
            vertices += static_cast<unsigned int>(model.submeshes[i].vertices.size());
            indices += static_cast<unsigned int>(model.submeshes[i].indices.size());
        }

        std::cout << path
                  << ": materials=" << model.materials.size()
                  << ", submeshes=" << model.submeshes.size()
                  << ", vertices=" << vertices
                  << ", indices=" << indices
                  << ", requires_skinning=" << (model.requires_skinning ? "yes" : "no")
                  << "\n";

        return !model.submeshes.empty() && vertices != 0 && indices != 0;
    }
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cerr << "Usage: spm-asset-probe file.spm [file.spm ...]\n";
        return 2;
    }

    for (int i = 1; i < argc; ++i)
    {
        if (!inspect(argv[i]))
            return 1;
    }

    return 0;
}
