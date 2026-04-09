/***********************FUNCTION*FOR*MODULE*BENDER********************************************************************************/
/*********************************************************************************************************************************/
/************FUNCTION*FOR*MOVING*NEUTRON*INSIDE*CHANNEL*OF*BENDER*****************************************************************/
/*********************************************************************************************************************************/

#ifdef VT_GRAPH
# include "cpgplot.h"
  extern int do_visualise; /* default : no visualisation */
#endif

#include "intersection.h"
#include "init.h"
#include "bender.h"
#include "bender_inter_data.h"


void gsl_ran_dir_3d (const gsl_rng * r, double * x, double * y, double * z);



double PathThroughChannelGravOrder2(Neutron *ThisNeutron,   Bender  MyBender,      BenderChannel ThisBenderChnl,
                                    long     ChnlNumber,    double  disabut,
                                    double  *ReflUpL,       double *ReflUpR,       double *ReflUpTB,
                                    double  *ReflDownL,     double *ReflDownR,     double *ReflDownTB,
                                    double   surfacerough,  long    keypol,        long   qspin,
                                    double  *entrdiscenter, double *exitdiscenter, double spacer,
                                    VtWndAbs ChnlMaterial,
                                    double  *aLambdaChnl,   double *aMuChnl,       long   nMuValuesChnl)
{
  /************************************************************************************/
  /* This routine calculates the trajectory a neutron follows through a simple        */
  /* neutron bender. It accepts two structured variables; a pointer to a neutron      */
  /* structure and a simple Bender structure. This latter consists simply of four     */
  /* infinite planes describing the two walls floor and ceiling of the bender and a   */
  /* fifth infinite plane at the exit of the bender. The structure has an assosciated */
  /* critical angle; any neutron that intercepts a wall at an angle greater than this */
  /* is absorbed.                                                                     */
  /* Neutron flight by parabolic trajectories with GRAVITY                            */
  /* Significant re-written by Manoshin Sergey in Feb 2001                            */
  /* Note! The function is return Time Of Flight                                      */
  /************************************************************************************/

  int i, PreviousCollision, ThisCollision=4, datanumber;
  long R1;
  double angular,degangular;
  double TimeOF, TimeOF1, TimeOFmin;
  double TimeOFTotal=0.0;
  double AP, BP, CP, DOTP, FP, VelocityReal;
  double VX, VY, VZ;
  double signl, signr, signt, signb;
  double mu=0.0;    // absorption coefficient in the channel for the given neutron wavelength 

  /* KL: new variable */
  double stepp;
  /* Local copy of neutrons for cylce.. */
  Neutron TempNeutron, TempNeutron1, NearestNeutron;


#ifdef VT_GRAPH
  if (do_visualise)
  {
    double tempx, tempy;

    /* visual path begin */
    cpgsci(6);
    tempx = ThisNeutron->Position[0];
    tempy = ThisNeutron->Position[1];
    cpgmove(tempx,tempy);
    /* visual path end */
  }
#endif


  /* COPY THE CURRENT CHANNEL, CHOOSED */
  /* ChnlNumber - number of current(entrance) channel of bender */
  /* left  plane */

  ThisBenderChnl.Surf[2].A = MyBender.SurfLeft[ChnlNumber].A;
  ThisBenderChnl.Surf[2].B = MyBender.SurfLeft[ChnlNumber].B;
  ThisBenderChnl.Surf[2].C = MyBender.SurfLeft[ChnlNumber].C;
  ThisBenderChnl.Surf[2].D = MyBender.SurfLeft[ChnlNumber].D;
  ThisBenderChnl.Surf[2].E = MyBender.SurfLeft[ChnlNumber].E;
  ThisBenderChnl.Surf[2].F = MyBender.SurfLeft[ChnlNumber].F;
  ThisBenderChnl.Surf[2].W = MyBender.SurfLeft[ChnlNumber].W;
  ThisBenderChnl.Surf[2].P = MyBender.SurfLeft[ChnlNumber].P;
  ThisBenderChnl.Surf[2].Q = MyBender.SurfLeft[ChnlNumber].Q;
  ThisBenderChnl.Surf[2].R = MyBender.SurfLeft[ChnlNumber].R;


  /* right plane */

  ThisBenderChnl.Surf[3].A = MyBender.SurfRight[ChnlNumber].A;
  ThisBenderChnl.Surf[3].B = MyBender.SurfRight[ChnlNumber].B;
  ThisBenderChnl.Surf[3].C = MyBender.SurfRight[ChnlNumber].C;
  ThisBenderChnl.Surf[3].D = MyBender.SurfRight[ChnlNumber].D;
  ThisBenderChnl.Surf[3].E = MyBender.SurfRight[ChnlNumber].E;
  ThisBenderChnl.Surf[3].F = MyBender.SurfRight[ChnlNumber].F;
  ThisBenderChnl.Surf[3].W = MyBender.SurfRight[ChnlNumber].W;
  ThisBenderChnl.Surf[3].P = MyBender.SurfRight[ChnlNumber].P;
  ThisBenderChnl.Surf[3].Q = MyBender.SurfRight[ChnlNumber].Q;
  ThisBenderChnl.Surf[3].R = MyBender.SurfRight[ChnlNumber].R;


  /* exit plane */

  ThisBenderChnl.Surf[4].A = MyBender.SurfExit[ChnlNumber].A;
  ThisBenderChnl.Surf[4].B = MyBender.SurfExit[ChnlNumber].B;
  ThisBenderChnl.Surf[4].C = MyBender.SurfExit[ChnlNumber].C;
  ThisBenderChnl.Surf[4].D = MyBender.SurfExit[ChnlNumber].D;
  ThisBenderChnl.Surf[4].E = MyBender.SurfExit[ChnlNumber].E;
  ThisBenderChnl.Surf[4].F = MyBender.SurfExit[ChnlNumber].F;
  ThisBenderChnl.Surf[4].W = MyBender.SurfExit[ChnlNumber].W;
  ThisBenderChnl.Surf[4].P = MyBender.SurfExit[ChnlNumber].P;
  ThisBenderChnl.Surf[4].Q = MyBender.SurfExit[ChnlNumber].Q;
  ThisBenderChnl.Surf[4].R = MyBender.SurfExit[ChnlNumber].R;


  /*   Define the stepp for given channel  */

  /* KL: correction fabs(x - y) instead of fabs(fabs(x) - fabs(y))
  and calculation in one equation instead of three
  KL: size of 'stepp' decreased;
  possible ERROR: result depends on size of 'stepp' (see example BEND_DIR)
  reason unknown                                                          */

  stepp = 0.01*(fabs(exitdiscenter[ChnlNumber+1] - exitdiscenter[ChnlNumber]) +
                fabs(entrdiscenter[ChnlNumber+1] - entrdiscenter[ChnlNumber]) - spacer);

  /* Convert stepp in time ms */

  VelocityReal = (double)(V_FROM_LAMBDA(ThisNeutron->Wavelength));

  if (VelocityReal != 0.0)
  {
    stepp = stepp/VelocityReal;
  }
  else
  {
    fprintf(LogFilePtr,"WARNING! Neutron Velocity is ZERO!!!, Wavelength is INFINITY! \n");
    return(-1);
  }



  /***********************************************************************************/
  /* The main loop here is continuous: the neutron will continue to bounce around,   */
  /* until it is absorbed or intercepts with the exit plane.                         */
  /***********************************************************************************/

  /*  fprintf(LogFilePtr,"weight min  %e \n",wei_min);*/

  R1 = 1;
  PreviousCollision = 6;

  while(GUIDEFLIGHT)
  {
    TimeOFmin = 99999999999999999999999.9;

    /***********************************************************************************/
    /* Loop through all five planes....                                                */
    /***********************************************************************************/
    for(i=0;i<5;i++)
    {
      /***********************************************************************************/
      /* If the plane currently indexed is the one that the neutron has just hit, return */
      /* contol to the top of the loop and look at the next plane.                       */
      /***********************************************************************************/
      //      if(i==PreviousCollision) continue;

      /***********************************************************************************/
      /* Find the point where this neutron trajectory intercepts the currently indexed   */
      /* plane  WITH GRAVITY                                       */
      /***********************************************************************************/

      /* Save current neutron, becouse function NeutronPlaneIntersectionGrav have
         modify Neutron data structure  */

      CopyNeutron(ThisNeutron, &TempNeutron);

      TimeOF=NeutronSurfaceSecIntersectionGr(&TempNeutron, ThisBenderChnl.Surf[i], keygrav);

      /*  Intercept = NeutronPlaneIntersection(*ThisNeutron, ThisGuide.Wall[i]); */
      //        fprintf(LogFilePtr,"num = %d time = %e \n",i, TimeOF);
      /***********************************************************************************/
      /* If this intercept point is behind the neutrons current position, pass control to*/
      /* the top of the loop: OR Time of flight <= 0.0, Fixed Manoshin Sergey 19.02.00   */
      /***********************************************************************************/

      /* ILLEGAL NEUTRON PARAMETERS */

      if (TempNeutron.Position[0] < ThisNeutron->Position[0]) continue;
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
    }

    /***********************************************************************************/
    /* Having looped through all five planes, the current values of NearestNeutron,    */
    /* TimeOFmin and ThisCollision, reflect the coordinates, distance and index        */
    /* of the neutrons interaction with a bender wall. If this bender wall is index 4  */
    /*(i.e. the exit window), reset the neutron coordinates to this point, add the path */
    /* length to this point to the running total and return that total.                */
    /***********************************************************************************/

    /* Neutrons going in the exit of the bender channel */

    if(ThisCollision == 4)
    {
      double prob=1.0; // transmission through the channel

      /* Illegal velocity */
      if(NearestNeutron.Vector[0]<0.0)  return(-1.0);

      /*  This feature rejects neutrons, which are reflected near the edges (exit) of the bender */
      if (disabut > 0.0)
      {
        VelocityReal = (double)(V_FROM_LAMBDA(NearestNeutron.Wavelength));
        if (TimeOFmin*VelocityReal <= disabut)  return(-1.0);
      }

      if (NearestNeutron.Probability <= wei_min)  return(-1.0);

      /* copy the parameters of the neutron with the closest reflection to those of the current neutron */
      ThisNeutron->Position[0] = NearestNeutron.Position[0];
      ThisNeutron->Position[1] = NearestNeutron.Position[1];
      ThisNeutron->Position[2] = NearestNeutron.Position[2];
      ThisNeutron->Vector[2]   = NearestNeutron.Vector[2];
      ThisNeutron->Probability = NearestNeutron.Probability;
      ThisNeutron->Wavelength  = NearestNeutron.Wavelength;

      TimeOFTotal = TimeOFTotal + TimeOFmin;
      VelocityReal = V_FROM_LAMBDA(ThisNeutron->Wavelength);

      /* Attenuation during passage through channel */
      if (ChnlMaterial!=VT_WABS_VAC)
      {
        mu = Interpol(ThisNeutron->Wavelength, aLambdaChnl, aMuChnl, nMuValuesChnl);
        if (mu == -10000.0)
          return(-10000.0);
        prob = exp(-mu * TimeOFTotal * VelocityReal);
        ThisNeutron->Probability *= prob;
      }

      WriteIAP(ThisNeutron, VT_EXITED);

  #ifdef VT_GRAPH
      if (do_visualise)
      {
        double tempx, tempy;
        /* visual path begin */
        tempx = ThisNeutron->Position[0];
        tempy = ThisNeutron->Position[1];
        cpgdraw(tempx,tempy);
        /* visual path end */
      }
  #endif

      return TimeOFTotal;
    }

    /***********************************************************************************/
    /* If the angle of intersection of the flight path and the bender wall exceed the   */
    /* critical angle of the bender, the neutron is absorbed.                           */
    /***********************************************************************************/

    /* normal vector in surface which will be reflected */
    AP = 2.0*ThisBenderChnl.Surf[ThisCollision].A*NearestNeutron.Position[0]
           + ThisBenderChnl.Surf[ThisCollision].B
           + ThisBenderChnl.Surf[ThisCollision].P*NearestNeutron.Position[1]
           + ThisBenderChnl.Surf[ThisCollision].R*NearestNeutron.Position[2];
    BP = 2.0*ThisBenderChnl.Surf[ThisCollision].C*NearestNeutron.Position[1]
           + ThisBenderChnl.Surf[ThisCollision].D
           + ThisBenderChnl.Surf[ThisCollision].P*NearestNeutron.Position[0]
           + ThisBenderChnl.Surf[ThisCollision].Q*NearestNeutron.Position[2];
    CP = 2.0*ThisBenderChnl.Surf[ThisCollision].E*NearestNeutron.Position[2]
           + ThisBenderChnl.Surf[ThisCollision].F
           + ThisBenderChnl.Surf[ThisCollision].Q*NearestNeutron.Position[1]
           + ThisBenderChnl.Surf[ThisCollision].R*NearestNeutron.Position[0];

    /* Normalize normale vector to the reflection plane */
    FP = sqrt(AP*AP + BP*BP + CP*CP);
    if (FP == 0.0) return(-1.0);
    AP = AP/FP;
    BP = BP/FP;
    CP = CP/FP;

    /* influence of rough surface */
    if (surfacerough != 0.0)
    {
      // len = vector3rand(&VX, &VY, &VZ);
      gsl_ran_dir_3d( vit_gsl_rng, &VX, &VY, &VZ);

      /*fprintf(LogFilePtr,"vx vy vz %f  %f  %f  %f  %f \n",VX,VY,VZ,len,surfacerough);*/
      AP = AP + surfacerough*VX;
      BP = BP + surfacerough*VY;
      CP = CP + surfacerough*VZ;

      /* Renormalize normale vector to the reflection plane */
      FP = sqrt(AP*AP + BP*BP + CP*CP);
      if (FP == 0.0) return(-1.0);
      AP = AP/FP;
      BP = BP/FP;
      CP = CP/FP;
    }

    angular=fabs(NeutronPlaneAngle2(&NearestNeutron, AP, BP, CP));

    /* Convert from radian to degree */
    degangular = angular*360.0/(2.0*M_PI);
    datanumber =  (int)(degangular*1000.0/(NearestNeutron.Wavelength));
    if (datanumber > 999) return(-1.0);

    /*Choose the reflectivity file */
    switch (ThisCollision)
    {
      case 0: /* top plane */
        if (keypol == 1)
        {
          if(NearestNeutron.Spin[qspin] == 1.0)
            NearestNeutron.Probability *= ReflUpTB[datanumber];
          if(NearestNeutron.Spin[qspin] == -1.0)
            NearestNeutron.Probability *= ReflDownTB[datanumber];
        }
        else
        {
          NearestNeutron.Probability *= ReflUpTB[datanumber];
        }
        break;

      case 1: /* bottom plane */
        if (keypol == 1)
        {
          if(NearestNeutron.Spin[qspin] == 1.0)
            NearestNeutron.Probability *= ReflUpTB[datanumber];
          if(NearestNeutron.Spin[qspin] == -1.0)
            NearestNeutron.Probability *= ReflDownTB[datanumber];
        }
        else
        {
          NearestNeutron.Probability *= ReflUpTB[datanumber];
        }
        break;

      case 2: /* right plane */
        if (keypol == 1)
        {
          if(NearestNeutron.Spin[qspin] == 1.0)
            NearestNeutron.Probability *= ReflUpL[datanumber];
          if(NearestNeutron.Spin[qspin] == -1.0)
            NearestNeutron.Probability *= ReflDownL[datanumber];
        }
        else
        {
          NearestNeutron.Probability *= ReflUpL[datanumber];
        }
        break;

      case 3:  /* left plane */
        if (keypol == 1)
        {
          if(NearestNeutron.Spin[qspin] == 1.0)
            NearestNeutron.Probability *= ReflUpR[datanumber];
          if(NearestNeutron.Spin[qspin] == -1.0)
            NearestNeutron.Probability *= ReflDownR[datanumber];
        }
        else
        {
          NearestNeutron.Probability *= ReflUpR[datanumber];
        }
        break;

      default:
        Error("No such plane!");
    }

     /* Reject neutrons with small probability */
     if (NearestNeutron.Probability <= wei_min)  return(-1.0);

    /***********************************************************************************/
    /*  Calculate the trajectory of the reflected neutron.                             */
    /***********************************************************************************/

    /* Make reflection */
    DOTP = AP*NearestNeutron.Vector[0] + BP*NearestNeutron.Vector[1] + CP*NearestNeutron.Vector[2];

    ThisNeutron->Vector[0] = NearestNeutron.Vector[0] - 2.0 * DOTP * AP;
    ThisNeutron->Vector[1] = NearestNeutron.Vector[1] - 2.0 * DOTP * BP;
    ThisNeutron->Vector[2] = NearestNeutron.Vector[2] - 2.0 * DOTP * CP;

    //  fprintf(LogFilePtr,"Make reflection\n");

    /***********************************************************************************/
    /* Reset the neutron coordinates to coordinates of the collision, add the path     */
    /* length to this point to the running total, keep track of which plane has just   */
    /* been hit and then return to the top of the top of the loop and find the next    */
    /* collision                                                                       */
    /***********************************************************************************/
    ThisNeutron->Position[0] = NearestNeutron.Position[0];
    ThisNeutron->Position[1] = NearestNeutron.Position[1];
    ThisNeutron->Position[2] = NearestNeutron.Position[2];
    ThisNeutron->Wavelength  = NearestNeutron.Wavelength;
    ThisNeutron->Probability = NearestNeutron.Probability;

    TimeOFTotal =  TimeOFTotal + TimeOFmin ;
    PreviousCollision = ThisCollision;

    WriteIAP(ThisNeutron, VT_REFLECTED);

#ifdef VT_GRAPH
    if (do_visualise)
    {
      double tempx, tempy;
      /* visual path begin */
      tempx = ThisNeutron->Position[0];
      tempy = ThisNeutron->Position[1];
      cpgdraw(tempx,tempy);
      cpgsci(7);
    }
#endif

    /*  angular1=fabs(NeutronPlaneAngle2(ThisNeutron, AP, BP, CP));

    fprintf(LogFilePtr,"angle inc = %f, refl = %f \n",angular,angular1); */

    CopyNeutron(ThisNeutron, &TempNeutron1);

    TimeOF1 = NeutronSurfaceSecIntersectionGr(&TempNeutron1, ThisBenderChnl.Surf[4], keygrav);

     /* CHECK incorrect flight of neutron, move neutron after reflection on small distance
     and check inside channel or no , no - exit(-1) */

    if (TimeOF1 > stepp)
    {
      VelocityReal = (double)(V_FROM_LAMBDA(ThisNeutron->Wavelength));
      ThisNeutron->Position[0] = ThisNeutron->Position[0] + VelocityReal*stepp*(ThisNeutron->Vector[0]);
      ThisNeutron->Position[1] = ThisNeutron->Position[1] + VelocityReal*stepp*(ThisNeutron->Vector[1]);
      ThisNeutron->Position[2] = ThisNeutron->Position[2] + VelocityReal*stepp*(ThisNeutron->Vector[2]);

      /* Include gravity */
      if (keygrav == 1)
      {
        ThisNeutron->Position[2] = ThisNeutron->Position[2] - 0.5*(G*1.0e-4)*stepp*stepp;
        ThisNeutron->Vector[2]   = ThisNeutron->Vector[2]   -    ((G*1.0e-4)*stepp/VelocityReal);
      }
      TimeOFTotal = TimeOFTotal + stepp;

      /* check the incorrect path of neutron */
      signl =
        ThisBenderChnl.Surf[2].A*ThisNeutron->Position[0]*ThisNeutron->Position[0] +
        ThisBenderChnl.Surf[2].B*ThisNeutron->Position[0] +
        ThisBenderChnl.Surf[2].C*ThisNeutron->Position[1]*ThisNeutron->Position[1]+
        ThisBenderChnl.Surf[2].D*ThisNeutron->Position[1] +
        ThisBenderChnl.Surf[2].E*ThisNeutron->Position[2]*ThisNeutron->Position[2]+
        ThisBenderChnl.Surf[2].F*ThisNeutron->Position[2] +
        ThisBenderChnl.Surf[2].W +
        ThisBenderChnl.Surf[2].P*ThisNeutron->Position[0]*ThisNeutron->Position[1]+
        ThisBenderChnl.Surf[2].Q*ThisNeutron->Position[1]*ThisNeutron->Position[2]+
        ThisBenderChnl.Surf[2].R*ThisNeutron->Position[2]*ThisNeutron->Position[0] ;

      signr =
        ThisBenderChnl.Surf[3].A*ThisNeutron->Position[0]*ThisNeutron->Position[0]+
        ThisBenderChnl.Surf[3].B*ThisNeutron->Position[0] +
        ThisBenderChnl.Surf[3].C*ThisNeutron->Position[1]*ThisNeutron->Position[1]+
        ThisBenderChnl.Surf[3].D*ThisNeutron->Position[1] +
        ThisBenderChnl.Surf[3].E*ThisNeutron->Position[2]*ThisNeutron->Position[2]+
        ThisBenderChnl.Surf[3].F*ThisNeutron->Position[2] +
        ThisBenderChnl.Surf[3].W +
        ThisBenderChnl.Surf[3].P*ThisNeutron->Position[0]*ThisNeutron->Position[1]+
        ThisBenderChnl.Surf[3].Q*ThisNeutron->Position[1]*ThisNeutron->Position[2]+
        ThisBenderChnl.Surf[3].R*ThisNeutron->Position[2]*ThisNeutron->Position[0] ;

      signt =
        ThisBenderChnl.Surf[0].A*ThisNeutron->Position[0]*ThisNeutron->Position[0] +
        ThisBenderChnl.Surf[0].B*ThisNeutron->Position[0] +
        ThisBenderChnl.Surf[0].C*ThisNeutron->Position[1]*ThisNeutron->Position[1]+
        ThisBenderChnl.Surf[0].D*ThisNeutron->Position[1] +
        ThisBenderChnl.Surf[0].E*ThisNeutron->Position[2]*ThisNeutron->Position[2]+
        ThisBenderChnl.Surf[0].F*ThisNeutron->Position[2] +
        ThisBenderChnl.Surf[0].W +
        ThisBenderChnl.Surf[0].P*ThisNeutron->Position[0]*ThisNeutron->Position[1]+
        ThisBenderChnl.Surf[0].Q*ThisNeutron->Position[1]*ThisNeutron->Position[2]+
        ThisBenderChnl.Surf[0].R*ThisNeutron->Position[2]*ThisNeutron->Position[0] ;

      signb =
        ThisBenderChnl.Surf[1].A*ThisNeutron->Position[0]*ThisNeutron->Position[0] +
        ThisBenderChnl.Surf[1].B*ThisNeutron->Position[0] +
        ThisBenderChnl.Surf[1].C*ThisNeutron->Position[1]*ThisNeutron->Position[1]+
        ThisBenderChnl.Surf[1].D*ThisNeutron->Position[1] +
        ThisBenderChnl.Surf[1].E*ThisNeutron->Position[2]*ThisNeutron->Position[2]+
        ThisBenderChnl.Surf[1].F*ThisNeutron->Position[2] +
        ThisBenderChnl.Surf[1].W +
        ThisBenderChnl.Surf[1].P*ThisNeutron->Position[0]*ThisNeutron->Position[1]+
        ThisBenderChnl.Surf[1].Q*ThisNeutron->Position[1]*ThisNeutron->Position[2]+
        ThisBenderChnl.Surf[1].R*ThisNeutron->Position[2]*ThisNeutron->Position[0] ;

#ifdef VT_GRAPH
    if (do_visualise)
    {
      double tempx, tempy;
      /* visual path begin */
      tempx = ThisNeutron->Position[0];
      tempy = ThisNeutron->Position[1];
      cpgdraw(tempx,tempy);
      cpgsci(7);
    }
#endif

      /* particle outside channel */
      /* KL: Warning: neutron must move from the surface, otherwise sign is a very low value,
      that can be above or below zero. Therefore, comparison: > 1e-x may be better than > 0.0 */

      if ((signl*signr) > 0.0)
        return(-1.0);

      if ((signt*signb) > 0.0)
        return(-1.0);
     }
  }
}
