#ifndef STK360_TRACK_DATA_READER_H
#define STK360_TRACK_DATA_READER_H

#include <string>
#include <vector>

namespace stk360
{
    struct TrackPoint
    {
        float x;
        float y;
        float z;
        TrackPoint();
    };

    struct TrackQuad
    {
        TrackPoint points[4];
        TrackPoint center;
        float min_y;
        float max_y;
        bool invisible;
        bool ai_ignore;
        TrackQuad();
    };

    struct TrackData
    {
        std::string name;
        unsigned int default_laps;
        bool reverse_available;
        float min_height_testing;
        float max_height_testing;
        unsigned int main_loop_start;
        unsigned int main_loop_end;
        std::vector<TrackQuad> quads;
        std::vector< std::vector<unsigned int> > successors;

        TrackData();
        void clear();

        bool containsPoint(unsigned int quad_index,
                           float x, float y, float z) const;
        bool findQuad(float x, float y, float z,
                      unsigned int hint, unsigned int* result) const;
        unsigned int firstSuccessor(unsigned int quad_index) const;
    };

    enum TrackReadResult
    {
        TRACK_READ_OK,
        TRACK_READ_INVALID_ARGUMENT,
        TRACK_READ_BAD_XML,
        TRACK_READ_TRUNCATED,
        TRACK_READ_INVALID_DATA,
        TRACK_READ_LIMIT_EXCEEDED
    };

    class TrackDataReader
    {
    public:
        // Reads the track's quads.xml. Point references such as "12:3" are
        // resolved against earlier quads, matching STK's DriveGraph format.
        static TrackReadResult readQuads(const char* xml,
                                         unsigned int size,
                                         TrackData* output,
                                         std::string* error_message = 0);

        // Reads graph.xml edge-loop, edge-line, and edge records.
        static TrackReadResult readGraph(const char* xml,
                                         unsigned int size,
                                         TrackData* output,
                                         std::string* error_message = 0);

        // Reads the track name, default lap count, and reverse availability.
        static TrackReadResult readTrackInfo(const char* xml,
                                             unsigned int size,
                                             TrackData* output,
                                             std::string* error_message = 0);
    };
}

#endif
