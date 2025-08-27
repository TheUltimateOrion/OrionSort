#pragma once

#include "utils/common.h"

#include <AL/al.h>
#include <AL/alc.h>

#ifndef AL_CHECK_ERR
    #define AL_CHECK_ERR(ret)            \
        if (alGetError() != AL_NO_ERROR) \
        {                                \
            this->m_err = alGetError();  \
            return ret;                  \
        }
#endif

class SoundEngine
{
public:
    enum class Waveform
    {
        Sine,
        Square,
        Triangle,
        Sawtooth,
    };

private:
    ALuint   m_src {0};
    short*   m_samples {nullptr};
    ALenum   m_err {AL_NO_ERROR};
    Waveform m_waveform {Waveform::Sine};

public:
    SoundEngine() noexcept = default;
    ~SoundEngine() noexcept;

    static SoundEngine*         Get();

    ALenum                      alGetLastError() const noexcept;
    char const*                 alErrorString(ALenum t_err) const noexcept;

    [[nodiscard]] Utils::Signal init();

    [[nodiscard]] Utils::Signal load(float t_freq, float t_seconds);

    inline void                 setWaveform(Waveform t_waveform) { m_waveform = t_waveform; }

    [[nodiscard]] Utils::Signal play();
};
