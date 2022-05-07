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

#ifndef __TRANSX_H
#define __TRANSX_H
#include "OrbiterAPI.h"
#include "graph.h"

using namespace oapi;

class TransxMFD: public MFD2 {
public:
	TransxMFD (int w, int h, VESSEL *Vessel, MfdId mfd);
	~TransxMFD();
	const char *ButtonLabel (int bt);
	int  ButtonMenu (const MFDBUTTONMENU **menu) const;
	bool ConsumeKeyImmediate(char *kstate);
	bool ConsumeButton(int bt, int event);
	bool ConsumeKeyBuffered(int key);
	bool Update (Sketchpad *sketchpad);
	void WriteStatus(FILEHANDLE scn) const;
	void ReadStatus(FILEHANDLE scn);
	static OAPI_MSGTYPE MsgProc (MFD_msg msg, MfdId mfd,  MFDMODEOPENSPEC *param, VESSEL *vessel);

	int getwidth();
	int getheight();
	static int GetMfdCount(){return MfdCount;}	
	bool isvalid() { return valid;};
private:
	class viewstate *viewstate;// Pointer to viewstate
	static int MfdCount;

	bool valid;
};

#endif