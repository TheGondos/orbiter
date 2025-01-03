/* Copyright (c) 2007 Duncan Sharpe, Steve Arch
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is
** furnished to do so, subject to the following conditions:
** 
** The above copyright notice and this permission notice shall be included in
** all copies or substantial portions of the Software.
** 
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
** OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
** THE SOFTWARE.*/

#include <stdio.h>
#include <math.h>
#include "Orbitersdk.h"
#include "mfd.h"
#include "transxstate.h"
#include "TransXFunction.h"
#include "transx.h"
#include <cstring>

TransXFunction::TransXFunction(class transxstate *tstate, OBJHANDLE thmajor, OBJHANDLE thminor, OBJHANDLE thtarget, OBJHANDLE thcraft, OBJHANDLE thbase)
{	
	state=tstate;
	sethandles(thmajor,thminor,thtarget,thcraft,thbase);
	simstartMJD=oapiTime2MJD(0);
	initpens();
}

TransXFunction::TransXFunction(class transxstate *tstate, OBJHANDLE thmajor, OBJHANDLE thminor,OBJHANDLE thcraft)
{
	hmajor=thmajor;
	hminor=thminor;
	state=tstate;
	hmajtarget=NULL;
	hbase=NULL;
	hcraft=thcraft;
	simstartMJD=oapiTime2MJD(0);
	initpens();
}

TransXFunction::~TransXFunction()
{
	//Need to delete the pens to stop them eating system resources
	deletepens();
}

void TransXFunction::saveself(FILEHANDLE scn)
{
	//Write terminator
	oapiWriteScenario_string(scn,"Finish","TransXFunction");
} //Should never be used - overloaded by further functions

void TransXFunction::restoreself(FILEHANDLE scn)
{
	//Search for the terminator, then return
	char *bufferpointer;
	char tempbuffer[18], finalbuffer[18];
	bool ok;
	do 
	{
		ok=oapiReadScenario_nextline(scn,bufferpointer);
		strncpy(tempbuffer,bufferpointer,16);
		sscanf(tempbuffer,"%s",finalbuffer);
	}
	while (strcmp(finalbuffer,"Finish")!=0 && ok==true);
}

bool TransXFunction::loadhandle(FILEHANDLE scn,OBJHANDLE *handle)
{
	char *member,*bufferpointer;
	int length;
	bool ok=oapiReadScenario_nextline(scn,bufferpointer);
	if (!ok) return false;//Unexpected end of file found
	parser.parseline(bufferpointer);
	if (!parser.getlineelement(0,&member,&length)) return false;
	if (strcmp(member,"Finish")==0) return false;
	if (strcmp(member,"Handle")!=0) return true;
	if (!parser.getlineelement(1,&member,&length)) return false;//Field missing
	if (strcmp(member,"NULL")==0)
	{
		*handle=NULL;
		return true;//Handle set null
	}
	OBJHANDLE thandle=oapiGetObjectByName(member);
	if (thandle!=NULL) *handle=thandle;
	return true;
}


void TransXFunction::findfinish(FILEHANDLE scn)
{
	char *tbuffer,*member;
	int length;
	do
	{
		if (!oapiReadScenario_nextline(scn,tbuffer)) return;
		parser.parseline(tbuffer);
		parser.getlineelement(0,&member, &length);
	}
	while (strcmp(member,"Finish"));
}


void TransXFunction::savedouble(FILEHANDLE scn, double savenumber)
{
	char buffer[80];
	sprintf(buffer," %.12g",savenumber);
	oapiWriteScenario_string(scn,"Double",buffer);
}

void TransXFunction::savevector(FILEHANDLE scn, VECTOR3 &vector)
{
	char buffer[100];
	sprintf(buffer," %.12g %.12g %.12g",vector.x,vector.y,vector.z);
	oapiWriteScenario_string(scn,"Vector",buffer);
}


bool TransXFunction::loaddouble(FILEHANDLE scn, double *loadednumber)
{
	char *member,*bufferpointer;
	int length;
	bool ok=oapiReadScenario_nextline(scn,bufferpointer);
	if (!ok) return false;//Unexpected end of file found
	parser.parseline(bufferpointer);
	if (!parser.getlineelement(0,&member,&length)) return false;
	if (strcmp(member,"Finish")==0) return false; //Unexpected end of function found
	if (strcmp(member,"Double")!=0) return true; //Not a double
	if (!parser.getlineelement(1,&member,&length)) return false;
	*loadednumber=atof(member);
	return true;
}

bool TransXFunction::loadint(FILEHANDLE scn, int *loadedint)
{
	char *member,*bufferpointer;
	int length;
	bool ok=oapiReadScenario_nextline(scn,bufferpointer);
	if (!ok) return false;//Unexpected end of file found
	parser.parseline(bufferpointer);
	if (!parser.getlineelement(0,&member,&length)) return false;
	if (strcmp(member,"Finish")==0) return false; //Unexpected end of function found
	if (strcmp(member,"Int")!=0) return true; //Not an Int
	if (!parser.getlineelement(1,&member,&length)) return false;
	sscanf(member," %i",loadedint);
	return true;
}


bool TransXFunction::loadvector(FILEHANDLE scn, VECTOR3 *loadedvector)
{
	char *bufferpointer,*member;
	int length;
	bool ok=oapiReadScenario_nextline(scn,bufferpointer);
	if (!ok) return false;//Unexpected end of file found
	parser.parseline(bufferpointer);
	if (!parser.getlineelement(0,&member,&length)) return false;
	if (strcmp(member,"Finish")==0) return false; //Unexpected end of function found
	if (strcmp(member,"Vector")!=0) return true; //Not a vector
	if (!parser.getlineelement(1,&member,&length)) return false;
	(*loadedvector).x=atof(member);
	if (!parser.getlineelement(2,&member,&length)) return false;
	(*loadedvector).y=atof(member);
	if (!parser.getlineelement(3,&member,&length)) return false;
	(*loadedvector).z=atof(member);
	return true;
}


bool TransXFunction::loadorbit(FILEHANDLE scn,OrbitElements *loadorbit)
{
	char *bufferpointer, *member;
	bool ok=oapiReadScenario_nextline(scn,bufferpointer);
	int length;
	if (!ok) return false;
	parser.parseline(bufferpointer);
	if (!parser.getlineelement(0,&member,&length)) return false;//No fields
	if (strcmp(member,"Orbit")!=0) return false; //Not an orbit
	if (!parser.getlineelement(1,&member,&length)) return false;//No second field
	if (strcmp(member,"False")==0)
	{
		//Uninitialised orbit structure
		loadorbit->setinvalid();
		return true;
	}
	VECTOR3 tpos,tvel;
	double gmplanet, timestamp;
	if (!loadvector(scn,&tpos)) return false;
	if (!loadvector(scn,&tvel)) return false;
	if (!loaddouble(scn,&gmplanet)) return false;
	if (!loaddouble(scn,&timestamp)) return false;
	timestamp=(timestamp-simstartMJD)*SECONDS_PER_DAY;
	loadorbit->init(tpos,tvel,timestamp,gmplanet);
	return true;
}

void TransXFunction::savehandle(FILEHANDLE scn, OBJHANDLE handle)
{
	char namebuffer[30];
	if (handle!=NULL)
		oapiGetObjectName(handle,namebuffer,30);
	else
		strcpy(namebuffer,"NULL");
	oapiWriteScenario_string(scn,"Handle",namebuffer);
	return;
}


void TransXFunction::saveorbit(FILEHANDLE scn, const OrbitElements &saveorbit)
{
	char validvalue[6];
	if (saveorbit.isvalid())
		strcpy(validvalue,"True");
	else
		strcpy(validvalue,"False");
	oapiWriteScenario_string(scn,"Orbit",validvalue);
	if (!saveorbit.isvalid()) return;
	VECTOR3 tpos,tvel;
	saveorbit.getcurrentvectors(&tpos,&tvel);
	savevector(scn,tpos);
	savevector(scn,tvel);
	double planet=saveorbit.getgmplanet();
	savedouble(scn,planet);
	double time=saveorbit.gettimestamp()/SECONDS_PER_DAY+simstartMJD;
	savedouble(scn,time);
}

MFDvarhandler *TransXFunction::getvariablehandler()
{
	return &vars;
}

MFDvariable *TransXFunction::getcurrentvariable(int view)
{
	return vars.getcurrent(view);
}

void TransXFunction::sethmajor(OBJHANDLE handle)
{
	hmajor=handle;
	hminor=hmajtarget=NULL;
}

bool TransXFunction::sethminor(OBJHANDLE handle)
{//Virtual to give local ability to tweak this function
	return sethminorstd(handle);
}

bool TransXFunction::sethminorstd(OBJHANDLE handle)
{//Non-virtual function
	if (hmajor==handle) return false;
	if (handle==hmajtarget) hmajtarget=NULL;
	hminor=handle;
	if (hminor==NULL) return false;
	gravbodyratio=pow(oapiGetMass(hminor)/oapiGetMass(hmajor), double (0.8));
	return true;
}

bool TransXFunction::sethmajtarget(OBJHANDLE handle)
{
	if (hmajor==handle) return false;
	if (hminor==handle) hminor=NULL;
	hmajtarget=handle;
	return true;
}

void TransXFunction::sethcraft(OBJHANDLE handle)
{
	hcraft=handle;
}

void TransXFunction::sethbase(OBJHANDLE handle)
{
	hbase=handle;
}

void TransXFunction::sethandles(OBJHANDLE thmajor, OBJHANDLE thminor, OBJHANDLE thtarget, OBJHANDLE thcraft, OBJHANDLE thbase)
{
	hmajor=thmajor;
	gravbodyratio=0;
	sethminor(thminor);
	hmajtarget=thtarget;
	hcraft=thcraft;
	hbase=thbase;
}

void TransXFunction::gethandles(OBJHANDLE *thmajor, OBJHANDLE *thminor, OBJHANDLE *thtarget, OBJHANDLE *thcraft, OBJHANDLE *thbase)
{
	*thmajor=hmajor;
	*thminor=hminor;
	*thtarget=hmajtarget;
	*thcraft=hcraft;
	*thbase=hbase;
}

void TransXFunction::initpens(void)								//(rbd+)
{
	if (!pens[Green])	pens[Green]		= oapiCreatePen(1, 1 , 0x0000FF00);	// Green - stands for craft
	if (!pens[Blue])	pens[Blue]		= oapiCreatePen(1, 1 , 0x000000CD);	// Blue - stands for planet
	if (!pens[Yellow])	pens[Yellow]	= oapiCreatePen(2, 1 , 0x00CDCD00);	// Bright yellow - hypos
	if (!pens[Red])		pens[Red]		= oapiCreatePen(1, 1 , 0x00FF0000);	// Bright red - unused, but danger
	if (!pens[Grey])	pens[Grey]		= oapiCreatePen(1, 1 , 0x00C0C0C0);	// Light Grey
	if (!pens[White])	pens[White]		= oapiCreatePen(1, 1 , 0x00FFFFFF);	// Bright white - unused
	if (!brush[Green])	brush[Green]    = oapiCreateBrush (0x0000FF00);
	if (!brush[Blue])	brush[Blue]		= oapiCreateBrush (0x000000CD);
	if (!brush[Yellow])	brush[Yellow]	= oapiCreateBrush (0x00CDCD00);
	if (!brush[Red])	brush[Red]		= oapiCreateBrush (0x00FF0000);
	if (!brush[Grey])	brush[Grey]		= oapiCreateBrush (0x00C0C0C0);
	if (!brush[White])	brush[White]	= oapiCreateBrush (0x00FFFFFF);
}


void TransXFunction::deletepens()
{
	for (int a=0;a<NUM_PENS;a++)
	{
		if (pens[a]) {
			oapiReleasePen (pens[a]);
			pens[a] = 0;
		}
		if (brush[a]) {
			oapiReleaseBrush (brush[a]);
			brush[a] = 0;
		}
	}
}
															//(rbd-)
Pen* TransXFunction::SelectDefaultPen(Sketchpad *sketchpad, int value)
{
	if(value < NUM_PENS) //(rbd+)
		return sketchpad->SetPen(pens[value]);
	else
		return sketchpad->SetPen(pens[Green]);
}

Brush* TransXFunction::SelectBrush(Sketchpad *sketchpad, int value)
{
	if(value < NUM_PENS && value >= 0) //(rbd+)
		return sketchpad->SetBrush(brush[value]);		// Custom brush
	else //(rbd-)
		return sketchpad->SetBrush(NULL);
}

void TransXFunction::sethelp(const char *help1,const char *help2,const char *help3,const char *help4,const char *help5)
{
	strcpy(helpstring1,help1);
	strcpy(helpstring2,help2);
	strcpy(helpstring3,help3);
	strcpy(helpstring4,help4);
	strcpy(helpstring5,help5);
}

void TransXFunction::gethelp(char *help1,char *help2,char *help3,char *help4,char *help5) const
{
	strcpy(help1,helpstring1);
	strcpy(help2,helpstring2);
	strcpy(help3,helpstring3);
	strcpy(help4,helpstring4);
	strcpy(help5,helpstring5);
}

Pen* TransXFunction::pens[NUM_PENS] = {0};
Brush* TransXFunction::brush[NUM_PENS] = {0};