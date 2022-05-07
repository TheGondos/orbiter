// ==============================================================
//  AeroBrakeMFD ORBITER ADDON
//  (c) Jarmo Nikkanen 2003
//      Gregorio Piccoli 2006
// ==============================================================




#include "Orbitersdk.h"

#include <stdio.h>
#include <math.h>
#include <malloc.h>
#include <cstring>

#include "ABToolkit.h"
#include "ABMFD.h"
#include "ABReference.h"
#include "Aero.h"
#include "ABOrbit.h"


#include "bitmap.h"

// included from CDK...

// Returns the distance between two Lat/Long positions over the surface of a sphere of a given
// radius.
double CalcSphericalDistance(VECTOR3 Pos1, VECTOR3 Pos2, double Radius)
{
	double DeltaLat = Pos2.x - Pos1.x;
	double DeltaLong = Pos2.y - Pos1.y;

	double A = pow(sin(DeltaLat/2), 2) + cos(Pos1.x) * cos (Pos2.x) * pow(sin(DeltaLong/2), 2);
	double C = 2 * atan2(sqrt(A), sqrt(1 - A));
	
	return (Radius * C);
}

//------------------------------------------------
//Aero-Brake program
//------------------------------------------------

void AtmBrakeMFD::Write(FILEHANDLE scn){

  char name[256];
  strcpy(name,ApproachPlanet);NameFix(name);
	oapiWriteScenario_string (scn, "Reference",name);

	sprintf(LineBuffer,"%d %f %d %d %d %d",Attitude,apr_zoom,aprmode,aprprj,centermode,aprpage);
  oapiWriteScenario_string (scn, "Data",LineBuffer);
  
  strcpy(name,Target);NameFix(name);
  oapiWriteScenario_string(scn,"Target",name);

  sprintf(LineBuffer,"%f %f %d",Tgt_lon,Tgt_lat,isValidTgt);
  oapiWriteScenario_string(scn,"LonLat",LineBuffer);

  sprintf(LineBuffer,"%d %f",autopilotmode,keepRef);
  oapiWriteScenario_string(scn,"AoA",LineBuffer);

  sprintf(LineBuffer,"%d %f",bankautopilotmode,bankRef);
  oapiWriteScenario_string(scn,"Bank",LineBuffer);
}

void AtmBrakeMFD::Read(FILEHANDLE scn){
	bool go=true;
	char *line;
  char tgt[256];

	while (go) {
		go=oapiReadScenario_nextline (scn, line);
    if (go) {
			if (!strncasecmp (line, "Reference", 9)) {
				sscanf (line+9,"%s",ApproachPlanet);
				NameUnFix(ApproachPlanet);
			}	else if (!strncasecmp (line, "Data", 4)) {
        float z;
				sscanf(line+4,"%d %f %d %d %d %d",&Attitude,&z,&aprmode,&aprprj,&centermode,&aprpage);
        apr_zoom=z;
        aprpage=aprpage%3;
        aprmode=aprmode%2;
        aprprj=aprprj%6;
      } else if (!strncasecmp (line, "Target",6)) {
        sscanf(line+6,"%s",tgt);
        NameUnFix(tgt); 
        strncpy(Target,tgt,31);
        //OBJHANDLE ref=oapiGetGbodyByName(ApproachPlanet);
        //OBJHANDLE bas=oapiGetBaseByName(ref,tgt);
	      //if (bas) {
	  	  //  oapiGetBaseEquPos(bas,&Aero_lon, &Aero_lat);
        //}
      } else if (!strncasecmp (line, "LonLat", 6)) {
        float lon,lat;
        int vtgt=0;
        sscanf(line+6,"%f %f %d",&lon,&lat,&vtgt);
        Tgt_lon=lon;
        Tgt_lat=lat;
        isValidTgt=vtgt;
      } else if (!strncasecmp (line, "AoA", 3)) {
        float a=0;
        int m=0;
        sscanf(line+3,"%d %f",&m,&a);
        SetAutopilotMode(m,a);
      } else if (!strncasecmp (line, "Bank", 4)) {
        float a=0;
        int m=0;
        sscanf(line+4,"%d %f",&m,&a);
        SetBankAutopilotMode(m,a);
			}	else if (!strncasecmp (line, "END", 3)) 
        go=false;	
		}
	}
}

AtmBrakeMFD::AtmBrakeMFD(AeroBrakeMFD *m,MfdId mfdidx,VESSEL* ship):mfd_id(mfdidx){
  MFD=m;
  Ship=ship;
  aerocal=NULL;

  strcpy(ApproachPlanet,"none");
  strcpy(Target,"none");
  // cerco di inizializzare il pianeta di approccio
  if (Ship && Refer) {
    OBJHANDLE ref = Refer->GetReference(Ship->GetHandle());
    if (ref) 
	    oapiGetObjectName(ref,ApproachPlanet,32);
  }
  //
		
  Traj_apr=NULL,Traj_ref=NULL;

  AprTime=0;
  apr_zoom=1;
  aprmode=1;
  aprpage=0;
  apr_calmode=0;
  aprprj=0;
  centermode=0;
  extmode=0;

  enabled=false;

  Pressure=0;
  Density=0;
  Attitude=0;

  keepRef = 0;
  autopilotmode = 0;
  bankautopilotmode = 0;
  aplasttime=0;

  Tgt_lon = 0;
  Tgt_lat = 0;
  isValidTgt=0;

  aerocal=new AeroCal(Ship);
  shiprecdata=getShipRec(Ship->GetClassName());
  bmp=NULL;
  bmpfile[0]='\0';
  bmploaded=false;

  refcnt=1;

  char error_message[256];
  sprintf(error_message,"Creato un AtmBrakeMFD at %p",this);
  Bugr(error_message);
}


AtmBrakeMFD::~AtmBrakeMFD(){
  Halt();
  delete aerocal;
  aerocal=NULL;
  shiprecdata->DecRef();
  shiprecdata=NULL;
  if (bmp){
    oapiReleaseTexture(bmp);
    bmp=NULL;
  }
  //removeAero(this);

  char error_message[256];
  sprintf(error_message,"Distrutto un AtmBrakeMFD at %p",this);
  Bugr(error_message);

}

void AtmBrakeMFD::IncRef(){
  refcnt++;
}

void AtmBrakeMFD::DecRef(){
  refcnt--;
  if (refcnt==0)
    delete this;
}

void AtmBrakeMFD::SaveLD(){
  if (shiprecdata)
    shiprecdata->Save();
}

void AtmBrakeMFD::Halt(){
  if (aerocal)
    aerocal->Stop(false);
}


double normElev(double t){
  if (t>1)
    t=1;
  if (t<-1)
    t=-1;
  return t;
}

double normThrust(double l, double d){
  if (l<0 && d>0) l=0;
  if (l>0 && d<0) l=0;
  return normElev(l+d);
}

void fireRcs(VESSEL* Ship, int up, double level){
  THGROUP_HANDLE g;
  if (up) {
    g=Ship->GetThrusterGroupHandle(THGROUP_ATT_PITCHUP);
    Ship->SetThrusterGroupLevel(g,fabs(level));
    g=Ship->GetThrusterGroupHandle(THGROUP_ATT_PITCHDOWN);
    Ship->SetThrusterGroupLevel(g,0);
  } else {
    g=Ship->GetThrusterGroupHandle(THGROUP_ATT_PITCHUP);
    Ship->SetThrusterGroupLevel(g,0);
    g=Ship->GetThrusterGroupHandle(THGROUP_ATT_PITCHDOWN);
    Ship->SetThrusterGroupLevel(g,fabs(level));
  }
}

void attuateElevator(VESSEL* Ship,int up,double deltalevel){
  double elev=Ship->GetControlSurfaceLevel(AIRCTRL_ELEVATORTRIM);
  //double elev=Ship->GetControlSurfaceLevel(AIRCTRL_ELEVATOR);
  double den=Ship->GetAtmDensity();
  double dyn=Ship->GetDynPressure();
  if (den>0 || dyn>100){
    Ship->SetControlSurfaceLevel(AIRCTRL_ELEVATORTRIM,normElev(elev+deltalevel));
    Ship->SetControlSurfaceLevel(AIRCTRL_ELEVATOR,normElev(elev+deltalevel));
  } else {
    Ship->SetControlSurfaceLevel(AIRCTRL_ELEVATORTRIM,0);
    Ship->SetControlSurfaceLevel(AIRCTRL_ELEVATOR,0);
  }
}

void setElevator(VESSEL* Ship,double level){
  double elev=Ship->GetControlSurfaceLevel(AIRCTRL_ELEVATORTRIM);
  //double elev=Ship->GetControlSurfaceLevel(AIRCTRL_ELEVATOR);
  Ship->SetControlSurfaceLevel(AIRCTRL_ELEVATORTRIM,normElev(level));
  Ship->SetControlSurfaceLevel(AIRCTRL_ELEVATOR,normElev(level));
}

void setRCS(VESSEL* Ship, double level){
  THGROUP_HANDLE g;
  if (level>0) {
    g=Ship->GetThrusterGroupHandle(THGROUP_ATT_PITCHUP);
    Ship->SetThrusterGroupLevel(g,normElev(level));
    g=Ship->GetThrusterGroupHandle(THGROUP_ATT_PITCHDOWN);
    Ship->SetThrusterGroupLevel(g,0.0);
  } else {
    g=Ship->GetThrusterGroupHandle(THGROUP_ATT_PITCHUP);
    Ship->SetThrusterGroupLevel(g,0.0);
    g=Ship->GetThrusterGroupHandle(THGROUP_ATT_PITCHDOWN);
    Ship->SetThrusterGroupLevel(g,normElev(-level));
  }
}

void setBankRCS(VESSEL* Ship, double level){
  THGROUP_HANDLE g;
  if (level>0) {
    g=Ship->GetThrusterGroupHandle(THGROUP_ATT_BANKLEFT);
    Ship->SetThrusterGroupLevel(g,normElev(level));
    g=Ship->GetThrusterGroupHandle(THGROUP_ATT_BANKRIGHT);
    Ship->SetThrusterGroupLevel(g,0.0);
  } else {
    g=Ship->GetThrusterGroupHandle(THGROUP_ATT_BANKLEFT);
    Ship->SetThrusterGroupLevel(g,0.0);
    g=Ship->GetThrusterGroupHandle(THGROUP_ATT_BANKRIGHT);
    Ship->SetThrusterGroupLevel(g,normElev(-level));
  }
}

void getElevPIDCoeff(double &P, double &I, double &D,int autopilotmode,VESSEL* Ship,double delta,double rate,double integ,ShipRec* sr){
  // i coefficienti PID dipendono dalla pressione dinamica
  // nel vuoto o quasi vuoto le supefici aerodinamiche non sono in grado di guidare, bisogna basarsi soprattutto su RCS
  double dyn=Ship->GetDynPressure();
  if (autopilotmode==1){
    // AoA
    P=sr->aoa_elev_p;
    I=sr->aoa_elev_i;
    D=sr->aoa_elev_d;
  } else {
    // Alt
    P=sr->alt_p;
    I=sr->alt_i;
    D=sr->alt_d;

    double bank=Ship->GetBank();
    if (fabs(bank)>PI05){
      // volo rovesciato!
      P=-P;
      I=-I;
      D=-D;
    }
  }
}

void getRcsPIDCoeff(double &P, double &I, double &D,int autopilotmode,VESSEL* Ship,double delta,double rate,double integ,ShipRec* sr){
  double dyn=Ship->GetDynPressure();
  if (autopilotmode==1){
    // AoA
    P=sr->aoa_rcs_p; //5.0 un po' debole
    I=sr->aoa_rcs_i; //-2.0;
    D=sr->aoa_rcs_d; //-8.0; //8.0 troppo frenato
  } else {
    // Alt
    P=0.0;
    I=0.0;
    D=0.0;
  }

}

void getBankPIDCoeff(double &P, double &I, double &D,int autopilotmode,VESSEL* Ship,double delta,double rate,double integ,ShipRec* sr){
  P=sr->bank_p; //5.0 un po' debole
  I=sr->bank_i; //-2.0;
  D=sr->bank_d; //-8.0; //8.0 troppo frenato
}

void AtmBrakeMFD::TimeStep(double time,double dt){
  if (Refer){ // e' riuscito ad inizializzare il sistema
    // 
    if (Traj_apr==NULL || enabled==false) 
	    Halt();
    // se si e' fermato, riparte, altrimenti fa un passo
	  if (aerocal->IsProgress()==false)   
	    Start();
	  else 
	    aerocal->Timestep();		
  }

  double currentvalue,rate,delta,slip,elev,dyn,aoa;
  slip=Ship->GetSlipAngle();
  if (shiprecdata && autopilotmode!=0 && fabs(slip)<0.5 && (Attitude==ATT_REALTIMEUP || Attitude==ATT_REALTIMEBANK)) {
    aoa=Ship->GetAOA();
    currentvalue=(autopilotmode==1?aoa:Ship->GetAltitude()/1000);
    rate=(currentvalue-oldRef)/dt;
    delta=currentvalue-keepRef;

    //
    dyn=Ship->GetDynPressure();
    elev=Ship->GetControlSurfaceLevel(AIRCTRL_ELEVATORTRIM);
    //elev=Ship->GetControlSurfaceLevel(AIRCTRL_ELEVATOR);
    //

    // nuovo algoritmo PID
    double P,I,D,outelev,outrcs;
    getElevPIDCoeff(P,I,D,autopilotmode,Ship,delta,rate,deltainteg,shiprecdata);
    outelev=P*delta+I*deltainteg+D*rate;
    getRcsPIDCoeff(P,I,D,autopilotmode,Ship,delta,rate,deltainteg,shiprecdata);
    outrcs=P*delta+I*deltainteg+D*rate;

    // controllo se sono a "fine corsa" con l' AoA, cioe' se e' inutile aumentare o diminure l' AoA attuale per modificare l' Alt
    if (autopilotmode==2){
      if (aoa*1.1>shiprecdata->maxLiftAoA) outelev=(aoa>oldAoA*0.999?elev*0.9:(elev+outelev)/2);
      if (aoa*1.1<shiprecdata->minLiftAoA) outelev=(aoa<oldAoA*0.999?elev*0.9:(elev+outelev)/2);
    }

    // aziona l' elevatore
    setElevator(Ship,outelev);
    if (oapiGetTimeAcceleration()<=100 && (dyn<100 || fabs(outelev)>1)){
      // aziona gli RCS solo nel vuoto o quando l' elevatore non basta 
      setRCS(Ship,outrcs);
    } else {
      outrcs=0;
      setRCS(Ship,0);
    }

    //sprintf(oapiDebugString(),"rate=%8.5f delta=%8.5f Idelta=%8.5f elev=%8.5f rcs=%8.5f",rate,delta,deltainteg,outelev,outrcs);


    oldRef=currentvalue;
    oldAoA=aoa;

    if ((autopilotmode==2 && recDRTime+0.5<time) || (autopilotmode==1 && recDRTime+0.1<time)){
      recDelta[recDRPos]=delta;
      recRate[recDRPos]=rate;
      recDRPos=(recDRPos+1)%RECDRPOS;
      recDRTime=time;
    }

    // --- tentativi di miglioramento ---
    // saturazione dei comandi
    if (autopilotmode==1){
      if (fabs(outrcs)<1 || fabs(outelev)<1)
        deltainteg+=delta*dt; // non deve conteggiare nell' integrale l' errore quando e' gia' in saturazione
    } else if (autopilotmode==2){
      if (fabs(outelev)<1 && shiprecdata->minLiftAoA<aoa && aoa<shiprecdata->maxLiftAoA)
        deltainteg+=delta*dt; // non deve conteggiare nell' integrale l' errore quando e' gia' in saturazione
    }
    

  } else {
    //sprintf(oapiDebugString(),"Non Pilota AOA");
  }
  // Bank autopilot ----------------------------------------------------------------------------------------------------------------------
  if (shiprecdata && bankautopilotmode!=0 && fabs(slip)<0.5 && (Attitude==ATT_REALTIMEUP || Attitude==ATT_REALTIMEBANK)) {
    currentvalue=Ship->GetBank();
    rate=(currentvalue-oldBank)/dt;
    delta=currentvalue-bankRef;

    //

    // nuovo algoritmo PID
    double P,I,D,outrcs;
    getBankPIDCoeff(P,I,D,bankautopilotmode,Ship,delta,rate,bankdeltainteg,shiprecdata);
    outrcs=P*delta+I*deltainteg+D*rate;
    setBankRCS(Ship,outrcs);

    //sprintf(oapiDebugString(),"rate=%8.5f delta=%8.5f Idelta=%8.5f elev=%8.5f rcs=%8.5f",rate,delta,bankdeltainteg,0.0,outrcs);

    oldBank=currentvalue;
    bankdeltainteg+=delta*dt; // non deve conteggiare nell' integrale l' errore quando e' gia' in saturazione
  }
}

double AtmBrakeMFD::TimeInOrbit(){
	VECTOR3 pos,vel;				
    
	if (Traj_apr) {

		double myy=oapiGetMass(Traj_apr)*GC;

		Ship->GetRelativePos(Traj_apr, pos);
		Ship->GetRelativeVel(Traj_apr, vel);
    	
		Orbit traj(pos,vel,myy);
		double tim;

		if (traj.ecc>=1 || traj.tra>PI) {
			tim=traj.TimeToPeriapis();
			if (tim<0) 
        tim=0;
		}	else 
      tim=0;

		double a=traj.PeriapisDistance();
		tim+=0.75*PI2*sqrt(a*a*a/myy);
		// tempo minimo di integrazione: un giro completo del pianeta ad altezza zero
    double radi=oapiGetSize(Traj_apr);
		double mtim=radi*2*PI/sqrt(myy/radi);
		if (tim<mtim)
		  tim=mtim;
		//
		return tim;
	}
	return 1000;
}


void AtmBrakeMFD::ZoomDn(){
  apr_zoom*=0.7;
  if (apr_zoom<0.1) apr_zoom=0.1;
}


void AtmBrakeMFD::ZoomUp(){
  apr_zoom*=1.2;
  if (apr_zoom>100) apr_zoom=100;
}

void AtmBrakeMFD::NextMode(){
  aprmode=(aprmode+1)%3;
}

void AtmBrakeMFD::SetAutopilotMode(int mode,double refvalue){
  if (mode!=0){
    autopilotmode=mode;
    keepRef=refvalue;
    oldRef=refvalue;
    oldAoA=Ship->GetAOA();
    deltainteg=0;
    Ship->SetADCtrlMode(6);
    // la registrazione dei dati
    recDRPos=0;
    recDRTime=0;
    for(int i=0;i<RECDRPOS;i++){
      recDelta[i]=0;
      recRate[i]=0;
    }
  } else {
    autopilotmode=0;
    keepRef=0;
    // azzera tutto
    THGROUP_HANDLE g;
    g=Ship->GetThrusterGroupHandle(THGROUP_ATT_PITCHUP);
    Ship->SetThrusterGroupLevel(g,0);
    g=Ship->GetThrusterGroupHandle(THGROUP_ATT_PITCHDOWN);
    Ship->SetThrusterGroupLevel(g,0);
    Ship->SetControlSurfaceLevel(AIRCTRL_ELEVATORTRIM,0);
    Ship->SetControlSurfaceLevel(AIRCTRL_ELEVATOR,0);
  }
}

void AtmBrakeMFD::SetBankAutopilotMode(int mode,double refvalue){
  if (mode!=0){
    bankautopilotmode=mode;
    bankRef=refvalue;
    oldBank=refvalue;
    bankdeltainteg=0;
  } else {
    bankautopilotmode=0;
    bankRef=0;
    // azzera tutto
    THGROUP_HANDLE g;
    g=Ship->GetThrusterGroupHandle(THGROUP_ATT_BANKLEFT);
    Ship->SetThrusterGroupLevel(g,0);
    g=Ship->GetThrusterGroupHandle(THGROUP_ATT_BANKRIGHT);
    Ship->SetThrusterGroupLevel(g,0);
  }
}

void setScale(double& scale, double& aplasttime){
  double t=oapiGetSysTime(),d=t-aplasttime;
  scale=1;
  if (d>0.8 && d<3) scale=0.1;
  if (d<0.2) scale=10;
  aplasttime=t;
}

void AtmBrakeMFD::AutopilotUp(){
  double delta,scale;
  setScale(scale,aplasttime);
  switch (autopilotmode){
  case 1:
    delta=0.01*scale;
    keepRef+=(delta*PI/180);
    break;
  case 2:
    delta=0.1*scale;
    keepRef+=delta;
    break;
  }
}

void AtmBrakeMFD::AutopilotDown(){
  double delta,scale;
  setScale(scale,aplasttime);
  switch (autopilotmode){
  case 1:
    delta=0.01*scale;
    keepRef-=(delta*PI/180);
    break;
  case 2:
    delta=0.1*scale;
    keepRef-=delta;
    break;
  }
}

void AtmBrakeMFD::AutopilotLeft(){
  double delta,scale;
  setScale(scale,aplasttime);
  switch (bankautopilotmode){
  case 1:
    delta=0.01*scale;
    bankRef+=(delta*PI/180);
    break;
  }
}

void AtmBrakeMFD::AutopilotRight(){
  double delta,scale;
  setScale(scale,aplasttime);
  switch (bankautopilotmode){
  case 1:
    delta=0.01*scale;
    bankRef-=(delta*PI/180);
    break;
  }
}

void AtmBrakeMFD::KeepAOA(){
  if (autopilotmode==1){
    SetAutopilotMode(0,0);
    Ship->SetADCtrlMode(7);
    Ship->SetControlSurfaceLevel(AIRCTRL_ELEVATORTRIM,0);
  } else if (Attitude==ATT_REALTIMEUP || Attitude==ATT_REALTIMEBANK) {
    SetAutopilotMode(1,Ship->GetAOA());
  }
}

void AtmBrakeMFD::KeepAlt(){
  if (autopilotmode==2){
    SetAutopilotMode(0,0);
    Ship->SetADCtrlMode(7);
    Ship->SetControlSurfaceLevel(AIRCTRL_ELEVATORTRIM,0);
  } else if (Attitude==ATT_REALTIMEUP || Attitude==ATT_REALTIMEBANK) {
    SetAutopilotMode(2,Ship->GetAltitude()/1000);
  }
}

void AtmBrakeMFD::KeepBank(){
  if (bankautopilotmode==1){
    SetBankAutopilotMode(0,0);
  } else if (Attitude==ATT_REALTIMEUP || Attitude==ATT_REALTIMEBANK) {
    SetBankAutopilotMode(1,Ship->GetBank());
  }
}

void AtmBrakeMFD::NextCenter(){
  centermode=(centermode+1)%3;
}

void AtmBrakeMFD::NextProjection() {
  aprprj=(aprprj+1)%6;
}

void AtmBrakeMFD::NextPage() {
  aprpage=(aprpage+1)%3;
}

void AtmBrakeMFD::NextExtendMode(){
  extmode=(extmode+1)%2;
}

void AtmBrakeMFD::Start(){	
	if (Traj_apr && enabled) {
		Setup();
		AprTime=TimeInOrbit();
		aerocal->Start(AprTime);	
	}
}

/*
void AtmBrakeMFD::NextAttitude(){	
	Attitude=(Attitude+1)%4;
  if (AeroBrake){
	  AeroBrake->SetAttitude(Attitude);
  }
}
*/

double AtmBrakeMFD::AltLimit(){
	return(-Pressure*log(8e-11/Density)/(Gravity*Density));
}

void AtmBrakeMFD::SetHDV(const char* str){
  hdv=atof(str);
  aerocal->SetHDV(hdv);
}

void AtmBrakeMFD::SetTDA(const char* str){
  tda=atof(str);
  aerocal->SetTDA(tda);
}

void AtmBrakeMFD::Setup(){
	ATMPARAM Atm;
	oapiGetPlanetAtmParams(Traj_apr,oapiGetSize(Traj_apr),&Atm);
  Pressure=Atm.p;
  Density=Atm.rho;

	aerocal->SetLoops(20,10);//Jarmo
	aerocal->SetAtmosphere(Traj_apr);  
   
	double a,b,c,d, Drag, Cross;
  double aoa,dynp,rho2d,rho2l;
  double bank,slip;
	VECTOR3 cs;

	Ship->GetCrossSections(cs);
	Ship->GetCW(a,b,c,d);

	//if (Attitude==0) Cross=cs.y, Drag=d;
	//if (Attitude==1) Cross=cs.z, Drag=a;
	//if (Attitude==2) Cross=cs.z, Drag=b;
  
  // Rho per traiettria Pro-grade
	Cross=cs.z; Drag=a;
	Rho = Drag*Cross;
	aerocal->SetShipRho(Rho);

	aerocal->SetShipHandle(Ship->GetHandle());
	aerocal->SetShipMass(Ship->GetMass());  
  aerocal->SetMach(Ship->GetMachNumber());
  //
  switch(Attitude) {
    case ATT_REALTIMEUP:
    case ATT_REALTIMEBANK:
      // ricava S*Cl e S*Cd per traiettoria Realtime 
      dynp=Ship->GetDynPressure();
      rho2d=0;
      rho2l=0;
      aoa=Ship->GetAOA();
      bank=Ship->GetBank();
      slip=Ship->GetSlipAngle();
      if (dynp>0){
	      rho2d=Ship->GetDrag()/dynp;
	      rho2l=Ship->GetLift()/dynp;
        // memorizza AoA,SCl e SCd
        if (fabs(bank)<0.02 && fabs(slip)<0.02 && Ship->GetAltitude()>100 && shiprecdata)
          shiprecdata->recLDPolar(aoa,rho2l,rho2d);
      } else if (shiprecdata) {
        rho2d=shiprecdata->getSCd(aoa);
        rho2l=shiprecdata->getSCl(aoa);
      }
      aerocal->SetSCd(rho2d);
      aerocal->SetSCl(rho2l);
      aerocal->SetAoA(aoa);
      aerocal->SetBank(bank);
      break;
    case ATT_RETROGRADE:
    	Cross=cs.z;
      Drag=b;
	    Rho = Drag*Cross;
	    aerocal->SetShipRho(Rho);
      break;
    default:
      break;
  }

  //sprintf(oapiDebugString(),"Setup %f",oapiGetSimMJD());
}

double eqvel(double alt,double grav,double mass,double rho,double rotv,double scl){
 return (
   sqrt(2*(grav*alt*alt*mass*rho*scl-alt*mass*rho*rotv*rotv*scl+2*grav*mass*mass*alt))
     -2*mass*rotv
     )/(alt*rho*scl+2*mass);
}

void SameZeroLevel(double& Max1, double& min1, double& Max2, double& min2){
  double m,m1,m2;
  if (Max1>0 && min1!=0 && min2!=0) {
    m1=Max1/min1;
    m2=Max2/min2;
    if (min1>0){
      m=(m1>m2?m2:m1);
      if (m>0) {
        min1=Max1/m;
        min2=Max2/m;
      }
    } else {
      m=(m1>m2?-m1:-m2);
      if (m>0){
        min1=-Max1/m;
        min2=-Max2/m;
      }
    }
  } else if (Max1<0 && Max2<0) {
    m1=min1/Max1;
    m2=min2/Max2;
    m=(m1>m2?m1:m2);
    Max1=min1/m;
    Max2=min2/m;
  }
}

void AtmBrakeMFD::Update(oapi::Sketchpad *skp){
  int pos;

  if (!MFD) return;

  int ld=MFD->LineHeight;
	int bd=MFD->ButtonSpacing;
	int fbp=MFD->FirstButton;fbp-=5;

	int last=fbp+(5*bd)+(2*ld);

	const char *Tit = {"AeroBrakeMFD\0"};

  double zf;
	Orbit AprOrbit,Project;
    
	int width=MFD->width;
	int height=MFD->height;

	double w=(double)width;
	double h=(double)height;
	double cx=w/2;
	double cy=h/2;

	VECTOR3 Traj_PeD,Traj_PeV;
	double  Traj_PeT;

	double myy,radi;

	char name[64];
    
  OBJHANDLE TgtHandle     = NULL;
	OBJHANDLE SunHandle		  = Refer->StarHandle;
	OBJHANDLE ShipReference	= Refer->GetReference(Ship->GetHandle());
	const ATMCONST* Atm     = NULL;

	class Orbit Ecliptic;

	Ecliptic.Ecliptic();

	Traj_apr = oapiGetGbodyByName(ApproachPlanet);

  if (Traj_apr==SunHandle) Traj_apr=NULL;
	
  // Get names
	if (Traj_apr) 
	  oapiGetObjectName(Traj_apr,RefName,32);
	else 
	  strcpy(ApproachPlanet,"none");  

	if (Traj_apr) {
    TgtHandle=oapiGetBaseByName(Traj_apr,Target);
	  if (TgtHandle) 
	    oapiGetBaseEquPos(TgtHandle,&Tgt_lon, &Tgt_lat);
	  Atm=oapiGetPlanetAtmConstants(Traj_apr);
	}

	classname=Ship->GetClassName();
  sprintf(name,"%s",classname);
  /*
  switch (Attitude){
    case ATT_REALTIMEUP:
      sprintf(name,"%s %s",classname,"Realtime (lift up)");
      break;
    case ATT_REALTIMEBANK:
      sprintf(name,"%s %s",classname,"Realtime");
      break;
    case ATT_PROGRADE:
      sprintf(name,"%s %s",classname,"Pro-grade");
      break;
    case ATT_RETROGRADE:
      sprintf(name,"%s %s",classname,"Retro-grade");
      break;
  }
  */

  // Do we have a valid approach, return if not
	if (Traj_apr==NULL) {
		pos=fbp;
		skp->SetTextColor(lgreen);
		skp->SetTextAlign(oapi::Sketchpad::CENTER);
		skp->Text(width/2,pos+10,"Invalid Reference", strlen("Invalid Reference"));
		skp->Text(width/2,pos+15+ld,"press (Shift-R) to setup", strlen("press (Shift-R) to setup"));
		return;
	}

  Orbit shiporbit(Ship->GetHandle(),Traj_apr);

	bool ok=true;

	if (shiporbit.ecc<1) {
		double p=shiporbit.Period();
		double t=shiporbit.TimeToPeriapis();
		if (t>20e3 && (p-t)>20e3) 
      ok=false;
	}

	if ((shiporbit.ecc>1 && fabs(shiporbit.TimeToPeriapis())>20e3) || !ok) {
	  pos=fbp;
	  skp->SetTextColor(lgreen);
	  skp->SetTextAlign(oapi::Sketchpad::CENTER);
	  Text(skp,width/2,pos+10,"The ship is too far a way");
	  Text(skp,width/2,pos+15+ld,"PeT < 20ks Required");
	  enabled=false;
	  return;
	} else 
	  enabled=true;

  // Setup reference and some data
	myy=oapiGetMass(Traj_apr)*GC;
	radi=oapiGetSize(Traj_apr);
	Gravity=myy/(radi*radi);
	
	Traj_ref=Refer->GetReference(Traj_apr);
	if (Traj_ref==SunHandle || Traj_ref==NULL) {	
	  Traj_ref=SunHandle;	
	}
    
	// Define and reset some variables
  bool trajok=false;
	bool aprok=false;
  
  // Receive data from trajectory engine
	if (Atm && aerocal->GetLastPoint()>0 && aerocal->GetReference()==Traj_apr) {
			
		// Receive final data for orbital information after aerobrake
		Traj_PeD = aerocal->GetFinalD();
		Traj_PeV = aerocal->GetFinalV();
		
		AprOrbit.Elements(Traj_PeD,Traj_PeV,myy);

		// Receive periapis information
		Traj_PeD = aerocal->GetPeD();
		Traj_PeV = aerocal->GetPeV();
		Traj_PeT = aerocal->GetPeT();
		trajok=true;
	}

  // Setup Orbit Projection Plane
  if (aprprj%2==1)
    Project=Ecliptic;
  else {
	  //Project=AprOrbit;
    Project.Elements(Ship->GetHandle(),Traj_apr);
  }

  // tutti i grafici usano il font piccolo
//  MFD->SelectDefaultFont(hDC,1);
  //

	if (trajok && aprpage==0) {
    // -----------------------------------------------------------------------------------------------------------------------------
    // Disegno della traiettoria
		double ocx,ocy,r;
		int i;

		zf=(w/2)/(length(Traj_PeD)*2)*apr_zoom;
		skp->SetPen(solid_pen);
    
		//
		double shx=0,shy=0;
    switch (centermode){
      case 1:
    		Project.Point(aerocal->GetTrajectoryPoint(0),(w/2),(h/2),zf,&ocx,&ocy);		
    		shx=w/2-ocx;
	    	shy=h/2-ocy;
        break;
      case 2:
    		Project.Point(aerocal->GetPeD(),(w/2),(h/2),zf,&ocx,&ocy);
    		shx=w/2-ocx;
	    	shy=h/2-ocy;
        break;
    }

		// disegna la line tra il centro del pianeta e la posizione attuale
		Project.Point(aerocal->GetTrajectoryPoint(0),(w/2),(h/2),zf,&ocx,&ocy);		
		DrawLine(skp,w/2+shx,h/2+shy,ocx+shx,ocy+shy,w,h,false);

    // Disegna la traiettoria
	  int last=aerocal->GetLastPoint();
		for (i=1;i<last;i++) {
			Project.Point(aerocal->GetTrajectoryPoint(i),(w/2),(h/2),zf,&cx,&cy);
      if (aerocal->GetTrajectoryAccel(i)/9.81>10) 
        skp->SetPen(solid_pen_red);
      else
        skp->SetPen(solid_pen);
			DrawLine(skp,ocx+shx,ocy+shy,cx+shx,cy+shy,w,h,false);
			ocx=cx; ocy=cy;			
		}

    // disegna il pianeta
    skp->SetPen(solid_pen_dgrey);
		cx=w/2; cy=h/2;
		r=oapiGetSize(Traj_apr)*zf;
		//DrawEllipse(hDC,cx-r+shx,cy-r+shy,cx+r+shx,cy+r+shy,w,h);  // non disegna ellissi grandissime rispetto allo schermo
    skp->Ellipse((int)floor(cx-r+shx),(int)floor(cy-r+shy),(int)floor(cx+r+shx),(int)floor(cy+r+shy));

    // disegna la linea tra il centro del pianeta e il perigeo
		skp->SetPen(solid_pen_grey); 
		Project.Point(aerocal->GetPeD(),(w/2),(h/2),zf*1.1,&ocx,&ocy);
		DrawLine(skp,cx+shx,cy+shy,ocx+shx,ocy+shy,w,h,false);
    // la crocetta per l' allineamento
    DrawLine(skp,ocx+shx-2,ocy+shy,ocx+shx+3,ocy+shy,w,h,false);
    DrawLine(skp,ocx+shx,ocy+shy-2,ocx+shx,ocy+shy+3,w,h,false);

    // disegna la linea tra il centro del pianeta e il target
    //if (Tgt_lat!=0 && Tgt_lon!=0){
    if (isValidTgt){
      double r=oapiGetSize(Traj_apr);
      double time;
  	  VECTOR3 OffsetV, RotAxis;
  	  time=(aerocal->GetStartMJD()-oapiGetSimMJD())+(Traj_PeT/86400);
  	  PlanetAxis(Traj_apr,time,&RotAxis,&OffsetV);
  	  
      //VECTOR3 tgtp=_V(Tgt_lat,Tgt_lon,r);
      skp->SetPen(solid_pen_y);
      Project.Point(VectorByLonLat(RotAxis,OffsetV,Tgt_lon,Tgt_lat)*r,(w/2),(h/2),zf*1.1,&ocx,&ocy);
      DrawLine(skp,cx+shx,cy+shy,ocx+shx,ocy+shy,w,h,false);
      // la crocetta per l' allineamento
      DrawLine(skp,ocx+shx,ocy+shy-2,ocx+shx,ocy+shy+3,w,h,false);
      DrawLine(skp,ocx+shx-2,ocy+shy,ocx+shx+3,ocy+shy,w,h,false);
      // asse polare
      //SelectObject(hDC,solid_pen_dgrey);
      //Project.Point(_V(0,PI05,r),(w/2),(h/2),zf*1.1,&ocx,&ocy);
      //DrawLine(hDC,cx+shx,cy+shy,ocx+shx,ocy+shy,w,h,false);
      //Project.Point(_V(0,PI05,-r),(w/2),(h/2),zf*1.1,&ocx,&ocy);
      //DrawLine(hDC,cx+shx,cy+shy,ocx+shx,ocy+shy,w,h,false);
    }
	}
	
  if (trajok && aprpage==1){
    // -----------------------------------------------------------------------------------------------------------------------------
    // traiettoria lineare
    if (aprprj%3==0){
      // -----------------------------------------------------------------------------------------------------------------------------
      // traiettoria lineare in proiezione verticale
  	  int i,first=1,last=aerocal->GetLastPoint(),isesc;
      double dist=0,maxr=0,maxa=0,maxv=0,maxq=0,maxm=0,totd,r,escvel;
		  VECTOR3 prev=aerocal->GetTrajectoryPoint(0),curr;		
	  	for (i=0;i<last-1;i++) {
		  	curr=aerocal->GetTrajectoryPoint(i);
        dist+=fabs(length(curr-prev));
        // altezza
        r=aerocal->GetTrajectoryAlt(i);
        if (maxr<r) maxr=r;
        // accelerazione
        r=aerocal->GetTrajectoryAccel(i);
        if (maxa<r) maxa=r;
        // velocita'
        r=aerocal->GetTrajectoryVel(i);
        if (maxv<r) maxv=r;
        // calore
        r=aerocal->GetTrajectoryQFlux(i);
        if (maxq<r) maxq=r;
        // mach
        r=aerocal->GetTrajectoryMach(i);
        if (maxm<r) maxm=r;
        //
        prev=curr;
      }
      totd=dist;
      //
      dist=0;
      prev=aerocal->GetTrajectoryPoint(0);
      double x,y,ox,oyr,oya,oyv,oyq,p;
      double ww=w*0.8,hh=h*0.8,dw=w*0.1,dh=h*0.1,hh2=h*0.6,hh1=h*0.1;
      // disegna delle altezze importanti
      skp->SetPen(dash_pen_ddgrey);
      skp->SetTextColor(green);
      y=hh2-1000.0/maxr*hh2; // 1000 metri
      if (maxr>1000.0){
        DrawLine(skp,dw,y+dh,dw+ww,y+dh,w,h,false);
        Text(skp,(int)(0.9*width),(int)((dh+y)/h*height-5),"1k");
      }
      y=hh2-10000.0/maxr*hh2; // 10000 metri
      if (maxr>10000){
        DrawLine(skp,dw,y+dh,dw+ww,y+dh,w,h,false);
        Text(skp,(int)(0.9*width),(int)((dh+y)/h*height-5),"10k");
      }
      y=hh2-50000.0/maxr*hh2; // 50000 metri
      if (maxr>50000.0){
        DrawLine(skp,dw,y+dh,dw+ww,y+dh,w,h,false);
        Text(skp,(int)(0.9*width),(int)((dh+y)/h*height-5),"50k");
      }
      /*
      y=hh2-1.0/maxm*hh2;
      if (maxm>1.0){
        DrawLine(hDC,dw,y+dh,dw+ww,y+dh,w,h,false);
        Text(hDC,(int)(0.9*width),(int)((dh+y)/h*height-5),"Mach");
      }
      */
      //
#define KM10K 10000000
#define KM1K 1000000
      if (totd>0){
        for(i=0;i*KM10K<totd;i++){
          p=(i*KM10K)/totd*hh;
          DrawLine(skp,dw+p,dh,dw+p,dh+hh,w,h,false);
        }
        int j=(i-1)*KM10K;
        for(i=0;i*KM1K<totd-j;i++){
          p=(j+i*KM1K)/totd*hh;
          DrawLine(skp,dw+p,dh,dw+p,dh+hh,w,h,false);
        }
      }
      // disegna gli assi
      skp->SetPen(solid_pen_dgrey);
      DrawLine(skp,dw,dh+hh2,dw+ww,dh+hh2,w,h,false);
      skp->SetTextColor(grey);
      Text(skp,(int)(0.8*width),(int)((dh+hh2+hh1*2)/h*height),"",totd/1000,"");
      DrawLine(skp,dw,dh+hh2+hh1,dw+ww,dh+hh2+hh1,w,h,false);
      DrawLine(skp,dw,dh+hh2+hh1*2,dw+ww,dh+hh2+hh1*2,w,h,false);
      // ora il grafico
      int firstgmax=1,firstdecel=1;
      //
      for (i=0;i<last-1;i++) { // ho messo last -1 per evitare l' ultimo dato che spesso non e' buono per troncamenti
	  		curr=aerocal->GetTrajectoryPoint(i);
        dist+=fabs(length(curr-prev));
        x=dist/totd*ww;
        // altezza
        r=aerocal->GetTrajectoryAlt(i);if (r<0) r=0;
        y=hh2-r/maxr*hh2;
        if (!first){
          skp->SetPen(solid_pen_dgreen);
          DrawLine(skp,ox+dw,oyr+dh,x+dw,y+dh,w,h,false);
        } else {
          skp->SetTextColor(green);
          Text(skp,(int)(0.1*width),(int)((y+dh+5)/h*height),"Alt");
        }
        oyr=y;
        // velocita'
        escvel=sqrt(2*myy/(radi+aerocal->GetTrajectoryAlt(i)));
        r=aerocal->GetTrajectoryVel(i);
        y=hh2-r/maxv*hh2;
        if (!first){         
          skp->SetPen(isesc?solid_pen_ddgrey:solid_pen_grey);
          DrawLine(skp,ox+dw,oyv+dh,x+dw,y+dh,w,h,false);
          int tesc=r>escvel;
          if (tesc!=isesc) {
            // e' stato catturato!
            skp->SetPen(solid_pen_grey);
            DrawLine(skp,x+dw-2,y+dh-2,x+dw+2,y+dh-2,w,h,false);
            DrawLine(skp,x+dw-2,y+dh+2,x+dw+2,y+dh+2,w,h,false);
            DrawLine(skp,x+dw-2,y+dh-2,x+dw-2,y+dh+2,w,h,false);
            DrawLine(skp,x+dw+2,y+dh-2,x+dw+2,y+dh+3,w,h,false);
            skp->SetTextColor(grey);
            Text(skp,(int)(x+dw-3/w*width),(int)((y+dh-15)/h*height),"e=1");
          }
          isesc=tesc;
        } else {
          skp->SetTextColor(grey);
          Text(skp,(int)(0.1*width),(int)((y+dh)/h*height),"Vel");
          isesc=r>escvel;
        }
        oyv=y;
        // mach
        /*
        r=AeroBrake->GetTrajectoryMach(i);
        y=hh2-r/maxm*hh2;
        if (!first){         
          SelectObject(hDC,(solid_pen_blue));
          DrawLine(hDC,ox+dw,oym+dh,x+dw,y+dh,w,h,false);
        } else {
          SetTextColor(hDC,blue);
          Text(hDC,(int)(0.1*width),(int)((y+dh)/h*height),"Mach");
        }
        oym=y;
        */
        //
        if (i>0) {
          // accelerazione
          r=aerocal->GetTrajectoryAccel(i);
          y=hh1-r/maxa*hh1;
          if (!firstdecel){
            //sprintf(oapiDebugString(),"acc= %8.5f",r/9.81);
            //SelectObject(hDC,(r/9.81>10?solid_pen_red:solid_pen_dgrey));
            skp->SetPen(solid_pen_dgrey);
            DrawLine(skp,ox+dw,oya+dh+hh2,x+dw,y+dh+hh2,w,h,false);
            if (r/9.81>10 && firstgmax){
              firstgmax=0;
              skp->SetPen(solid_pen_red);
              p=hh1-98.1/maxa*hh1;
              DrawLine(skp,ox+dw-(ww/10),p+dh+hh2,ww+dw,p+dh+hh2,w,h,false);
              skp->SetTextColor(red);
              Text(skp,(int)(0.9*width),(int)((p+dh+hh2)/h*height),"10G");
            }
          } else {
            skp->SetTextColor(grey);
            Text(skp,(int)(0.1*width),(int)((y+dh+hh2+10)/h*height-10),"Decel");
          }
          oya=y;
          // calore
          r=aerocal->GetTrajectoryQFlux(i);
          y=hh1-r/maxq*hh1;
          if (!firstdecel){
            skp->SetPen(solid_pen_y);
            DrawLine(skp,ox+dw,oyq+dh+hh1+hh2,x+dw,y+dh+hh1+hh2,w,h,false);
          } else {
            skp->SetTextColor(yellow);
            Text(skp,(int)(0.1*width),(int)((y+dh+hh1+hh2+10)/h*height-10),"Heat");
          }
          oyq=y;
          //
          firstdecel=0;
        }
        ox=x;
        first=0;
        prev=curr;
      }
    } 
    // -----------------------------------------------------------------------------------------------------------------------------
    // Grafico Alt/Vel
    if (aprprj%3==2){
      // -----------------------------------------------------------------------------------------------------------------------------
      // traiettoria lineare in proiezione verticale
      const ATMCONST* atm=oapiGetPlanetAtmConstants(Traj_apr);
  	  int i,first=1,last=aerocal->GetLastPoint();
      double maxr=0,maxa=0,maxv=0,maxq=0,maxm=0,r,firstr;
		  VECTOR3 prev=aerocal->GetTrajectoryPoint(0),curr;		
	  	for (i=0;i<last-1;i++) {
		  	curr=aerocal->GetTrajectoryPoint(i);
        // altezza
        r=aerocal->GetTrajectoryAlt(i);
        if (maxr<r) maxr=r;
        if (i==0) firstr=r;
        // velocita'
        r=aerocal->GetTrajectoryVel(i);
        if (maxv<r) maxv=r;
        //
        prev=curr;
      }
      if (atm){
        if (maxr>atm->altlimit && maxr>firstr) maxr=(firstr>atm->altlimit?firstr:atm->altlimit);
      }
      //
      prev=aerocal->GetTrajectoryPoint(0);
      double x,y,ox,oy;
      double ww=w*0.8,hh=h*0.8,dw=w*0.1,dh=h*0.1;
      // i riferimenti
      skp->SetPen(dash_pen_ddgrey);
      skp->SetTextColor(green);
      x=1000.0/maxr*ww; // 1000 metri
      if (maxr>1000.0){
        DrawLine(skp,dw+x,dh,dw+x,dh+hh,w,h,false);
        //Text(skp,(int)(0.9*width),(int)((dh+y)/h*height-5),"1k");
      }
      x=10000.0/maxr*ww; // 10000 metri
      if (maxr>10000){
        DrawLine(skp,dw+x,dh,dw+x,dh+hh,w,h,false);
      }
      x=50000.0/maxr*w; // 50000 metri
      if (maxr>50000.0){
        DrawLine(skp,dw+x,dh,dw+x,dh+hh,w,h,false);
      }
      x=100000.0/maxr*w; // 100000 metri
      if (maxr>100000.0){
        DrawLine(skp,dw+x,dh,dw+x,dh+hh,w,h,false);
      }
      x=200000.0/maxr*w; // 200000 metri
      if (maxr>200000.0){
        DrawLine(skp,dw+x,dh,dw+x,dh+hh,w,h,false);
      }
      for(i=0;i<maxv/1000;i++){
        y=hh-i*1000/maxv*hh;
        DrawLine(skp,dw,dh+y,dw+ww,dh+y,w,h,false);
      }
      // ora il grafico
      for (i=0;i<last-1;i++) { // ho messo last -1 per evitare l' ultimo dato che spesso non e' buono per troncamenti
	  		curr=aerocal->GetTrajectoryPoint(i);
        // altezza
        r=aerocal->GetTrajectoryAlt(i);if (r<0) r=0;
        x=r/maxr*ww;
        r=aerocal->GetTrajectoryVel(i);
        y=hh-r/maxv*hh;
        if (!first){
          skp->SetPen(solid_pen_dgreen);
          DrawLine(skp,ox+dw,oy+dh,x+dw,y+dh,w,h,false);
        } else {
          skp->SetPen(solid_pen_grey);
          DrawLine(skp,x+dw-2,y+dh-2,x+dw+2,y+dh-2,w,h,false);
          DrawLine(skp,x+dw-2,y+dh+2,x+dw+2,y+dh+2,w,h,false);
          DrawLine(skp,x+dw-2,y+dh-2,x+dw-2,y+dh+2,w,h,false);
          DrawLine(skp,x+dw+2,y+dh-2,x+dw+2,y+dh+3,w,h,false);
        }
        oy=y;
        ox=x;
        first=0;
        prev=curr;
      }
      // il riferimento dell' orbita circolare e della velocita' di fuga
      double v,oyc,oye;
      first=1;
      for (i=0;i<=10;i++){
        v=i*maxr/10.0;
        x=v/maxr*ww;
        y=hh-sqrt(myy/(radi+v))/maxv*hh;
        if (!first) {
          skp->SetPen(solid_pen_y);
          DrawLine(skp,ox+dw,oyc+dh,x+dw,y+dh,w,h,false);
        }
        oyc=y;
        y=hh-sqrt(2*myy/(radi+v))/maxv*hh;
        if (!first) {
          skp->SetPen(solid_pen_y);
          DrawLine(skp,ox+dw,oye+dh,x+dw,y+dh,w,h,false);
        }
        oye=y;
        ox=x;
        first=0;
      }
      Text(skp,width-60,(int)((oyc+dh)/h*height),"circular");
      Text(skp,width-60,(int)((oye+dh)/h*height),"escape");
      Text(skp,width-50,height-30,"Alt.");
      //MFD->SelectDefaultFont(hDC,2);
      Text(skp,15,height-40,"Vel.");
     // MFD->SelectDefaultFont(hDC,1);
      //
      double airv=Ship->GetAirspeed();
      double scl=Ship->GetLift()/Ship->GetDynPressure();
      double maxSCl=shiprecdata->maxLift;
      double minSCl=shiprecdata->minLift;
      if (fabs(Ship->GetBank())>PI05){
        // volo rovesciato
        maxSCl=shiprecdata->minLift;
        minSCl=shiprecdata->maxLift;
      }
      VECTOR3 _V;
      Ship->GetRelativeVel(Traj_apr,_V);
      double relv=length(_V);
      double rotv=relv-airv;
      double mass=Ship->GetMass();
      // il riferimento della velocita' di eq. con lift
      double rho,alt,oymax,oymin,oycur;
      first=1;
      for (i=0;i<=50;i++){
        alt=i*maxr/50.0;
        //rho=atm->rho0*exp(-atm->rho0/atm->p0*Gravity*alt);
        ATMPARAM Atm;
        oapiGetPlanetAtmParams(Traj_apr,alt+radi,&Atm);
        rho=Atm.rho;
        //
        x=alt/maxr*ww;
        //
        v=eqvel(alt+radi,myy/((radi+alt)*(radi+alt)),mass,rho,rotv,maxSCl)+rotv;
        if (v<0) v=maxv*10;
        y=hh-v/maxv*hh;
        if (!first) {
          skp->SetPen(solid_pen_dgrey);
          DrawLine(skp,ox+dw,oymax+dh,x+dw,y+dh,w,h,false);
        }
        oymax=y;
        //
        v=eqvel(alt+radi,myy/((radi+alt)*(radi+alt)),mass,rho,rotv,minSCl)+rotv;
        if (v<0) v=maxv*10;
        y=hh-v/maxv*hh;
        if (!first) {
          skp->SetPen(solid_pen_dgrey);
          DrawLine(skp,ox+dw,oymin+dh,x+dw,y+dh,w,h,false);
        }
        oymin=y;
        //
        v=eqvel(alt+radi,myy/((radi+alt)*(radi+alt)),mass,rho,rotv,scl)+rotv;
        if (v<0) v=maxv*10;
        y=hh-v/maxv*hh;
        if (!first) {
          skp->SetPen(solid_pen_ddgrey);
          DrawLine(skp,ox+dw,oycur+dh,x+dw,y+dh,w,h,false);
        }
        oycur=y;
        ox=x;
        first=0;
      }      
      //Text(skp,width-60,(int)((oycur+dh)/h*height),"current");
      Text(skp,width-60,(int)((oymax+dh)/h*height)+5,"min eq.");
      Text(skp,width-60,(int)((oymin+dh)/h*height)-10,"max eq.");
    } 
    // -----------------------------------------------------------------------------------------------------------------------------
    // Mappa
    if (aprprj%3==1) {
      // -----------------------------------------------------------------------------------------------------------------------------
      // traiettoria lineare in proiezione orizzontale
    	double time,lon,lat,timeoffset;
	    VECTOR3 OffsetV, RotAxis;

	    // days to  periapis
      timeoffset=aerocal->GetStartMJD()-oapiGetSimMJD();
	    time=timeoffset+(Traj_PeT/86400);
	    PlanetAxis(Traj_apr,time,&RotAxis,&OffsetV);
      // disegna gli assi
      //double ww=w*0.8,hh=h*0.4,dw=w*0.1,dh=h*0.1;
      double ww=w,hh=h*0.5,dw=0,dh=h*0.5;
      double odh=h*0.1,odw=w*0.6,ow=w*0.3,oh=h*0.3;
      double x,y,ox,oy;
      int i;
      // il bitmap del pianeta
      if (strcmp(RefName,bmpfile)!=0){
        char fn[128];
        if(bmp!=NULL)
          oapiReleaseTexture(bmp);
        strcpy(fn,"textures/");strcat(fn,RefName);strcat(fn,"M.bmp");
        bmp = oapiLoadTexture(fn);
        bmploaded=true;

        //void oapiBlt (SURFHANDLE tgt, SURFHANDLE src, RECT *tgtr, RECT *srcr, uint32_t ck, int rotation)

      }

      if(bmploaded) {
        int wbmp,hbmp;
        int wtgt,htgt;
        oapiGetTextureSize(bmp, &wbmp,&hbmp);
        oapiGetTextureSize(skp->GetSurface(), &wtgt,&htgt);
/*
typedef struct {
  int left;
  int top;
  int right;
  int bottom;
} RECT;
*/
        RECT tgt{0,htgt/2,wtgt,htgt};
        RECT src{0,0,wbmp,hbmp};
        oapiBlt (skp->GetSurface(), bmp, &tgt, &src);
      }
//      if (bmploaded)
  //      bmp->Paint(hDC,0,height/2,width,height/2,0,0,bmp->Width(),bmp->Height());
      // meridiani e paralleli
      skp->SetPen(solid_pen_dgrey);
      for (i=-6;i<=6;i++){
        x=(i+6)*30.0/360.0*ww;
        DrawLine(skp,x+dw,dh,x+dw,dh+hh,w,h,false);
      }
      for (i=-2;i<=3;i++){
        y=(i+3)*30.0/180.0*hh;
        DrawLine(skp,dw,y+dh,dw+ww,y+dh,w,h,false);
      }
      // la 'bussola' per l' obiettivo
      //if (Tgt_lat!=0 && Tgt_lon!=0){
      if (isValidTgt){
        skp->SetPen(dash_pen_ddgrey);
        for (i=-3;i<=3;i++){
          x=(i+3)/6.0*ow;
          DrawLine(skp,x+odw,odh,x+odw,odh+oh,w,h,false);
          y=oh-(i+3)/6.0*oh;
          DrawLine(skp,odw,odh+y,odw+ow,odh+y,w,h,false);
        }
        skp->SetPen(solid_pen_dgrey);
        DrawLine(skp,odw,odh+oh/2,odw+ow,odh+oh/2,w,h,false);
        DrawLine(skp,odw+ow/2,odh,odw+ow/2,odh+oh,w,h,false);
      }
      // la traccia sul pianeta 
  	  int last=aerocal->GetLastPoint(),first=1;
      VECTOR3 curr,rotaxis,offsetv;
      for (i=0;i<last;i++) {
	  		curr=aerocal->GetTrajectoryPoint(i);
        PlanetAxis(Traj_apr,timeoffset+aerocal->GetTrajectoryTime(i)/86400,&rotaxis,&offsetv);
  	    LonLat(curr,rotaxis,offsetv,&lon,&lat);
        x=((lon*DEG)+180.0)/360.0*ww;
        y=hh-((lat*DEG)+90.0)/180.0*hh;
        if (!first){
          if (x<ww/5.0 && ox>4.0*ww/5.0) {ox=ox-ww;}
          if (x>4.0*ww/5.0 && ox<ww/5.0) {ox=ox+ww;}
          skp->SetPen(solid_pen);
          DrawLine(skp,dw+ox,dh+oy,dw+x,dh+y,w,h,false);
          //DrawLine(skp,dw+x,dh+y,dw+x+1,dh+y,w,h,false);
        } else {
          skp->SetPen(solid_pen_white);
          DrawLine(skp,dw+x-3,dh+y,dw+x+4,dh+y,w,h,false);
          DrawLine(skp,dw+x,dh+y-3,dw+x,dh+y+4,w,h,false);
          first=0;
        }
        ox=x;
        oy=y;
        //DrawLine(skp,dw+x,dh+y,dw+x,dh+y+1,w,h,false);
        //if (Tgt_lat!=0 && Tgt_lon!=0){
        if (isValidTgt){
          double dlat=(Tgt_lat-lat)*DEG,dlon=(Tgt_lon-lon)*DEG;
          if (fabs(dlat)<3 && fabs(dlon)<3){
            x=(-dlon+3)/6.0*ow;
            y=oh-(-dlat+3)/6.0*oh;
            DrawLine(skp,x+odw,odh+y,x+odw+1,odh+y,w,h,false);
          }
        }
      }
      // il punto di atterraggio
      if (length(Traj_PeD)-oapiGetSize(Traj_apr)<=0){
	      LonLat(Traj_PeD,RotAxis,OffsetV,&lon,&lat);
        x=((lon*DEG)+180.0)/360.0*ww;
        y=hh-((lat*DEG)+90.0)/180.0*hh;
        DrawLine(skp,dw+x-2,dh+y-2,dw+x+2,dh+y-2,w,h,false);
        DrawLine(skp,dw+x-2,dh+y+2,dw+x+2,dh+y+2,w,h,false);
        DrawLine(skp,dw+x-2,dh+y-2,dw+x-2,dh+y+2,w,h,false);
        DrawLine(skp,dw+x+2,dh+y-2,dw+x+2,dh+y+3,w,h,false);
        //if (Tgt_lat!=0 && Tgt_lon!=0){
        if (isValidTgt){
          // nel dettaglio
          double dlat=(Tgt_lat-lat)*DEG,dlon=(Tgt_lon-lon)*DEG;
          if (fabs(dlat)<3 && fabs(dlon)<3){
            x=(-dlon+3)/6.0*ow;
            y=oh-(-dlat+3)/6.0*oh;
            DrawLine(skp,x+odw-2,y+odh-2,x+odw+2,y+odh-2,w,h,false);
            DrawLine(skp,x+odw-2,y+odh+2,x+odw+2,y+odh+2,w,h,false);
            DrawLine(skp,x+odw-2,y+odh-2,x+odw-2,y+odh+2,w,h,false);
            DrawLine(skp,x+odw+2,y+odh-2,x+odw+2,y+odh+3,w,h,false);
          }
        }
      }
      // l' obiettivo
      //if (Tgt_lat!=0 && Tgt_lon!=0){
      if (isValidTgt){
        x=((Tgt_lon*DEG)+180.0)/360.0*ww;
        y=hh-((Tgt_lat*DEG)+90.0)/180.0*hh;
        skp->SetPen(solid_pen_y);
        DrawLine(skp,dw+x-2,dh+y-2,dw+x+2,dh+y-2,w,h,false);
        DrawLine(skp,dw+x-2,dh+y+2,dw+x+2,dh+y+2,w,h,false);
        DrawLine(skp,dw+x-2,dh+y-2,dw+x-2,dh+y+2,w,h,false);
        DrawLine(skp,dw+x+2,dh+y-2,dw+x+2,dh+y+3,w,h,false);
        //
        DrawLine(skp,odw+ow/2-2,odh+oh/2-2,odw+ow/2+2,odh+oh/2-2,w,h,false);
        DrawLine(skp,odw+ow/2-2,odh+oh/2+2,odw+ow/2+2,odh+oh/2+2,w,h,false);
        DrawLine(skp,odw+ow/2-2,odh+oh/2-2,odw+ow/2-2,odh+oh/2+2,w,h,false);
        DrawLine(skp,odw+ow/2+2,odh+oh/2-2,odw+ow/2+2,odh+oh/2+3,w,h,false);
      }
    }
  }
  if (aprpage==2 && shiprecdata){
    if (aprprj%3!=2){
      // -----------------------------------------------------------------------------------------------------------------------------
      // disegno della polare
      double minaoa=PI,maxaoa=-PI;
      double minlift=1000000,maxlift=-1000000;
      double mindrag=1000000,maxdrag=-1000000;
      double minlod=1000000,maxlod=-1000000,lod;
      int i,first=1;
      double x,y,ox,oyl,oyd,oylod;
      for(i=0;i<RECPOSITIONS;i++){
        if (minaoa>shiprecdata->recAoA[i]) minaoa=shiprecdata->recAoA[i];
        if (maxaoa<shiprecdata->recAoA[i]) maxaoa=shiprecdata->recAoA[i];
        if (minlift>shiprecdata->recSCl[i]) minlift=shiprecdata->recSCl[i];
        if (maxlift<shiprecdata->recSCl[i]) maxlift=shiprecdata->recSCl[i];
        if (mindrag>shiprecdata->recSCd[i]) mindrag=shiprecdata->recSCd[i];
        if (maxdrag<shiprecdata->recSCd[i]) maxdrag=shiprecdata->recSCd[i];
        if (shiprecdata->recSCd[i]!=0){
          lod=shiprecdata->recSCl[i]/shiprecdata->recSCd[i];
          if (minlod>lod) minlod=lod;
          if (maxlod<lod) maxlod=lod;
        }
      }
      if (minaoa<maxaoa){
        // ha raccolto i dati della polare, puo' disegnare il grafico
        double ww=w*0.8,hh=h*0.8,dw=w*0.1,dh=h*0.1;
        double maxld=(maxlift>maxdrag?maxlift:maxdrag);
        double minld=(minlift<mindrag?minlift:mindrag);
        if (aprprj%3==0) {
          // ----------------------------------------------------
          // grafico Lift, Drag, Lift/Drag in funzione dell' AoA
          // ----------------------------------------------------
          SameZeroLevel(maxld,minld,maxlod,minlod);
          // la scatola che contiene il grafico
          skp->SetPen(dash_pen_dgrey);
          /*
          DrawLine(hDC,w*0.1,h*0.1,w*0.9,h*0.1,w,h,false);
          DrawLine(hDC,w*0.1,h*0.9,w*0.9,h*0.9,w,h,false);
          DrawLine(hDC,w*0.1,h*0.1,w*0.1,h*0.9,w,h,false);
          DrawLine(hDC,w*0.9,h*0.1,w*0.9,h*0.9,w,h,false);
          */
          // misure verticali
          double r;
          skp->SetPen(dash_pen_ddgrey);
          for(i=-9;i<=9;i++){
            r=i*PI/18;
            if (minaoa<r && r<maxaoa) {
              x=(r-minaoa)/(maxaoa-minaoa)*ww;
              DrawLine(skp,dw+x,h*0.1,dw+x,h*0.9,w,h,false);
            }
          }
          // misure orizzontali
          skp->SetPen(dash_pen_ddgrey);
          for (i=(int)minlod;i<maxlod;i++){
            y=hh-(i-minlod)/(maxlod-minlod)*hh;
            DrawLine(skp,w*0.1,y+dh,w*0.9,y+dh,w,h,false);
            char buff[20];
            sprintf(buff,"%3d",i);
            /*
            if (x!=0) {
              // sull' asse delle x
            	SetTextColor(skp,grey);
              Text(skp,(int)((x+dw)/w*width),(int)((y+dh)/h*height),buff);
            }
            */
          	skp->SetTextColor(green);
            Text(skp,(int)((w-dw)/w*width)-15,(int)((y+dh)/h*height),buff);
          }
          // asse verticale sullo 0
          if (minaoa<0 && 0<maxaoa){
            skp->SetPen(solid_pen_dgrey);
            x=(-minaoa)/(maxaoa-minaoa)*ww;
            DrawLine(skp,x+dw,h*0.1,x+dw,h*0.9,w,h,false);
          }
          // asse orizzontale per L=0
          if (minld<0 && 0<maxld){
            skp->SetPen(solid_pen_dgrey);
            y=hh-(-minld)/(maxld-minld)*hh;
            DrawLine(skp,w*0.1,y+dh,w*0.9,y+dh,w,h,false);
          }
          // il grafico vero e proprio
          for(i=0;i<RECPOSITIONS;i++){
            if (shiprecdata->recAoA[i]!=0){
              x=(shiprecdata->recAoA[i]-minaoa)/(maxaoa-minaoa)*ww;
              // lift
              y=hh-(shiprecdata->recSCl[i]-minld)/(maxld-minld)*hh;
              if (!first){
                skp->SetPen(solid_pen_dgrey);
                DrawLine(skp,ox+dw,oyl+dh,x+dw,y+dh,w,h,false);
              }
              oyl=y;
              // drag
              y=hh-(shiprecdata->recSCd[i]-minld)/(maxld-minld)*hh;
              if (!first){
                skp->SetPen(solid_pen_dgrey);
                DrawLine(skp,ox+dw,oyd+dh,x+dw,y+dh,w,h,false);
              }
              oyd=y;
              // lift/drag
              y=hh-((shiprecdata->recSCd[i]!=0?shiprecdata->recSCl[i]/shiprecdata->recSCd[i]:0)-minlod)/(maxlod-minlod)*hh;
              if (!first){
                skp->SetPen(solid_pen_dgreen);
                DrawLine(skp,ox+dw,oylod+dh,x+dw,y+dh,w,h,false);
              }
              oylod=y;
              //
              ox=x;
              first=0;
            }
          }
          // mette le etichette
          skp->SetTextColor(grey);
          Text(skp,(int)(0.9*width),(int)((oyl+dh)/h*height-10),"L");
          Text(skp,(int)(0.9*width),(int)((oyd+dh)/h*height-10),"D");
         	skp->SetTextColor(green);
          Text(skp,(int)(0.9*width),(int)((oylod+dh)/h*height),"L/D");
          // segna l' angolo corrente
          double aoa=Ship->GetAOA();
          if (minaoa<=aoa && aoa<=maxaoa) {
            x=(Ship->GetAOA()-minaoa)/(maxaoa-minaoa)*ww;
            skp->SetPen(solid_pen_y);
            DrawLine(skp,x+dw,dh,x+dw,h-dh,w,h,false);
            Text(skp,(int)((x+dw)/w*width),(int)(0.9*height),"AOA");
          }
          skp->SetPen(solid_pen);
        } else if (aprprj%3==1) {
          // -----------------
          // polare Lift/Drag
          // -----------------
          // misure orizzontali
          skp->SetPen(dash_pen_ddgrey);
          for (i=(int)minld/10;i<maxld/10;i++){
            y=hh-(i*10-minld)/(maxld-minld)*hh;
            DrawLine(skp,w*0.1,y+dh,w*0.9,y+dh,w,h,false);
            x=(i*10-minld)/(maxld-minld)*hh;
            DrawLine(skp,x+dw,h*0.1,x+dw,h*0.9,w,h,false);
          }
          // asse orizzontale per L=0
          if (minld<0 && 0<maxld){
            skp->SetPen(solid_pen_dgrey);
            y=hh-(-minld)/(maxld-minld)*hh;
            DrawLine(skp,w*0.1,y+dh,w*0.9,y+dh,w,h,false);
            skp->SetTextColor(grey);
            Text(skp,(int)(0.85*width),(int)((y+dh)/h*height),"Drag");
          }
          // asse verticale per D=0
          if (minld<0 && 0<maxld){
            skp->SetPen(solid_pen_dgrey);
            x=(-minld)/(maxld-minld)*ww;
            DrawLine(skp,x+dw,h*0.1,x+dw,h*0.9,w,h,false);
            skp->SetTextColor(grey);
            Text(skp,(int)((x+dw-15)/w*width),(int)(height*0.9),"Lift");
          }
          int lastanglelabel=-10000;
          // il grafico vero e proprio
          for(i=0;i<RECPOSITIONS;i++){
            if (shiprecdata->recAoA[i]!=0){
              // lift
              y=hh-(shiprecdata->recSCl[i]-minld)/(maxld-minld)*hh;
              x=(shiprecdata->recSCd[i]-minld)/(maxld-minld)*ww;
              if (!first){
                skp->SetPen(solid_pen_dgreen);
                DrawLine(skp,ox+dw,oyl+dh,x+dw,y+dh,w,h,false);
              }
              // l' etichetta per i gradi
              int a=(int)(shiprecdata->recAoA[i]*DEG);
              if (a%5==0 && lastanglelabel!=a){
                char buff[20];
                sprintf(buff,"%d",a);
                skp->SetTextColor(grey);
                Text(skp,(int)((x+dw)/w*width),(int)((y+dh-5)/h*height),buff);
                lastanglelabel=a;
              }
              oyl=y;
              ox=x;
              first=0;
            }
          }
          // il vettore corrente
          double aoa=Ship->GetAOA();
          double scl=shiprecdata->getSCl(aoa);
          double scd=shiprecdata->getSCd(aoa);
          if (scd!=0){
            x=(scd-minld)/(maxld-minld)*ww;
            y=hh-(scl-minld)/(maxld-minld)*hh;
            ox=(-minld)/(maxld-minld)*ww;
            oyl=hh-(-minld)/(maxld-minld)*hh;          
            skp->SetPen(solid_pen_y);
            DrawLine(skp,ox+dw,oyl+dh,x+dw,y+dh,w,h,false);
          }
        }
      }
    } 
    if (aprprj%3==2){
      // mappa di poincare' dei dati dell' autopilota
      double mindelta=0,maxdelta=0;
      double minrate=0,maxrate=0;
      int i,first=1;
      double x,y,ox,oy;
      for(i=0;i<RECDRPOS;i++){
        if (minrate>recRate[i]) minrate=recRate[i];
        if (maxrate<recRate[i]) maxrate=recRate[i];
        if (mindelta>recDelta[i]) mindelta=recDelta[i];
        if (maxdelta<recDelta[i]) maxdelta=recDelta[i];
      }
      //if (minrate>0) minrate=0;
      //if (maxrate<0) maxrate=0;
      //if (mindelta<0) mindelta=0;
      //if (maxdelta<0) maxdelta=0;
      if (-minrate<maxrate) minrate=-maxrate; else maxrate=-minrate;
      if (-mindelta<maxdelta) mindelta=-maxdelta; else maxdelta=-mindelta;
      double ww=w*0.8,hh=h*0.8,dw=w*0.1,dh=h*0.1;
      // gli assi
      skp->SetPen(solid_pen_grey);
      DrawLine(skp,dw,dh+hh/2,dw+ww,dh+hh/2,w,h,false);
      DrawLine(skp,dw+ww/2,dh,dw+ww/2,dh+hh,w,h,false);
      //
      for(i=recDRPos;i<recDRPos+RECDRPOS;i++){
        x=(recDelta[i%RECDRPOS]-mindelta)/(maxdelta-mindelta)*ww;
        y=(maxrate-recRate[i%RECDRPOS])/(maxrate-minrate)*hh;
        if (!first){
          skp->SetPen(solid_pen);
          DrawLine(skp,dw+ox,dh+oy,dw+x,dh+y,w,h,false);
        }
        ox=x;
        oy=y;
        first=0;
      }
      //
      skp->SetPen(solid_pen_y);
      DrawLine(skp,dw+x-3,dh+y,dw+x+4,dh+y,w,h,false);
      DrawLine(skp,dw+x,dh+y-3,dw+x,dh+y+4,w,h,false);
      Text(skp,width-50,height/2,"delta");
      //MFD->SelectDefaultFont(hDC,2);
      Text(skp,width/2-12,height-30,"rate");
      //MFD->SelectDefaultFont(hDC,1);
      // i parametri PID
      char buff[256];
      if (autopilotmode==1){
        sprintf(buff,"rcs P=%5.2f",shiprecdata->aoa_rcs_p);
        Text(skp,10,30,buff);
        sprintf(buff,"      I=%5.2f",shiprecdata->aoa_rcs_i);
        Text(skp,10,40,buff);
        sprintf(buff,"     D=%5.2f",shiprecdata->aoa_rcs_d);
        Text(skp,10,50,buff);
        sprintf(buff,"elev P=%5.2f",shiprecdata->aoa_elev_p);
        Text(skp,10,60,buff);
        sprintf(buff,"       I=%5.2f",shiprecdata->aoa_elev_i);
        Text(skp,10,70,buff);
        sprintf(buff,"      D=%5.2f",shiprecdata->aoa_elev_d);
        Text(skp,10,80,buff);
      } else if (autopilotmode==2){
        sprintf(buff,"P=%5.2f",shiprecdata->alt_p);
        Text(skp,10,30,buff);
        sprintf(buff,"I=%5.2f",shiprecdata->alt_i);
        Text(skp,10,40,buff);
        sprintf(buff,"D=%5.2f",shiprecdata->alt_d);
        Text(skp,10,50,buff);
      }
    }
    
  }

  //MFD->SelectDefaultFont(hDC,0);
	if (aprmode!=0 && trajok) { // Do we show the text           					
  	// DRAW TEXT DISPLAY

  	double time,lon,lat;
	  VECTOR3 air2,air,OffsetV, RotAxis;

	  // days to  periapis
	  time=(aerocal->GetStartMJD()-oapiGetSimMJD())+(Traj_PeT/86400);
	  PlanetAxis(Traj_apr,time,&RotAxis,&OffsetV);
	  LonLat(Traj_PeD,RotAxis,OffsetV,&lon,&lat);

	  air2=AirSpeed(Traj_PeD,RotAxis,oapiGetPlanetPeriod(Traj_apr));
	  air=Traj_PeV-air2;

	  pos=fbp;					
	  Ship->GetShipAirspeedVector(air2);
	
		skp->SetTextAlign (oapi::Sketchpad::LEFT);
    skp->SetTextColor(lgreen);
		Text(skp,5,pos,name); 
		pos+=(int)(1.5*ld);

		// Attitude Warning
		//bool att=false;
		//if (Attitude==0) Attitude=1;	
		//if (Attitude==1 && theShip->GetNavmodeState(NAVMODE_PROGRADE)) att=true;
		//if (Attitude==2 && theShip->GetNavmodeState(NAVMODE_RETROGRADE)) att=true;	
   	
		// Draw text display
    double scl,scd;
    switch(Attitude){
      case ATT_PROGRADE:
  	  	skp->SetTextColor(lgreen);
        //Text(hDC,5,pos,"Attitude : Pro-grade"); 
		    //pos+=ld;
        Text(skp,5,pos,"Rho      : ",Rho); 
    		pos+=ld;
  		  //Text(hDC,5,pos,"Pressure : ",Pressure," Pa");   
	  	  pos+=ld;
		    //Text(hDC,5,pos,"Density  : ",Density," kg/m�"); 
  		  pos+=ld;
  		  pos+=ld;
        break;
      case ATT_RETROGRADE:
  	  	skp->SetTextColor(lgreen);
        //Text(hDC,5,pos,"Attitude : Retro-grade"); 
		    //pos+=ld;
        Text(skp,5,pos,"Rho      : ",Rho); 
    		pos+=ld;
  		  //Text(hDC,5,pos,"Pressure : ",Pressure," Pa");   
	  	  pos+=ld;
		    //Text(hDC,5,pos,"Density  : ",Density," kg/m�"); 
  		  pos+=ld;
  		  pos+=ld;
        break;
      case ATT_REALTIMEUP:
      case ATT_REALTIMEBANK:
  	  	skp->SetTextColor(lgreen);
        //Text(hDC,5,pos,"Attitude : Realtime"); 
		    //pos+=ld;
        scl=aerocal->GetSCl();
        scd=aerocal->GetSCd();
        Text(skp,5,pos,"AoA      : ",aerocal->GetAoA()*DEG," Deg");
	    	pos+=ld;
        Text(skp,5,pos,"L/D      : ",scl/(scd==0?1:scd));
	    	pos+=ld;
	  	  Text(skp,5,pos,"S*Cl     : ",scl); 
  		  pos+=ld;
    		Text(skp,5,pos,"S*Cd     : ",scd); 
    		pos+=ld;
        break;
    }

    
		//Text(hDC,5,pos,"AirSpeed : ",length(air2)," m/s"); 
		//pos+=ld;
		//Text(hDC,5,pos,"Dyn      : ",Ship->GetDynPressure()," Pa"); 
		//pos+=ld;

		pos+=ld;

    if (aprmode==1){
      if ((aerocal->GetDeAcc()/9.81)>10) skp->SetTextColor(red);
	  	Text(skp,5,pos,"G-Max    : ",aerocal->GetDeAcc()/9.81," G");
  		skp->SetTextColor(lgreen);
		  pos+=ld;

	  	double alt=length(Traj_PeD)-oapiGetSize(Traj_apr);
	  	if (alt<0) alt=0;
      if (aerocal->isTouchDown()) alt=0;

		  char buf[64], buf2[64];
		  memset(buf,0,10);
		  memset(buf2,0,10);

      if (alt>0) {

		    Text(skp,5,pos,"Pe Alt.  : ",alt," m"); 
	  	  pos+=ld;
	  	  Text(skp,5,pos,"'' Time  : ",Traj_PeT," s");
	  	  pos+=ld;
	  	  Text(skp,5,pos,"'' Vel.  : ",length(air)," m/s");		
	  	  pos+=ld;			
		
		    value(fabs(lon)*DEG,buf,0);
		    value(fabs(lat)*DEG,buf2,0);          
		    if (lon<0 && lat>0)	sprintf(LineBuffer,"'' Pos   : %sW  %sN",buf,buf2);
		    if (lon<0 && lat<0)	sprintf(LineBuffer,"'' Pos   : %sW  %sS",buf,buf2);
		    if (lon>0 && lat>0)	sprintf(LineBuffer,"'' Pos   : %sE  %sN",buf,buf2);
		    if (lon>0 && lat<0)	sprintf(LineBuffer,"'' Pos   : %sE  %sS",buf,buf2);
            
		    Text(skp,5,pos,LineBuffer);
		    pos+=ld;	
      
        if (AprOrbit.ecc<1)
          Text(skp,5,pos,"Apo Alt. : ",AprOrbit.ApoapisDistance()-radi," m");		
        else
          Text(skp,5,pos,"Apo Alt. : #");		
   	    pos+=ld;			
  
	  	  Text(skp,5,pos,"Ecc      : ",AprOrbit.ecc);
		    pos+=ld;	
	  	  Text(skp,5,pos,"dV       : ",-aerocal->GetdVLose()," m/s"); 
	    	pos+=ld;
	    	Text(skp,5,pos,"RInc.    : ",(shiporbit.inc-AprOrbit.inc)*DEG);		
	  	  pos+=ld;			

      } else {

 		    //Text(skp,5,pos,"Land Alt.: ",alt," m"); 
	  	  //pos+=ld;
	    	Text(skp,5,pos,"Land Time: ",Traj_PeT," s");
	    	pos+=ld;
	    	Text(skp,5,pos," ''  Vel.: ",length(air)," m/s");		
	    	pos+=ld;			
		
	  	  value(fabs(lon)*DEG,buf,0);
		    value(fabs(lat)*DEG,buf2,0);          
		    if (lon<0 && lat>0)	sprintf(LineBuffer," ''  Pos : %sW  %sN",buf,buf2);
		    if (lon<0 && lat<0)	sprintf(LineBuffer," ''  Pos : %sW  %sS",buf,buf2);
		    if (lon>0 && lat>0)	sprintf(LineBuffer," ''  Pos : %sE  %sN",buf,buf2);
		    if (lon>0 && lat<0)	sprintf(LineBuffer," ''  Pos : %sE  %sS",buf,buf2);
            
		    Text(skp,5,pos,LineBuffer);
		    pos+=ld;	
		    pos+=ld;	
		    pos+=ld;	
		    pos+=ld;	

      }

      pos+=ld;
      //if (TgtHandle!=NULL){
      //if (alt<=0 && Tgt_lat!=0 && Tgt_lon!=0) {
      if (alt<=0 && isValidTgt) {
        VECTOR3 pell=_V(lat,lon,0);
	  	  memset(buf,0,10);
	  	  memset(buf2,0,10);
  		  value(fabs(Tgt_lon)*DEG,buf,0);
  		  value(fabs(Tgt_lat)*DEG,buf2,0);          
  		  if (Tgt_lon<0 && Tgt_lat>0)	sprintf(LineBuffer,"Tgt Pos  : %sW  %sN",buf,buf2);
  		  if (Tgt_lon<0 && Tgt_lat<0)	sprintf(LineBuffer,"Tgt Pos  : %sW  %sS",buf,buf2);
  		  if (Tgt_lon>0 && Tgt_lat>0)	sprintf(LineBuffer,"Tgt Pos  : %sE  %sN",buf,buf2);  
	  	  if (Tgt_lon>0 && Tgt_lat<0)	sprintf(LineBuffer,"Tgt Pos  : %sE  %sS",buf,buf2);
  	  	Text(skp,5,pos,LineBuffer);
    		pos+=ld;	
        VECTOR3 tgtll=_V(Tgt_lat,Tgt_lon,0);
    		Text(skp,5,pos,"''  Dist : ",CalcSphericalDistance(tgtll,pell,oapiGetSize(Traj_apr)));
    		pos+=ld;	
      }
    }
    if (aprmode==2) {
      // velocita' di equilibrio
      double airv=Ship->GetAirspeed();
      double dynp=Ship->GetDynPressure();
      double mass=Ship->GetMass();
      double alt=radi+Ship->GetAltitude();
      double SCl=Ship->GetLift()/dynp;
      double maxSCl=shiprecdata->maxLift;
      double minSCl=shiprecdata->minLift;
      double rho=Ship->GetAtmDensity();
      VECTOR3 _V;
      Ship->GetRelativeVel(Traj_apr,_V);
      double relv=length(_V);
      double grav=myy/(alt*alt);
      double rotv=relv-airv;
      //

      //double r=mass*(grav-(relv*relv)/alt);
      //Text(hDC,5,pos,"req lift:",r); 
  		//pos+=ld;
      //Text(hDC,5,pos,"rtv:",relv-airv); 
  		//pos+=ld;
      //Text(hDC,5,pos,"lift:",Ship->GetLift()); 
  		//pos+=ld;

      double vmin=eqvel(alt,grav,mass,rho,rotv,maxSCl);
      Text(skp,5,pos,"min Vel. :",vmin); 
  	  pos+=ld;

      double v=eqvel(alt,grav,mass,rho,rotv,SCl);
      Text(skp,5,pos,"eq. Vel. :",v); 
  		pos+=ld;

      double vmax=eqvel(alt,grav,mass,rho,rotv,minSCl);
      Text(skp,5,pos,"Max Vel. :",vmax); 
		  pos+=ld;

  		pos+=ld;

      char buff[256];
      sprintf(buff," (%4.3f Deg)",shiprecdata->maxLDAoA*DEG);
      Text(skp,5,pos,"Max L/D  : ",shiprecdata->maxLD,buff); 
	  	pos+=ld;
      sprintf(buff," (%4.3f Deg)",shiprecdata->minLDAoA*DEG);
      Text(skp,5,pos,"min L/D  : ",shiprecdata->minLD,buff); 
	  	pos+=ld;
      Text(skp,5,pos,"Max Lift : ",shiprecdata->maxLiftAoA*DEG," Deg"); 
  		pos+=ld;
      Text(skp,5,pos,"min Lift : ",shiprecdata->minLiftAoA*DEG," Deg"); 
  		pos+=ld;
      Text(skp,5,pos,"Max Drag : ",shiprecdata->maxDragAoA*DEG," Deg"); 
  		pos+=ld;
      Text(skp,5,pos,"min Drag : ",shiprecdata->minDragAoA*DEG," Deg"); 
  		pos+=ld;
    }

	} // mode
 
	// SHOW WARNINGS

	//if (trajok && length(Traj_PeD)<oapiGetSize(Traj_apr)) {
	//	SetTextAlign(hDC,TA_CENTER);
	//	SetTextColor(hDC,lgreen);
	//	Text(hDC,width/2,last-ld/4,"Re-Entry"); 
	//}

	if (aerocal->GetLastPoint()==0) {
		pos=fbp;
		skp->SetTextColor(lgreen);
		skp->SetTextAlign (oapi::Sketchpad::CENTER);
		if (!aerocal->IsProgress()) Text(skp,width/2,pos+10,"No Trajectory"); 
	}

	pos=0;
	skp->SetTextColor(grey);
  skp->SetTextAlign (oapi::Sketchpad::LEFT);
	Text(skp,width/2,pos,"Ref ",RefName), pos+=ld-2;
	Text(skp,width/2,pos,"Tgt ",Target), pos+=ld-2;
  if (autopilotmode==1){
  	skp->SetTextColor(yellow);
	  Text(skp,width/2,pos,"AoA ",keepRef*DEG);
    pos+=ld;
  } else if (autopilotmode==2){
  	skp->SetTextColor(yellow);
	  Text(skp,width/2,pos,"Alt ",keepRef);
    pos+=ld;
  }
  if (bankautopilotmode==1){
  	skp->SetTextColor(yellow);
	  Text(skp,width/2,pos,"Bank ",bankRef*DEG);
    pos+=ld;
  }
	// Title
	skp->SetTextAlign (oapi::Sketchpad::LEFT);
	skp->SetTextColor(white);
	skp->Text(5,1,Tit,strlen(Tit));
  switch(aprpage){
    case 0:
      skp->Text(5,ld-1,"Path",4);
      break;
    case 1:
      skp->Text(5,ld-1,"Graph/Map",9);
      break;
    case 2:
      skp->Text(5,ld-1,"Lift/Drag",9);
      break;
  }
  // HDV
  if (hdv!=0){
    skp->SetTextColor(yellow);
    Text(skp,5,height-20,"HDV:",hdv,"");
  }
  // TDA
  if (tda!=0){
    skp->SetTextColor(yellow);
    Text(skp,5,height-20,"TDA:",tda,"");
  }

}
