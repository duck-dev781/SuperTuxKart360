#ifndef STK360_KART_DYNAMICS_H
#define STK360_KART_DYNAMICS_H

#include "TrackDataReader.h"

namespace stk360
{
    struct KartInput
    {
        float steering;
        float throttle;
        float brake;
        KartInput();
    };

    struct KartState
    {
        TrackPoint position;
        float yaw;
        float speed;
        unsigned int current_quad;
        unsigned int completed_laps;
        KartState();
    };

    class KartDynamics
    {
    public:
        static bool initialize(const TrackData& track, KartState* state);
        static void update(const TrackData& track,
                           const KartInput& input,
                           float delta_seconds,
                           KartState* state);
    };
}

#endif
