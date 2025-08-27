#pragma once

#include "utils/common.h"

#include <AL/al.h>
#include <AL/alc.h>

#include <vector>

namespace Core::Audio
{
    class System
    {
    private:
        ALCboolean                  m_hasEnumeration {AL_FALSE};
        ALCdevice*                  m_audioDevice;
        ALCcontext*                 m_audioContext;

        ALenum                      m_lastAlError {AL_NO_ERROR};
        ALCenum                     m_lastAlcError {ALC_NO_ERROR};
        std::string                 m_activeDeviceName;

        std::vector<std::string>    m_audioDeviceList {};
        bool                        m_isInitialized {false};

        std::string                 alErrorToString(ALenum t_err) const noexcept;
        std::string                 alcErrorToString(ALenum t_err) const noexcept;
        void                        checkEnumerationExtension() noexcept;
        void                        acquireAudioDeviceList() noexcept;

        [[nodiscard]] Utils::Signal openDefaultAudioDevice() noexcept;
        [[nodiscard]] Utils::Signal createAudioContext() noexcept;

    public:
        inline static System* get()
        {
            static System instance;
            return &instance;
        }

        System() noexcept = default;
        ~System() noexcept;

        System(System const&)                                = delete;
        System& operator=(System const&)                     = delete;
        System(System&&)                                     = delete;
        System&                          operator=(System&&) = delete;

        [[nodiscard]] Utils::Signal      init();

        [[nodiscard]] inline std::string activeDeviceName() const { return m_activeDeviceName; }

        bool                             isInitialized() const noexcept { return m_isInitialized; }
    };
}  // namespace Core::Audio
