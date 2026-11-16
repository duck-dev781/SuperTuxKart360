#include "KartDynamics.h"

#include <cmath>

namespace stk360
{
    namespace
    {
        float clamp(float value, float minimum, float maximum)
        {
            if (value < minimum) return minimum;
            if (value > maximum) return maximum;
            return value;
        }
    }

    KartInput::KartInput() : steering(0.0f), throttle(0.0f), brake(0.0f)
    {
    }

    KartState::KartState()
        : yaw(0.0f), speed(0.0f), current_quad(0), completed_laps(0)
    {
    }

    bool KartDynamics::initialize(const TrackData& track, KartState* state)
    {
        if (!state || track.quads.empty())
            return false;

        unsigned int start = track.main_loop_start;
        if (start >= track.quads.size())
            start = 0;

        const unsigned int next = track.firstSuccessor(start);
        const TrackPoint& start_center = track.quads[start].center;
        const TrackPoint& next_center = track.quads[next].center;

        state->position = start_center;
        state->position.y += 0.25f;
        state->yaw = std::atan2(next_center.x - start_center.x,
                                next_center.z - start_center.z);
        state->speed = 0.0f;
        state->current_quad = start;
        state->completed_laps = 0;
        return true;
    }

    void KartDynamics::update(const TrackData& track,
                              const KartInput& input,
                              float delta_seconds,
                              KartState* state)
    {
        if (!state || track.quads.empty())
            return;

        const float dt = clamp(delta_seconds, 0.001f, 0.05f);
        const float steering = clamp(input.steering, -1.0f, 1.0f);
        const float throttle = clamp(input.throttle, 0.0f, 1.0f);
        const float brake = clamp(input.brake, 0.0f, 1.0f);

        state->speed += throttle * 18.0f * dt;
        if (brake > 0.0f)
        {
            const float brake_rate = state->speed >= 0.0f ? 30.0f : 14.0f;
            if (state->speed > 0.0f)
                state->speed -= brake * brake_rate * dt;
            else
                state->speed += brake * brake_rate * dt;
        }

        state->speed *= 1.0f - 0.22f * dt;
        state->speed = clamp(state->speed, -8.0f, 28.0f);

        const float speed_factor = 0.55f + 1.2f *
            (std::fabs(state->speed) / 28.0f);
        state->yaw += steering * speed_factor * dt *
                      (state->speed < 0.0f ? -1.0f : 1.0f);

        const TrackPoint old_position = state->position;
        const unsigned int old_quad = state->current_quad;

        state->position.x += std::sin(state->yaw) * state->speed * dt;
        state->position.z += std::cos(state->yaw) * state->speed * dt;

        unsigned int new_quad = old_quad;
        if (!track.findQuad(state->position.x, state->position.y,
                            state->position.z, old_quad, &new_quad))
        {
            state->position = old_position;
            state->speed *= 0.35f;
            return;
        }

        state->current_quad = new_quad;
        state->position.y = track.quads[new_quad].center.y + 0.25f;

        const unsigned int loop_start = track.main_loop_start;
        const unsigned int loop_end = track.main_loop_end;
        if (old_quad >= loop_end - (loop_end > loop_start + 3 ? 3 : 0) &&
            new_quad <= loop_start + 3)
            ++state->completed_laps;
    }
}
