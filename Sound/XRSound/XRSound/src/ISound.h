#pragma once

#include <stdio.h>
#include <AL/al.h>
#include <AL/alc.h>
#include <unordered_map>
#include <string>

class ISound {
public:
    ISound(ALuint buffer, bool looped, bool paused);
    ~ISound();
    bool isFinished();
    void setVolume(float vol);
    void setIsLooped(bool looped);
    void setIsPaused(bool paused);
    void setPan(float pan);
    float getPan();
    bool setPlaybackSpeed(float speed);
    float getPlaybackSpeed();
    bool setPlayPosition(unsigned int pos);
    unsigned int getPlayPosition();
    void drop();
    void stop();
private:
    ALuint m_source;
};

class ISoundEngine {
public:
    ISoundEngine();
    ~ISoundEngine();
    void drop();
    ISound *play2D(const char *soundFileName, bool playLooped=false, bool startPaused=false, bool track=false);
    const char *getDriverName();
    ALuint preload(const char *soundFileName);
private:
    bool m_Initialised;
    ALCdevice *m_device;
    ALCcontext *m_context;
    std::unordered_map<std::string, ALuint> m_buffers;
};
