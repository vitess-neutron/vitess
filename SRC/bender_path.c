/***********************FUNCTION*FOR*MODULE*BENDER********************************************************************************/
/*********************************************************************************************************************************/
/***************FUNCTION*FOR*MOVING*NEUTRON*INSIDE*OF*BENDER**********************************************************************/
/*********************************************************************************************************************************/
/* Last change 30 Sep 2003 */

#ifdef VT_GRAPH
# include "cpgplot.h"
  extern int do_visualise; /* default : no visualisation */
#endif

#include "intersection.h"
#include "init.h"
#include "message.h"
#include "bender_inter_data.h"
#include "bender.h"

void gsl_ran_dir_3d (const gsl_rng * r, double * x, double * y, double * z);


/************************************************************************************************************************/
/* PathThroughChannelGravOrder2                                                                                         */
/* PathThroughBenderGravOrder2                                                                                          */
/*   written by Manoshin Sergey in Feb 2001                                                                             */
/* These routines calculate the trajectory of a neutron through the bender.                                             */
/* The former one, (...Channel...) removes all neutrons that are not reflected at the channel walls.                    */
/* The latter one, (...Bender...) includes a trajectory continuation in neighboring channels if it is not reflected     */
/* The trajectory is calculated with GRAVITY for straight channels                                                      */
/* Input:                                                                                                               */
/*   The routines accept a number of variable input parameters including three structures:                              */
/*   a pointer to a 'Neutron' structure                                                                                 */
/*   a 'Bender' structure describing the N bender channels by four infinite planes each (top, bottom, left, right)      */
/*   a 'BenderChannel' structure describing the current channel by five infinite planes (top, bottom, left, right, exit)*/
/* Return:                                                                                                              */
/*   TOF through channel/bender  (negative value: didn't work for this neutron                                          */
/************************************************************************************************************************/
double PathThroughChannelGravOrder2(Neutron *ThisNeutron,   Bender  MyBender,      BenderChannel ThisBenderChnl,
                                    long     ChnlNumber,    double  disabut,
                                    double  *ReflUpL,       double *ReflUpR,       double *ReflUpTB,
                                    double  *ReflDownL,     double *ReflDownR,     double *ReflDownTB,
                                    double   surfacerough,  long    keypol,        long   qspin,
                                    double  *entrdiscenter, double *exitdiscenter, double spacer,
                                    VtWndMat eChnlMat,      double *aLambdaChnl,   double *aMuChnl,   long nMuValuesChnl)
{

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
    CountMessageID(ALL_ZERO_VELOCITY, ThisNeutron->ID);
    return(-1);
  }



  /***********************************************************************************/
  /* The main loop here is continuous: the neutron will continue to bounce around,   */
  /* until it is absorbed or intercepts with the exit plane.                         */
  /***********************************************************************************/

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

    /* Neutrons reaching the exit of the bender channel */

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
      if (eChnlMat!=VT_WND_VAC)
      {
        mu = Interpol(ThisNeutron->Wavelength, aLambdaChnl, aMuChnl, nMuValuesChnl);
        if (mu == -10000.0)
        { CountMessageID(WNDO_L_RANGE_TOO_SMALL, ThisNeutron->ID);
          prob = 1.0;     // ideal transmission assumed inside channel for wavelenghts out of given range
        }
        else if (mu == 10000.0)
        { prob = 0.0;
        }
        else
        { prob = exp(-mu * TimeOFTotal * VelocityReal);
        }
        
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

    /*  angular1=fabs(NeutronPlaneAngle2(ThisNeutron, AP, BP, CP)); */

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


double  PathThroughBenderGravOrder2(Neutron *ThisNeutron,   Bender  MyBender,      BenderChannel ThisBenderCh,
                                    long     ChnlNumber,    long    nSurfaces,     double  disabut,
                                    double  *ReflUpL,       double *ReflUpR,       double *ReflUpTB,
                                    double  *ReflDownL,     double *ReflDownR,     double *ReflDownTB,
                                    double  surfacerough,   long    keypol,        long    qspin,
                                    double  *entrdiscenter, double *exitdiscenter, double  spacer,
                                    VtWndMat eChnlMat,      double *aLambdaChnl,   double *aMuChnl,    long nMuValuesChnl,
                                    VtWndMat eAbsMatL,      double *aLambdaL,      double *aMuL,       long nMuValuesL,
                                    VtWndMat eAbsMatR,      double *aLambdaR,      double *aMuR,       long nMuValuesR)
{

  int    i, PreviousCollision, ThisCollision=4, datanumber, ThisColl;
  long   R1; /* for random generate chance */
  long   ext_left, ext_right; /* extreme numbers of left and right channels */
  double angular,degangular, chance;
  double TimeOF, TimeOF1, TimeOFmin, TimeOFpass=0.0, TimeOFtop, TimeOFbot, TimeOFm[4], TimeOFmi;
  double TimeOFTotal=0.0;
  double AP, BP, CP, DOTP, FP, VelocityReal;
  double VX, VY, VZ;
  double signl, signr, signt, signb;
  double ReflectionProb=0.0; /* variable for current reflection probability, 0 - mean transmission */
  /* KL: new variable */
  double stepp;
  double N_Wavelength, prob, mu;
  /* Local copy of neutrons for cylce.. */
  Neutron TempNeutron, TempNeutron1, NearestNeutron;
  /* For pass in to new channel */
  SurfaceSecond NewSurf, OldSurf;
  long keytemp=0;

  InitSurface(&OldSurf);
  InitSurface(&NewSurf);

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

  /* Extreme left and right surfaces: neutron,
  which pass via extreme surface ---> ABSORB */

  ext_left = nSurfaces - 1;
  ext_right = 1;

  /* COPY THE CURRENT CHANNEL */
  /* ChnlNumber - number of current(entrance) channel of bender */
  /* left  plane */
  ThisBenderCh.Surf[2].A = MyBender.SurfLeft[ChnlNumber].A;
  ThisBenderCh.Surf[2].B = MyBender.SurfLeft[ChnlNumber].B;
  ThisBenderCh.Surf[2].C = MyBender.SurfLeft[ChnlNumber].C;
  ThisBenderCh.Surf[2].D = MyBender.SurfLeft[ChnlNumber].D;
  ThisBenderCh.Surf[2].E = MyBender.SurfLeft[ChnlNumber].E;
  ThisBenderCh.Surf[2].F = MyBender.SurfLeft[ChnlNumber].F;
  ThisBenderCh.Surf[2].W = MyBender.SurfLeft[ChnlNumber].W;
  ThisBenderCh.Surf[2].P = MyBender.SurfLeft[ChnlNumber].P;
  ThisBenderCh.Surf[2].Q = MyBender.SurfLeft[ChnlNumber].Q;
  ThisBenderCh.Surf[2].R = MyBender.SurfLeft[ChnlNumber].R;

  /* right plane */
  ThisBenderCh.Surf[3].A = MyBender.SurfRight[ChnlNumber].A;
  ThisBenderCh.Surf[3].B = MyBender.SurfRight[ChnlNumber].B;
  ThisBenderCh.Surf[3].C = MyBender.SurfRight[ChnlNumber].C;
  ThisBenderCh.Surf[3].D = MyBender.SurfRight[ChnlNumber].D;
  ThisBenderCh.Surf[3].E = MyBender.SurfRight[ChnlNumber].E;
  ThisBenderCh.Surf[3].F = MyBender.SurfRight[ChnlNumber].F;
  ThisBenderCh.Surf[3].W = MyBender.SurfRight[ChnlNumber].W;
  ThisBenderCh.Surf[3].P = MyBender.SurfRight[ChnlNumber].P;
  ThisBenderCh.Surf[3].Q = MyBender.SurfRight[ChnlNumber].Q;
  ThisBenderCh.Surf[3].R = MyBender.SurfRight[ChnlNumber].R;

  /* exit plane */
  ThisBenderCh.Surf[4].A = MyBender.SurfExit[ChnlNumber].A;
  ThisBenderCh.Surf[4].B = MyBender.SurfExit[ChnlNumber].B;
  ThisBenderCh.Surf[4].C = MyBender.SurfExit[ChnlNumber].C;
  ThisBenderCh.Surf[4].D = MyBender.SurfExit[ChnlNumber].D;
  ThisBenderCh.Surf[4].E = MyBender.SurfExit[ChnlNumber].E;
  ThisBenderCh.Surf[4].F = MyBender.SurfExit[ChnlNumber].F;
  ThisBenderCh.Surf[4].W = MyBender.SurfExit[ChnlNumber].W;
  ThisBenderCh.Surf[4].P = MyBender.SurfExit[ChnlNumber].P;
  ThisBenderCh.Surf[4].Q = MyBender.SurfExit[ChnlNumber].Q;
  ThisBenderCh.Surf[4].R = MyBender.SurfExit[ChnlNumber].R;

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
    CountMessageID(ALL_ZERO_VELOCITY, ThisNeutron->ID);
    return(-1);
  }


  /***********************************************************************************/
  /* The main loop here is continuous: the neutron will continue to bounce around,   */
  /* until it is absorbed or intercepts with the exit plane.                         */
  /***********************************************************************************/

  R1 = 1;
  PreviousCollision = 6;
  keytemp = 0;

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

      /*Save current neutron, becouse function NeutronPlaneIntersectionGrav have
        modify Neutron data structure  */

      CopyNeutron(ThisNeutron, &TempNeutron);

      TimeOF=NeutronSurfaceSecIntersectionGr(&TempNeutron, ThisBenderCh.Surf[i], keygrav);

      /*  Intercept = NeutronPlaneIntersection(*ThisNeutron, ThisGuide.Wall[i]); */
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
    /* of the neutrons interaction with a bender wall. If this bender wall is index 4    */
    /*(ie: the exit  window) reset the neutron coordinates to this point, add the path  */
    /* length to this point to the running total and return that total.                */
    /***********************************************************************************/


 /* Neutrons reaching the exit of the bender channel */

    if(ThisCollision == 4)
    {

  /* Illegal velocity */
      if(NearestNeutron.Vector[0]<0.0)  return(-1.0);

      /*  This feature is reject neutrons, which make reflection near edges (exit) of bender */
      if (disabut > 0.0)
      {
        VelocityReal = (double)(V_FROM_LAMBDA(NearestNeutron.Wavelength));
        if (TimeOFmin*VelocityReal <= disabut)  return(-1.0);
      }

      if (NearestNeutron.Probability <= wei_min)  return(-1.0);


      ThisNeutron->Position[0] = NearestNeutron.Position[0];
      ThisNeutron->Position[1] = NearestNeutron.Position[1];
      ThisNeutron->Position[2] = NearestNeutron.Position[2];
      ThisNeutron->Vector[2] = NearestNeutron.Vector[2];
      ThisNeutron->Probability = NearestNeutron.Probability;
      ThisNeutron->Wavelength = NearestNeutron.Wavelength;

      /* check the incorrect path of neutron */

      signl =
        ThisBenderCh.Surf[2].A*ThisNeutron->Position[0]*ThisNeutron->Position[0] +
        ThisBenderCh.Surf[2].B*ThisNeutron->Position[0] +
        ThisBenderCh.Surf[2].C*ThisNeutron->Position[1]*ThisNeutron->Position[1]+
        ThisBenderCh.Surf[2].D*ThisNeutron->Position[1] +
        ThisBenderCh.Surf[2].E*ThisNeutron->Position[2]*ThisNeutron->Position[2]+
        ThisBenderCh.Surf[2].F*ThisNeutron->Position[2] +
        ThisBenderCh.Surf[2].W +
        ThisBenderCh.Surf[2].P*ThisNeutron->Position[0]*ThisNeutron->Position[1]+
        ThisBenderCh.Surf[2].Q*ThisNeutron->Position[1]*ThisNeutron->Position[2]+
        ThisBenderCh.Surf[2].R*ThisNeutron->Position[2]*ThisNeutron->Position[0] ;

      signr =
        ThisBenderCh.Surf[3].A*ThisNeutron->Position[0]*ThisNeutron->Position[0]+
        ThisBenderCh.Surf[3].B*ThisNeutron->Position[0] +
        ThisBenderCh.Surf[3].C*ThisNeutron->Position[1]*ThisNeutron->Position[1]+
        ThisBenderCh.Surf[3].D*ThisNeutron->Position[1] +
        ThisBenderCh.Surf[3].E*ThisNeutron->Position[2]*ThisNeutron->Position[2]+
        ThisBenderCh.Surf[3].F*ThisNeutron->Position[2] +
        ThisBenderCh.Surf[3].W +
        ThisBenderCh.Surf[3].P*ThisNeutron->Position[0]*ThisNeutron->Position[1]+
        ThisBenderCh.Surf[3].Q*ThisNeutron->Position[1]*ThisNeutron->Position[2]+
        ThisBenderCh.Surf[3].R*ThisNeutron->Position[2]*ThisNeutron->Position[0] ;

      signt =
        ThisBenderCh.Surf[0].A*ThisNeutron->Position[0]*ThisNeutron->Position[0] +
        ThisBenderCh.Surf[0].B*ThisNeutron->Position[0] +
        ThisBenderCh.Surf[0].C*ThisNeutron->Position[1]*ThisNeutron->Position[1]+
        ThisBenderCh.Surf[0].D*ThisNeutron->Position[1] +
        ThisBenderCh.Surf[0].E*ThisNeutron->Position[2]*ThisNeutron->Position[2]+
        ThisBenderCh.Surf[0].F*ThisNeutron->Position[2] +
        ThisBenderCh.Surf[0].W +
        ThisBenderCh.Surf[0].P*ThisNeutron->Position[0]*ThisNeutron->Position[1]+
        ThisBenderCh.Surf[0].Q*ThisNeutron->Position[1]*ThisNeutron->Position[2]+
        ThisBenderCh.Surf[0].R*ThisNeutron->Position[2]*ThisNeutron->Position[0] ;

      signb =
        ThisBenderCh.Surf[1].A*ThisNeutron->Position[0]*ThisNeutron->Position[0] +
        ThisBenderCh.Surf[1].B*ThisNeutron->Position[0] +
        ThisBenderCh.Surf[1].C*ThisNeutron->Position[1]*ThisNeutron->Position[1]+
        ThisBenderCh.Surf[1].D*ThisNeutron->Position[1] +
        ThisBenderCh.Surf[1].E*ThisNeutron->Position[2]*ThisNeutron->Position[2]+
        ThisBenderCh.Surf[1].F*ThisNeutron->Position[2] +
        ThisBenderCh.Surf[1].W +
        ThisBenderCh.Surf[1].P*ThisNeutron->Position[0]*ThisNeutron->Position[1]+
        ThisBenderCh.Surf[1].Q*ThisNeutron->Position[1]*ThisNeutron->Position[2]+
        ThisBenderCh.Surf[1].R*ThisNeutron->Position[2]*ThisNeutron->Position[0] ;

      if ((signl*signr) > 0.0) return(-1.0);
      if ((signt*signb) > 0.0) return(-1.0);

      TimeOFTotal =  TimeOFTotal + TimeOFmin;

      /* Attenuation during pass of channel */
      VelocityReal = (double)(V_FROM_LAMBDA(ThisNeutron->Wavelength));
      N_Wavelength = ThisNeutron->Wavelength;

      mu = Interpol(N_Wavelength, aLambdaChnl, aMuChnl, nMuValuesChnl);
      if (mu == -10000.0)
      { CountMessageID(WNDO_L_RANGE_TOO_SMALL, ThisNeutron->ID);
        prob = 1.0;     // ideal transmission assumed inside channel for wavelenghts out of available range
      }
      else if (mu == 10000.0)
      { prob = 0.0;
      }
      else
      { prob = exp(-mu*TimeOFmin*VelocityReal);
      }
      ThisNeutron->Probability = ThisNeutron->Probability*prob;

      WriteIAP(ThisNeutron, VT_REFLECTED);

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


    AP = 2.0*ThisBenderCh.Surf[ThisCollision].A*NearestNeutron.Position[0]
           + ThisBenderCh.Surf[ThisCollision].B
           + ThisBenderCh.Surf[ThisCollision].P*NearestNeutron.Position[1]
           + ThisBenderCh.Surf[ThisCollision].R*NearestNeutron.Position[2];

    BP = 2.0*ThisBenderCh.Surf[ThisCollision].C*NearestNeutron.Position[1]
           + ThisBenderCh.Surf[ThisCollision].D
           + ThisBenderCh.Surf[ThisCollision].P*NearestNeutron.Position[0]
           + ThisBenderCh.Surf[ThisCollision].Q*NearestNeutron.Position[2];

    CP = 2.0*ThisBenderCh.Surf[ThisCollision].E*NearestNeutron.Position[2]
           + ThisBenderCh.Surf[ThisCollision].F
           + ThisBenderCh.Surf[ThisCollision].Q*NearestNeutron.Position[1]
           + ThisBenderCh.Surf[ThisCollision].R*NearestNeutron.Position[0];


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

    chance = Vran();

    angular=fabs(NeutronPlaneAngle2(&NearestNeutron, AP, BP, CP));

    /* Convert from radian to degree */

    degangular = angular*360.0/(2.0*M_PI);

    datanumber =  (int)(degangular*1000.0/(NearestNeutron.Wavelength));


    if (datanumber <= 999)
    {
      /*Choose the reflectivity file */

      switch(ThisCollision)
      {
        /* top plane */
        case 0:
          if (keypol == 1)
          {
            if(NearestNeutron.Spin[qspin] == 1.0)
            {
              ReflectionProb = ReflUpTB[datanumber];
            }
            if(NearestNeutron.Spin[qspin] == -1.0)
            {
              ReflectionProb = ReflDownTB[datanumber];
            }
          }
          else
          {
            ReflectionProb = ReflUpTB[datanumber];
          }
          break;

        /* bottom plane */
        case 1:
          if (keypol == 1)
          {
            if(NearestNeutron.Spin[qspin] == 1.0)
            {
              ReflectionProb = ReflUpTB[datanumber];
            }
            if(NearestNeutron.Spin[qspin] == -1.0)
            {
              ReflectionProb = ReflDownTB[datanumber];
            }
          }
          else
          {
            ReflectionProb = ReflUpTB[datanumber];
          }
          break;

        /* right plane */
        case 2:
          if (keypol == 1)
          {
            if(NearestNeutron.Spin[qspin] == 1.0)
            {
              ReflectionProb = ReflUpL[datanumber];
            }
            if(NearestNeutron.Spin[qspin] == -1.0)
            {
              ReflectionProb = ReflDownL[datanumber];
            }
          }
          else
          {
            ReflectionProb = ReflUpL[datanumber];
          }
          break;

        /* left plane */

        case 3:
          if (keypol == 1)
          {
            if(NearestNeutron.Spin[qspin] == 1.0)
            {
              ReflectionProb = ReflUpR[datanumber];
            }
            if(NearestNeutron.Spin[qspin] == -1.0)
            {
              ReflectionProb = ReflDownR[datanumber];
            }
          }
          else
          {
            ReflectionProb = ReflUpR[datanumber];
          }
          break;

        default:
          Error("PathThroughBenderGravOrder2: Wrong ID for bender wall found");
          break;
      }

    }
    else
    {
      ReflectionProb = 0.0;
    }


    /* Monte Carlo for decision if neutron passes or is reflected */

    if ((chance >= ReflectionProb)||(ReflectionProb == 0.0))
    //    if (keytemp == 0 || keytemp ==1)
    {

      /* Neutron pass in the next channel */

      if ((ThisCollision == 0)||(ThisCollision == 1))
      {
        /* Neutron pass via top or bottom planes, so absorb */
        return(-1);
      }

      /* Check next left or right channels */

      if ((ThisCollision == 2)&&(ChnlNumber == ext_left))
      {
  #ifdef VT_GRAPH
        if (do_visualise)
        {
          double tempx, tempy;
          tempx = NearestNeutron.Position[0];
          tempy = NearestNeutron.Position[1];
          cpgdraw(tempx,tempy);
          cpgpt1(tempx,tempy,22);
        }
  #endif
        return(-1);
      }

      if ((ThisCollision == 3)&&(ChnlNumber == ext_right))
      {
  #ifdef VT_GRAPH
        if (do_visualise)
        {
          double tempx, tempy;
          tempx = NearestNeutron.Position[0];
          tempy = NearestNeutron.Position[1];
          cpgdraw(tempx,tempy);
          cpgpt1(tempx,tempy,16);
        }
  #endif
        return(-1);
      }

      /* Add time OF */
      TimeOFTotal =  TimeOFTotal + TimeOFmin ;

      /* Attenuation during pass of channel */
      VelocityReal = (double)(V_FROM_LAMBDA(NearestNeutron.Wavelength));
      N_Wavelength = NearestNeutron.Wavelength;

      mu = Interpol(N_Wavelength, aLambdaChnl, aMuChnl, nMuValuesChnl);
      if (mu == -10000.0)
      { CountMessageID(WNDO_L_RANGE_TOO_SMALL, ThisNeutron->ID);
        prob = 1.0;     // ideal transmission assumed inside channel for wavelenghts out of available range
      }
      else if (mu == 10000.0)
      { prob = 0.0;
      }
      else
      { prob = exp(-mu*TimeOFmin*VelocityReal);
      }
      NearestNeutron.Probability = NearestNeutron.Probability*prob;

      /* pass via left surface of current channel */
      if (ThisCollision == 2)
      {
        OldSurf.A = MyBender.SurfLeft[ChnlNumber].A;
        OldSurf.B = MyBender.SurfLeft[ChnlNumber].B;
        OldSurf.C = MyBender.SurfLeft[ChnlNumber].C;
        OldSurf.D = MyBender.SurfLeft[ChnlNumber].D;
        OldSurf.E = MyBender.SurfLeft[ChnlNumber].E;
        OldSurf.F = MyBender.SurfLeft[ChnlNumber].F;
        OldSurf.W = MyBender.SurfLeft[ChnlNumber].W;
        OldSurf.P = MyBender.SurfLeft[ChnlNumber].P;
        OldSurf.Q = MyBender.SurfLeft[ChnlNumber].Q;
        OldSurf.R = MyBender.SurfLeft[ChnlNumber].R;

        ChnlNumber = ChnlNumber + 1;

        NewSurf.A = MyBender.SurfRight[ChnlNumber].A;
        NewSurf.B = MyBender.SurfRight[ChnlNumber].B;
        NewSurf.C = MyBender.SurfRight[ChnlNumber].C;
        NewSurf.D = MyBender.SurfRight[ChnlNumber].D;
        NewSurf.E = MyBender.SurfRight[ChnlNumber].E;
        NewSurf.F = MyBender.SurfRight[ChnlNumber].F;
        NewSurf.W = MyBender.SurfRight[ChnlNumber].W;
        NewSurf.P = MyBender.SurfRight[ChnlNumber].P;
        NewSurf.Q = MyBender.SurfRight[ChnlNumber].Q;
        NewSurf.R = MyBender.SurfRight[ChnlNumber].R;
      }

      /* pass via right surface of current channel */
      if (ThisCollision == 3)
      {
        OldSurf.A = MyBender.SurfRight[ChnlNumber].A;
        OldSurf.B = MyBender.SurfRight[ChnlNumber].B;
        OldSurf.C = MyBender.SurfRight[ChnlNumber].C;
        OldSurf.D = MyBender.SurfRight[ChnlNumber].D;
        OldSurf.E = MyBender.SurfRight[ChnlNumber].E;
        OldSurf.F = MyBender.SurfRight[ChnlNumber].F;
        OldSurf.W = MyBender.SurfRight[ChnlNumber].W;
        OldSurf.P = MyBender.SurfRight[ChnlNumber].P;
        OldSurf.Q = MyBender.SurfRight[ChnlNumber].Q;
        OldSurf.R = MyBender.SurfRight[ChnlNumber].R;

        ChnlNumber = ChnlNumber - 1;

        NewSurf.A = MyBender.SurfLeft[ChnlNumber].A;
        NewSurf.B = MyBender.SurfLeft[ChnlNumber].B;
        NewSurf.C = MyBender.SurfLeft[ChnlNumber].C;
        NewSurf.D = MyBender.SurfLeft[ChnlNumber].D;
        NewSurf.E = MyBender.SurfLeft[ChnlNumber].E;
        NewSurf.F = MyBender.SurfLeft[ChnlNumber].F;
        NewSurf.W = MyBender.SurfLeft[ChnlNumber].W;
        NewSurf.P = MyBender.SurfLeft[ChnlNumber].P;
        NewSurf.Q = MyBender.SurfLeft[ChnlNumber].Q;
        NewSurf.R = MyBender.SurfLeft[ChnlNumber].R;
      }

      /*  Redefine step  */
      stepp = 0.01*(fabs(exitdiscenter[ChnlNumber+1] - exitdiscenter[ChnlNumber]) +
                    fabs(entrdiscenter[ChnlNumber+1] - entrdiscenter[ChnlNumber]) - spacer);

      /* Convert stepp in time ms */
      VelocityReal = (double)(V_FROM_LAMBDA(NearestNeutron.Wavelength));
      if (VelocityReal != 0.0)
      {
        stepp = stepp/VelocityReal;
      }
      else
      {
        CountMessageID(ALL_ZERO_VELOCITY, NearestNeutron.ID);
        return(-1);
      }

      /* visualisation */
      WriteIAP(&NearestNeutron, VT_REFLECTED);

  #ifdef VT_GRAPH
      if (do_visualise)
      {
        double tempx, tempy;
        /* visual path begin */
        tempx = NearestNeutron.Position[0];
        tempy = NearestNeutron.Position[1];
        cpgdraw(tempx,tempy);
        if (ThisCollision == 2)  cpgpt1(tempx,tempy,22);
        if (ThisCollision == 3) cpgpt1(tempx,tempy,16);
        /* visual path end */
      }
  #endif

      /* copy the current channel, which was chosen */
      /* left plane */
      ThisBenderCh.Surf[2].A = MyBender.SurfLeft[ChnlNumber].A;
      ThisBenderCh.Surf[2].B = MyBender.SurfLeft[ChnlNumber].B;
      ThisBenderCh.Surf[2].C = MyBender.SurfLeft[ChnlNumber].C;
      ThisBenderCh.Surf[2].D = MyBender.SurfLeft[ChnlNumber].D;
      ThisBenderCh.Surf[2].E = MyBender.SurfLeft[ChnlNumber].E;
      ThisBenderCh.Surf[2].F = MyBender.SurfLeft[ChnlNumber].F;
      ThisBenderCh.Surf[2].W = MyBender.SurfLeft[ChnlNumber].W;
      ThisBenderCh.Surf[2].P = MyBender.SurfLeft[ChnlNumber].P;
      ThisBenderCh.Surf[2].Q = MyBender.SurfLeft[ChnlNumber].Q;
      ThisBenderCh.Surf[2].R = MyBender.SurfLeft[ChnlNumber].R;

      /* right plane */
      ThisBenderCh.Surf[3].A = MyBender.SurfRight[ChnlNumber].A;
      ThisBenderCh.Surf[3].B = MyBender.SurfRight[ChnlNumber].B;
      ThisBenderCh.Surf[3].C = MyBender.SurfRight[ChnlNumber].C;
      ThisBenderCh.Surf[3].D = MyBender.SurfRight[ChnlNumber].D;
      ThisBenderCh.Surf[3].E = MyBender.SurfRight[ChnlNumber].E;
      ThisBenderCh.Surf[3].F = MyBender.SurfRight[ChnlNumber].F;
      ThisBenderCh.Surf[3].W = MyBender.SurfRight[ChnlNumber].W;
      ThisBenderCh.Surf[3].P = MyBender.SurfRight[ChnlNumber].P;
      ThisBenderCh.Surf[3].Q = MyBender.SurfRight[ChnlNumber].Q;
      ThisBenderCh.Surf[3].R = MyBender.SurfRight[ChnlNumber].R;


      /* exit plane */
      ThisBenderCh.Surf[4].A = MyBender.SurfExit[ChnlNumber].A;
      ThisBenderCh.Surf[4].B = MyBender.SurfExit[ChnlNumber].B;
      ThisBenderCh.Surf[4].C = MyBender.SurfExit[ChnlNumber].C;
      ThisBenderCh.Surf[4].D = MyBender.SurfExit[ChnlNumber].D;
      ThisBenderCh.Surf[4].E = MyBender.SurfExit[ChnlNumber].E;
      ThisBenderCh.Surf[4].F = MyBender.SurfExit[ChnlNumber].F;
      ThisBenderCh.Surf[4].W = MyBender.SurfExit[ChnlNumber].W;
      ThisBenderCh.Surf[4].P = MyBender.SurfExit[ChnlNumber].P;
      ThisBenderCh.Surf[4].Q = MyBender.SurfExit[ChnlNumber].Q;
      ThisBenderCh.Surf[4].R = MyBender.SurfExit[ChnlNumber].R;


      /* PASS neutron in the next channel WITHOUT REFRACTION */

      /* find the nearest wall*/

      CopyNeutron(&NearestNeutron, &TempNeutron1);
      TimeOFm[0] = NeutronSurfaceSecIntersectionGr(&TempNeutron1,
      NewSurf, keygrav);

      CopyNeutron(&NearestNeutron, &TempNeutron1);
      TimeOFm[1] = NeutronSurfaceSecIntersectionGr(&TempNeutron1,
      ThisBenderCh.Surf[4], keygrav);

      CopyNeutron(&NearestNeutron, &TempNeutron1);
      TimeOFm[2] = NeutronSurfaceSecIntersectionGr(&TempNeutron1,
      ThisBenderCh.Surf[0], keygrav);

      CopyNeutron(&NearestNeutron, &TempNeutron1);
      TimeOFm[3] = NeutronSurfaceSecIntersectionGr(&TempNeutron1,
      ThisBenderCh.Surf[1], keygrav);


      TimeOFmi = 999999999999999999999999.9;
      ThisColl = 0;

      for(i = 0; i < 4; i++)
      {
        if (TimeOFm[i] <= 0.0 ) continue;
        if (TimeOFm[i] > TimeOFmi) continue;
        TimeOFmi = TimeOFm[i];
        ThisColl = i;
      }

      /* neutron hits the top or bottom wall */
      if ((ThisColl == 2)||(ThisColl == 3))
        return(-1);

      if (ThisColl == 0)
      {
        /* Pass in the next channel */

        TimeOFpass = NeutronSurfaceSecIntersectionGr(&NearestNeutron, NewSurf, keygrav);
        TimeOFTotal = TimeOFTotal + TimeOFpass;
      }

      if (ThisColl == 1)
      {
        /* Neutron pass via surface material and exit from bender */

        TimeOFpass = NeutronSurfaceSecIntersectionGr(&NearestNeutron, ThisBenderCh.Surf[4], keygrav);
        TimeOFTotal = TimeOFTotal + TimeOFpass;

        /* Illegal velocity */
        if(NearestNeutron.Vector[0]<0.0)  return(-1.0);

        /* losses via pass part of surface */
        VelocityReal = (double)(V_FROM_LAMBDA(NearestNeutron.Wavelength));

        /*  Attenuation  */
        if (ThisCollision == 2)
        {
          N_Wavelength = NearestNeutron.Wavelength;

          mu = Interpol(N_Wavelength, aLambdaL, aMuL, nMuValuesL);
          if (mu == -10000.0)
          { CountMessageID(WNDO_L_RANGE_TOO_SMALL, NearestNeutron.ID);
            prob = 0.0;     // ideal absorption assumed between channels for wavelenghts out of given range
          }
          else if (mu == 10000.0)
          { prob = 0.0;
          }
          else
          { prob = exp(-mu*TimeOFpass*VelocityReal);
          }
          NearestNeutron.Probability = NearestNeutron.Probability * prob;
        }

        if (ThisCollision == 3)
        {
          N_Wavelength = NearestNeutron.Wavelength;

          mu = Interpol(N_Wavelength, aLambdaR, aMuR, nMuValuesR);
          if (mu == -10000.0)
          { CountMessageID(WNDO_L_RANGE_TOO_SMALL, NearestNeutron.ID);
            prob = 0.0;     // ideal absorption assumed between channels for wavelenghts out of given range
          }
          else if (mu == 10000.0)
          { prob = 0.0;
          }
          else
          { prob = exp(-mu*TimeOFpass*VelocityReal);
          }
          NearestNeutron.Probability = NearestNeutron.Probability * prob;
        }

        if (NearestNeutron.Probability <= wei_min)  return(-1.0);

        ThisNeutron->Position[0] = NearestNeutron.Position[0];
        ThisNeutron->Position[1] = NearestNeutron.Position[1];
        ThisNeutron->Position[2] = NearestNeutron.Position[2];
        ThisNeutron->Vector[2] = NearestNeutron.Vector[2];
        ThisNeutron->Probability = NearestNeutron.Probability;
        ThisNeutron->Wavelength = NearestNeutron.Wavelength;

        /* Check incorrect neutron path flight */

        signl =
          OldSurf.A*ThisNeutron->Position[0]*ThisNeutron->Position[0] +
          OldSurf.B*ThisNeutron->Position[0] +
          OldSurf.C*ThisNeutron->Position[1]*ThisNeutron->Position[1]+
          OldSurf.D*ThisNeutron->Position[1] +
          OldSurf.E*ThisNeutron->Position[2]*ThisNeutron->Position[2]+
          OldSurf.F*ThisNeutron->Position[2] +
          OldSurf.W +
          OldSurf.P*ThisNeutron->Position[0]*ThisNeutron->Position[1]+
          OldSurf.Q*ThisNeutron->Position[1]*ThisNeutron->Position[2]+
          OldSurf.R*ThisNeutron->Position[2]*ThisNeutron->Position[0] ;

        signr =
          NewSurf.A*ThisNeutron->Position[0]*ThisNeutron->Position[0]+
          NewSurf.B*ThisNeutron->Position[0] +
          NewSurf.C*ThisNeutron->Position[1]*ThisNeutron->Position[1]+
          NewSurf.D*ThisNeutron->Position[1] +
          NewSurf.E*ThisNeutron->Position[2]*ThisNeutron->Position[2]+
          NewSurf.F*ThisNeutron->Position[2] +
          NewSurf.W +
          NewSurf.P*ThisNeutron->Position[0]*ThisNeutron->Position[1]+
          NewSurf.Q*ThisNeutron->Position[1]*ThisNeutron->Position[2]+
          NewSurf.R*ThisNeutron->Position[2]*ThisNeutron->Position[0] ;

        signt =
          ThisBenderCh.Surf[0].A*ThisNeutron->Position[0]*ThisNeutron->Position[0] +
          ThisBenderCh.Surf[0].B*ThisNeutron->Position[0] +
          ThisBenderCh.Surf[0].C*ThisNeutron->Position[1]*ThisNeutron->Position[1]+
          ThisBenderCh.Surf[0].D*ThisNeutron->Position[1] +
          ThisBenderCh.Surf[0].E*ThisNeutron->Position[2]*ThisNeutron->Position[2]+
          ThisBenderCh.Surf[0].F*ThisNeutron->Position[2] +
          ThisBenderCh.Surf[0].W +
          ThisBenderCh.Surf[0].P*ThisNeutron->Position[0]*ThisNeutron->Position[1]+
          ThisBenderCh.Surf[0].Q*ThisNeutron->Position[1]*ThisNeutron->Position[2]+
          ThisBenderCh.Surf[0].R*ThisNeutron->Position[2]*ThisNeutron->Position[0] ;

        signb =
          ThisBenderCh.Surf[1].A*ThisNeutron->Position[0]*ThisNeutron->Position[0] +
          ThisBenderCh.Surf[1].B*ThisNeutron->Position[0] +
          ThisBenderCh.Surf[1].C*ThisNeutron->Position[1]*ThisNeutron->Position[1]+
          ThisBenderCh.Surf[1].D*ThisNeutron->Position[1] +
          ThisBenderCh.Surf[1].E*ThisNeutron->Position[2]*ThisNeutron->Position[2]+
          ThisBenderCh.Surf[1].F*ThisNeutron->Position[2] +
          ThisBenderCh.Surf[1].W +
          ThisBenderCh.Surf[1].P*ThisNeutron->Position[0]*ThisNeutron->Position[1]+
          ThisBenderCh.Surf[1].Q*ThisNeutron->Position[1]*ThisNeutron->Position[2]+
          ThisBenderCh.Surf[1].R*ThisNeutron->Position[2]*ThisNeutron->Position[0] ;


        if ((signl*signr) > 0.0) return(-1.0);

        if ((signt*signb) > 0.0) return(-1.0);


        WriteIAP(ThisNeutron, VT_EXITED);

    #ifdef VT_GRAPH
        if (do_visualise)
        {
          double tempx, tempy;
          /* visual path begin */
          tempx = ThisNeutron->Position[0];
          tempy = ThisNeutron->Position[1];
          cpgdraw(tempx,tempy);
          if (ThisCollision == 2)  cpgpt1(tempx,tempy,22);
          if (ThisCollision == 3) cpgpt1(tempx,tempy,16);
          /* visual path end */
        }
    #endif

        /* EXIT */
        return TimeOFTotal;
      }

      ThisNeutron->Position[0] = NearestNeutron.Position[0];
      ThisNeutron->Position[1] = NearestNeutron.Position[1];
      ThisNeutron->Position[2] = NearestNeutron.Position[2];
      ThisNeutron->Vector[2] = NearestNeutron.Vector[2];
      ThisNeutron->Probability = NearestNeutron.Probability;
      ThisNeutron->Wavelength = NearestNeutron.Wavelength;

      WriteIAP(ThisNeutron, VT_REFLECTED);

      /* losses via pass surface */
      /* Attenuation */
      VelocityReal = (double)(V_FROM_LAMBDA(ThisNeutron->Wavelength));

      if (ThisCollision == 2)
      {
        N_Wavelength = ThisNeutron->Wavelength;

        mu = Interpol(N_Wavelength, aLambdaL, aMuL, nMuValuesL);
        if (mu == -10000.0)
        { CountMessageID(WNDO_L_RANGE_TOO_SMALL, ThisNeutron->ID);
          prob = 0.0;     // ideal absorption assumed between channels for wavelenghts out of given range
        }
        else if (mu == 10000.0)
        { prob = 0.0;
        }
        else
        { prob = exp(-mu * TimeOFpass * VelocityReal);
        }
        ThisNeutron->Probability = ThisNeutron->Probability * prob;
      }

      if (ThisCollision == 3)
      {
        N_Wavelength = ThisNeutron->Wavelength;
        mu = Interpol(N_Wavelength, aLambdaR, aMuR, nMuValuesR);
        if (mu == -10000.0)
        { CountMessageID(WNDO_L_RANGE_TOO_SMALL, ThisNeutron->ID);
          prob = 0.0;     // ideal absorption assumed between channels for wavelenghts out of given range
        }
        else if (mu == 10000.0)
        { prob = 0.0;
        }
        else
        { prob = exp(-mu * TimeOFpass * VelocityReal);
        }
        ThisNeutron->Probability = ThisNeutron->Probability * prob;
      }

      /* visualisation */
   #ifdef VT_GRAPH
      if (do_visualise)
      {
        double tempx, tempy;
        /* visual path begin */
        tempx = ThisNeutron->Position[0];
        tempy = ThisNeutron->Position[1];
        cpgdraw(tempx,tempy);
        if (ThisCollision == 2)  cpgpt1(tempx,tempy,22);
        if (ThisCollision == 3) cpgpt1(tempx,tempy,16);
        /* visual path end */
      }
   #endif

      CopyNeutron(ThisNeutron, &TempNeutron1);
      TimeOF1=NeutronSurfaceSecIntersectionGr(&TempNeutron1, ThisBenderCh.Surf[4], keygrav);

      CopyNeutron(ThisNeutron, &TempNeutron1);

      TimeOFtop=NeutronSurfaceSecIntersectionGr(&TempNeutron1, ThisBenderCh.Surf[1], keygrav);

      CopyNeutron(ThisNeutron, &TempNeutron1);

      TimeOFbot=NeutronSurfaceSecIntersectionGr(&TempNeutron1, ThisBenderCh.Surf[0], keygrav);

      /* CHECK incorrect flight of neutron, move neutron after reflection on small distance
         and check inside channel or no , no - exit(-1) */
      if ((fabs(TimeOF1) > stepp)&&(fabs(TimeOFtop) > stepp)&&(fabs(TimeOFbot) > stepp))
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

        N_Wavelength = ThisNeutron->Wavelength;

        mu = Interpol(N_Wavelength, aLambdaChnl, aMuChnl, nMuValuesChnl);
        if (mu == -10000.0)
        { CountMessageID(WNDO_L_RANGE_TOO_SMALL, ThisNeutron->ID);
          prob = 1.0;     // ideal transmission assumed inside channel for wavelenghts out of given range
        }
        else if (mu == 10000.0)
        { prob = 0.0;
        }
        else
        { prob = exp(-mu*stepp*VelocityReal);
        }
        ThisNeutron->Probability = ThisNeutron->Probability*prob;

        /* check the incorrect path of neutron */

        signl =
          ThisBenderCh.Surf[2].A*ThisNeutron->Position[0]*ThisNeutron->Position[0] +
          ThisBenderCh.Surf[2].B*ThisNeutron->Position[0] +
          ThisBenderCh.Surf[2].C*ThisNeutron->Position[1]*ThisNeutron->Position[1]+
          ThisBenderCh.Surf[2].D*ThisNeutron->Position[1] +
          ThisBenderCh.Surf[2].E*ThisNeutron->Position[2]*ThisNeutron->Position[2]+
          ThisBenderCh.Surf[2].F*ThisNeutron->Position[2] +
          ThisBenderCh.Surf[2].W +
          ThisBenderCh.Surf[2].P*ThisNeutron->Position[0]*ThisNeutron->Position[1]+
          ThisBenderCh.Surf[2].Q*ThisNeutron->Position[1]*ThisNeutron->Position[2]+
          ThisBenderCh.Surf[2].R*ThisNeutron->Position[2]*ThisNeutron->Position[0] ;

        signr =
          ThisBenderCh.Surf[3].A*ThisNeutron->Position[0]*ThisNeutron->Position[0]+
          ThisBenderCh.Surf[3].B*ThisNeutron->Position[0] +
          ThisBenderCh.Surf[3].C*ThisNeutron->Position[1]*ThisNeutron->Position[1]+
          ThisBenderCh.Surf[3].D*ThisNeutron->Position[1] +
          ThisBenderCh.Surf[3].E*ThisNeutron->Position[2]*ThisNeutron->Position[2]+
          ThisBenderCh.Surf[3].F*ThisNeutron->Position[2] +
          ThisBenderCh.Surf[3].W +
          ThisBenderCh.Surf[3].P*ThisNeutron->Position[0]*ThisNeutron->Position[1]+
          ThisBenderCh.Surf[3].Q*ThisNeutron->Position[1]*ThisNeutron->Position[2]+
          ThisBenderCh.Surf[3].R*ThisNeutron->Position[2]*ThisNeutron->Position[0] ;

        signt =
          ThisBenderCh.Surf[0].A*ThisNeutron->Position[0]*ThisNeutron->Position[0] +
          ThisBenderCh.Surf[0].B*ThisNeutron->Position[0] +
          ThisBenderCh.Surf[0].C*ThisNeutron->Position[1]*ThisNeutron->Position[1]+
          ThisBenderCh.Surf[0].D*ThisNeutron->Position[1] +
          ThisBenderCh.Surf[0].E*ThisNeutron->Position[2]*ThisNeutron->Position[2]+
          ThisBenderCh.Surf[0].F*ThisNeutron->Position[2] +
          ThisBenderCh.Surf[0].W +
          ThisBenderCh.Surf[0].P*ThisNeutron->Position[0]*ThisNeutron->Position[1]+
          ThisBenderCh.Surf[0].Q*ThisNeutron->Position[1]*ThisNeutron->Position[2]+
          ThisBenderCh.Surf[0].R*ThisNeutron->Position[2]*ThisNeutron->Position[0] ;

        signb =
          ThisBenderCh.Surf[1].A*ThisNeutron->Position[0]*ThisNeutron->Position[0] +
          ThisBenderCh.Surf[1].B*ThisNeutron->Position[0] +
          ThisBenderCh.Surf[1].C*ThisNeutron->Position[1]*ThisNeutron->Position[1]+
          ThisBenderCh.Surf[1].D*ThisNeutron->Position[1] +
          ThisBenderCh.Surf[1].E*ThisNeutron->Position[2]*ThisNeutron->Position[2]+
          ThisBenderCh.Surf[1].F*ThisNeutron->Position[2] +
          ThisBenderCh.Surf[1].W +
          ThisBenderCh.Surf[1].P*ThisNeutron->Position[0]*ThisNeutron->Position[1]+
          ThisBenderCh.Surf[1].Q*ThisNeutron->Position[1]*ThisNeutron->Position[2]+
          ThisBenderCh.Surf[1].R*ThisNeutron->Position[2]*ThisNeutron->Position[0] ;

        /* particle outside channel*/
        /* KL: Warning: neutron must move from the surface, otherwise sign is a very low value,
        that can be above or below zero. Therefore, comparison: > 1e-x may be better than > 0.0 */

    #ifdef VT_GRAPH
        if (do_visualise)
        {
          double tempx, tempy;
          /* visual path begin */
          tempx = ThisNeutron->Position[0];
          tempy = ThisNeutron->Position[1];
          cpgdraw(tempx,tempy);
          if (ThisCollision == 2)  cpgpt1(tempx,tempy,22);
          if (ThisCollision == 3) cpgpt1(tempx,tempy,16);
          /* visual path end */
        }
    #endif

        if ((signl*signr) > 0.0) return(-1.0);

        if ((signt*signb) > 0.0) return(-1.0);
      }

      WriteIAP(ThisNeutron, VT_REFLECTED);

      keytemp++;
      /* Continue moving in the next channel */
      continue;

    }
    else
    {
      /***********************************************************************************/
      /*  Calculate the trajectory of the reflected neutron.                             */
      /***********************************************************************************/

      /* Make reflection */

      DOTP = AP*NearestNeutron.Vector[0] + BP*NearestNeutron.Vector[1] +
             CP*NearestNeutron.Vector[2];

      ThisNeutron->Vector[0] = NearestNeutron.Vector[0] - 2.0*DOTP*AP;
      ThisNeutron->Vector[1] = NearestNeutron.Vector[1] - 2.0*DOTP*BP;
      ThisNeutron->Vector[2] = NearestNeutron.Vector[2] - 2.0*DOTP*CP;

      /***********************************************************************************/
      /* Reset the neutron coordinates to coordinates of the collision, add the path     */
      /* length to this point to the running total, keep track of which plane has just   */
      /* been hit and then return to the top of the top of the loop and find the next    */
      /* collision                                                                       */
      /***********************************************************************************/

      ThisNeutron->Position[0] = NearestNeutron.Position[0];
      ThisNeutron->Position[1] = NearestNeutron.Position[1];
      ThisNeutron->Position[2] = NearestNeutron.Position[2];
      ThisNeutron->Wavelength = NearestNeutron.Wavelength;
      ThisNeutron->Probability = NearestNeutron.Probability;

      WriteIAP(ThisNeutron, VT_REFLECTED);


      TimeOFTotal =  TimeOFTotal + TimeOFmin ;

      VelocityReal = (double)(V_FROM_LAMBDA(ThisNeutron->Wavelength));
      N_Wavelength = ThisNeutron->Wavelength;

      mu = Interpol(N_Wavelength, aLambdaChnl, aMuChnl, nMuValuesChnl);
      if (mu == -10000.0)
      { CountMessageID(WNDO_L_RANGE_TOO_SMALL, ThisNeutron->ID);
        prob = 1.0;     // ideal transmission assumed inside channel for wavelenghts out of given range
      }
      else if (mu == 10000.0)
      { prob = 0.0;
      }
      else
      { prob = exp(-mu*TimeOFmin*VelocityReal);
      }
      ThisNeutron->Probability = ThisNeutron->Probability*prob;

      PreviousCollision = ThisCollision;


  #ifdef VT_GRAPH
      if (do_visualise)
      {
        double tempx, tempy;
        /* visual path begin */
        tempx = ThisNeutron->Position[0];
        tempy = ThisNeutron->Position[1];
        cpgdraw(tempx,tempy);
        //      cpgpt1(tempx,tempy,23);
        /* visual path end */
        cpgsci(7);
      }
  #endif

      /*  angular1=fabs(NeutronPlaneAngle2(ThisNeutron, AP, BP, CP));

      CopyNeutron(ThisNeutron, &TempNeutron1);
      TimeOF1=NeutronSurfaceSecIntersectionGr(&TempNeutron1, ThisBenderCh.Surf[4], keygrav);

      CopyNeutron(ThisNeutron, &TempNeutron1);

      TimeOFtop = NeutronSurfaceSecIntersectionGr(&TempNeutron1, ThisBenderCh.Surf[1], keygrav);

      CopyNeutron(ThisNeutron, &TempNeutron1);
      TimeOFbot = NeutronSurfaceSecIntersectionGr(&TempNeutron1, ThisBenderCh.Surf[0], keygrav);

       /* CHECK incorrect flight of neutron, move neutron after reflection on small distance
          and check inside channel or no , no - exit(-1) */

      if ((fabs(TimeOF1) > stepp)&&(fabs(TimeOFtop) > stepp)&&(fabs(TimeOFbot) > stepp))
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

        VelocityReal = (double)(V_FROM_LAMBDA(ThisNeutron->Wavelength));
        N_Wavelength = ThisNeutron->Wavelength;

        mu = Interpol(N_Wavelength, aLambdaChnl, aMuChnl, nMuValuesChnl);
        if (mu == -10000.0)
        { CountMessageID(WNDO_L_RANGE_TOO_SMALL, ThisNeutron->ID);
          prob = 1.0;     // ideal transmission assumed inside channel for wavelenghts out of given range
        }
        else if (mu == 10000.0)
        { prob = 0.0;
        }
        else
        { prob = exp(-mu*stepp*VelocityReal);
        }
        ThisNeutron->Probability = ThisNeutron->Probability*prob;

        /* check the incorrect path of neutron */

        signl =
          ThisBenderCh.Surf[2].A*ThisNeutron->Position[0]*ThisNeutron->Position[0] +
          ThisBenderCh.Surf[2].B*ThisNeutron->Position[0] +
          ThisBenderCh.Surf[2].C*ThisNeutron->Position[1]*ThisNeutron->Position[1]+
          ThisBenderCh.Surf[2].D*ThisNeutron->Position[1] +
          ThisBenderCh.Surf[2].E*ThisNeutron->Position[2]*ThisNeutron->Position[2]+
          ThisBenderCh.Surf[2].F*ThisNeutron->Position[2] +
          ThisBenderCh.Surf[2].W +
          ThisBenderCh.Surf[2].P*ThisNeutron->Position[0]*ThisNeutron->Position[1]+
          ThisBenderCh.Surf[2].Q*ThisNeutron->Position[1]*ThisNeutron->Position[2]+
          ThisBenderCh.Surf[2].R*ThisNeutron->Position[2]*ThisNeutron->Position[0] ;

        signr =
          ThisBenderCh.Surf[3].A*ThisNeutron->Position[0]*ThisNeutron->Position[0]+
          ThisBenderCh.Surf[3].B*ThisNeutron->Position[0] +
          ThisBenderCh.Surf[3].C*ThisNeutron->Position[1]*ThisNeutron->Position[1]+
          ThisBenderCh.Surf[3].D*ThisNeutron->Position[1] +
          ThisBenderCh.Surf[3].E*ThisNeutron->Position[2]*ThisNeutron->Position[2]+
          ThisBenderCh.Surf[3].F*ThisNeutron->Position[2] +
          ThisBenderCh.Surf[3].W +
          ThisBenderCh.Surf[3].P*ThisNeutron->Position[0]*ThisNeutron->Position[1]+
          ThisBenderCh.Surf[3].Q*ThisNeutron->Position[1]*ThisNeutron->Position[2]+
          ThisBenderCh.Surf[3].R*ThisNeutron->Position[2]*ThisNeutron->Position[0] ;


        signt =
          ThisBenderCh.Surf[0].A*ThisNeutron->Position[0]*ThisNeutron->Position[0] +
          ThisBenderCh.Surf[0].B*ThisNeutron->Position[0] +
          ThisBenderCh.Surf[0].C*ThisNeutron->Position[1]*ThisNeutron->Position[1]+
          ThisBenderCh.Surf[0].D*ThisNeutron->Position[1] +
          ThisBenderCh.Surf[0].E*ThisNeutron->Position[2]*ThisNeutron->Position[2]+
          ThisBenderCh.Surf[0].F*ThisNeutron->Position[2] +
          ThisBenderCh.Surf[0].W +
          ThisBenderCh.Surf[0].P*ThisNeutron->Position[0]*ThisNeutron->Position[1]+
          ThisBenderCh.Surf[0].Q*ThisNeutron->Position[1]*ThisNeutron->Position[2]+
          ThisBenderCh.Surf[0].R*ThisNeutron->Position[2]*ThisNeutron->Position[0] ;

        signb =
          ThisBenderCh.Surf[1].A*ThisNeutron->Position[0]*ThisNeutron->Position[0] +
          ThisBenderCh.Surf[1].B*ThisNeutron->Position[0] +
          ThisBenderCh.Surf[1].C*ThisNeutron->Position[1]*ThisNeutron->Position[1]+
          ThisBenderCh.Surf[1].D*ThisNeutron->Position[1] +
          ThisBenderCh.Surf[1].E*ThisNeutron->Position[2]*ThisNeutron->Position[2]+
          ThisBenderCh.Surf[1].F*ThisNeutron->Position[2] +
          ThisBenderCh.Surf[1].W +
          ThisBenderCh.Surf[1].P*ThisNeutron->Position[0]*ThisNeutron->Position[1]+
          ThisBenderCh.Surf[1].Q*ThisNeutron->Position[1]*ThisNeutron->Position[2]+
          ThisBenderCh.Surf[1].R*ThisNeutron->Position[2]*ThisNeutron->Position[0] ;

    #ifdef VT_GRAPH
        if (do_visualise)
        {
          double tempx, tempy;
          /* visual path begin */
          tempx = ThisNeutron->Position[0];
          tempy = ThisNeutron->Position[1];
          cpgdraw(tempx,tempy);
          /* visual path end */
          cpgsci(7);
        }
    #endif

        WriteIAP(ThisNeutron, VT_REFLECTED);


        /* particle outside channel*/
        /* KL: Warning: neutron must move from the surface, otherwise sign is a very low value,
               that can be above or below zero. Therefore, comparison: > 1e-x may be better than > 0.0 */
         if ((signl*signr) > 0.0) return(-1.0);
         if ((signt*signb) > 0.0) return(-1.0);
       }
    }
  }
}

