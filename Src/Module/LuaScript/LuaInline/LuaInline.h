// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ==============================================================
//           ORBITER MODULE: LUA Inline Interpreter
//                  Part of the ORBITER SDK
//            Copyright (C) 2007 Martin Schweiger
//                   All rights reserved
//
// LuaInline.h
// This library is loaded by the Orbiter core on demand to provide
// interpreter instances to modules via API requests.
//
// Notes:
// * LuaInline.dll must be placed in the Orbiter root directory
//   (not in the Modules subdirectory). It is loaded automatically
//   by Orbiter when required. It must not be loaded manually via
//   the Launchpad "Modules" tab.
// * LuaInline.dll depends on LuaInterpreter.dll and Lua5.1.dll
//   which must also be present in the Orbiter root directory.
// ==============================================================

#ifndef __LUAINLINE_H
#define __LUAINLINE_H

#include "Interpreter.h"
#include <thread>

// ==============================================================
// class InterpreterList: interface

class InterpreterList: public oapi::Module {
public:
	struct Environment {    // interpreter environment
		Environment();
		~Environment();
		Interpreter *CreateInterpreter ();
		Interpreter *interp;  // interpreter instance
		std::thread hThread;       // interpreter thread
		bool termInterp;      // interpreter kill flag
		bool singleCmd;       // terminate after single command
		char *cmd;            // interpreter command
		static unsigned int InterpreterThreadProc (Environment *);
	};

	InterpreterList (MODULEHANDLE hDLL);
	~InterpreterList ();

	void clbkSimulationEnd () override;
	virtual void clbkPostStep (double simt, double simdt, double mjd) override;

	Environment *AddInterpreter ();
	int DelInterpreter (Environment *env);

private:

	Environment **list;     // interpreter list
	int nlist;            // list size
	int nbuf;             // buffer size
};

#endif // !__LUAINLINE_H