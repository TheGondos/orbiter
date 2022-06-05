
// ==============================================================
//  AeroBrakeMFD ORBITER ADDON
//  (c) Jarmo Nikkanen 2004
//      Gregorio Piccoli 2006
// ==============================================================


#define STRICT
#define ORBITER_MODULE

#include "Orbitersdk.h"

#include <stdio.h>
#include <math.h>
#include <malloc.h>
#include <cstring>

#include "ABToolkit.h"
#include "ABMFD.h"
#include "ABReference.h"



oapi::Brush *green_brush=NULL, *dgreen_brush=NULL;
oapi::Brush *yellow_brush=NULL;
oapi::Brush *black_brush=NULL,*grey_brush=NULL;
oapi::Pen *dash_pen=NULL,*solid_pen=NULL,*solid_pen_y=NULL,*dash_pen_y=NULL,*solid_pen_grey=NULL,*solid_pen_red=NULL,*solid_pen_blue=NULL;
oapi::Pen *dash_pen_dgrey=NULL,*solid_pen_dgrey=NULL,*solid_pen_dgreen=NULL, *solid_pen_white=NULL, *solid_pen_ddgrey=NULL, *dash_pen_ddgrey=NULL;
oapi::Brush *white_brush=NULL,*red_brush=NULL,*dgrey_brush=NULL,*no_brush=NULL;

static AtmBrakeMFD* saveprm;

// --------------------------------------------------------------------------------------------
//	GLOBAL DATA AND FUNCTIONS
// --------------------------------------------------------------------------------------------

int mode,CW;
bool error_flag;
char LineBuffer[4096];
char error_message[256];

#define MAX_MFD 100
AeroBrakeMFD* hMFD[MAX_MFD];
int nMFD;

#define MAX_AERO 100
AtmBrakeMFD* hAero[MAX_AERO];
int nAero; // the number of MFD created 

AtmBrakeMFD* getAero(AeroBrakeMFD* ab,MfdId mfd,VESSEL* vessel){
  int reusablepos=-1;
  for(int i=0;i<nAero;i++){
    if (hAero[i] && hAero[i]->mfd_id==mfd && hAero[i]->Ship==vessel){
      hAero[i]->IncRef();
      return hAero[i];
    }
    if (hAero[i]==NULL)
      reusablepos=i;
  }
  AtmBrakeMFD* newmfd=new AtmBrakeMFD(ab,mfd,vessel);
  if (reusablepos!=-1)
    hAero[reusablepos]=newmfd;
  else
    hAero[nAero++]=newmfd;
  newmfd->IncRef();
  return newmfd;
}

#define MAX_SHIPREC 100
ShipRec* hShipRec[MAX_SHIPREC]; 
int nShipRec;

ShipRec* getShipRec(char* shipname){
  int reusablepos=-1;
  for(int i=0;i<nShipRec;i++){
    if (hShipRec[i] && strcmp(hShipRec[i]->shipname,shipname)==0){
      hShipRec[i]->IncRef();
      return hShipRec[i];
    }
    if (hShipRec[i]==NULL)
      reusablepos=i;
  }
  ShipRec* newrec=new ShipRec(shipname);
  if (reusablepos!=-1)
    hShipRec[reusablepos]=newrec;
  else
    hShipRec[nShipRec++]=newrec;
  newrec->IncRef();
  return newrec;
}

// --------------------------------------------------------------------------------------------

void Bugr(const char *c){
#ifdef AEROBRAKEMFD_DEBUG
  if (AEROBRAKEMFD_DEBUG){
    FILE *fil;  
    fil=fopen("AEROBRAKEBUGREPORT.txt","ab");
    fputs(c,fil);
    fputs("\n",fil);
    fclose(fil); 
  }
#endif
} 

void MFDErr(const char *c){  
   //strcpy(error_message,c);
   sprintf(error_message,"ERR:%s",c);
   Bugr(error_message);
   error_flag=true;
}

DLLCLBK void opcDLLInit (MODULEHANDLE hDLL){
		int i;

		// Setup Classes
		Refer = NULL;	

		for(i=0;i<MAX_MFD;i++)
      hMFD[i]=NULL;
    nMFD=0;

		for (i=0;i<MAX_AERO;i++) 
      hAero[i]=NULL;
    nAero=0;
	
    for (i=0;i<MAX_SHIPREC;i++) {
      hShipRec[i]=NULL;
    }
    nShipRec=0;

		int letter = OAPI_KEY_E;

    /*
		FILE *fil;
		char *line;
		fil=fopen("Config/IMFD.cfg","r");
 
		if (fil) {
			while ((line=fgets(LineBuffer,128,fil)) != NULL) {		
				if (!strnicmp (line, "Color_01", 8)) sscanf (line+8, "%x",&green);
				else if (!strnicmp (line, "Color_02", 8)) sscanf (line+8, "%x",&dgreen);
				else if (!strnicmp (line, "Color_03", 8)) sscanf (line+8, "%x",&lgreen);
				else if (!strnicmp (line, "Color_04", 8)) sscanf (line+8, "%x",&hgreen);
				else if (!strnicmp (line, "Color_05", 8)) sscanf (line+8, "%x",&yellow);
				else if (!strnicmp (line, "Color_06", 8)) sscanf (line+8, "%x",&lyellow);
				else if (!strnicmp (line, "Color_07", 8)) sscanf (line+8, "%x",&grey);
				else if (!strnicmp (line, "Color_08", 8)) sscanf (line+8, "%x",&dgrey);
				else if (!strnicmp (line, "Color_09", 8)) sscanf (line+8, "%x",&red);
				else if (!strnicmp (line, "Color_10", 8)) sscanf (line+8, "%x",&white);
				else if (!strnicmp (line, "AeroBrakeKey", 12)) sscanf (line+12, "%x",&letter);
			}			
			fclose(fil); 
		}
    */

		static const char *name = "AeroBrakeMFD";
    MFDMODESPEC spec;
    spec.name    = name;
    spec.key     = letter;
    spec.msgproc = MsgProc;

    mode = oapiRegisterMFDMode (spec);	

		green_brush=oapiCreateBrush(green);
		dgreen_brush=oapiCreateBrush(dgreen);

		dash_pen=oapiCreatePen(1,1,green);  //FIXME : dotted
		solid_pen=oapiCreatePen(1,1,green);  
		black_brush=oapiCreateBrush(0x00000000);      
		grey_brush=oapiCreateBrush(grey);   

		dash_pen_y=oapiCreatePen(1,1,yellow);     //FIXME : dotted  
		solid_pen_y=oapiCreatePen(1,1,yellow);  
		yellow_brush=oapiCreateBrush(yellow);

    solid_pen_red=oapiCreatePen(1,1,red);
    solid_pen_blue=oapiCreatePen(1,1,blue);

		solid_pen_grey=oapiCreatePen(1,1,grey);  
		solid_pen_dgreen=oapiCreatePen(1,1,dgreen);  
		solid_pen_dgrey=oapiCreatePen(1,1,dgrey);  
		solid_pen_ddgrey=oapiCreatePen(1,1,0x00303030);  
		dash_pen_dgrey=oapiCreatePen(1,1,dgrey);    //FIXME : dotted
		dash_pen_ddgrey=oapiCreatePen(1,1,0x00303030);      //FIXME : dotted

		white_brush=oapiCreateBrush(0x00ffffff);  
		red_brush=oapiCreateBrush(red); 
		dgrey_brush=oapiCreateBrush(dgrey);

		solid_pen_white=oapiCreatePen(1,1,white);  

//		LOGBRUSH br;
//		br.lbStyle=BS_HOLLOW;
//		no_brush=CreateBrushIndirect(&br);
		no_brush=oapiCreateBrush(0x00c0c0c0); // FIXME: BS_HOLLOW??

}


DLLCLBK void opcDLLExit (MODULEHANDLE hDLL)
{
	Refer=NULL;

	oapiUnregisterMFDMode (mode);

	oapiReleaseBrush(green_brush);   
	oapiReleaseBrush(dgreen_brush);     
	oapiReleaseBrush(black_brush);  
	oapiReleaseBrush(grey_brush);  
	oapiReleasePen(dash_pen); 
	oapiReleasePen(solid_pen); 
	oapiReleasePen(dash_pen_y); 
	oapiReleasePen(solid_pen_y);
	oapiReleaseBrush(yellow_brush);
	oapiReleasePen(solid_pen_red); 
	oapiReleasePen(solid_pen_blue); 
	oapiReleasePen(solid_pen_grey);
	oapiReleasePen(solid_pen_dgrey);	
	oapiReleasePen(dash_pen_dgrey);
	oapiReleasePen(solid_pen_ddgrey);
	oapiReleasePen(dash_pen_ddgrey);
	oapiReleaseBrush(white_brush);
	oapiReleaseBrush(red_brush);
	oapiReleaseBrush(dgrey_brush);
	oapiReleaseBrush(no_brush);
	oapiReleasePen(solid_pen_dgreen);
	oapiReleasePen(solid_pen_white);
       
  int i;
  for(i=0;i<nMFD;i++)
    hMFD[i]=NULL;
  nMFD=0;

  for (i=0;i<nAero;i++){
    hAero[i]=NULL;
  }
  nAero=0;

  for (i=0;i<nShipRec;i++){
    hShipRec[i]=NULL;
  }
  nShipRec=0;
}



DLLCLBK void opcFocusChanged(OBJHANDLE n, OBJHANDLE o){
  //if (n) 
	//  theShip=oapiGetVesselInterface(n);
}
                      


DLLCLBK void opcCloseRenderViewport(void){  

  if (Refer) 
    delete Refer;
	Refer=NULL;
	error_flag=false;

  int i;

  for(i=0;i<nMFD;i++){
    hMFD[i]=NULL;
  }
  nMFD=0;

  for (i=0;i<nAero;i++){
    if (hAero[i])
      hAero[i]->DecRef();
    hAero[i]=NULL;
  }
  nAero=0;

  for (i=0;i<nShipRec;i++){
    if (hShipRec[i])
      hShipRec[i]->DecRef();
    hShipRec[i]=NULL;
  }
  nShipRec=0;

}


DLLCLBK void opcPreStep (double simt, double simdt, double mjd){   
  if (Refer) {
    int i;
    for(i=0;i<nMFD;i++)
      if (hMFD[i])
        hMFD[i]->TimeStep(simt,simdt);
  }
}


// ==============================================================
// Custom MFD implementation

AeroBrakeMFD::AeroBrakeMFD (int w, int h, VESSEL *vessel, MfdId mfd): MFD (w, h, vessel), MFD_ID(mfd){
  width=w;
  height=h;
  error_flag=false;

  if (h>280 && h<300) {
	  LineHeight=14;
  	ButtonSpacing=41;
	  FirstButton=40;
	  CW=w/36;
  }	else {
	  LineHeight=(h/21); 
	  ButtonSpacing=(h/7); 
	  FirstButton=(h/7); 
	  CW=w/33;
  }

  if (Refer==NULL) {
	  Refer = new ReferenceClass();
  }

  if (Refer==NULL) { 
	  MFDErr("Refer Init Failed");
  }

  // Do not reconstruct classes if they are already constructed, just get the pointers	for MFD-ship
  Aero=getAero(this,mfd,vessel);

}

AeroBrakeMFD::~AeroBrakeMFD (){
  sprintf(error_message,"cancella un mfd");
  Bugr(error_message);
  int i;
  for(i=0;i<nMFD;i++){
    if (hMFD[i]==this)
      hMFD[i]=NULL;
  }
  if (Aero){
    Aero->MFD=NULL;
    Aero->DecRef();
    Aero=NULL;
  }
}

void AeroBrakeMFD::TimeStep(double t,double dt){															
  if (Aero) 
    Aero->TimeStep(t,dt); 								
}

OAPI_MSGTYPE MsgProc (MFD_msg msg, MfdId mfd,  MFDMODEOPENSPEC *param, VESSEL *vessel){
  AeroBrakeMFD* MFD;
  if (msg==OAPI_MSG_MFD_OPENEDEX) {	
    //
//    sprintf(error_message,"crea mfd=%d vessel=%d",mfd,vessel);
  //  Bugr(error_message);
    //
	  MFD = new AeroBrakeMFD(param->w, param->h, vessel, mfd); 
    int i,p=-1;
    for(i=0;i<nMFD;i++){
      if (hMFD[i]==NULL){
        // ha trovato un posto vuoto nella lista
        hMFD[i]=MFD;
        p=i;
      }
    }
    if (p<0){
      // aggiunge l' mfd alla lista allargando la lista
      hMFD[nMFD++]=MFD;
    }
	  return MFD;		
  }
  return nullptr;
}

#define DEFINED_BUTTONS 12

bool AeroBrakeMFD::ConsumeKeyBuffered (int key){
  bool DataInput (void *id, const char *str, void *data);
  //static const DWORD keys[16] = { OAPI_KEY_T, OAPI_KEY_R, OAPI_KEY_G, OAPI_KEY_M, OAPI_KEY_P,  
	//                                OAPI_KEY_Z, OAPI_KEY_X, OAPI_KEY_C, OAPI_KEY_K, OAPI_KEY_A ,0,0};
  switch (key) {	  			
    case OAPI_KEY_T:			
      ID=4;
      oapiOpenInputBox ("Target Base (leave blank to deselect)", DataInput, 0, 20, (void*)this);
      return true;  
    /*
    case OAPI_KEY_N:
      Aero->Tgt_lat = 0;
      Aero->Tgt_lon = 0;
      strcpy(Aero->Target,"none");
      return true;
    */
    case OAPI_KEY_R: 
	    ID=6;
      oapiOpenInputBox ("Reference planet:  ('a'=auto reference)", DataInput, 0, 20, (void*)this);
	    return true; 
    case OAPI_KEY_G:
      Aero->NextPage();
      InvalidateDisplay(); 
      return true;
    case OAPI_KEY_M: 
	    Aero->NextMode();
      InvalidateDisplay(); 
	    return true; 
    case OAPI_KEY_P: 
	    Aero->NextProjection();
      InvalidateDisplay(); 
	    return true; 
    case OAPI_KEY_Z:
	    Aero->ZoomUp();
      InvalidateDisplay(); 
	    return true;
    case OAPI_KEY_X:
	    Aero->ZoomDn();
      InvalidateDisplay(); 
	    return true;
    /*
    case OAPI_KEY_A:
      Aero->NextAttitude();
      return true;
    */
    case OAPI_KEY_C:
      Aero->NextCenter();
      InvalidateDisplay(); 
      return true;
    case OAPI_KEY_K:
      Aero->KeepAOA();
      InvalidateDisplay(); 
      return true;
    case OAPI_KEY_A:
      Aero->KeepAlt();
      InvalidateDisplay(); 
      return true;
    case OAPI_KEY_B:
      Aero->KeepBank();
      InvalidateDisplay();
      return true;
    case OAPI_KEY_H:
      ID=8;
      oapiOpenInputBox ("Hypothetical DeltaV:", DataInput, 0, 20, (void*)this);
      return true;
    case OAPI_KEY_U:
      ID=10;
      oapiOpenInputBox ("Touchdown altitude:", DataInput, 0, 20, (void*)this);
      return true;
  }
  // tasti speciali
  switch(key){
    case OAPI_KEY_NUMPAD2:
      Aero->AutopilotUp();
      InvalidateDisplay(); 
      break;
    case OAPI_KEY_NUMPAD8:
      Aero->AutopilotDown();
      InvalidateDisplay(); 
      break;
    case OAPI_KEY_NUMPAD4:
      Aero->AutopilotLeft();
      InvalidateDisplay(); 
      break;
    case OAPI_KEY_NUMPAD6:
      Aero->AutopilotRight();
      InvalidateDisplay(); 
      break;
    case OAPI_KEY_S:
      Aero->SaveLD();
      return true;
  }
  return false;	
}



bool AeroBrakeMFD::ConsumeButton (int bt, int event){  
  static const int btkey[16] = { OAPI_KEY_T, OAPI_KEY_R, OAPI_KEY_G, OAPI_KEY_M, OAPI_KEY_P, OAPI_KEY_H,
	                                 OAPI_KEY_Z, OAPI_KEY_X, OAPI_KEY_C, OAPI_KEY_K, OAPI_KEY_A, OAPI_KEY_B};
  /*
  static long nxt_rpt_time;															//(rbd+)
  static long t_mouse_down;
  if ((event & (PANEL_MOUSE_LBDOWN + PANEL_MOUSE_LBPRESSED)) == 0) 
    return false;	// Uninteresting event
  if (event&PANEL_MOUSE_LBDOWN) {						// Just clicked
 	  t_mouse_down = GetTickCount();						// Remember down-time
	  nxt_rpt_time = GetTickCount() + 500;				// 0.5 second until repeat starts
	  ID = 0;												// Hack: see comments below
	  return ConsumeKeyBuffered(btkey[bt]);				// Translate button to key
  }

  // Must be PANEL_MOUSE_LBPRESSED. This is ugly (sorry)					
  //if (bt!=6 && bt!=7) 
  //  return false;					
	
  if (ID!= 0) 
    return false;	
  // Hack: see comments above
  // Finally(!) we do the repeat processing...
  if (GetTickCount() < (unsigned)nxt_rpt_time) 
    return false;			                            // Not time for a repeat
  long dt = 200;										// Start with 5Hz repeat
  double thold = GetTickCount() - t_mouse_down;			// Been down this long...
  if (thold > 2000) dt /= 3;							// 2 sec, up to 15Hz
  if (thold > 4000) dt /= 3;							// 4 sec, up to 45Hz (max?)
  nxt_rpt_time = GetTickCount() + dt;					// Set next repeat time
  ID = 0;												// Hack: see comments above
  return ConsumeKeyBuffered(btkey[bt]);					// Translate button to key
  */
	if (!(event & PANEL_MOUSE_LBDOWN)) 
    return false;
	if (bt<12) 
    return ConsumeKeyBuffered (btkey[bt]);
	else 
    return false;

}														//(rbd-)

const char *AeroBrakeMFD::ButtonLabel (int bt){
  const char *label[16] = {"TGT", "REF", "PG",  "MOD", "PRJ", "HDv",
   	                 "Z+",  "Z-",  "CNT", "AoA", "Alt", "Bnk" };	
  return (bt < DEFINED_BUTTONS ? label[bt] : 0);
}

int AeroBrakeMFD::ButtonMenu (const MFDBUTTONMENU **menu) const{
  static const MFDBUTTONMENU mnu[16] = {
                {"Target Base", 0, 'T'},
                {"Reference", 0, 'R'},	
                {"MFD Page", 0, 'G'},
                {"Display Mode", 0, 'M'},
                {"Projection", 0, 'P'},
                {"Hypothetical DeltaV", 0, 'H'},

                {"Zoom+", 0, 'Z'},
                {"Zoom- ", 0, 'X'},
                {"Center",0,'C'},
                {"Hold AoA",0,'K'},
                {"Hold Alt", 0, 'A'},
                {"Hold Bank", 0, 'B'},
  };
  if (menu) 
    *menu = mnu;
  return DEFINED_BUTTONS;
}

bool AeroBrakeMFD::Update (oapi::Sketchpad* skp){ 
  if (!Aero) return false;
  if (Aero->MFD==NULL) Aero->MFD=this;
  Aero->Update(skp);
  return true;
}

void AeroBrakeMFD::WriteStatus(FILEHANDLE scn) const{  
  if (Aero) Aero->Write(scn);
}

void AeroBrakeMFD::ReadStatus (FILEHANDLE scn){
  if (Aero) Aero->Read(scn);	
}

void AeroBrakeMFD::StoreStatus (void) const{ 
  //Bugr("Store...");
  //saveprm=Aero;
}

void AeroBrakeMFD::RecallStatus (void){	
  //Bugr("Recall..");
  //Aero=saveprm;
}

bool DataInput (void *id, const char *str, void *data){
  AeroBrakeMFD *MFD=(AeroBrakeMFD *)data;
  AtmBrakeMFD  *Aero = MFD->Aero;

  //bool ok=false;
  switch(ID){
    case 10:
      Aero->SetTDA(str);
      break;
    case 8:
      Aero->SetHDV(str);
      break;
    case 6:
      strcpy(Aero->ApproachPlanet,str);
	    break;
    case 4:
      if (str[0]==0){
        Aero->Tgt_lat = 0;
        Aero->Tgt_lon = 0;
        Aero->isValidTgt=false;
        strcpy(Aero->Target,"none");
      } else {
	      OBJHANDLE ref=oapiGetGbodyByName(Aero->RefName);
	      if (ref) {
          OBJHANDLE bas=oapiGetBaseByName(ref,str);
	        if (bas) {
		        strncpy(Aero->Target,str,31);
	  	      oapiGetBaseEquPos(bas,&Aero->Tgt_lon, &Aero->Tgt_lat);
            Aero->isValidTgt=true;
          }	else {
            float lon,lat;
            int n=sscanf(str,"%f %f",&lon,&lat);
            if (n==2){
              sprintf(Aero->Target,"%5.2f %5.2f",lon,lat);
              Aero->Tgt_lat=lat*RAD;
              Aero->Tgt_lon=lon*RAD;
              Aero->isValidTgt=true;
            } else
	            return false;
          }
        }
      }
	    break;
  }
  return true;
}
