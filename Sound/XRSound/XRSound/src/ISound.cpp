#include "ISound.h"
#include <stdlib.h>
#include <sndfile.h>
#include <vector>
#include <cstring>

static void CheckError(const char *msg) {
    ALCenum error;

    error = alGetError();
    if (error != AL_NO_ERROR) {
        fprintf(stderr, "alGetError == %d\n", error);
        exit(1);
    }
}

static std::vector<std::string> GetAudioDevices()
{
    const ALCchar *devices = alcGetString(NULL, ALC_DEVICE_SPECIFIER);
    std::vector<std::string> ret;

    const ALCchar *next = devices + 1;
    size_t len = 0;

    while (devices && *devices != '\0' && next && *next != '\0') {
            ret.push_back(devices);
            len = strlen(devices);
            devices += (len + 1);
            next += (len + 2);
    }
    return ret;
}

ISoundEngine::ISoundEngine() {
    m_Initialised = false;

    auto v = GetAudioDevices();

    m_device = alcOpenDevice(v[0].c_str());
    if(m_device) {
        m_context = alcCreateContext(m_device, NULL);
        if(m_context) {
            if (alcMakeContextCurrent(m_context)) {
                m_Initialised = true;
            } else {
                alcDestroyContext(m_context);
                alcCloseDevice(m_device);
            }
        } else {
            alcCloseDevice(m_device);
        }
    }
    CheckError("ISoundEngine::ISoundEngine");
}
 
ISoundEngine::~ISoundEngine() {
    if(m_Initialised) {
        alcMakeContextCurrent(NULL);
        alcDestroyContext(m_context);
        alcCloseDevice(m_device);
    }
}

void ISoundEngine::drop() {
    printf("ISoundEngine::drop\n");
}
//Only returns a pointer to an ISound if the parameters 'track', 'startPaused' or 'enableSoundEffects' have been set to true.
ISound *ISoundEngine::play2D(const char *soundFileName, bool playLooped, bool startPaused, bool track) {
    //printf("ISoundEngine::play2D %s %x\n", soundFileName, this);

    auto v = m_buffers.find(soundFileName);
    ALuint buffer = 0;
    if(v==m_buffers.end()) {
        //printf("ISoundEngine::play2D: late loading of %s\n", soundFileName);
        buffer = preload(soundFileName);
    } else {
        //printf("ISoundEngine::play2D: using cache\n");
        buffer = v->second;
    }

    if(buffer == 0) {
        fprintf(stderr,"failed to create buffer for %s\n", soundFileName);
        exit(EXIT_FAILURE);
    }

    ISound *snd = new ISound(buffer, playLooped, startPaused);

    if(startPaused || track)
        return snd;
    else
        return nullptr;
}

const char *ISoundEngine::getDriverName() { 
    printf("ISoundEngine::getDriverName\n");
    return "OpenAL";
}

static ALuint LoadSound(const char *Filename)
{
    printf("Loading %s\n", Filename);
    // Ouverture du fichier audio avec libsndfile
    SF_INFO FileInfos;
    SNDFILE* File = sf_open(Filename, SFM_READ, &FileInfos);
    if (!File) {
        fprintf(stderr, "Failed to open %s\n", Filename);
        abort();
        return 0;
    }

    // Lecture du nombre d'échantillons et du taux d'échantillonnage (nombre d'échantillons à lire par seconde)
    ALsizei NbSamples  = static_cast<ALsizei>(FileInfos.channels * FileInfos.frames);
    ALsizei SampleRate = static_cast<ALsizei>(FileInfos.samplerate);

    // Lecture des échantillons audio au format entier 16 bits signé (le plus commun)
    std::vector<ALshort> Samples(NbSamples);
    if (sf_read_short(File, &Samples[0], NbSamples) < NbSamples) {
        fprintf(stderr, "Failed to read %s\n", Filename);
        abort();
        return 0;
    }

    // Fermeture du fichier
    sf_close(File);

    // Détermination du format en fonction du nombre de canaux
    ALenum Format;
    switch (FileInfos.channels)
    {
        case 1 :  Format = AL_FORMAT_MONO16;   break;
        case 2 :  Format = AL_FORMAT_STEREO16; break;
        default : return 0;
    }

    // Création du tampon OpenAL
    ALuint Buffer;
    alGenBuffers(1, &Buffer);

    // Remplissage avec les échantillons lus
    alBufferData(Buffer, Format, &Samples[0], NbSamples * sizeof(ALushort), SampleRate);
 
    // Vérification des erreurs
    if (alGetError() != AL_NO_ERROR) {
        fprintf(stderr, "alBufferData Error on %s\n", Filename);
        abort();
        return 0;
    }
 
    return Buffer;
}

ALuint ISoundEngine::preload(const char *soundFileName) {
    auto v = m_buffers.find(soundFileName);
    if(v == m_buffers.end()) {
        ALuint buffer = LoadSound(soundFileName);
        if(buffer == 0) {
            fprintf(stderr,"failed to create buffer for %s\n", soundFileName);
            exit(EXIT_FAILURE);
        }
        m_buffers.insert({soundFileName, buffer});
        return buffer;
    }
    return v->second;
}

ISound::ISound(ALuint buffer, bool looped, bool paused) {
    alGenSources((ALuint)1, &m_source);
    CheckError("alGenSources");

    alSourcef(m_source, AL_PITCH, 1);
    CheckError("alSourcef(m_source, AL_PITCH, 1)");

    alSourcef(m_source, AL_GAIN, 1);
    CheckError("alSourcef(m_source, AL_GAIN, 1)");

    alSource3f(m_source, AL_POSITION, 0, 0, 0);
    CheckError("alSource3f(m_source, AL_POSITION, 0, 0, 0)");

    alSource3f(m_source, AL_VELOCITY, 0, 0, 0);
    CheckError("alSource3f(m_source, AL_VELOCITY, 0, 0, 0)");

    alSourcei(m_source, AL_LOOPING, looped ? AL_TRUE : AL_FALSE);
    CheckError("alSourcei(m_source, AL_LOOPING, looped ? AL_TRUE : AL_FALSE)");

    alSourcei(m_source, AL_BUFFER, buffer);
    CheckError("alSourcei(m_source, AL_BUFFER, buffer)");
}
ISound::~ISound() {
    alDeleteSources(1, &m_source);
    CheckError("alDeleteSources(1, &m_source)");
}

bool ISound::isFinished() {
    ALint source_state;
    alGetSourcei(m_source, AL_SOURCE_STATE, &source_state);
    CheckError("alGetSourcei(m_source, AL_SOURCE_STATE, &source_state)");
    // check for errors
    return source_state == AL_STOPPED;
}
void ISound::setVolume(float vol) {
    //printf("ISound::setvolume %f\n", vol);
    alSourcef(m_source, AL_GAIN,vol);
    CheckError("alSourcef(m_source, AL_GAIN,vol)");
}
void ISound::setIsLooped(bool looped) {
    //printf("ISound::setIsLooped %d\n",looped);
    alSourcei(m_source, AL_LOOPING, looped ? AL_TRUE : AL_FALSE);
    CheckError("alSourcei(m_source, AL_LOOPING, looped ? AL_TRUE : AL_FALSE)");
}
void ISound::setIsPaused(bool paused) {
    //printf("ISound::setIsPaused %d\n",paused);
    if(paused) {
        alSourcePause(m_source);
    } else {
        ALint source_state;
        alGetSourcei(m_source, AL_SOURCE_STATE, &source_state);
        if(source_state != AL_PLAYING)
            alSourcePlay(m_source);
    }
    CheckError("alSourcePlay/alSourcePause");
}
void ISound::setPan(float pan) {
    printf("ISound::setpan %f\n", pan);
}
float ISound::getPan() {
    printf("ISound::getPan\n");
    return 0.0f;
}
bool ISound::setPlaybackSpeed(float speed) {
    printf("ISound::setPlaybackSpeed %f\n", speed); return true;
}
float ISound::getPlaybackSpeed() {
    printf("ISound::getPlaybackSpeed\n");
    return 1.0f;
}
bool ISound::setPlayPosition(unsigned int pos) {
    printf("ISound::setPlayPosition %d\n", pos);
    return true;
}
unsigned int ISound::getPlayPosition() {
    printf("ISound::getPlayPosition\n");
    return 0;
}
void ISound::drop() {
    //printf("ISound::drop\n");
    alSourceStop(m_source);
    delete this;
}
void ISound::stop() {
//    printf("ISound::stop\n");
    alSourceStop(m_source);
    CheckError("alSourceStop(m_source)/alSourcePause");
}
