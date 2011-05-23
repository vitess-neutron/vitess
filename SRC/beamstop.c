/*********************************************************************************************/
/*  VITESS module  slit                                                                      */
/* The free non-commercial use of these routines is granted providing due credit is given to */
/* the authors.                                                                              */
/*                                                                                           */
/* 1.0  Apr 2011  K. Lieutenant   initial version                                            */
/*********************************************************************************************/

#include "init.h"
#include "softabort.h"
#include "intersection.h"


/******************************/
/** Prototypes               **/
/******************************/
void  OwnInit(int argc, char *argv[]);


/******************************/
/** Global Variables         **/
/******************************/
Plane  Endpoint;                 // vertical Plane through the position of the beamstop
                                 //  (Endpoint.D = distance to end of free flight path along x-axis [cm]) 
double VelocityReal,             // velocity of the neutron 
       Width   =0.0, Height=0.0, // width and height of an rectangular beamstop 
       Radius  =0.0,             // radius of an circular beamstop  
       DistMove=0.0,             // distance between starting point and beamstop 
       CenterY =0.0, CenterZ=0.0,// center of the beamstop position
       DistCenter;               // distance between center of beamstop and point of striking of the neutron
short  bCircularWindow=FALSE,    // criterion: shape of window, TRUE: circular, FALSE rectangular 
       bOnBeamstop,              // criterion: beamstop hit or not 
       bProp=FALSE;              // criterion: propagate to beamstop  0: no,  1: yes


/******************************/
/** Program                  **/
/******************************/

int main(int argc, char *argv[])
{
  long   i, BufferIndex;
  double TimeOF,               /* time of flight of the neutron to the window */
         NewPosY, NewPosZ;     /* hor. and vert. position of neutron at slit  */
  Neutron TestNeutron;

  /******************/
  /* initialisation */
  /******************/
  BufferIndex     = 0;

  Init(argc, argv, VT_BEAMSTOP);
  print_module_name("Beamstop 1.0");
  OwnInit(argc, argv);

  DECLARE_ABORT

  while(ReadNeutrons()!= 0)
  {
    for(i=0; i<NumNeutGot; i++)
    {
      CHECK

      if (InputNeutrons[i].Wavelength == 0.0) continue;
      VelocityReal = V_FROM_LAMBDA(InputNeutrons[i].Wavelength); 
      if (VelocityReal <= 0.0) continue;

      /*************************************************************************/
      /* 	Check if neutron would hit the beamstop                              */
      /*************************************************************************/
      TestNeutron=InputNeutrons[i]; //memcpy(&TestNeutron, &InputNeutrons[i], sizeof(Neutron));

      if (keygrav == 1)
      {
        TimeOF = NeutronPlaneIntersectionGrav(&TestNeutron, Endpoint);
      }
      else
      {
        TimeOF = NeutronPlaneIntersection1(&TestNeutron, Endpoint);
      }

      /* If plane through beamstop surface is reached:                   */
      /*  Calculate position on beamstop  and  decide if beamstop is hit */
      if (TimeOF >= 0.0)
      { NewPosY = TestNeutron.Position[1];
        NewPosZ = TestNeutron.Position[2];

			  if(bCircularWindow==TRUE)
			  {	
          DistCenter = sqrt(sq(NewPosY - CenterY) + sq(NewPosZ - CenterZ));
				  if (DistCenter <= Radius)
					  bOnBeamstop=TRUE;
				  else
					  bOnBeamstop=FALSE;
			  }
			  else
        { 
          if (fabs(NewPosY - CenterY) <= 0.5*Width  &&  fabs(NewPosZ - CenterZ) <= 0.5*Height)
				    bOnBeamstop=TRUE;
			    else
				    bOnBeamstop=FALSE;
        }
      }
      else
      { bOnBeamstop=FALSE;
      }

      /*************************************************************************/
      /* if beamstop is missed: writeout original data set for 'progation'=no  */
      /*                                   or new data set for 'progation'=yes */
      /*************************************************************************/      
			if (!bOnBeamstop)
      { 
			  if (bProp)
          WriteNeutron(&TestNeutron);
        else
          WriteNeutron(&InputNeutrons[i]);
      }
    }
  }	

 my_exit:
  if (bCircularWindow)
  { fprintf(LogFilePtr, "Beamstop of %6.2f cm diameter in a distance of %7.2f cm \n",             2.0*Radius, DistMove);
    stPicture.dWPar = Radius;
  }
  else
  { fprintf(LogFilePtr, "Beamstop of size %6.2f x %6.2f cm (W x H) in a distance of %7.2f cm \n", Width, Height, DistMove);
    stPicture.dWPar = Width;
    stPicture.dHPar = Height;
  }

  Cleanup(DistMove,0.0,0.0, 0.0,0.0);	

  return(0);
}



void  OwnInit(int argc, char *argv[])
{
  int i;

  for(i=1; i<argc; i++)
  {
    if(argv[i][0]!='+') 
    {
      switch(argv[i][1])
      {
        case 'p':
          bProp = atoi(&argv[i][2]);        // criterion: propagate to beamstop  0: no,  1: yes
          if (bProp==FALSE)
            bOldFrame=TRUE;
          break;
        case 'R':
          bCircularWindow = atoi(&argv[i][2]);
          break;

        case 'd':
          DistMove = atof(&argv[i][2]);
          break;

        case 'r':
          Radius = atof(&argv[i][2]);
          break;

        case 'W':
          Width  = atof(&argv[i][2]);
          break;
        case 'H':
          Height = atof(&argv[i][2]);
          break;

        default:
          fprintf(LogFilePtr,"ERROR: unknown command option: %s\n",argv[i]);
          exit(-1);
          break;
      }
    }
  }

  Endpoint.A = 1.0;
  Endpoint.B = 0.0;
  Endpoint.C = 0.0;
  Endpoint.D = -1.0*DistMove;
}

  

	    

      
 



      



