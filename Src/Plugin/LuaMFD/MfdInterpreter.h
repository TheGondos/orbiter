// Copyright (c) Martin Schweiger
// Licensed under the MIT License

#ifndef __MFDINTERPRETER_H
#define __MFDINTERPRETER_H

#include "Interpreter.h"
#include <thread>

#define NCHAR 80 // characters per line in console buffer
#define NLINE 50 // number of buffered lines

class InterpreterList;

// ==============================================================
// MFD interpreter class interface

class MFDInterpreter: public Interpreter {
public:
	struct LineSpec {
		char buf[NCHAR];
		uint32_t col;
		LineSpec *prev, *next;
	};

	MFDInterpreter ();
	void SetSelf (OBJHANDLE hV);
	void LoadAPI();
	void AddLine (const char *line, uint32_t col);
	inline LineSpec *FirstLine() const { return lineFirst; }
	inline int LineCount() const { return nline; }
	void term_strout (const char *str, bool iserr=false);
	void term_out (lua_State *L, bool iserr=false);
	void term_clear ();

protected:
	static int termOut (lua_State *L);
	static int termLineUp (lua_State *L);
	static int termSetVerbosity (lua_State *L);
	static int termClear (lua_State *L);

private:
	LineSpec *lineFirst, *lineLast;
	int nline;
};

// ==============================================================
// Interpreter repository

class InterpreterList {
public:
	struct Environment {
		Environment (OBJHANDLE hV);
		~Environment();
		MFDInterpreter *CreateInterpreter (OBJHANDLE hV);
		MFDInterpreter *interp;
		std::thread hThread;
		char cmd[1024];
		static unsigned int InterpreterThreadProc (void *context);
	};
	struct VesselInterp {
		OBJHANDLE hVessel;
		Environment **env;
		int nenv;
	} *list;
	int nlist;
	int nbuf;

	InterpreterList();
	~InterpreterList();

	void Update (double simt, double simdt, double mjd);

	Environment *AddInterpreter (OBJHANDLE hV);
	bool DeleteInterpreter (OBJHANDLE hV, int idx);
	bool DeleteInterpreters (OBJHANDLE hV);
	void DeleteList ();
	VesselInterp *FindVesselInterp (OBJHANDLE hV);
	Environment *FindInterpreter (OBJHANDLE hV, int idx);
	int InterpreterCount (OBJHANDLE hV);
};

#endif // !__MFDINTERPRETER_H