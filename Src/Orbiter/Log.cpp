// Copyright (c) Martin Schweiger
// Licensed under the MIT License

#define __LOG_CPP

#include <string.h>
#include <fstream>
#include "Log.h"
#include "Orbiter.h"
#include <unistd.h>
#include <stdarg.h>
using namespace std;

extern char DBG_MSG[256];
extern TimeData td;

char logname[256] = "/tmp/Orbiter.log";
char logs[256] = "";
bool finelog = false;

LogOutFunc logOut = 0;

void InitLog (const char *logfile, bool append)
{
	strcpy (logname, logfile);
	ofstream ofs (logname, append ? ios::app : ios::out);
	ofs << "**** " << logname << endl;
}

void SetLogOutFunc(LogOutFunc func)
{
	logOut = func;
}

void SetLogVerbosity (bool verbose)
{
	finelog = verbose;
}

void LogOut (const char *msg, ...)
{
	va_list ap;
	va_start (ap, msg);
	LogOutVA(msg, ap);
	va_end (ap);
}

void LogOutVA(const char *format, va_list ap)
{
	extern TimeData td;
	char pwd[256];
	getcwd(pwd,256);
	FILE *f = fopen(logname, "a+t");
	if(f==NULL) {
		fprintf(stderr,"LogOutVA f==NULL %s\n",pwd);
		exit(-1);
		return;
	}
	fprintf(f, "%010.3f: ", td.SysT0);
	vfprintf(f, format, ap);
	fputc('\n', f);
	fclose(f);
	if (logOut) {
		vsnprintf(logs, 255, format, ap);
		(*logOut)(logs);
	}
}

void LogOutFine (const char *msg, ...)
{
	if (finelog) {
		extern TimeData td;
		va_list ap;
		va_start (ap, msg);
		FILE *f = fopen (logname, "a+t");
		fprintf (f, "%010.3f: ", td.SysT0);
		vfprintf (f, msg, ap);
		fputc ('\n', f);
		fclose (f);
		if (logOut) {
			vsnprintf (logs, 255, msg, ap);
			(*logOut)(logs);
		}
		va_end (ap);
	}
}

void LogOut ()
{
	LogOut (logs);
}

void LogOut_Error (const char *func, const char *file, int line, const char *msg, ...)
{
	va_list ap;
	va_start (ap, msg);
	LogOut_ErrorVA(func, file, line, msg, ap);
	va_end(ap);
}

void LogOut_Error_Start()
{
	LogOut("============================ ERROR: ===========================");
}

void LogOut_Error_End()
{
	LogOut("===============================================================");
}

void LogOut_Location(const char* func, const char* file, int line)
{
	LogOut("[%s | %s | %d]", func, file, line);

}

void LogOut_ErrorVA(const char *func, const char *file, int line, const char *msg, va_list ap)
{
	LogOut_Error_Start();
	LogOutVA(msg, ap);
	LogOut_Location(func, file, line);
	LogOut_Error_End();
}

void LogOut_LastError (const char *func, const char *file, int line)
{/*
	int err = GetLastError();
	LPTSTR errString = NULL;
	FormatMessage (FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_IGNORE_INSERTS,NULL,err,MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),(LPTSTR)&errString,0,NULL);
	LogOut_Error (func, file, line, errString);
	if (errString) LocalFree(errString);*/
}

void LogOut_Obsolete (const char *func, const char *msg)
{
	LogOut ("---------------------------------------------------------------");
	strcpy (logs, ">>> WARNING: Obsolete API function used: ");
	strcat (logs, func);
	LogOut ();
	if (msg) LogOut (msg);
	else {
		LogOut ("At least one active module is accessing an obsolete interface function.");
		LogOut ("Addons which rely on obsolete functions may not be compatible with");
		LogOut ("future versions of Orbiter.");
	}
	LogOut ("---------------------------------------------------------------");
}

void LogOut_Warning (const char *func, const char *file, int line, const char *msg, ...)
{
	va_list ap;
	FILE *f = fopen (logname, "a+t");
	fputs ("--------------------------- WARNING: --------------------------\n>>> ", f);
	va_start (ap,msg);
	vfprintf (f, msg, ap);
	va_end(ap);
	fprintf (f, "\n>>> [%s | %s | %d]\n", func, file, line);
	fputs ("---------------------------------------------------------------\n", f);
	fclose (f);
}

void tracenew (char *fname, int line)
{
#define TESTALLOC 2
#if TESTALLOC == 1
	ofstream ofs("tracenew.txt", ios::app);
	sprintf (DBG_MSG, "T=%f, %s: %d", SimT, fname, line);
	ofs << DBG_MSG << endl;
	ofs.close();
#elif TESTALLOC == 2
	sprintf (DBG_MSG, "T=%f, %s: %d", td.SimT0, fname, line);
#else
	MessageBeep (-1);
#endif
}
