/***********************FUNCTION*FOR*MODULE*LENSE*******************************************************************************/
/*********************************************************************************************************************************/
/************FUNCTION*FOR*MOVING*NEUTRONS*****************************************************************/

 
#include "intersection.h"
#include "init.h"

#ifdef VT_GRAPH
# include "cpgplot.h"
int do_visualise; /* default : no visualisation */
#endif
 
void gsl_ran_dir_3d (const gsl_rng * r, double * x, double * y, double * z);

/* Please REMEMBER: Structure definition also exists 
   in the external functions!!! IF YOU CHANGED HERE, PLEASE CHANDE IN
   OTHER FUNCTIONS! */
  
/* Structure for describe mirror */

typedef struct 
{ 
  SurfaceSecond Surf[5]; 
} 
  LenseSecond; 


/* 1 surface: first surface of lense 0
   2 surface: second surface of lense 1
   3 surface: cylindrical surface of lense 2
   4 surface: output plane 3   		*/
 
  
/* prototypes */
double MakeRefract(Neutron *, LenseSecond, double, double, long );
double SurfaceRougnn(double *, double *, double *, double );
double NeutronSurfaceSecIntersectionGrav(Neutron *, SurfaceSecond, long );
double NeutronSurfaceSecIntersectionGravLen(Neutron *, SurfaceSecond );
double NeutronMove(Neutron *, double );
double SolveQuadraticEq(double, double, double );

                               
double  PathThroughLenseOrder2(Neutron *ThisNeutron, LenseSecond MyLense, double Radius1, 
			       double Radius2, double RadiusMain, double Thickness, double Refract, double Atten, double AttScat,
			       VectorType PosMain, VectorType TransOut , double wei_min, double surfacerough,long keygrav, 
			       long NeutronLoss, long Attenkey,  long CurrentLense, long LenseForOut, long LenseForOutVis, 
			       long ServiceInfoK, FILE *COLLFILE, long LenseType, long KeyRefract )
			      				    
{ 

  /* Note! The function returns Time Of Flight					   */ 
  
 
  int i, ThisCollision=0; 
  double  TimeOF, TimeOFmin; 
  double  TimeOFTotal=0.0; 
  double  VelocityReal, Refr, Att, signl, stepp, temp;
  
  /* Local copy of neutrons for cylce.. */ 
  Neutron TempNeutron, NearestNeutron; 

  /* Neutron is lossed, other (next) lenses are excluded */
  if (NeutronLoss == 1) return(-1.0);
  
  /* illegal velocity */ 
  if (ThisNeutron->Vector[0] <= 0.0)  return(-1.0);
  
  /* illegal wavelength */  
  if (ThisNeutron->Wavelength <= 0.0)  return(-1.0);

  
  /* Calculate refraction and attenuation coeff for given wavelength */
  Refr = 1.0 - ((ThisNeutron->Wavelength)*(ThisNeutron->Wavelength)*Refract);
  if (Refr <= 0.0) return(-1.0);
  
  Att = Atten*(ThisNeutron->Wavelength);
  if (Att < 0.0) return(-1.0);
  
  /* Choose small constat for movement*/
		   
  stepp = 0.01; /* in cm */
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
	    

  /*    set initial tof for lense */      
  TimeOFTotal=0.0;
      
  /* Moving into additional plane if necessary */         

  if (LenseType == 0)
    {
      if ((MyLense.Surf[4].W < 0.0)&&(Radius1 > 0.0))
	{
	  TimeOF = NeutronSurfaceSecIntersectionGrav(ThisNeutron, MyLense.Surf[4], keygrav);
	  if (TimeOF <= 0.0) return(-1.0);
	  TimeOFTotal = TimeOFTotal + TimeOF; 
	}
    }    
      
  /* Starting of moving into the first surface (number 0 -> [0]) of the lense */  


  if (keygrav == 1)
    {
      TimeOF = NeutronSurfaceSecIntersectionGravLen(ThisNeutron, MyLense.Surf[0]);            
    }
  else
    {
      TimeOF = NeutronSurfaceSecIntersectionGrav(ThisNeutron, MyLense.Surf[0], 0);      
    }
          
  if (TimeOF <= 0.0) return(-1.0);
  TimeOFTotal = TimeOFTotal + TimeOF; 
  if (ThisNeutron->Probability <= wei_min)  return(-1.0);       

  /* check the neutron inside cylinder, otherwise neutron loss  */ 
 
  signl = 
    MyLense.Surf[2].A*ThisNeutron->Position[0]*ThisNeutron->Position[0] + 
    MyLense.Surf[2].B*ThisNeutron->Position[0] + 
    MyLense.Surf[2].C*ThisNeutron->Position[1]*ThisNeutron->Position[1]+ 
    MyLense.Surf[2].D*ThisNeutron->Position[1] + 
    MyLense.Surf[2].E*ThisNeutron->Position[2]*ThisNeutron->Position[2]+ 
    MyLense.Surf[2].F*ThisNeutron->Position[2] + 
    MyLense.Surf[2].W + 
    MyLense.Surf[2].P*ThisNeutron->Position[0]*ThisNeutron->Position[1]+ 
    MyLense.Surf[2].Q*ThisNeutron->Position[1]*ThisNeutron->Position[2]+ 
    MyLense.Surf[2].R*ThisNeutron->Position[2]*ThisNeutron->Position[0] ; 
  /*       fprintf(LogFilePtr,"signs l = %f \n",signl); */
    
  /* particle outside cylinder surface */ 
  if ((signl) >= 0.0) return(-1.0);  
 
  /*	fprintf(LogFilePtr,"weight min  %e \n",wei_min);*/ 

  /* Move a neutron a little bit and check the outside cylynder */  
  
  CopyNeutron(ThisNeutron, &TempNeutron); 
  
  TempNeutron.Position[0] = TempNeutron.Position[0] + VelocityReal*stepp*(TempNeutron.Vector[0]); 
  TempNeutron.Position[1] = TempNeutron.Position[1] + VelocityReal*stepp*(TempNeutron.Vector[1]); 
  TempNeutron.Position[2] = TempNeutron.Position[2] + VelocityReal*stepp*(TempNeutron.Vector[2]); 
	
  /* check the neutron inside cylinder, otherwise neutron loss  */ 
 
  signl = 
    MyLense.Surf[2].A*TempNeutron.Position[0]*TempNeutron.Position[0] + 
    MyLense.Surf[2].B*TempNeutron.Position[0] + 
    MyLense.Surf[2].C*TempNeutron.Position[1]*TempNeutron.Position[1]+ 
    MyLense.Surf[2].D*TempNeutron.Position[1] + 
    MyLense.Surf[2].E*TempNeutron.Position[2]*TempNeutron.Position[2]+ 
    MyLense.Surf[2].F*TempNeutron.Position[2] + 
    MyLense.Surf[2].W + 
    MyLense.Surf[2].P*TempNeutron.Position[0]*TempNeutron.Position[1]+ 
    MyLense.Surf[2].Q*TempNeutron.Position[1]*TempNeutron.Position[2]+ 
    MyLense.Surf[2].R*TempNeutron.Position[2]*TempNeutron.Position[0] ; 
  /*       fprintf(LogFilePtr,"signs l = %f \n",signl); */
    
  /* particle outside cylinder surface */ 
  if ((signl) >= 0.0) return(-1.0);    


#ifdef VT_GRAPH 
  if (do_visualise) 
    { 
      double tempx, tempy; 
 
      /* set point */ 
    
      if (LenseForOutVis == 0)
	{
	  cpgsci(6); 
	  tempx = ThisNeutron->Position[0]; 
	  tempy = ThisNeutron->Position[2]; 
	  cpgpt1( (float) tempx,(float) tempy,-2 ); 
	}
      else
	{
	  if ( CurrentLense == LenseForOutVis )
	    {
	      cpgsci(6); 
	      tempx = ThisNeutron->Position[0]; 
	      tempy = ThisNeutron->Position[2]; 
	      cpgpt1( (float) tempx,(float) tempy,-2 ); 	       
	    }
	}
        
    } 
#endif 


  if( ServiceInfoK == 1 ) 
    {
	
      if (LenseForOut == 0)
	{
	  fprintf(COLLFILE, " %ld  %12.5f  %12.5f  %12.5f  %d \n", 
		  CurrentLense, 
		  ThisNeutron->Position[0], 
		  ThisNeutron->Position[1], 
		  ThisNeutron->Position[2], 
		  ThisNeutron->Color);      
	}
      else
	{
	  if ( CurrentLense == LenseForOut )
	    {
	      fprintf(COLLFILE, " %ld  %12.5f  %12.5f  %12.5f  %d \n", 
		      CurrentLense, 
		      ThisNeutron->Position[0], 
		      ThisNeutron->Position[1], 
		      ThisNeutron->Position[2], 
		      ThisNeutron->Color);      	       
	    }
	}
	    		
    }   	        

  /* TEMPORARY STOP see above two signl 
     return(-1.0); */


  /* make refraction at the first surface */
  
  temp = MakeRefract(ThisNeutron, MyLense, Refr, surfacerough, 0);
  if (temp == -1.0) return(-1.0); 

 
  TimeOFmin = 99999999999999999999999.9; 
  /* Loop through all 2 surface */                                              
  for(i=1;i<3;i++) 
    { 
    
      CopyNeutron(ThisNeutron, &TempNeutron); 
      
      if (keygrav == 1)
	{
          TimeOF = NeutronSurfaceSecIntersectionGravLen(&TempNeutron, MyLense.Surf[i]);            
	}
      else
	{
          TimeOF = NeutronSurfaceSecIntersectionGrav(&TempNeutron, MyLense.Surf[i], 0);      
	}          
      
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
  /*    	fprintf(LogFilePtr,"Number plane of coll: %d \n",ThisCollision); */

  /* Neutron is hitted the cylindrical surface */
  if (ThisCollision == 2) 
    {
      
      CopyNeutron(&NearestNeutron, ThisNeutron); 
      TimeOFTotal =  TimeOFTotal + TimeOFmin; 
            
      /* Attenuation inside lense */
      if (Attenkey == 1)
	{
	  ThisNeutron->Probability = ThisNeutron->Probability*exp(-1.0*(Att+AttScat)*VelocityReal*TimeOFmin);
	}
      /* check weight */
      if (ThisNeutron->Probability <= wei_min)  return(-1.0);       

#ifdef VT_GRAPH 
      if (do_visualise) 
	{ 
	  double tempx, tempy; 
 
	  /* set point */ 
    
	  cpgsci(4); 
    
	  tempx = ThisNeutron->Position[0]; 
	  tempy = ThisNeutron->Position[2]; 
    
	  cpgpt1( (float) tempx,(float) tempy,-2 ); 
	} 
#endif 
            
      temp = MakeRefract(ThisNeutron, MyLense, (1.0/Refr), surfacerough, 2);
      if (temp == -1.0) return(-1.0); 
            
      TimeOF = NeutronSurfaceSecIntersectionGrav(ThisNeutron, MyLense.Surf[3], keygrav);
      if (TimeOF <= 0.0) return(-1.0);            
      TimeOFTotal = TimeOFTotal + TimeOF; 
            
      /* Convert position into output frame */     
      ThisNeutron->Position[0] = ThisNeutron->Position[0]  - TransOut[0];
	    	    
      return(-1.0);
    }	


            
  /* Neutrons going in the second surface of lense with refraction */
      
  CopyNeutron(&NearestNeutron, ThisNeutron); 
  TimeOFTotal =  TimeOFTotal + TimeOFmin; 
      
  /* attenuation inside lense */
  if (Attenkey == 1)
    {
      ThisNeutron->Probability = ThisNeutron->Probability*exp(-1.0*(Att+AttScat)*VelocityReal*TimeOFmin);
    }
      
  /* check weight */
  if (ThisNeutron->Probability <= wei_min)  return(-1.0);       


#ifdef VT_GRAPH 
  if (do_visualise) 
    { 
      double tempx, tempy; 
 
      /* set point */ 
	
      if (LenseForOutVis == 0)
	{
	  cpgsci(5); 
	  tempx = ThisNeutron->Position[0]; 
	  tempy = ThisNeutron->Position[2]; 
	  cpgpt1( (float) tempx,(float) tempy,-2 ); 	    
	}
      else
	{
	  if ( CurrentLense == LenseForOutVis )
	    {
	      cpgsci(5); 
	      tempx = ThisNeutron->Position[0]; 
	      tempy = ThisNeutron->Position[2]; 
	      cpgpt1( (float) tempx,(float) tempy,-2 ); 	       
	    }
	}
	
    } 
#endif 

  if( ServiceInfoK == 1 ) 
    {
      if (LenseForOut == 0)
	{
	  fprintf(COLLFILE, " %ld  %12.5f  %12.5f  %12.5f  %d \n", 
		  CurrentLense, 
		  ThisNeutron->Position[0], 
		  ThisNeutron->Position[1], 
		  ThisNeutron->Position[2], 
		  ThisNeutron->Color);      
	}
      else
	{
	  if ( CurrentLense == LenseForOut )
	    {
	      fprintf(COLLFILE, " %ld  %12.5f  %12.5f  %12.5f  %d \n", 
		      CurrentLense, 
		      ThisNeutron->Position[0], 
		      ThisNeutron->Position[1], 
		      ThisNeutron->Position[2], 
		      ThisNeutron->Color);      	       
	    }
	}
	
    }   	        
      
  /* refraction at the exit surface */    
  
  temp = MakeRefract(ThisNeutron, MyLense, (1.0/Refr), surfacerough, 1);
  if (temp == -1.0) return(-1.0); 
      
      
  /* check the neutron inside cylinder, otherwise neutron loss  */ 
 
  signl = 
    MyLense.Surf[2].A*ThisNeutron->Position[0]*ThisNeutron->Position[0] + 
    MyLense.Surf[2].B*ThisNeutron->Position[0] + 
    MyLense.Surf[2].C*ThisNeutron->Position[1]*ThisNeutron->Position[1]+ 
    MyLense.Surf[2].D*ThisNeutron->Position[1] + 
    MyLense.Surf[2].E*ThisNeutron->Position[2]*ThisNeutron->Position[2]+ 
    MyLense.Surf[2].F*ThisNeutron->Position[2] + 
    MyLense.Surf[2].W + 
    MyLense.Surf[2].P*ThisNeutron->Position[0]*ThisNeutron->Position[1]+ 
    MyLense.Surf[2].Q*ThisNeutron->Position[1]*ThisNeutron->Position[2]+ 
    MyLense.Surf[2].R*ThisNeutron->Position[2]*ThisNeutron->Position[0] ; 
    
  /* particle outside cylinder surface */ 
  if ((signl) >= 0.0) return(-1.0);        


  /* neutron is out of lense */
  /* transport neutrons into output plane */

  TimeOF = NeutronSurfaceSecIntersectionGrav(ThisNeutron, MyLense.Surf[3], keygrav);
  if (TimeOF <= 0.0) return(-1.0);      
  TimeOFTotal = TimeOFTotal + TimeOF; 



#ifdef VT_GRAPH 
  if (do_visualise) 
    { 
      double tempx, tempy; 
      /* set point */ 
     
      if (LenseForOutVis == 0)
        {
          cpgsci(3); 
          tempx = ThisNeutron->Position[0]; 
          tempy = ThisNeutron->Position[2]; 
          cpgpt1( (float) tempx,(float) tempy,-2 );         
        }
      else
        {
	  if ( CurrentLense == LenseForOutVis )
            {
              cpgsci(3); 
              tempx = ThisNeutron->Position[0]; 
              tempy = ThisNeutron->Position[2]; 
              cpgpt1( (float) tempx,(float) tempy,-2 );             
            }
        }
         
    } 
#endif       


  //       fprintf(LogFilePtr,"Translout before 012   %f  %f  %f  \n", ThisNeutron->Position[0], ThisNeutron->Position[1], ThisNeutron->Position[2]); 
  /* Convert position into output frame */     
 
  ThisNeutron->Position[0] = ThisNeutron->Position[0]  - TransOut[0];
      
  //       fprintf(LogFilePtr,"Translout after  012   %f  %f  %f  \n", ThisNeutron->Position[0], ThisNeutron->Position[1], ThisNeutron->Position[2]); 
  /* end of lense, return tof for current lense */

  return(TimeOFTotal);

} 


/*   function to make refraction procedure
     refraction formula has taken from wikipedia   */    

double MakeRefract(Neutron *ThisNeutron, LenseSecond MyLense, double Refr, double surfacerough, long sn)
{
    
  double AP, BP, CP, FP, DOTP, DOTPnew;
  double coss, cossn, cossn1, sinn, temp;

  
  /* illegal velocity */ 
  if (ThisNeutron->Vector[0] < 0.0)  return(-1.0);
    
  /* No refraction -> any velocity components change */
  if (Refr == 1.0) return(1.0);
  /* Refr - ratio(new/old) of indices of refraction */
	    
  /* calculate the normal vector in surface which will be refracted/reflected 
     sn - is the number of surface in lense */ 
 
  AP = 2.0*MyLense.Surf[sn].A*ThisNeutron->Position[0] 
    + MyLense.Surf[sn].B 
    + MyLense.Surf[sn].P*ThisNeutron->Position[1] 
    + MyLense.Surf[sn].R*ThisNeutron->Position[2]; 
 
 
  BP = 2.0*MyLense.Surf[sn].C*ThisNeutron->Position[1] 
    + MyLense.Surf[sn].D 
    + MyLense.Surf[sn].P*ThisNeutron->Position[0] 
    + MyLense.Surf[sn].Q*ThisNeutron->Position[2]; 
 
 
  CP = 2.0*MyLense.Surf[sn].E*ThisNeutron->Position[2] 
    + MyLense.Surf[sn].F 
    + MyLense.Surf[sn].Q*ThisNeutron->Position[1] 
    + MyLense.Surf[sn].R*ThisNeutron->Position[0]; 
 
 
  /* Normalize normale vector to the reflection plane */	 
 
  FP = sqrt(AP*AP + BP*BP + CP*CP); 
  /*	FP = -1.0*FP; */
  if (FP == 0.0) return(-1.0); 
  AP = AP/FP; 
  BP = BP/FP; 
  CP = CP/FP; 
  
  /* influence of rough surface */		 
  temp = SurfaceRougnn(&AP, &BP, &CP, surfacerough) ;
  if (temp == -1.0) return(-1.0); 
	
  /* Velocity components normalisation */
	
  DOTP = ThisNeutron->Vector[0]*ThisNeutron->Vector[0] + 
    ThisNeutron->Vector[1]*ThisNeutron->Vector[1] + 
    ThisNeutron->Vector[2]*ThisNeutron->Vector[2];   
  DOTP = sqrt(DOTP);       
  if (DOTP == 0.0) return(-1.0);               
  ThisNeutron->Vector[0] = (ThisNeutron->Vector[0]/DOTP); 
  ThisNeutron->Vector[1] = (ThisNeutron->Vector[1]/DOTP); 
  ThisNeutron->Vector[2] = (ThisNeutron->Vector[2]/DOTP);		 
	
  coss = AP*ThisNeutron->Vector[0] + BP*ThisNeutron->Vector[1] + 
    CP*ThisNeutron->Vector[2]; /* scalar production -> cos angle of incidence */       
	
  /* No refraction: vectors of velocity and of normal are colinnear
     scalar production = +/- 1.0 -> Moving later : for lense */
  if (fabs(coss) == 1.0) return(1.0);      
	
  /* No refraction: vectors of velocity and of normal are perpindicular
     scalar production = 0.0  -> disregard such neutron */
  if (fabs(coss) == 0.0) return(-1.0);
               
  /*        fprintf(LogFilePtr,"====================================\n");
	    fprintf(LogFilePtr,"00 cos of incidence = %f acos = %f velmod = %f \n", coss, acos(coss), DOTP);	*/
	
  if (Refr > 0.0)
    {
      sinn = (1.0 - coss*coss)/Refr/Refr ; /* square of sine of refr. angle*/
      if (sinn < 1.0) 
	{
	  /* refraction */
	  cossn = sqrt(1.0 - sinn); /* cos angle of refraction */
	        	    
	  /*	        	    OLD kind of ident: if (coss < 0.0)  cossn = -1.0*cossn ;  if cos angle of refraction negative */
                            
	  if (coss > 0.0)  /* new kind of identification */
	    {
	      ThisNeutron->Vector[0] = (ThisNeutron->Vector[0]/Refr) + AP*(cossn - (coss/Refr));         	   
	      ThisNeutron->Vector[1] = (ThisNeutron->Vector[1]/Refr) + BP*(cossn - (coss/Refr)); 
	      ThisNeutron->Vector[2] = (ThisNeutron->Vector[2]/Refr) + CP*(cossn - (coss/Refr));		 
	    }
	  else
	    {
	      ThisNeutron->Vector[0] = (ThisNeutron->Vector[0]/Refr) - AP*(cossn + (coss/Refr));         	   
	      ThisNeutron->Vector[1] = (ThisNeutron->Vector[1]/Refr) - BP*(cossn + (coss/Refr)); 
	      ThisNeutron->Vector[2] = (ThisNeutron->Vector[2]/Refr) - CP*(cossn + (coss/Refr));		
	    }
		    
	  /* Velocity normalisation after refraction*/
			    
	  DOTPnew = ThisNeutron->Vector[0]*ThisNeutron->Vector[0] + 
	    ThisNeutron->Vector[1]*ThisNeutron->Vector[1] + 
	    ThisNeutron->Vector[2]*ThisNeutron->Vector[2];   
	  DOTPnew = sqrt(DOTPnew);               	 
	  if (DOTPnew == 0.0) return(-1.0);          
	  ThisNeutron->Vector[0] = (ThisNeutron->Vector[0]/DOTPnew); 
	  ThisNeutron->Vector[1] = (ThisNeutron->Vector[1]/DOTPnew); 
	  ThisNeutron->Vector[2] = (ThisNeutron->Vector[2]/DOTPnew);		 
	  cossn1 = AP*ThisNeutron->Vector[0] + BP*ThisNeutron->Vector[1] + 
	    CP*ThisNeutron->Vector[2]; 
		                       
	  /*        			fprintf(LogFilePtr,"1 cos of refraction = %f acos = %f   Refr = %f  \n", cossn, acos(cossn), Refr);
		    fprintf(LogFilePtr,"2 cos of refraction = %f acos = %f velmodnew = %f  \n", cossn1, acos(cossn1), DOTPnew);		    
		    fprintf(LogFilePtr,"----------------------------------------\n");	            */
	}
      else
	{
	  /* Critical angle reflection */
		    
	  coss = AP*ThisNeutron->Vector[0] + BP*ThisNeutron->Vector[1] + 
	    CP*ThisNeutron->Vector[2]; 
	  ThisNeutron->Vector[0] = ThisNeutron->Vector[0] - 2.0*coss*AP; 
	  ThisNeutron->Vector[1] = ThisNeutron->Vector[1] - 2.0*coss*BP; 
	  ThisNeutron->Vector[2] = ThisNeutron->Vector[2] - 2.0*coss*CP;		 
	  fprintf(LogFilePtr,"WARNING: critical angle reflection takes place \n");
	}
    }
  else
    {
      /* refraction coeff <= 0.0 ->>> reflection */
      coss = AP*ThisNeutron->Vector[0] + BP*ThisNeutron->Vector[1] + 
	CP*ThisNeutron->Vector[2]; 
      ThisNeutron->Vector[0] = ThisNeutron->Vector[0] - 2.0*coss*AP; 
      ThisNeutron->Vector[1] = ThisNeutron->Vector[1] - 2.0*coss*BP; 
      ThisNeutron->Vector[2] = ThisNeutron->Vector[2] - 2.0*coss*CP;		 
      fprintf(LogFilePtr,"WARNING: reflection takes place on negative refraction coeff. \n");
    }
    
  return(1.0);    
}



/* function for simulations of surface rougness */

double SurfaceRougnn(double *AP, double *BP, double *CP, double surfacerough)
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


////////////////////////////////////////////
double NeutronSurfaceSecIntersectionGrav(Neutron *ThisNeutron, SurfaceSecond ThisSurfaceSecond, long keygrav)
{
  /***********************************************************************************/
  /* This part calculates the time of flight of neutron INCLUDE gravity with   */
  /* plane.	This functions is core for transporting neutrons!									   */
  /***********************************************************************************/

  double  Time, AA, BB, CC, VelocityReal;
  double  VX, VY, VZ, X, Y, Z;
  double  NewWavelength, OldWavelength, DOTP, DOTPnew;
		
  /*	Make the koefficients of quadratic equation    */
  /*	G = 9.8 m/c**2, we need to cm/ms**2 (100/1000/1000)  */
  /*	Velocity cm/ms, Time ms, Position cm	*/

  /*	Calculating real velocity, cm/ms  */
		
  VelocityReal = (double)(V_FROM_LAMBDA(ThisNeutron->Wavelength));
  OldWavelength = ThisNeutron->Wavelength ;
		
  /*	Components of velocity, projections, and position coordinats */
  /*	Local copy */
		
  X = ThisNeutron->Position[0];
  Y = ThisNeutron->Position[1];
  Z = ThisNeutron->Position[2];
		
  VX = VelocityReal*ThisNeutron->Vector[0];
  VY = VelocityReal*ThisNeutron->Vector[1];
  VZ = VelocityReal*ThisNeutron->Vector[2];
		
  /*	Find the coefficients of the quadratic equation */	
		
  if (keygrav == 1)
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

	
  //	fprintf(LogFilePtr,"AA = %f  BB = %f  CC = %f  \n", AA, BB, CC);

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
  if (keygrav == 1)
    {	
      ThisNeutron->Position[2] = ThisNeutron->Position[2] - 0.5*(G*1.0e-4)*Time*Time;
				
      /*OLD:		ThisNeutron->Vector[2] = ThisNeutron->Vector[2] - ((G*1.0e-4)*Time/VelocityReal);	*/	
	
      DOTP = ThisNeutron->Vector[0]*ThisNeutron->Vector[0] +
	ThisNeutron->Vector[1]*ThisNeutron->Vector[1] +
	ThisNeutron->Vector[2]*ThisNeutron->Vector[2] ;
		
      DOTP = sqrt(DOTP);
      if (DOTP == 0.0) return(-1.0);
		
      //		fprintf(LogFilePtr,"DOTPold = %f VelocityReal = %f \n", DOTP, VelocityReal);	
	
      ThisNeutron->Vector[0] = (ThisNeutron->Vector[0])*VelocityReal ;	
      ThisNeutron->Vector[1] = (ThisNeutron->Vector[1])*VelocityReal ;
      ThisNeutron->Vector[2] = (ThisNeutron->Vector[2])*VelocityReal 
	- ((G*1.0e-4)*Time) ;	
	
      DOTPnew = ThisNeutron->Vector[0]*ThisNeutron->Vector[0] +
	ThisNeutron->Vector[1]*ThisNeutron->Vector[1] +
	ThisNeutron->Vector[2]*ThisNeutron->Vector[2] ;
		
      DOTPnew = sqrt(DOTPnew);
      if (DOTPnew == 0.0) return(-1.0);
	
      NewWavelength = LAMBDA_FROM_V(DOTPnew);
	
      //		fprintf(LogFilePtr,"DOTPnew = %f  Wave_old = %f  Wave_new = %f \n", DOTPnew, OldWavelength, NewWavelength);
      //		fprintf(LogFilePtr,"---------------------------------------------------\n");
		
      ThisNeutron->Wavelength	= NewWavelength;	
      ThisNeutron->Vector[0] = (ThisNeutron->Vector[0])/DOTPnew ;	
      ThisNeutron->Vector[1] = (ThisNeutron->Vector[1])/DOTPnew ;
      ThisNeutron->Vector[2] = (ThisNeutron->Vector[2])/DOTPnew ;
	
    }
  /* return Time; ms */

  return Time;
}

/* Procedure for moving the neutron WITH gravity on time Time */

double NeutronMove(Neutron *ThisNeutron, double Time)
{

  double  VelocityReal;
  double  NewWavelength, OldWavelength, DOTP, DOTPnew;
		
  /*	Make the koefficients of quadratic equation    */
  /*	G = 9.8 m/c**2, we need to cm/ms**2 (100/1000/1000)  */
  /*	Velocity cm/ms, Time ms, Position cm	*/

  /*	Calculating real velocity, cm/ms  */
		
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
  ThisNeutron->Vector[2] = (ThisNeutron->Vector[2])*VelocityReal 
    - ((G*1.0e-4)*Time) ;	
	
  DOTPnew = ThisNeutron->Vector[0]*ThisNeutron->Vector[0] +
    ThisNeutron->Vector[1]*ThisNeutron->Vector[1] +
    ThisNeutron->Vector[2]*ThisNeutron->Vector[2] ;
		
  DOTPnew = sqrt(DOTPnew);
  if (DOTPnew == 0.0) return(-1.0);

	
  NewWavelength = LAMBDA_FROM_V(DOTPnew);
  ThisNeutron->Wavelength	= NewWavelength;	
  ThisNeutron->Vector[0] = (ThisNeutron->Vector[0])/DOTPnew ;	
  ThisNeutron->Vector[1] = (ThisNeutron->Vector[1])/DOTPnew ;
  ThisNeutron->Vector[2] = (ThisNeutron->Vector[2])/DOTPnew ;
	
  return(1.0);
}



double NeutronSurfaceSecIntersectionGravLen(Neutron *ThisNeutron, SurfaceSecond ThisSurfaceSecond)
{
  double  Time, Time1, Time2, Time22, AA, BB, CC, VelocityReal;
  double  TimeInit, TimeInit1, TimeCalc, diss, disscalc, di;
  double  VX, VY, VZ, X, Y, Z;
  double  Xnew, Ynew, Znew, AP, BP, CP, FP;
  long	countt, countt_more, countt_less;
  SurfaceSecond NormalPlane ;
  Neutron TempNeutron, TempNeutron1 ;
		
  /* Some constants */	
  diss = 0.01; /* in cm */
  countt_more = 50;
  countt_less = 500;
  TimeCalc = 0.0; /* total time of flight -> returned by the func */
	
  countt = 1; /* variable for while cycles */
	
  /*	Make the koefficients of quadratic equation    */
  /*	G = 9.8 m/c**2, we need to cm/ms**2 (100/1000/1000)  */
  /*	Velocity cm/ms, Time ms, Position cm	*/

  /*	Calculating real velocity, cm/ms  */
		
	
  VelocityReal = (double)(V_FROM_LAMBDA(ThisNeutron->Wavelength));
  if (VelocityReal == 0.0) return(-1.0);
		
  /*	Components of velocity, projections, and position coordinats */
  /*	Local copy */
		
  X = ThisNeutron->Position[0];
  Y = ThisNeutron->Position[1];
  Z = ThisNeutron->Position[2];
		
  VX = VelocityReal*ThisNeutron->Vector[0];
  VY = VelocityReal*ThisNeutron->Vector[1];
  VZ = VelocityReal*ThisNeutron->Vector[2];


  /* First task: find the coordinates of the point, where the current neutron path is crossed with
     surface WITHOUT gravity	*/		
   
  /*	Find the coefficients of the quadratic equation */	
		
	    
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
     perpindicular of this point -> calculate the normal	 */

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
      
      
  /*      fprintf(LogFilePtr,"------------------------------------------\n");
	  fprintf(LogFilePtr,"CROSS POINT: Xnew =  %f  Ynew =  %f  Znew =  %f  Time =  %f \n", Xnew, Ynew, Znew, Time);
	  fprintf(LogFilePtr,"NORMAL: AP = %f  BP = %f  CP = %f \n", AP, BP, CP);
	  fprintf(LogFilePtr,"PLANE POINT : Xppp =  %f  Yppp =  %f  Zppp =  %f  Time1 = %f \n", TempNeutron.Position[0], 
	  TempNeutron.Position[1], TempNeutron.Position[2], Time1);*/
      
      
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
      /*              fprintf(LogFilePtr,"FINAL POINT :  Xfffff =  %f   Yffff =  %f  Zffff =  %f  Time2 = %f \n", TempNeutron1.Position[0], 
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

