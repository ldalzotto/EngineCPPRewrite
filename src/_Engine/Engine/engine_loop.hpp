#pragma once

struct EngineLoop
{
    // To avoid crazy amount of update if the elapsed time from last frame is huge
    static const uint8 MAX_UPDATE_CALL_PER_FRAME = 2;

    time_t timebetweenupdates_mics;

    time_t previousUpdateTime_mics;
    time_t accumulatedelapsedtime_mics;

    inline static EngineLoop allocate_default(const time_t p_timebetweenupdates_mics)
    {
        return EngineLoop{p_timebetweenupdates_mics, 0, clock_currenttime_mics()};
    };

    inline int8 update(float32* out_delta)
    {
        time_t l_currentTime = clock_currenttime_mics();
        time_t l_elapsed = l_currentTime - this->previousUpdateTime_mics;

        if (l_elapsed > (time_t)(this->timebetweenupdates_mics) * EngineLoop::MAX_UPDATE_CALL_PER_FRAME)
        {
            l_elapsed = (time_t)(this->timebetweenupdates_mics) * EngineLoop::MAX_UPDATE_CALL_PER_FRAME;
        }

        this->previousUpdateTime_mics = l_currentTime;
        this->accumulatedelapsedtime_mics += l_elapsed;

        if (this->accumulatedelapsedtime_mics >= this->timebetweenupdates_mics)
        {

            *out_delta = this->timebetweenupdates_mics * 0.000001f;
            // this->update_internal(this->timebetweenupdates_mics * 0.000001f, p_callback);

            // TODO -> having a more precise loop (while delta < max  {} delta -= max) but preventing some piece of code to run twice.
            this->accumulatedelapsedtime_mics = 0;
            return 1;
        }
        else
        {
            Thread::wait((uimax)((this->timebetweenupdates_mics - this->accumulatedelapsedtime_mics) * 0.0009999));
        }
        return 0;
    };

    inline int8 update_forced_delta(const float32 p_delta)
    {
        time_t l_currentTime = clock_currenttime_mics();
        this->previousUpdateTime_mics = l_currentTime;
        this->accumulatedelapsedtime_mics += (time_t)p_delta;
        return 1;
    };
};