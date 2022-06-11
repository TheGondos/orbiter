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
#define _snprintf snprintf
#endif // !__ORBITERSDK_H