#pragma once

#include <etl/chrono.h>

class CPULoad
{
    public:
        using clock = etl::chrono::high_resolution_clock;

        CPULoad();

        void add(const clock::duration& duration);
        void newPeriod();
        float lastLoad() const;

    class IdleGuard
    {
        public:
            IdleGuard(CPULoad& cpuLoad);
            ~IdleGuard();

        private:
            CPULoad& _cpuLoad;
            clock::time_point _start;
    };

    private:
        static constexpr clock::duration _measurementPeriod = etl::chrono::seconds(1);

        clock::time_point _measurementPoint;
        clock::duration _idleDuration;

        float _lastLoad = 0.0f;
};
