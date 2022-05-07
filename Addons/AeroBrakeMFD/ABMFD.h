#ifndef __CUSTOMMFD_H
#define __CUSTOMMFD_H

#include "Aero.h"
#include "ABOrbit.h"
#include "bitmap.h"


#define AEROBRAKEMFD_DEBUG 0
#define RECPOSITIONS 350
#define RECDRPOS 200

extern oapi::Brush *green_brush,*dgreen_brush;
extern oapi::Brush *yellow_brush;
extern oapi::Brush *black_brush,*grey_brush;
extern oapi::Pen *dash_pen,*solid_pen,*solid_pen_y,*dash_pen_y,*solid_pen_grey,*solid_pen_red,*solid_pen_blue;
extern oapi::Pen *dash_pen_dgrey,*solid_pen_dgrey,*solid_pen_dgreen, *solid_pen_white, *solid_pen_ddgrey, *dash_pen_ddgrey;
extern oapi::Brush *white_brush,*red_brush,*dgrey_brush,*no_brush;

OAPI_MSGTYPE		MsgProc (MFD_msg msg, MfdId mfd,  MFDMODEOPENSPEC *param, VESSEL *vessel);

extern char   LineBuffer[4096];
static int		ID;
extern bool   error_flag;
extern void		Bugr(const char *);
extern void		MFDErr(const char *);
extern void		MFDWarn(const char *);

bool PlanetaryParameters(OBJHANDLE obj, double *obli,double *trans,double *rot,double *off);
bool PlanetaryAtmospheres(OBJHANDLE obj, double *pres, double *dens);

class ShipRec {
public:
  ShipRec(char*);
  ~ShipRec();
  void   recLDPolar(double,double,double);
  double getSCl(double aoa);
  double getSCd(double aoa);

  char shipname[30];
  
  // ricostruzione della polare dell' astronave
  double recAoA[RECPOSITIONS];
  double recSCl[RECPOSITIONS];
  double recSCd[RECPOSITIONS];

  double maxLift,maxLiftAoA;
  double minLift,minLiftAoA;
  double maxDrag,maxDragAoA;
  double minDrag,minDragAoA;
  double maxLD,maxLDAoA;
  double minLD,minLDAoA;

  double alt_p,alt_i,alt_d;
  double aoa_rcs_p,aoa_rcs_i,aoa_rcs_d;
  double aoa_elev_p,aoa_elev_i,aoa_elev_d;
  double bank_p,bank_i,bank_d;

  void Save();
  void Read();

  void IncRef();
  void DecRef();

protected:
  int refcnt;
};

ShipRec* getShipRec(char*);

class AtmBrakeMFD {
public:
	AtmBrakeMFD(class AeroBrakeMFD *,MfdId mdfidx,VESSEL* ship);
	~AtmBrakeMFD();

	void	 Update(oapi::Sketchpad *skp);	
	void	 TimeStep(double t,double dt);

	void	 ZoomUp();
	void	 ZoomDn();

  void	 NextPage();
	void	 NextMode();
	void	 NextProjection(); 
	void   NextCenter();
  void   NextExtendMode();
	void	 Start();
	void   Setup();
	//void   NextAttitude();
  void   KeepAOA();
  void   KeepAlt();
  void   KeepBank();
  void   SetAutopilotMode(int mode,double refvalue);
  void   SetBankAutopilotMode(int mode,double refvalue);

  void   AutopilotUp();
  void   AutopilotDown();
  void   AutopilotLeft();
  void   AutopilotRight();

	void   Halt();
	double AltLimit();

  void   SetHDV(const char*);
  void   SetTDA(const char*);

	void   Write(FILEHANDLE);
	void   Read(FILEHANDLE);

  void   SaveLD();

	AeroBrakeMFD* MFD;

  double AprTime;
	double Pressure,Density,Gravity;
  double Rho;
	double Rwy,Tgt_lon,Tgt_lat;
	int    Attitude;
	char	 ApproachPlanet[32];
	char   RefName[32];
	char   Target[32];
	char*  classname;

  bool isValidTgt;
  MfdId   mfd_id;
  VESSEL* Ship;

  void IncRef();
  void DecRef();

private:
  AeroCal* aerocal;

	double TimeInOrbit();
	
	OBJHANDLE Traj_apr,Traj_ref;
	 
	double apr_zoom;
	bool	 swp,enabled;
  int    aprmode,aprpage;
	int		 apr_calmode,extmode;
	int		 aprprj;
	int    centermode;

  double keepRef,bankRef,oldRef,oldAoA,oldBank;
  int    autopilotmode,bankautopilotmode;
  double deltainteg,bankdeltainteg,aplasttime;
  ShipRec* shiprecdata;

  double hdv;
  double tda;

  SURFHANDLE bmp;
//  CPDIB* bmp;
  char   bmpfile[32];
  bool   bmploaded;

  double recDelta[RECDRPOS];
  double recRate[RECDRPOS];
  int    recDRPos;
  double recDRTime;

  int refcnt;

};

AtmBrakeMFD* getAero(AeroBrakeMFD*,MfdId,VESSEL*);
void removeAero(AtmBrakeMFD*);

class AeroBrakeMFD: public MFD {
public:
    AeroBrakeMFD(int w, int h, VESSEL *Vessel, MfdId mfd);
    ~AeroBrakeMFD();

    bool ConsumeKeyBuffered (int key);
    bool ConsumeButton (int bt, int event);
    const char *ButtonLabel (int bt);
    int  ButtonMenu (const MFDBUTTONMENU **menu) const;
    bool Update (oapi::Sketchpad *skp);
    void WriteStatus (FILEHANDLE scn) const;
    void ReadStatus (FILEHANDLE scn);
    void StoreStatus (void) const;
    void RecallStatus (void);
		void TimeStep(double t,double dt);		
		
    AtmBrakeMFD* Aero;
		
		int LineHeight;
		int ButtonSpacing;
		int FirstButton;
					
		MfdId  MFD_ID;
		int  width, height; 		
private:
					
};


#endif //!__CUSTOMMFD_H
