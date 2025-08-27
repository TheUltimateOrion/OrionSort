#include "core/audio/sound_engine.h"

#include "core/logging/logging.h"

#include <cmath>

namespace
{
    constexpr int      kNumBuffers = 3;       // small, avoids starvation
    constexpr unsigned kRate       = 48000u;  // typical Android mixer rate
    ALuint             s_buffers[kNumBuffers] {};
    bool               s_bufsMade = false;
}  // namespace

SoundEngine* SoundEngine::Get()
{
    static SoundEngine instance;
    return &instance;
}

char const* SoundEngine::alErrorString(ALenum t_err) const noexcept
{
    switch (t_err)
    {
        [[likely]]
        case AL_NO_ERROR:
            return "AL_NO_ERROR";
        case ALC_INVALID_DEVICE : return "ALC_INVALID_DEVICE";
        case ALC_INVALID_CONTEXT: return "ALC_INVALID_CONTEXT";
        case AL_INVALID_VALUE:
            return "AL_INVALID_VALUE";
        [[unlikely]]
        case AL_OUT_OF_MEMORY:
            return "AL_OUT_OF_MEMORY";
        default: return "Unknown error code";
    }
}

Utils::Signal SoundEngine::init()
{
    LOG_INFO("Opening default OpenAL audio device");
    char const* defaultDevice = alcGetString(nullptr, ALC_DEFAULT_ALL_DEVICES_SPECIFIER);
    LOG_INFO("Default Device: " << (defaultDevice ? defaultDevice : "<null>"));

    ALCdevice* dev = alcOpenDevice(defaultDevice);
    if (!dev)
    {
        m_err = alcGetError(nullptr);
        return Utils::Signal::Error;
    }

    LOG_INFO("Creating new OpenAL context");
    ALCcontext* ctx = alcCreateContext(dev, nullptr);
    if (!ctx || alcMakeContextCurrent(ctx) == ALC_FALSE)
    {
        m_err = alcGetError(dev);
        if (ctx) { alcDestroyContext(ctx); }
        alcCloseDevice(dev);
        return Utils::Signal::Error;
    }

    LOG_INFO("Generating OpenAL source");
    alGenSources(1, &m_src);
    AL_CHECK_ERR(Utils::Signal::Error);

    alListenerf(AL_GAIN, 1.0f);
    AL_CHECK_ERR(Utils::Signal::Error);

    alSourcef(m_src, AL_GAIN, 1.0f);
    alSourcei(m_src, AL_LOOPING, AL_FALSE);
    AL_CHECK_ERR(Utils::Signal::Error);

    if (!s_bufsMade)
    {
        alGenBuffers(kNumBuffers, s_buffers);
        AL_CHECK_ERR(Utils::Signal::Error);
        s_bufsMade = true;
    }

    return Utils::Signal::Success;
}

// Stream one chunk into the queue. Safe to call every tick.
Utils::Signal SoundEngine::load(float t_freq, float t_seconds)
{
    if (!s_bufsMade)
    {
        alGenBuffers(kNumBuffers, s_buffers);
        AL_CHECK_ERR(Utils::Signal::Error);
        s_bufsMade = true;
    }

    float const       seconds = t_seconds;
    std::size_t const frames  = static_cast<std::size_t>(seconds * static_cast<float>(kRate));

    // Reuse sample workspace.
    if (m_samples)
    {
        delete[] m_samples;
        m_samples = nullptr;
    }
    m_samples         = new short[frames];

    float const twoPi = 2.0f * static_cast<float>(M_PI);

    // Generate mono waveform into workspace.
    for (std::size_t i = 0; i < frames; ++i)
    {
        float const phase = twoPi * t_freq * (static_cast<float>(i) / static_cast<float>(kRate));
        float       s     = 0.0f;

        switch (m_waveform)
        {
            case Waveform::Sine  : s = std::sin(phase); break;
            case Waveform::Square: s = std::sin(phase) >= 0.0f ? 1.0f : -1.0f; break;
            case Waveform::Triangle:
                s = 2.0f * std::abs(2.0f * (phase / twoPi - std::floor(phase / twoPi + 0.5f)))
                  - 1.0f;
                break;
            case Waveform::Sawtooth:
                s = 2.0f * (phase / twoPi - std::floor(phase / twoPi + 0.5f));
                break;
        }

        m_samples[i] = static_cast<short>(std::round(32760.0f * s));
    }

    // Choose a free buffer: prefer processed, else take a never-queued one while priming.
    ALint processed = 0, queued = 0;
    alGetSourcei(m_src, AL_BUFFERS_PROCESSED, &processed);
    AL_CHECK_ERR(Utils::Signal::Error);
    alGetSourcei(m_src, AL_BUFFERS_QUEUED, &queued);
    AL_CHECK_ERR(Utils::Signal::Error);

    ALuint buf = 0;
    if (processed > 0)
    {
        // Unqueue a finished buffer and reuse it.
        alSourceUnqueueBuffers(m_src, 1, &buf);
        AL_CHECK_ERR(Utils::Signal::Error);
    }
    else if (queued < kNumBuffers)
    {
        // Prime: use the next never-queued buffer by index.
        buf = s_buffers[queued];
    }
    else
    {
        // All buffers are in-flight; drop this chunk silently.
        return Utils::Signal::Success;
    }

    // (Re)fill chosen buffer and queue it back.
    alBufferData(
        buf, AL_FORMAT_MONO16, m_samples, static_cast<ALsizei>(frames * sizeof(short)), kRate
    );
    AL_CHECK_ERR(Utils::Signal::Error);

    alSourceQueueBuffers(m_src, 1, &buf);
    AL_CHECK_ERR(Utils::Signal::Error);

    return Utils::Signal::Success;
}

// Ensure playback is running; does not touch buffer bindings.
Utils::Signal SoundEngine::play()
{
    ALint state = AL_STOPPED;
    alGetSourcei(m_src, AL_SOURCE_STATE, &state);
    AL_CHECK_ERR(Utils::Signal::Error);

    if (state != AL_PLAYING)
    {
        alSourcePlay(m_src);
        AL_CHECK_ERR(Utils::Signal::Error);
    }

    return Utils::Signal::Success;
}

SoundEngine::~SoundEngine() noexcept
{
    // Ensure we still have a valid context before issuing al* calls.
    ALCcontext* ctx = alcGetCurrentContext();
    ALCdevice*  dev = ctx ? alcGetContextsDevice(ctx) : nullptr;

    if (ctx) { alcMakeContextCurrent(ctx); }

    if (m_src)
    {
        // Stop and fully drain the queue so buffers can be deleted safely.
        alSourceStop(m_src);
        ALint queued = 0;
        alGetSourcei(m_src, AL_BUFFERS_QUEUED, &queued);
        while (queued > 0)
        {
            ALuint tmp = 0;
            alSourceUnqueueBuffers(m_src, 1, &tmp);
            alGetSourcei(m_src, AL_BUFFERS_QUEUED, &queued);
        }
        alDeleteSources(1, &m_src);
        m_src = 0;
    }

    if (s_bufsMade)
    {
        alDeleteBuffers(kNumBuffers, s_buffers);
        for (int i = 0; i < kNumBuffers; ++i) { s_buffers[i] = 0; }
        s_bufsMade = false;
    }

    if (m_samples)
    {
        delete[] m_samples;  // array allocation above
        m_samples = nullptr;
    }

    if (ctx)
    {
        alcMakeContextCurrent(nullptr);
        alcDestroyContext(ctx);
    }

    if (dev) { alcCloseDevice(dev); }
}

ALenum SoundEngine::alGetLastError() const noexcept { return m_err; }
