

//------------------------------------------------
// Aero-space engine 
// (c) Jarmo Nikkanen 2003 
//------------------------------------------------

#include <stdio.h>
#include <math.h>
#include <malloc.h>
#include "OrbiterAPI.h"
#include "Orbitersdk.h"
#include "ABToolkit.h"
#include "ABMFD.h"
#include "Aero.h"

#ifdef ORBITER2006 
#define Normalize(x) x/length(x)
#else
#ifdef ORBITER2010
#define Normalize(x) x/length(x)
#else
#include "Cdk/OrbiterMath.h"
#endif
#endif

AeroCal::AeroCal(VESSEL* ship){
	int i;

  Ship=ship;
  Attitude=0;
  MAX_SAMPLES=MAX_INTEGRATION_STEP-2;
	traj_ped=_V(1e20,0,0);
	traj_time=0;
	traj_pos=0;
  traj_pev=_V(0,0,0);
  traj_pet=0;
  traj_peindex=0;
  traj_rep=_V(0,0,0);
  maxdeacc=0;
  dvlose=0;

  Traj_Pos=0;
	Traj_PeD=Traj_PeV=_V(0,0,0);
	Traj_PeT=0;

	FinalD=FinalV=_V(0,0,0);

	Cal_progress=false;

	refhandle=NULL;
	RefHandle=NULL;

	PeD=1e20;
	
	for (i=0;i<4;i++) 
    Traj_handle[i]=NULL;  	

  Rho = 0;
  SCl = 0;
  SCd = 0;
  BankAngle=0;

}

AeroCal::~AeroCal(){
}

void AeroCal::Stop(bool isTouchDown){
	Cal_progress=false;
  
	FinalD=Traj_position[0]-Traj_position[1];
	FinalV=Traj_velocity[0]-Traj_velocity[1];
	
	Traj_ReP=traj_rep; // Reference position at pe timing
	Traj_Pos=traj_pos;
	Traj_PeV=traj_pev;
	Traj_PeD=traj_ped;
	Traj_PeT=traj_pet;	
	Traj_PeIndex=traj_peindex;

	MaxDeAcc=maxdeacc;
	dVLose=dvlose;

	RefHandle=refhandle;
  //sprintf(oapiDebugString(),"stop %10.5f",StartMJD);
  isTchDn=isTouchDown;
}

void AeroCal::Start(double time){
  fso=1;
  traj_ped=_V(1e20,0,0);
	traj_time=0;
	traj_pos=0;	
	step=0;

	PeD=1e20;

	maxdeacc=0;
	dvlose=0;
  segmentdecel=0;

	stop=time;

	// Check that planet and vessel exist
	if (Traj_handle[0]==NULL || Traj_handle[1]==NULL) 
    return;
			
	// Setup Planet
	Traj_velocity[0]=_V(0,0,0);
	Traj_position[0]=_V(0,0,0);
	Traj_myy[0] = oapiGetMass(Traj_handle[0]) * GC;
         	
	// Setup Vessel
	oapiGetRelativePos(Traj_handle[1],Traj_handle[0],&Traj_position[1]);
	oapiGetRelativeVel(Traj_handle[1],Traj_handle[0],&Traj_velocity[1]);				
	Traj_myy[1] = oapiGetMass(Traj_handle[1]) * GC;; 

	StartMJD=oapiGetSimMJD();
	Cal_progress=true;

  if (hdv){
    VECTOR3 nv=Normalize(Traj_velocity[1]);
    Traj_velocity[1]-=nv*hdv;
  }

  //sprintf(oapiDebugString(),"start %10.2f",length(Traj_position[1]-Traj_position[0]));
  //fso=1;
}

void AeroCal::SetHDV(double v){
  hdv=v;
}

void AeroCal::SetTDA(double a){
  TouchDownAlt=a;
}

VECTOR3 AeroCal::GetReP(){
  return(Traj_ReP);
}

VECTOR3 AeroCal::GetPeD(){ // Periapis Distance
  return(Traj_PeD);
}

VECTOR3 AeroCal::GetPeV(){ // Periapis Velocity
  return(Traj_PeV);
}

double AeroCal::GetPeT(){ // Time To Periapis 
  return(Traj_PeT);
}

double AeroCal::GetStartMJD(){
  return(StartMJD);
}

double AeroCal::GetDeAcc(){
  return(MaxDeAcc);
}

double AeroCal::GetdVLose(){
  return(dVLose);
}

double AeroCal::GetStepping(){ // Time To Periapis 
  return(step);
}

bool AeroCal::IsProgress(){
  return(Cal_progress);
}

VECTOR3 AeroCal::GetFinalD(){
  return(FinalD);
}

VECTOR3 AeroCal::GetFinalV(){
  return(FinalV);
}

void AeroCal::SetLoops(int o,int i){
  OuterLoop=o;
  InnerLoop=i;
}

void AeroCal::SetShipMass(double mass){
  Mass=mass;
}

void AeroCal::SetShipRho(double rho){
  Rho = rho;
}

void AeroCal::SetMach(double mach){
  MachNumber = mach;
}

void AeroCal::SetSCl(double lift){
  SCl = lift;
}

void AeroCal::SetSCd(double drag){
  SCd = drag;
}

void AeroCal::SetAoA(double aoa){
  AoA = aoa;
}

void AeroCal::SetBank(double bank){
  BankAngle=bank;
}

void AeroCal::SetAttitude(int attitude){
  if (Cal_progress)
    Stop(false);
  Attitude = attitude;
}

void AeroCal::SetAtmosphere(OBJHANDLE obj){
  Traj_handle[0] = obj;
  refhandle	   = obj;

  RotAxis		= RotationAxis(obj);	
  RotPeriod	= oapiGetPlanetPeriod(obj);
  Radius		= oapiGetSize(obj);
  TouchDownAlt = 0;

  const ATMCONST *Atmconst = oapiGetPlanetAtmConstants(obj);
  if (Atmconst)
	  AltLimit = Atmconst->altlimit;
  else
	  AltLimit = 0;
}

OBJHANDLE AeroCal::GetReference(){
  return(RefHandle);
}

bool AeroCal::GetBodyName(int i,char *s,int max){
  if (Traj_handle[i]) {
	  oapiGetObjectName(Traj_handle[i],s,max);
  	return true;
  }
  return false;
}

void AeroCal::SetShipHandle(OBJHANDLE bod){
  Traj_handle[1]=bod;	
}

VECTOR3 AeroCal::GetTrajectoryPoint(int index){
  return(TrajPathMap[index]);
}

double AeroCal::GetTrajectoryAccel(int index){
  return(TrajPathAccel[index]);
}

double AeroCal::GetTrajectoryVel(int index){
  return(TrajPathVel[index]);
}

double AeroCal::GetTrajectoryAlt(int index){
  return(TrajPathAlt[index]);
}

double AeroCal::GetTrajectoryQFlux(int index){
  return(TrajPathQFlux[index]);
}

double AeroCal::GetTrajectoryMach(int index){
  return(TrajPathMach[index]);
}

double AeroCal::GetTrajectoryTime(int index){
  return(TrajPathTime[index]);
}

int AeroCal::GetLastPoint(){
  return(Traj_Pos);
}

double AeroCal::GetTime(){	
  return traj_time;
}

// Here is some heavy calculations

double AeroCal::ComputeStep(double alt,double decel){
  double step=alt/1e6;step=step/2;
  if (alt<10000) {
    step=0.25;
  } else if (alt<100000){
    step=0.5;
  } else if (step<2)
    step=2;
  if (step>30) 
    step=30;
  // piu' di 10G
  if (decel>100){
    step=0.05;
    if (decel>400){
      step=0.01;
    }
  }
       // if (fso<10) {
       //   if (fso==9) sprintf(oapiDebugString(),"alt %5.4f decel %5.4f step %5.4f",alt,decel,step);
       //   fso++;
       // }
  return step;
}

void AeroCal::Segment(){
  int i;
  double step;

  ATMPARAM Atm;
  const ATMCONST* AtmConst=oapiGetPlanetAtmConstants(refhandle);
  VECTOR3 repo=Traj_position[1] - Traj_position[0];
  double dis  = length(repo);
	double alt  = dis - Radius;		
  step=ComputeStep(alt,segmentdecel);
  segmentdecel=0;
  segmentqflux=0;

  // Do we have a ground contact
  if (dis<Radius+TouchDownAlt) {
	  Stop(true);	
	  return;
  }
    
  for (i=0;i<InnerLoop;i++) {
  	RungeKutta2(Traj_position,Traj_velocity,Traj_myy,2,step);
	
    VECTOR3 relv = Traj_velocity[1] - Traj_velocity[0];
	  VECTOR3 repo = Traj_position[1] - Traj_position[0];
			
		double DeAcc = 0;

           
		if (alt<AltLimit) {

			VECTOR3 Air = relv - AirSpeed(repo,RotAxis,RotPeriod);
			double  vel = length(Air);	
      
			oapiGetPlanetAtmParams(refhandle,dis,&Atm);

      segmentmach=vel/sqrt(AtmConst->gamma*Atm.p/Atm.rho);

			double dyn = Atm.rho * vel * vel * 0.5;

			double hAcc,vAcc;
      double lift= dyn*SCl;

      switch(Attitude){
        case ATT_REALTIMEUP:
        case ATT_REALTIMEBANK:
          // Realtime
          hAcc = -dyn * (SCd>0?SCd:Rho) / Mass; 
          vAcc = lift/Mass;
          //sprintf(oapiDebugString(),"rt %f",(SCd?SCd:Rho));
          break;
        default:  
          // Pro-grade
          hAcc = -dyn * Rho / Mass;
          vAcc = 0;
          //sprintf(oapiDebugString(),"pro %f",Rho);
          break;
      }

      double DeAcc=sqrt(hAcc*hAcc+vAcc*vAcc);

      if (dis>Radius+TouchDownAlt){
        // questi dati sono validi solo se non si e' atterrati
			  if (DeAcc>maxdeacc) 
			    maxdeacc=DeAcc;
        if (DeAcc>segmentdecel)
          segmentdecel=DeAcc;
      
			  dvlose += hAcc * step;
      }

       //if (fso==1) {
       //  sprintf(oapiDebugString(),"alt %5.4f deacc %5.4f",alt,segmentdecel);
       //  fso++;
       //}

      if (Atm.rho>0 && dis>Radius+TouchDownAlt) {
        // Jarmo: drag
        Traj_velocity[1] += Air * (hAcc*step/vel);
  
        // Greg: lift
        VECTOR3 LiftDir,h;
        
        h=crossp(repo,Air); // Posizione x Velocita' nell' aria (identifico il piano)
        LiftDir=crossp(Air,h); // ruoto di 90 gradi rispetto all' asse y (v?)

        h=Normalize(h);
        LiftDir=Normalize(LiftDir);

        if (Attitude==ATT_REALTIMEBANK){
          double vAccLateral = vAcc*sin(BankAngle);
          double vAccVert = vAcc*cos(BankAngle);
          // componente verticale
          VECTOR3 Lift = LiftDir * vAccVert * step;
		      Traj_velocity[1] += Lift;
          // componente laterale
          Lift = h * -vAccLateral * step;
		      Traj_velocity[1] += Lift;

          //sprintf(oapiDebugString(),"Bank %5.4f",BankAngle);
        } else { 
          VECTOR3 Lift= LiftDir * vAcc * step ;
		      Traj_velocity[1] += Lift;
        }
        
        double q=Atm.rho*vel*vel*vel/2;
        if (segmentqflux<q)
          segmentqflux=q;
        //if (fso) {
          //sprintf(oapiDebugString(),"nAir(%5.4f,%5.4f,%5.4f) LiftDir(%5.4f,%5.4f,%5.4f) pos(%5.4f,%5.4f,%5.4f)",nAir.x,nAir.y,nAir.z,LiftDir.x,LiftDir.y,LiftDir.z,Normalize(repo).x,Normalize(repo).y,Normalize(repo).z);
          //sprintf(oapiDebugString(),"Lift %5.4f %5.4f, Drag %5.4f %5.4f",lift,theShip->GetLift(),SCd*dyn,theShip->GetDrag());
          //sprintf(oapiDebugString(),"Bank %5.4f %d DeAcc %5.4f DeAcc*step %5.4f step %5.4f segdec %5.4f",Ship->GetBank(),liftMode,DeAcc,DeAcc*step,step,segmentdecel);
        //  fso=0;
        //}
      }
		}

		traj_time+=step;


		// Store Periapis Data
		if (dis<PeD) {
			PeD=dis;
			traj_peindex=traj_pos;
			traj_pet=traj_time;
			traj_ped=repo;
			traj_pev=relv;
		}


		// Compute step size
		step=ComputeStep(alt,segmentdecel);  
  
		// Do we have a ground contact
		if (dis<Radius+TouchDownAlt) {
			Stop(true);
			return;
		}

	} // Inner Loop
}


void AeroCal::Timestep(){
  int i;
  if (Cal_progress){ // il calcolo e' in corso 	 
    for (i=0;i<OuterLoop;i++) { // esegue per OL volte
      if (traj_pos<(MAX_SAMPLES-1)) { // nella posizione tray_pos mette la posizione calcolata
        TrajPathMap[traj_pos]=(Traj_position[1]-Traj_position[0]); 
        TrajPathAccel[traj_pos]=segmentdecel;
        TrajPathAlt[traj_pos]=length(Traj_position[1]-Traj_position[0])-Radius;
        TrajPathQFlux[traj_pos]=segmentqflux;
        TrajPathVel[traj_pos]=length(Traj_velocity[1] - Traj_velocity[0]);
        TrajPathMach[traj_pos]=(traj_pos?segmentmach:MachNumber);
        TrajPathTime[traj_pos]=traj_time;
		    traj_pos++;
      }
      Segment();
	    if (traj_time > stop) {	// TRAJECTORY COMPLETED	
		    Stop(false);
		    break;
      }	
    }
  }
}




