// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ========================================================================
// To be linked into all Orbiter addon modules.
// Contains standard module entry point and version information.
// ========================================================================

#include <fstream>
#include <stdio.h>
#include <dlfcn.h>
#include <cassert>
#include "ModuleAPI.h"

#define DLLCLBK extern "C"
#define OAPIFUNC
/*
#define DLLCLBK extern "C" __declspec(dllexport)
#define OAPIFUNC __declspec(dllimport)
*/
static MODULEHANDLE myModule;
static void (*myExit)(MODULEHANDLE);

static void __attribute__ ((constructor)) setup(void) {
	static bool need_init = true;
	if(!need_init) {
		return;
	}
	need_init = false;
	Dl_info info;
	int ret = dladdr((void *)setup, &info);
	if(ret == 0) {
		printf("dladdr failed\n");
		assert(false);
	}

	myModule = oapiModuleLoad(info.dli_fname);

	void (*InitModule)(MODULEHANDLE) = (void(*)(MODULEHANDLE))oapiModuleGetProcAddress(myModule, "InitModule");
	if(InitModule)
		InitModule(myModule);

	myExit = (void(*)(MODULEHANDLE))oapiModuleGetProcAddress(myModule, "ExitModule");

	oapiModuleUnload(myModule);
}

static void __attribute__ ((destructor)) destroy(void) {
	if(myExit)
		myExit(myModule);
}


int oapiGetModuleVersion ()
{
	static int v = 0;
	if (!v) {
		OAPIFUNC int Date2Int (const char *date);
		v = Date2Int (__DATE__);
	}
	return v;
}

DLLCLBK int GetModuleVersion (void)
{
	return oapiGetModuleVersion();
}

void dummy () {}
