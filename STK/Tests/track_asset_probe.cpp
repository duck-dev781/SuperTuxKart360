#include "../Source/TrackDataReader.h"

#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace
{
    bool readText(const char* path, std::string* output)
    {
        std::ifstream input(path, std::ios::binary);
        if (!input)
            return false;
        output->assign((std::istreambuf_iterator<char>(input)),
                       std::istreambuf_iterator<char>());
        return !output->empty();
    }
}

int main(int argc, char** argv)
{
    if (argc != 4)
    {
        std::cerr << "Usage: track-asset-probe quads.xml graph.xml track.xml\n";
        return 2;
    }

    std::string quads_xml;
    std::string graph_xml;
    std::string track_xml;
    if (!readText(argv[1], &quads_xml) ||
        !readText(argv[2], &graph_xml) ||
        !readText(argv[3], &track_xml))
    {
        std::cerr << "Could not read one or more official track XML files\n";
        return 1;
    }

    stk360::TrackData track;
    std::string error;
    stk360::TrackReadResult result =
        stk360::TrackDataReader::readQuads(quads_xml.data(),
            static_cast<unsigned int>(quads_xml.size()), &track, &error);
    if (result != stk360::TRACK_READ_OK)
    {
        std::cerr << "quads.xml parse failed: " << error << "\n";
        return 1;
    }

    result = stk360::TrackDataReader::readGraph(graph_xml.data(),
        static_cast<unsigned int>(graph_xml.size()), &track, &error);
    if (result != stk360::TRACK_READ_OK)
    {
        std::cerr << "graph.xml parse failed: " << error << "\n";
        return 1;
    }

    result = stk360::TrackDataReader::readTrackInfo(track_xml.data(),
        static_cast<unsigned int>(track_xml.size()), &track, &error);
    if (result != stk360::TRACK_READ_OK)
    {
        std::cerr << "track.xml parse failed: " << error << "\n";
        return 1;
    }

    if (track.main_loop_start >= track.quads.size() ||
        track.main_loop_end >= track.quads.size() ||
        track.successors.size() != track.quads.size())
    {
        std::cerr << "Track graph references invalid quad indices\n";
        return 1;
    }

    const stk360::TrackPoint& start = track.quads[track.main_loop_start].center;
    unsigned int located_quad = 0xFFFFFFFFu;
    if (!track.findQuad(start.x, start.y + 0.25f, start.z,
                        track.main_loop_start, &located_quad) ||
        located_quad != track.main_loop_start)
    {
        std::cerr << "Could not locate the official track start quad\n";
        return 1;
    }

    std::cout << "Track: " << track.name
              << ", quads=" << track.quads.size()
              << ", main_loop=" << track.main_loop_start << "-"
              << track.main_loop_end
              << ", laps=" << track.default_laps
              << ", reverse=" << (track.reverse_available ? "yes" : "no")
              << ", start_successors="
              << track.successors[track.main_loop_start].size()
              << "\n";
    return 0;
}
