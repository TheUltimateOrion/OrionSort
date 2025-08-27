#include "core/audio/system.h"

#include "core/logging/logging.h"

#include <format>

#include <cstring>

namespace Core::Audio
{
    System::~System() noexcept
    {
        if (alcGetCurrentContext() == m_audioContext) { alcMakeContextCurrent(nullptr); }

        if (m_audioContext)
        {
            alcDestroyContext(m_audioContext);
            m_audioContext = nullptr;
        }
        if (m_audioDevice)
        {
            alcCloseDevice(m_audioDevice);
            m_audioDevice = nullptr;
        }
    }

    std::string System::alErrorToString(ALenum t_err) const noexcept
    {
        switch (t_err)
        {
            case AL_NO_ERROR     : return "AL_NO_ERROR";
            case AL_INVALID_NAME : return "AL_INVALID_NAME: bad name (ID)";
            case AL_INVALID_ENUM : return "AL_INVALID_ENUM: invalid enum";
            case AL_INVALID_VALUE: return "AL_INVALID_VALUE: invalid value";
            case AL_INVALID_OPERATION:
                return "AL_INVALID_OPERATION: no current context or invalid state";
            case AL_OUT_OF_MEMORY: return "AL_OUT_OF_MEMORY";
            default              : return std::format("UNKNOWN AL ERROR: {}", static_cast<int>(t_err));
        }
    }

    std::string System::alcErrorToString(ALCenum t_err) const noexcept
    {
        switch (t_err)
        {
            case ALC_NO_ERROR       : return "ALC_NO_ERROR";
            case ALC_INVALID_DEVICE : return "ALC_INVALID_DEVICE";
            case ALC_INVALID_CONTEXT: return "ALC_INVALID_CONTEXT";
            case ALC_INVALID_ENUM   : return "ALC_INVALID_ENUM";
            case ALC_INVALID_VALUE  : return "ALC_INVALID_VALUE";
            case ALC_OUT_OF_MEMORY  : return "ALC_OUT_OF_MEMORY";
            default                 : return std::format("UNKNOWN ALC ERROR: {}", static_cast<int>(t_err));
        }
    }

    void System::checkEnumerationExtension() noexcept
    {
        m_hasEnumeration = alcIsExtensionPresent((ALCdevice*) nullptr, "ALC_ENUMERATION_EXT");
    }

    void System::acquireAudioDeviceList() noexcept
    {
        bool const     hasEnumerateAll = alcIsExtensionPresent(nullptr, "ALC_ENUMERATE_ALL_EXT");
        ALCenum const  deviceSpecifier = hasEnumerateAll ? ALC_ALL_DEVICES_SPECIFIER
                                                         : ALC_DEVICE_SPECIFIER;

        ALCchar const* device          = alcGetString(nullptr, deviceSpecifier);

        LOG_INFO("Found Devices: ");
        while (*device != '\0')
        {
            m_audioDeviceList.emplace_back(device);
            device += strlen(device) + 1;
        }

        for (auto it = m_audioDeviceList.begin(); it != m_audioDeviceList.end(); ++it)
        {
            LOG_INFO(std::format("    {}", *it));
        }
    }

    Utils::Signal System::openDefaultAudioDevice() noexcept
    {
        ALCchar const* defaultDevice = alcGetString(nullptr, ALC_DEFAULT_DEVICE_SPECIFIER);
        m_activeDeviceName           = defaultDevice ? defaultDevice : "";
        LOG_INFO(
            std::format(
                "Opening default OpenAL audio device: {}", defaultDevice ? defaultDevice : "<null>"
            )
        );

        m_audioDevice = alcOpenDevice(defaultDevice);
        if (!m_audioDevice)
        {
            m_lastAlcError = alcGetError(nullptr);
            LOG_ERROR(
                std::format("Failed to open OpenAL device: ", alcErrorToString(m_lastAlcError))
            );
            return Utils::Signal::Error;
        }

        m_lastAlError = alGetError();
        return Utils::Signal::Success;
    }

    Utils::Signal System::createAudioContext() noexcept
    {
        LOG_INFO("Creating new OpenAL context");
        m_audioContext = alcCreateContext(m_audioDevice, nullptr);
        if (!m_audioContext)
        {
            m_lastAlcError = alcGetError(m_audioDevice);
            LOG_ERROR(
                std::format("Failed to create OpenAL context: ", alcErrorToString(m_lastAlcError))
            );
            alcCloseDevice(m_audioDevice);
            m_audioDevice = nullptr;
            return Utils::Signal::Error;
        }

        if (alcMakeContextCurrent(m_audioContext) == ALC_FALSE)
        {
            m_lastAlcError = alcGetError(m_audioDevice);
            LOG_ERROR(
                std::format("Could not bind OpenAL context: ", alcErrorToString(m_lastAlcError))
            );
            alcDestroyContext(m_audioContext);
            m_audioContext = nullptr;
            alcCloseDevice(m_audioDevice);
            m_audioDevice = nullptr;
            return Utils::Signal::Error;
        }

        m_lastAlError = alGetError();
        return Utils::Signal::Success;
    }

    Utils::Signal System::init()
    {
        if (!m_isInitialized) { return Utils::Signal::Success; }

        checkEnumerationExtension();

        if (m_hasEnumeration) { acquireAudioDeviceList(); }

        if (openDefaultAudioDevice() == Utils::Signal::Error) { return Utils::Signal::Error; }

        if (createAudioContext() == Utils::Signal::Error) { return Utils::Signal::Error; }

        m_isInitialized = true;

        return Utils::Signal::Success;
    }
}  // namespace Core::Audio
