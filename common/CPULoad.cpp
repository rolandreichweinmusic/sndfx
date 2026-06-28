#include "CPULoad.h"

using namespace etl::chrono_literals;

CPULoad::CPULoad()
  : etl::singleton_base<CPULoad>(*this)
  , _measurementPoint(clock::now())
  , _idleDuration(clock::duration::zero())
{
}

void CPULoad::add(const clock::duration& duration)
{
    _idleDuration += duration;
}

void CPULoad::newPeriod()
{
    if (clock::now() - _measurementPoint < _measurementPeriod) {
        return;
    }

    _measurementPoint = clock::now();
    _lastLoad = 1.0f - static_cast<etl::chrono::duration<double, clock::period>>(_idleDuration) / _measurementPeriod;
    _idleDuration = clock::duration::zero();
}

float CPULoad::lastLoad() const
{
    return _lastLoad;
}

CPULoad::clock::time_point CPULoad::measurementPoint() const
{
    return _measurementPoint;
}

CPULoad::IdleGuard::IdleGuard(CPULoad& cpuLoad): _cpuLoad(cpuLoad), _start(clock::now())
{
}

CPULoad::IdleGuard::~IdleGuard()
{
    const auto duration = clock::now() - _start;
    _cpuLoad.add(duration);
}
