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

//#include "CDK/OrbiterMath.h"
//#include "CDK\gravbodydata.h"

ShipRec::ShipRec(char* s){
  //  Record di AoA,SCl e SCd
  for(int i=0;i<RECPOSITIONS;i++){
    recAoA[i]=0;
    recSCl[i]=0;
    recSCd[i]=0;
  }
  strcpy(shipname,s);
  maxLift=-100000000.0;
  maxLiftAoA=0;
  minLift=10000000.0;
  minLiftAoA=0;
  maxDrag=-10000000.0;
  maxDragAoA=0;
  minDrag=10000000.0;
  minDragAoA=0;
  maxLD=-100000000.0;
  maxLDAoA=0;
  minLD=1000000.0;
  minLDAoA=0;

  alt_p=-0.5;
  alt_i=-0.05;
  alt_d=-8.0;

  bank_p=-0.5;
  bank_i=-0.05;
  bank_d=-2.5;

  aoa_rcs_p=-8.0;
  aoa_rcs_i=-4.0;
  aoa_rcs_d=-8.0;

  aoa_elev_p=-20.0;
  aoa_elev_i=-8.0;
  aoa_elev_d=-20.0;

  Read();

  refcnt=1;

  char error_message[256];
  sprintf(error_message,"Creato ShipRec at %p per %s",this,s);
  Bugr(error_message);

}

ShipRec::~ShipRec(){
  char error_message[256];
  sprintf(error_message,"Distrutto ShipRec at %p",this);
  Bugr(error_message);
}

void ShipRec::IncRef(){
  refcnt++;
}

void ShipRec::DecRef(){
  refcnt--;
  if (refcnt==0)
    delete this;
}

void ShipRec::Save(){
  char fn[256];
  strcpy(fn,"Modules/Plugin/");
  strcat(fn,shipname);
  strcat(fn,".ld");

  FILE* f;
  f=fopen(fn,"wt");
  if (f){
    for(int i=0;i<RECPOSITIONS;i++){
      if (recAoA[i]!=0){
        char buff[256];
        sprintf(buff,"LD %f %f %f\n",recAoA[i],recSCl[i],recSCd[i]);
        fputs(buff,f);
      }
    }
    //
    char buff[256];
    sprintf(buff,"ALT P %f\n",alt_p);fputs(buff,f);
    sprintf(buff,"ALT I %f\n",alt_i);fputs(buff,f);
    sprintf(buff,"ALT D %f\n",alt_d);fputs(buff,f);
    //
    sprintf(buff,"BANK P %f\n",bank_p);fputs(buff,f);
    sprintf(buff,"BANK I %f\n",bank_i);fputs(buff,f);
    sprintf(buff,"BANK D %f\n",bank_d);fputs(buff,f);
    //
    sprintf(buff,"AOA RCS P %f\n",aoa_rcs_p);fputs(buff,f);
    sprintf(buff,"AOA RCS I %f\n",aoa_rcs_i);fputs(buff,f);
    sprintf(buff,"AOA RCS D %f\n",aoa_rcs_d);fputs(buff,f);
    //
    sprintf(buff,"AOA ELEV P %f\n",aoa_elev_p);fputs(buff,f);
    sprintf(buff,"AOA ELEV I %f\n",aoa_elev_i);fputs(buff,f);
    sprintf(buff,"AOA ELEV D %f\n",aoa_elev_d);fputs(buff,f);
    //
    fclose(f);
  }
}

void ShipRec::Read(){
  char fn[256];
  strcpy(fn,"Modules/Plugin/");
  strcat(fn,shipname);
  strcat(fn,".ld");
  FILE* f;
  f=fopen(fn,"rt");
  if (f){
    while(!feof(f)){
      char buff[256];
      fgets(buff,256,f);
      if (strncmp(buff,"LD ",3)==0){
        buff[strlen(buff)-2]=0;
        float a,l,d;
        sscanf(buff+3,"%f %f %f",&a,&l,&d);
        recLDPolar(a,l,d); 
      }
      if (strncmp(buff,"ALT P ",6)==0){
        buff[strlen(buff)-2]=0;
        float n;
        sscanf(buff+6,"%f",&n);
        alt_p=n;
      } else if (strncmp(buff,"ALT I ",6)==0){
        buff[strlen(buff)-2]=0;
        float n;
        sscanf(buff+6,"%f",&n);
        alt_i=n;
      } else if (strncmp(buff,"ALT D ",6)==0){
        buff[strlen(buff)-2]=0;
        float n;
        sscanf(buff+6,"%f",&n);
        alt_d=n;
      } else if (strncmp(buff,"BANK P ",6)==0){
        buff[strlen(buff)-2]=0;
        float n;
        sscanf(buff+7,"%f",&n);
        bank_p=n;
      } else if (strncmp(buff,"BANK I ",6)==0){
        buff[strlen(buff)-2]=0;
        float n;
        sscanf(buff+7,"%f",&n);
        bank_i=n;
      } else if (strncmp(buff,"BANK D ",6)==0){
        buff[strlen(buff)-2]=0;
        float n;
        sscanf(buff+7,"%f",&n);
        bank_d=n;
      } else if (strncmp(buff,"AOA RCS P ",10)==0){
        buff[strlen(buff)-2]=0;
        float n;
        sscanf(buff+10,"%f",&n);
        aoa_rcs_p=n;
      } else if (strncmp(buff,"AOA RCS I ",10)==0){
        buff[strlen(buff)-2]=0;
        float n;
        sscanf(buff+10,"%f",&n);
        aoa_rcs_i=n;
      } else if (strncmp(buff,"AOA RCS D ",10)==0){
        buff[strlen(buff)-2]=0;
        float n;
        sscanf(buff+10,"%f",&n);
        aoa_rcs_d=n;
      } else if (strncmp(buff,"AOA ELEV P ",11)==0){
        buff[strlen(buff)-2]=0;
        float n;
        sscanf(buff+11,"%f",&n);
        aoa_elev_p=n;
      } else if (strncmp(buff,"AOA ELEV I ",11)==0){
        buff[strlen(buff)-2]=0;
        float n;
        sscanf(buff+11,"%f",&n);
        aoa_elev_i=n;
      } else if (strncmp(buff,"AOA ELEV D ",11)==0){
        buff[strlen(buff)-2]=0;
        float n;
        sscanf(buff+11,"%f",&n);
        aoa_elev_d=n;
      }
    }
    fclose(f);
  }
}

void ShipRec::recLDPolar(double aoa,double lift,double drag){
  int p=(int)floor(aoa*RECPOSITIONS/PI/2)+RECPOSITIONS/2;
  if (p>=0 && p<RECPOSITIONS){
    if (recAoA[p]==0 || recAoA[p]>aoa){
      // se non era stato registrato o e' piu' vicino allora registra
      recAoA[p]=aoa;
      recSCl[p]=lift;
      recSCd[p]=drag;
      // il massimo
    }
  }
  if (maxLift<lift){
    maxLift=lift;
    maxLiftAoA=aoa;
  }
  if (minLift>lift){
    minLift=lift;
    minLiftAoA=aoa;
  }
  if (maxDrag<drag){
    maxDrag=drag;
    maxDragAoA=aoa;
  }
  if (minDrag>drag){
    minDrag=drag;
    minDragAoA=aoa;
  }
  if (drag!=0){
    double ld=lift/drag;
    if (maxLD<ld){
      maxLD=ld;
      maxLDAoA=aoa;
    }
    if (minLD>ld){
      minLD=ld;
      minLDAoA=aoa;
    }
  }
}

double ShipRec::getSCd(double aoa){
  int p=(int)floor(aoa*RECPOSITIONS/PI/2)+RECPOSITIONS/2;
  int p1=p;
  while(p1>0 && recAoA[p1]==0) p1--;
  int p2=p+1;
  while(p2<RECPOSITIONS && recAoA[p2]==0) p2++;
  if (p1>0 && p2<RECPOSITIONS){
    double d=(aoa-recAoA[p1])/(recAoA[p2]-recAoA[p1]);
    return recSCd[p1]+d*(recSCd[p2]-recSCd[p1]);
  }
  return recSCd[p];
}

double ShipRec::getSCl(double aoa){
  int p=(int)floor(aoa*RECPOSITIONS/PI/2)+RECPOSITIONS/2;
  int p1=p;
  while(p1>0 && recAoA[p1]==0) p1--;
  int p2=p+1;
  while(p2<RECPOSITIONS && recAoA[p2]==0) p2++;
  if (p1>0 && p2<RECPOSITIONS){
    double d=(aoa-recAoA[p1])/(recAoA[p2]-recAoA[p1]);
    return recSCl[p1]+d*(recSCl[p2]-recSCl[p1]);
  }
  return recSCl[p];
}
