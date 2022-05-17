// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ======================================================================
//                     ORBITER SOFTWARE DEVELOPMENT KIT
// Orbitersdk.h
// ORBITER Application Programming Interface (OAPI)
// ======================================================================

#ifndef __ORBITERSDK_H
#define __ORBITERSDK_H

#include "DrawAPI.h"
#include "OrbiterAPI.h"
#include "ModuleAPI.h"
#include "DrawAPI.h"
#include "CelBodyAPI.h"
#include "VesselAPI.h"
#include "MFDAPI.h"

inline int sprintf_s(char *buffer, size_t s, const char* str) {
	return sprintf(buffer, "%s", str);
}
inline int sprintf_s(char *buffer, const char* str) {
	return sprintf(buffer, "%s", str);
}

template<size_t sizeOfBuffer, typename ...Args>
int sprintf_s(char (&buffer)[sizeOfBuffer], const char* format, Args ...args) {
	return sprintf(buffer, format, args...);
}
template<typename ...Args>
int sprintf_s(char *buffer, size_t s, const char* format, Args ...args) {
	return sprintf(buffer, format, args...);
}

#define stricmp strcasecmp
#define strnicmp strncasecmp
#define _stricmp strcasecmp
#define _strnicmp strncasecmp
#define UINT unsigned int
#define LPVOID void *
#define VOID void
#define _countof std::size
#define PLAYCOUNTDOWNWHENTAKEOFF 0
#define PLAYCABINAIRCONDITIONING 0
#define PLAYCABINRANDOMAMBIANCE 0
#define PLAYLANDINGANDGROUNDSOUND 0
#define PLAYRADIOATC 0
#define PLAYRADARBIP 0
#define DISPLAYTIMER 0
#define BOTHVIEW_FADED_MEDIUM 0
#define INTERNAL_ONLY 0
#define BOTHVIEW_FADED_CLOSE 0
#define PLAYWHENATTITUDEMODECHANGE 0
#define PLAYDOCKINGSOUND 0

#define _snprintf snprintf
inline bool PlayVesselWave(int,int,int,int,int) {return true;}
inline void StopVesselWave(int, int) {}
inline int IsPlaying(int, int) {return 0;}
inline int ConnectToOrbiterSoundDLL(OBJHANDLE) {return 0;}
inline int RequestLoadVesselWave(int, int, const char *, int) {return 0;}
inline void SoundOptionOnOff(int, int, bool) {}
const int LOOP = 0;
#endif // !__ORBITERSDK_H