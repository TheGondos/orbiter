// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// Some strings to be encrypted to make them harder to locate in the executable
// (use program "scramble" to encrypt, and function uscram (util.cpp) to decrypt)
// Encrypted string versions written to cryptstring.cpp

#ifndef __CRYPTSTRING_H
#define __CRYPTSTRING_H

// ORBITER Space Flight Simulator
#define NAME1 "ORBITER Space Flight Simulator"

// � 2000-2021 Martin Schweiger
#define SIG1A "� 2000-2021 Martin Schweiger"

// Copyright (c) 2000-2021 Martin Schweiger
#define SIG1B "Copyright (c) 2000-2021 Martin Schweiger"

// (c) 2000-2021
#define SIG1AA "(c) 2000-2021"

// Martin Schweiger
#define SIG1AB "Martin Schweiger"

// orbit.medphys.ucl.ac.uk/
#define SIG2 "orbit.medphys.ucl.ac.uk"

#ifdef ISBETA
// Build __DATE__ BETA
#define SIG4 "Build "__DATE__ "[v."__VERSION__" BETA]"
#else
// Build __DATE__
#define SIG4 "Build " __DATE__ "[v." __VERSION__ "]"
#endif

// orbit.m6.net/Forum/default.aspx
#define SIG5 "orbiter-forum.com"

#define SIG7 "v." __VERSION__

// https://www.youtube.com/user/orbitersim
#define SIG6 "www.youtube.com/user/orbitersim"

#endif // !__CRYPTSTRING_H