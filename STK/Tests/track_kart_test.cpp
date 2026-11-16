#include "../Source/TrackDataReader.h"
#include "../Source/KartDynamics.h"

#include <cmath>
#include <cstring>
#include <iostream>
#include <string>

namespace
{
    const char* QUADS_XML =
        "<?xml version=\"1.0\"?>"
        "<quads>"
        "<height-testing min=\"-1\" max=\"5\"/>"
        "<quad p0=\"0 0 0\" p1=\"0 0 10\" p2=\"5 0 10\" p3=\"5 0 0\"/>"
        "<quad p0=\"0:1\" p1=\"0:2\" p2=\"5 0 20\" p3=\"0 0 20\"/>"
        "<quad p0=\"1:1\" p1=\"1:2\" p2=\"5 0 30\" p3=\"0 0 30\"/>"
        "</quads>";

    const char* GRAPH_XML =
        "<graph><node-list from-quad=\"0\" to-quad=\"2\"/>"
        "<edge-loop from=\"0\" to=\"2\"/></graph>";

    const char* TRACK_XML =
        "<track name=\"Test Circuit\" default-number-of-laps=\"2\" reverse=\"Y\"/>";

    bool closeEnough(float a, float b)
    {
        return std::fabs(a - b) < 0.001f;
    }
}

int main()
{
    stk360::TrackData track;
    std::string error;

    if (stk360::TrackDataReader::readQuads(
            QUADS_XML, static_cast<unsigned int>(std::strlen(QUADS_XML)),
            &track, &error) != stk360::TRACK_READ_OK)
    {
        std::cerr << "Track quads parse failed: " << error << "\n";
        return 1;
    }
    if (track.quads.size() != 3 ||
        !closeEnough(track.quads[1].points[0].x, 0.0f) ||
        !closeEnough(track.quads[1].points[0].z, 10.0f))
    {
        std::cerr << "Track quad references were not resolved\n";
        return 1;
    }

    if (stk360::TrackDataReader::readGraph(
            GRAPH_XML, static_cast<unsigned int>(std::strlen(GRAPH_XML)),
            &track, &error) != stk360::TRACK_READ_OK)
    {
        std::cerr << "Track graph parse failed: " << error << "\n";
        return 1;
    }
    if (track.main_loop_start != 0 || track.main_loop_end != 2 ||
        track.firstSuccessor(0) != 1 || track.firstSuccessor(2) != 0)
    {
        std::cerr << "Track graph edges were not decoded correctly\n";
        return 1;
    }

    if (stk360::TrackDataReader::readTrackInfo(
            TRACK_XML, static_cast<unsigned int>(std::strlen(TRACK_XML)),
            &track, &error) != stk360::TRACK_READ_OK ||
        track.name != "Test Circuit" || track.default_laps != 2 ||
        !track.reverse_available)
    {
        std::cerr << "Track metadata parse failed: " << error << "\n";
        return 1;
    }

    unsigned int quad = 99;
    if (!track.findQuad(2.5f, 0.25f, 5.0f, 0, &quad) || quad != 0 ||
        !track.findQuad(2.5f, 0.25f, 15.0f, 0, &quad) || quad != 1 ||
        track.findQuad(10.0f, 0.25f, 15.0f, 0, &quad))
    {
        std::cerr << "Track point-in-quad test failed\n";
        return 1;
    }

    stk360::KartState kart;
    if (!stk360::KartDynamics::initialize(track, &kart) ||
        kart.current_quad != 0 || !closeEnough(kart.yaw, 0.0f))
    {
        std::cerr << "Kart start state initialization failed\n";
        return 1;
    }

    stk360::KartInput input;
    input.throttle = 1.0f;
    for (unsigned int i = 0; i < 60; ++i)
        stk360::KartDynamics::update(track, input, 1.0f / 60.0f, &kart);

    if (kart.speed <= 0.0f || kart.position.z <= 5.0f || kart.current_quad != 1)
    {
        std::cerr << "Kart did not accelerate along the track\n";
        return 1;
    }

    const float speed_before_braking = kart.speed;
    input.throttle = 0.0f;
    input.brake = 1.0f;
    for (unsigned int i = 0; i < 20; ++i)
        stk360::KartDynamics::update(track, input, 1.0f / 60.0f, &kart);
    if (kart.speed >= speed_before_braking)
    {
        std::cerr << "Kart brake input did not reduce speed\n";
        return 1;
    }

    std::cout << "Track data and kart dynamics tests passed\n";
    return 0;
}
