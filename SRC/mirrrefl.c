/***************************************************************************/
/* mirrrefl: Functions to move neutrons, used in module mirror_elliptical  */
/***************************************************************************/

#include "mirror.h"


/****************************************************************/
/* Propagation of the neutron to the end of the mirror          */
/****************************************************************/
double  PathThroughMirrorGravOrder2(Neutron *ThisNeutron, MirrorSecond MyMirror, double X_MIN, double X_MAX, double Y_MIN, double Y_MAX, double Z_MIN, double Z_MAX, double halfaxis[3], VectorType PosMain,
                                    double WeightMin, double reflectivitylup[1000], double reflectivityldo[1000], double surfacerough, long keyGrav, long keypol, long qspin, int vistype, long visall,
                                    long keyreflect, int *reflperf, double RotMatrixMirror[3][3], long keyfluxair, double mu1, double mu2)
{

  /* Note! The function is return Time Of Flight             */

  int i, ThisCollision=1, datanumber;
  double angular,degangular;
  double  TimeOF, TimeOFmin;
  double  TimeOFTotal=0.0;
  double  AP, BP, CP, DOTP, FP, VelocityReal;
  double signl, mutotal, prob;
  /* KL: new variable */
  double stepp, temp,
         OneMatrix[3][3];
  /* Local copy of neutrons for cylce.. */
  Neutron TempNeutron, NearestNeutron;
  VectorType Pos, Dir;

  InitRotMatrix(OneMatrix);


#ifdef VT_GRAPH
  if (do_visualise)
  {
    double tempx=0.0, tempy=0.0;

    cpgsci(6);

    if (vistype == 0)
    {
      tempx = ThisNeutron->Position[0] + PosMain[0];
      tempy = ThisNeutron->Position[2] + PosMain[2];
    }

    if (vistype == 1)
    {
      tempx = ThisNeutron->Position[0] + PosMain[0];
      tempy = ThisNeutron->Position[1] + PosMain[1];
    }

    if (vistype == 2)
    {
      tempx = ThisNeutron->Position[1] + PosMain[1];
      tempy = ThisNeutron->Position[2] + PosMain[2];
    }

    cpgmove(tempx,tempy);
  }
#endif

  TimeOFTotal = 0.0 ;
  stepp = 0.05; /* in cm */
  if (ThisNeutron->Wavelength == 0.0) return(-1.0);
  if (ThisNeutron->Vector[0] < 0.0)  return(-1.0);

  /* Convert stepp in time ms */
  VelocityReal = (double)(V_FROM_LAMBDA(ThisNeutron->Wavelength));
  if (VelocityReal == 0.0) return(-1.0);
  stepp = stepp/VelocityReal;
  /***********************************************************************************/
  /* The main loop here is continuous: the neutron will continue to bounce around,   */
  /* until it is absorbed or intercepts with the exit plane.                         */
  /***********************************************************************************/

  /*  fprintf(LogFilePtr,"weight min  %e \n",WeightMin); */

  *reflperf = 0;

  while(GUIDEFLIGHT)
  {
    TimeOFmin = 99999999999999999999999.9;


    /***********************************************************************************/
    /* Loop through all 2 planes....                                                */
    /***********************************************************************************/
    for (i=0; i < 2; i++)
    {
      /***********************************************************************************/
      /* If the plane currently indexed is the one that the neutron has just hit, return */
      /* contol to the top of the loop and look at the next plane.                       */
      /***********************************************************************************/

      /***********************************************************************************/
      /* Find the point where this neutron trajectory intercepts the currently indexed   */
      /* plane                                         */
      /***********************************************************************************/

      /*Save current neutron, becouse function NeutronPlaneIntersectionGrav have
      modify Neutron data structure  */

      CopyNeutron(ThisNeutron, &TempNeutron);

      if (i == 0)
      {
        CopyVector(TempNeutron.Position, Pos) ;
        CopyVector(TempNeutron.Vector, Dir) ;
        RotVector(RotMatrixMirror, Pos) ;
        RotVector(RotMatrixMirror, Dir) ;
        CopyVector(Pos, TempNeutron.Position) ;
        CopyVector(Dir, TempNeutron.Vector) ;
      }

      //      TimeOF=NeutronSurfaceSecIntersectionLimit(&TempNeutron, MyMirror.Surf[i], keyGrav, i, X_MIN, X_MAX, Y_MIN, Y_MAX, Z_MIN, Z_MAX, PosMain);
      if (keyGrav == 1)
      {
        TimeOF = NeutronSurfaceSecIntersectionGravLen(&TempNeutron, MyMirror.Surf[i]);
        TimeOF = Checklimits(&TempNeutron, i, TimeOF, X_MIN, X_MAX, Y_MIN, Y_MAX, Z_MIN, Z_MAX);
        //    fprintf(LogFilePtr,"Grav ON \n");
      }
      else
      {
        TimeOF = NeutronSurfaceSecIntersectionGrav(&TempNeutron, MyMirror.Surf[i], 0);
        TimeOF = Checklimits(&TempNeutron, i, TimeOF, X_MIN, X_MAX, Y_MIN, Y_MAX, Z_MIN, Z_MAX);
        //    fprintf(LogFilePtr,"Grav OFF \n");
      }

      if (i == 0)
      {
        CopyVector(TempNeutron.Position, Pos) ;
        CopyVector(TempNeutron.Vector, Dir) ;
        RotBackVector(RotMatrixMirror, Pos ) ;
        RotBackVector(RotMatrixMirror, Dir ) ;
        CopyVector(Pos, TempNeutron.Position) ;
        CopyVector(Dir, TempNeutron.Vector) ;
      }

      //        fprintf(LogFilePtr,"num = %d time = %e \n",i, TimeOF);
      /***********************************************************************************/
      /* If this intercept point is behind the neutrons current position, pass control to*/
      /* the top of the loop: OR Time of flight <= 0.0, Fixed Manoshin Sergey 19.02.00   */
      /***********************************************************************************/

      /* ILLEGAL NEUTRON PARAMETERS */

      //      if (TempNeutron.Position[0] < ThisNeutron->Position[0]) continue;
      if (TempNeutron.Vector[0] < 0.0) continue;
      if (TimeOF <= 0.0) continue;

      /***********************************************************************************/
      /* If this calculated distance is not the shortest so far, return to the top of the*/
      /* loop.  TimeOF -> min                                                            */
      /***********************************************************************************/
      if(TimeOF > TimeOFmin) continue;

      /***********************************************************************************/
      /* The intercept of the neutron with this wall is the nearest so far, so accept it */
      /* temporarily.                                                                    */
      /***********************************************************************************/

      CopyNeutron(&TempNeutron, &NearestNeutron);
      TimeOFmin = TimeOF;
      ThisCollision = i;
      if (ThisCollision==0)  // neutrons exiting the mirror will be written in the main() program
        WriteScatIAP(&NearestNeutron, VT_REFLECTED, OneMatrix, PosMain);
    }

    /* protect against cycling */
    if ((ThisCollision < 0)||(ThisCollision > 1)) return(-1.0);
    if (TimeOF <= 0.0) return(-1.0);

    /* Neutrons going in the exit  */

    if(ThisCollision == 1)
    {
      /* Illegal velocity */
      if(NearestNeutron.Vector[0] < 0.0)  return(-1.0);
      if (NearestNeutron.Probability <= WeightMin)  return(-1.0);

      CopyNeutron(&NearestNeutron, ThisNeutron);
      TimeOFTotal =  TimeOFTotal + TimeOFmin;

      /* air attenuation block 2 */
      if (keyfluxair == 1)
      {
        mutotal = ThisNeutron->Wavelength*mu2 + mu1; /* absrorption depends from lambda, scattring not */
        mutotal = mutotal/100.0;
        prob = exp(-mutotal*TimeOFmin*VelocityReal/100.0); /* /100 convert cm to m */
        ThisNeutron->Probability = ThisNeutron->Probability*prob;
      }
      /* air attenuation block 2 end */

    #ifdef VT_GRAPH
      if (do_visualise)
      {
        double tempx=0.0, tempy=0.0;
        if (vistype == 0)
        {
          tempx = ThisNeutron->Position[0] + PosMain[0];
          tempy = ThisNeutron->Position[2] + PosMain[2];
        }

        if (vistype == 1)
        {
          tempx = ThisNeutron->Position[0] + PosMain[0];
          tempy = ThisNeutron->Position[1] + PosMain[1];
        }

        if (vistype == 2)
        {
          tempx = ThisNeutron->Position[1] + PosMain[1];
          tempy = ThisNeutron->Position[2] + PosMain[2];
        }

        if (visall == 0)
        {
          if (*reflperf == 1)
          {
            cpgdraw(tempx,tempy);
            cpgsci(3);
            cpgpt1(tempx,tempy,-2);
          }
        }
        else
        {
          cpgdraw(tempx,tempy);
          cpgsci(3);
          cpgpt1(tempx,tempy,-2);
        }
      }
      #endif

      return TimeOFTotal;

    }

    /* normal vector in surface which will be reflected */
    AP = 2.0*MyMirror.Surf[ThisCollision].A*NearestNeutron.Position[0]
    + MyMirror.Surf[ThisCollision].B
    + MyMirror.Surf[ThisCollision].P*NearestNeutron.Position[1]
    + MyMirror.Surf[ThisCollision].R*NearestNeutron.Position[2];

    BP = 2.0*MyMirror.Surf[ThisCollision].C*NearestNeutron.Position[1]
    + MyMirror.Surf[ThisCollision].D
    + MyMirror.Surf[ThisCollision].P*NearestNeutron.Position[0]
    + MyMirror.Surf[ThisCollision].Q*NearestNeutron.Position[2];

    CP = 2.0*MyMirror.Surf[ThisCollision].E*NearestNeutron.Position[2]
    + MyMirror.Surf[ThisCollision].F
    + MyMirror.Surf[ThisCollision].Q*NearestNeutron.Position[1]
    + MyMirror.Surf[ThisCollision].R*NearestNeutron.Position[0];

    /* Normalize normale vector to the reflection plane */
    FP = sqrt(AP*AP + BP*BP + CP*CP);
    if (FP == 0.0) return(-1.0);
    AP = AP/FP;
    BP = BP/FP;
    CP = CP/FP;

    /* influence of rough surface */
    temp = SurfaceRough(&AP, &BP, &CP, surfacerough) ;
    if (temp == -1.0) return(-1.0);

    if (keyreflect == 0)
    {
      angular=fabs(NeutronPlaneAngle2(&NearestNeutron, AP, BP, CP));
      /* Convert from radian to degree */
      degangular = angular*360.0/(2.0*M_PI);
      datanumber =  (int)(degangular*1000.0/(NearestNeutron.Wavelength));
      if (datanumber > 999) return(-1.0);

      if (keypol == 1)
      {
        if(NearestNeutron.Spin[qspin] == 1.0)
        {
          NearestNeutron.Probability *= reflectivitylup[datanumber];
        }
        if(NearestNeutron.Spin[qspin] == -1.0)
        {
          NearestNeutron.Probability *= reflectivityldo[datanumber];
        }
      }
      else
      {
        NearestNeutron.Probability *= reflectivitylup[datanumber];
      }


      if (NearestNeutron.Probability <= WeightMin)  return(-1.0);
    }

    CopyNeutron(&NearestNeutron, ThisNeutron);

    /***********************************************************************************/
    /*  Calculate the trajectory of the reflected neutron.                             */
    /***********************************************************************************/
    /* Make reflection */

    DOTP = AP*NearestNeutron.Vector[0] + BP*NearestNeutron.Vector[1] +
    CP*NearestNeutron.Vector[2];

    ThisNeutron->Vector[0] = NearestNeutron.Vector[0] - 2.0*DOTP*AP;
    ThisNeutron->Vector[1] = NearestNeutron.Vector[1] - 2.0*DOTP*BP;
    ThisNeutron->Vector[2] = NearestNeutron.Vector[2] - 2.0*DOTP*CP;

    TimeOFTotal =  TimeOFTotal + TimeOFmin ;

    /* air attenuation block 2 */
    if (keyfluxair == 1)
    {
      mutotal = ThisNeutron->Wavelength*mu2 + mu1; /* absrorption depends from lambda, scattring not */
      mutotal = mutotal/100.0;
      prob = exp(-mutotal*TimeOFmin*VelocityReal/100.0); /* /100 convert cm to m */
      ThisNeutron->Probability = ThisNeutron->Probability*prob;
    }

    /* air attenuation block 2 end */

  #ifdef VT_GRAPH
    if (do_visualise)
    {
      double tempx=0.0, tempy=0.0;
      /* visual path begin */

      if (vistype == 0)
      {
        tempx = ThisNeutron->Position[0] + PosMain[0];
        tempy = ThisNeutron->Position[2] + PosMain[2];
      }

      if (vistype == 1)
      {
        tempx = ThisNeutron->Position[0] + PosMain[0];
        tempy = ThisNeutron->Position[1] + PosMain[1];
      }

      if (vistype == 2)
      {
        tempx = ThisNeutron->Position[1] + PosMain[1];
        tempy = ThisNeutron->Position[2] + PosMain[2];
      }

      cpgdraw(tempx,tempy);
      cpgsci(7);
    }
  #endif


    /* CHECK incorrect flight of neutron, move neutron after reflection on small distance
       and check inside channel or no , no - exit(-1) */

    ThisNeutron->Position[0] = ThisNeutron->Position[0] + VelocityReal*stepp*(ThisNeutron->Vector[0]);
    ThisNeutron->Position[1] = ThisNeutron->Position[1] + VelocityReal*stepp*(ThisNeutron->Vector[1]);
    ThisNeutron->Position[2] = ThisNeutron->Position[2] + VelocityReal*stepp*(ThisNeutron->Vector[2]);

    TimeOFTotal = TimeOFTotal + stepp;

    /* check the rteflection from inner surfaces, otherwise neutron loss  */

    signl =
      MyMirror.Surf[0].A*ThisNeutron->Position[0]*ThisNeutron->Position[0] +
      MyMirror.Surf[0].B*ThisNeutron->Position[0] +
      MyMirror.Surf[0].C*ThisNeutron->Position[1]*ThisNeutron->Position[1]+
      MyMirror.Surf[0].D*ThisNeutron->Position[1] +
      MyMirror.Surf[0].E*ThisNeutron->Position[2]*ThisNeutron->Position[2]+
      MyMirror.Surf[0].F*ThisNeutron->Position[2] +
      MyMirror.Surf[0].W +
      MyMirror.Surf[0].P*ThisNeutron->Position[0]*ThisNeutron->Position[1]+
      MyMirror.Surf[0].Q*ThisNeutron->Position[1]*ThisNeutron->Position[2]+
      MyMirror.Surf[0].R*ThisNeutron->Position[2]*ThisNeutron->Position[0] ;

    /* particle outside ellipsoid */
    if ((signl) > 0.0) return(-1.0);

    *reflperf = 1;
  }
}


/****************************************************************/
/* Check if neutron position is within the limits of the mirror */
/****************************************************************/
double Checklimits(Neutron *ThisNeutron, int ikey, double Time, double X_MIN, double X_MAX, double Y_MIN, double Y_MAX, double Z_MIN, double Z_MAX)
{
  double TimeOut;

  TimeOut=-1.0;
  if (Time <= 0.0) return(TimeOut);

  if (ikey == 0)
  {
    if ((X_MIN <= ThisNeutron->Position[0]) && (X_MAX >= ThisNeutron->Position[0]) &&
        (Y_MIN <= ThisNeutron->Position[1]) && (Y_MAX >= ThisNeutron->Position[1]) &&
        (Z_MIN <= ThisNeutron->Position[2]) && (Z_MAX >= ThisNeutron->Position[2])   )
    {
      TimeOut = Time;
    }
    else
    {
      TimeOut = -1.0;
    }
  }
  else
  {
    TimeOut = Time;
  }

  return(TimeOut);
}


/***********************************************************************/
/* Procedure for moving the neutron WITH gravity for a given time      */
/***********************************************************************/
double NeutronMove(Neutron *ThisNeutron, double Time)
{

  double  VelocityReal;
  double  NewWavelength, OldWavelength, DOTP, DOTPnew;

  /*  Make the koefficients of quadratic equation    */
  /*  G = 9.8 m/c**2, we need to cm/ms**2 (100/1000/1000)  */
  /*  Velocity cm/ms, Time ms, Position cm  */

  /*  Calculating real velocity, cm/ms  */

  VelocityReal = (double)(V_FROM_LAMBDA(ThisNeutron->Wavelength));
  OldWavelength = ThisNeutron->Wavelength ;
  if (Time <= 0.0) return(-1.0);


  ThisNeutron->Position[0] = ThisNeutron->Position[0] + VelocityReal*Time*(ThisNeutron->Vector[0]);
  ThisNeutron->Position[1] = ThisNeutron->Position[1] + VelocityReal*Time*(ThisNeutron->Vector[1]);
  ThisNeutron->Position[2] = ThisNeutron->Position[2] + VelocityReal*Time*(ThisNeutron->Vector[2]);
  ThisNeutron->Position[2] = ThisNeutron->Position[2] - 0.5*(G*1.0e-4)*Time*Time;

  DOTP = ThisNeutron->Vector[0]*ThisNeutron->Vector[0] +
         ThisNeutron->Vector[1]*ThisNeutron->Vector[1] +
         ThisNeutron->Vector[2]*ThisNeutron->Vector[2] ;

  DOTP = sqrt(DOTP);
  if (DOTP == 0.0) return(-1.0);

  ThisNeutron->Vector[0] = (ThisNeutron->Vector[0])*VelocityReal ;
  ThisNeutron->Vector[1] = (ThisNeutron->Vector[1])*VelocityReal ;
  ThisNeutron->Vector[2] = (ThisNeutron->Vector[2])*VelocityReal - ((G*1.0e-4)*Time) ;

  DOTPnew = ThisNeutron->Vector[0]*ThisNeutron->Vector[0] +
            ThisNeutron->Vector[1]*ThisNeutron->Vector[1] +
            ThisNeutron->Vector[2]*ThisNeutron->Vector[2] ;

  DOTPnew = sqrt(DOTPnew);
  if (DOTPnew == 0.0) return(-1.0);


  NewWavelength = LAMBDA_FROM_V(DOTPnew);
  ThisNeutron->Wavelength  = NewWavelength;
  ThisNeutron->Vector[0] = (ThisNeutron->Vector[0])/DOTPnew ;
  ThisNeutron->Vector[1] = (ThisNeutron->Vector[1])/DOTPnew ;
  ThisNeutron->Vector[2] = (ThisNeutron->Vector[2])/DOTPnew ;

  return(1.0);
}


/*******************************************************/
/* Function for simulation of surface roughness        */
/*******************************************************/
double SurfaceRough(double *AP, double *BP, double *CP, double surfacerough)
{
  double VX, VY, VZ, FP;

  /* check datas */
  if (surfacerough == 0.0) return(1.0);
  if (surfacerough < 0.0)  surfacerough = fabs(surfacerough);

  // len = vector3rand(&VX, &VY, &VZ);
  gsl_ran_dir_3d( vit_gsl_rng, &VX, &VY, &VZ);
  /*fprintf(LogFilePtr,"vx vy vz %f  %f  %f  %f  %f \n",VX,VY,VZ,len,surfacerough);*/
  *AP = *AP + surfacerough*VX;
  *BP = *BP + surfacerough*VY;
  *CP = *CP + surfacerough*VZ;
  /* Renormalize normale vector to the reflection plane */
  FP = sqrt((*AP)*(*AP) + (*BP)*(*BP) + (*CP)*(*CP));
  if (FP == 0.0) return(-1.0);
  *AP = *AP/FP;
  *BP = *BP/FP;
  *CP = *CP/FP;
  return(1.0);
}


/************************************************************************/
/* Calculation of the point of reflection on the mirror without gravity */
/************************************************************************/
double NeutronSurfaceSecIntersectionGrav(Neutron *ThisNeutron, SurfaceSecond ThisSurfaceSecond, long keyGrav)
{
  /***********************************************************************************/
  /* This part calculates the time of flight of neutron INCLUDE gravity with   */
  /* plane.  This functions is core for transporting neutrons!                     */
  /***********************************************************************************/

  double  Time, AA, BB, CC, VelocityReal;
  double  VX, VY, VZ, X, Y, Z;
  double  NewWavelength, OldWavelength, DOTP, DOTPnew;

  /*  Make the koefficients of quadratic equation    */
  /*  G = 9.8 m/c**2, we need to cm/ms**2 (100/1000/1000)  */
  /*  Velocity cm/ms, Time ms, Position cm  */

  /*  Calculating real velocity, cm/ms  */

  VelocityReal = (double)(V_FROM_LAMBDA(ThisNeutron->Wavelength));
  OldWavelength = ThisNeutron->Wavelength ;

  /*  Components of velocity, projections, and position coordinats */
  /*  Local copy */

  X = ThisNeutron->Position[0];
  Y = ThisNeutron->Position[1];
  Z = ThisNeutron->Position[2];

  VX = VelocityReal*ThisNeutron->Vector[0];
  VY = VelocityReal*ThisNeutron->Vector[1];
  VZ = VelocityReal*ThisNeutron->Vector[2];

  /*  Find the coefficients of the quadratic equation */

  if (keyGrav == 1)
  {
    if ((ThisSurfaceSecond.A == 0.0)&&(ThisSurfaceSecond.C == 0.0)&&
        (ThisSurfaceSecond.E == 0.0)&&(ThisSurfaceSecond.P == 0.0)&&
        (ThisSurfaceSecond.Q == 0.0)&&(ThisSurfaceSecond.R == 0.0))
    {
      AA = -0.5*(G*1.0e-4)*ThisSurfaceSecond.F;
    }
    else
    {
      return(-1.0);
    }
  }
  else
  {
    AA = ThisSurfaceSecond.A*VX*VX +
    ThisSurfaceSecond.C*VY*VY +
    ThisSurfaceSecond.E*VZ*VZ +
    ThisSurfaceSecond.P*VX*VY +
    ThisSurfaceSecond.Q*VY*VZ +
    ThisSurfaceSecond.R*VX*VZ;
  }

  BB = 2.0*(ThisSurfaceSecond.A*VX*X + ThisSurfaceSecond.C*VY*Y + ThisSurfaceSecond.E*VZ*Z) +
  ThisSurfaceSecond.B*VX + ThisSurfaceSecond.D*VY + ThisSurfaceSecond.F*VZ +
  ThisSurfaceSecond.P*(X*VY+Y*VX) +
  ThisSurfaceSecond.Q*(Y*VZ+Z*VY) +
  ThisSurfaceSecond.R*(X*VZ+Z*VX);

  CC = ThisSurfaceSecond.A*X*X + ThisSurfaceSecond.B*X+
  ThisSurfaceSecond.C*Y*Y + ThisSurfaceSecond.D*Y+
  ThisSurfaceSecond.E*Z*Z + ThisSurfaceSecond.F*Z + ThisSurfaceSecond.W +
  ThisSurfaceSecond.P*X*Y + ThisSurfaceSecond.Q*Y*Z + ThisSurfaceSecond.R*X*Z;


  //  fprintf(LogFilePtr,"AA = %f  BB = %f  CC = %f  \n", AA, BB, CC);

  /***********************************************************************************/
  /* Now we must to decide quadratic equation for find */
  /* Time AA*Time*Time + BB*Time + CC = 0 */
  /***********************************************************************************/

  Time = SolveQuadraticEq(AA,BB,CC);
  if (Time <= 0.0) return(-1.0);


  ThisNeutron->Position[0] = ThisNeutron->Position[0] + VelocityReal*Time*(ThisNeutron->Vector[0]);
  ThisNeutron->Position[1] = ThisNeutron->Position[1] + VelocityReal*Time*(ThisNeutron->Vector[1]);
  ThisNeutron->Position[2] = ThisNeutron->Position[2] + VelocityReal*Time*(ThisNeutron->Vector[2]);

  /* Include gravity */
  if (keyGrav == 1)
  {
    ThisNeutron->Position[2] = ThisNeutron->Position[2] - 0.5*(G*1.0e-4)*Time*Time;

    /*OLD:    ThisNeutron->Vector[2] = ThisNeutron->Vector[2] - ((G*1.0e-4)*Time/VelocityReal);  */

    DOTP = ThisNeutron->Vector[0]*ThisNeutron->Vector[0] +
    ThisNeutron->Vector[1]*ThisNeutron->Vector[1] +
    ThisNeutron->Vector[2]*ThisNeutron->Vector[2] ;

    DOTP = sqrt(DOTP);
    if (DOTP == 0.0) return(-1.0);

    //    fprintf(LogFilePtr,"DOTPold = %f VelocityReal = %f \n", DOTP, VelocityReal);

    ThisNeutron->Vector[0] = (ThisNeutron->Vector[0])*VelocityReal ;
    ThisNeutron->Vector[1] = (ThisNeutron->Vector[1])*VelocityReal ;
    ThisNeutron->Vector[2] = (ThisNeutron->Vector[2])*VelocityReal - ((G*1.0e-4)*Time) ;

    DOTPnew = ThisNeutron->Vector[0]*ThisNeutron->Vector[0] +
              ThisNeutron->Vector[1]*ThisNeutron->Vector[1] +
              ThisNeutron->Vector[2]*ThisNeutron->Vector[2] ;

    DOTPnew = sqrt(DOTPnew);
    if (DOTPnew == 0.0) return(-1.0);

    NewWavelength = LAMBDA_FROM_V(DOTPnew);

    //    fprintf(LogFilePtr,"DOTPnew = %f  Wave_old = %f  Wave_new = %f \n", DOTPnew, OldWavelength, NewWavelength);
    //    fprintf(LogFilePtr,"---------------------------------------------------\n");

    ThisNeutron->Wavelength  = NewWavelength;
    ThisNeutron->Vector[0] = (ThisNeutron->Vector[0])/DOTPnew ;
    ThisNeutron->Vector[1] = (ThisNeutron->Vector[1])/DOTPnew ;
    ThisNeutron->Vector[2] = (ThisNeutron->Vector[2])/DOTPnew ;
  }
  /* return Time; ms */

  return Time;
}


/************************************************************************/
/* Calculation of the point of reflection on the mirror with gravity    */
/************************************************************************/
double NeutronSurfaceSecIntersectionGravLen(Neutron *ThisNeutron, SurfaceSecond ThisSurfaceSecond)
{
  double  Time, Time1, Time2, Time22, AA, BB, CC, VelocityReal;
  double  TimeInit, TimeInit1, TimeCalc, diss, disscalc, di;
  double  VX, VY, VZ, X, Y, Z;
  double  Xnew, Ynew, Znew, AP, BP, CP, FP;
  long  countt, countt_more, countt_less;
  SurfaceSecond NormalPlane ;
  Neutron TempNeutron, TempNeutron1 ;

  /* Some constants */
  diss = 0.01; /* in cm */
  countt_more = 50;
  countt_less = 500;
  TimeCalc = 0.0; /* total time of flight -> returned by the func */

  countt = 1; /* variable for while cycles */

  /*  Make the koefficients of quadratic equation    */
  /*  G = 9.8 m/c**2, we need to cm/ms**2 (100/1000/1000)  */
  /*  Velocity cm/ms, Time ms, Position cm  */

  /*  Calculating real velocity, cm/ms  */


  VelocityReal = (double)(V_FROM_LAMBDA(ThisNeutron->Wavelength));
  if (VelocityReal == 0.0) return(-1.0);

  /*  Components of velocity, projections, and position coordinats */
  /*  Local copy */

  X = ThisNeutron->Position[0];
  Y = ThisNeutron->Position[1];
  Z = ThisNeutron->Position[2];

  VX = VelocityReal*ThisNeutron->Vector[0];
  VY = VelocityReal*ThisNeutron->Vector[1];
  VZ = VelocityReal*ThisNeutron->Vector[2];


  /* First task: find the coordinates of the point, where the current neutron path is crossed with
     surface WITHOUT gravity  */

  /*  Find the coefficients of the quadratic equation */


  AA = ThisSurfaceSecond.A*VX*VX +
       ThisSurfaceSecond.C*VY*VY +
       ThisSurfaceSecond.E*VZ*VZ +
       ThisSurfaceSecond.P*VX*VY +
       ThisSurfaceSecond.Q*VY*VZ +
       ThisSurfaceSecond.R*VX*VZ;


  BB = 2.0*(ThisSurfaceSecond.A*VX*X + ThisSurfaceSecond.C*VY*Y + ThisSurfaceSecond.E*VZ*Z) +
            ThisSurfaceSecond.B*VX + ThisSurfaceSecond.D*VY + ThisSurfaceSecond.F*VZ +
            ThisSurfaceSecond.P*(X*VY+Y*VX) +
            ThisSurfaceSecond.Q*(Y*VZ+Z*VY) +
            ThisSurfaceSecond.R*(X*VZ+Z*VX);


  CC = ThisSurfaceSecond.A*X*X + ThisSurfaceSecond.B*X+
       ThisSurfaceSecond.C*Y*Y + ThisSurfaceSecond.D*Y+
       ThisSurfaceSecond.E*Z*Z + ThisSurfaceSecond.F*Z + ThisSurfaceSecond.W +
       ThisSurfaceSecond.P*X*Y + ThisSurfaceSecond.Q*Y*Z + ThisSurfaceSecond.R*X*Z;


  Time = SolveQuadraticEq(AA,BB,CC);
  if (Time <= 0.0) return(-1.0);


  Xnew = ThisNeutron->Position[0] + VelocityReal*Time*(ThisNeutron->Vector[0]);
  Ynew = ThisNeutron->Position[1] + VelocityReal*Time*(ThisNeutron->Vector[1]);
  Znew = ThisNeutron->Position[2] + VelocityReal*Time*(ThisNeutron->Vector[2]);

  /* Cross Point (Xnew, Ynew, Znew) is found, next step is to find the equation of the plane,
     perpindicular of this point -> calculate the normal   */

  AP = 2.0*ThisSurfaceSecond.A*Xnew
         + ThisSurfaceSecond.B
         + ThisSurfaceSecond.P*Ynew
         + ThisSurfaceSecond.R*Znew;


  BP = 2.0*ThisSurfaceSecond.C*Ynew
         + ThisSurfaceSecond.D
         + ThisSurfaceSecond.P*Xnew
         + ThisSurfaceSecond.Q*Znew;


  CP = 2.0*ThisSurfaceSecond.E*Znew
         + ThisSurfaceSecond.F
         + ThisSurfaceSecond.Q*Ynew
         + ThisSurfaceSecond.R*Xnew;


  /* Normalize normale vector to the reflection plane */
  FP = sqrt(AP*AP + BP*BP + CP*CP);
  if (FP == 0.0) return(-1.0);
  AP = AP/FP;
  BP = BP/FP;
  CP = CP/FP;

  /* Normal vector (AP, BP, CP) is calculated */

  /* Find the plane */
  NormalPlane.A = 0.0;
  NormalPlane.B = AP;
  NormalPlane.C = 0.0;
  NormalPlane.D = BP;
  NormalPlane.E = 0.0;
  NormalPlane.F = CP;
  NormalPlane.W = -1.0*(AP*Xnew + BP*Ynew + CP*Znew);
  NormalPlane.P = 0.0;
  NormalPlane.Q = 0.0;
  NormalPlane.R = 0.0;


  /* Move the neutron into plane WITH GRAVUTY  */

  CopyNeutron(ThisNeutron, &TempNeutron);
  Time1 = NeutronSurfaceSecIntersectionGrav(&TempNeutron, NormalPlane, 1);
  if (Time1 <= 0.0) return(-1.0);
  TimeInit = Time1;
  disscalc = VelocityReal*TimeInit;
  disscalc = 0.01*disscalc;
  if (disscalc >= diss) disscalc = diss;
  if (disscalc <= (0.01*diss)) disscalc = 50.0*disscalc;
  //      fprintf(LogFilePtr,"Disscalc = %f \n", disscalc);


  /*fprintf(LogFilePtr,"------------------------------------------\n");
    fprintf(LogFilePtr,"CROSS POINT: Xnew =  %f  Ynew =  %f  Znew =  %f  Time =  %f \n", Xnew, Ynew, Znew, Time);
    fprintf(LogFilePtr,"NORMAL: AP = %f  BP = %f  CP = %f \n", AP, BP, CP);
    fprintf(LogFilePtr,"PLANE POINT : Xppp =  %f  Yppp =  %f  Zppp =  %f  Time1 = %f \n", TempNeutron.Position[0],
    TempNeutron.Position[1], TempNeutron.Position[2], Time1);*/


  CopyNeutron(&TempNeutron, &TempNeutron1);
  Time2 = NeutronSurfaceSecIntersectionGrav(&TempNeutron1, ThisSurfaceSecond, 0);
  Time22 = Time2;
  //      fprintf(LogFilePtr,"Time2 = %f \n", Time2);


  if (Time22 == 0.0)
  {
    CopyNeutron(&TempNeutron, ThisNeutron);
    TimeCalc = TimeInit ;
    return(TimeCalc);
  }


  if (Time22 > 0.0)
  {
    countt = 1 ;
    TimeInit1 = TimeInit - (disscalc/VelocityReal);
    /* protect TimeInit against negative value */
    while(TimeInit1 <= 0.0)
    {
      if (countt == countt_more) return(-1.0); /* protect agains cycling*/
      TimeInit1 = TimeInit - (disscalc/(((double)(countt))*VelocityReal));
      countt++;
    }
    TimeInit = TimeInit1 ;

    CopyNeutron(ThisNeutron, &TempNeutron1);
    di = NeutronMove(&TempNeutron1, TimeInit);
    if (di < 0.0) return(-1.0);
    Time2 = NeutronSurfaceSecIntersectionGrav(&TempNeutron1, ThisSurfaceSecond, 0);
    if (Time2 <= 0.0) return(-1.0);
    CopyNeutron(&TempNeutron1, ThisNeutron);
    /*  fprintf(LogFilePtr,"FINAL POINT :  Xfffff =  %f   Yffff =  %f  Zffff =  %f  Time2 = %f \n", TempNeutron1.Position[0],
        TempNeutron1.Position[1], TempNeutron1.Position[2], Time22);
        fprintf(LogFilePtr,"==================================================\n"); */
    TimeCalc = TimeInit + Time2 ;
    return(TimeCalc);
  }


  if (Time22 < 0.0)
  {
    countt = 1 ;
    while(Time2 < 0.0)
    {
      //            fprintf(LogFilePtr,"Moving back: Cycle \n");
      /* decrease a time a litte bit step by step until time2 > 0.0 */
      TimeInit = TimeInit - (disscalc/VelocityReal);
      if ((TimeInit <= 0.0)&&(Time2 < 0.0)) return(-1.0);/* illegal time */
      if (countt == countt_less) return(-1.0); /* protect agains cycling*/
      CopyNeutron(ThisNeutron, &TempNeutron1);
      di = NeutronMove(&TempNeutron1, TimeInit);
      if (di < 0.0) return(-1.0);
      Time2 = NeutronSurfaceSecIntersectionGrav(&TempNeutron1, ThisSurfaceSecond, 0);
      countt++;
    }
    if (Time2 <= 0.0) return(-1.0);
    CopyNeutron(&TempNeutron1, ThisNeutron);
    /*              fprintf(LogFilePtr,"FINAL POINT :  Xfffff =  %f   Yffff =  %f  Zffff =  %f  Time2 = %f \n", TempNeutron1.Position[0],
        TempNeutron1.Position[1], TempNeutron1.Position[2], Time2);
        fprintf(LogFilePtr,"==================================================\n"); */
    TimeCalc = TimeInit + Time2 ;
    return(TimeCalc);
  }

  /* bad situation */
  return(-1.0);
}
