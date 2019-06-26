/* The free non-commercial use of these routines is granted */
/* providing due credit is given to the authors.            */
/* Author: Géza Zsigmond, last change JUL 2002              */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "general.h"
#include "init.h"
#include "matrix.h"
#include "intersection.h"

/* START HEADER STORY */

#define	STRING_BUFFER 1000

	FILE		*Par_Field, *COLLFILE;
	char		Option[STRING_BUFFER], *ParameterFileName, *ReflUpFileName, *ReflDownFileName, COLLFILEName[STRING_BUFFER];
	int			j, p, datanumber;
	long		User, NumOut, Repetition, repet,  i, m, nocol, nocolM = 10000, Wallonoff, NoCh, ok;
	double		TOF, TOFprec, WL, Prob, phi, the;
	double		rupdata[1001], rdowndata[1001], OutputAngleHoriz, OutputAngleVert, RotMatrixOut[3][3];
	double		IntegralIntensity, Path, Path0;
	double      prob_A_, prob_B_, prob_C_, prob_D_, prob_E_, prob_F_, prob_G_, prob_H_, prob_I_, prob_J_;
	VectorType	Pos, Dir, SpinVector, n, TranslOutput;
	Neutron		Neutrons;

	VectorType	WallOffset_A_, WallNormal_A_, r1_A_, r2_A_, r3_A_, r4_A_;
	double		RotMatrixWall_A_[3][3], WallVert_A_, WallHoriz_A_, Path_A_, thetaC_A_[2], thetaCSM_A_[2], RthetaCSM_A_[2], mued_A_[4], mrangh_A_, mrangv_A_; int Wallonoff_A_=0;

	VectorType	WallOffset_B_, WallNormal_B_, r1_B_, r2_B_, r3_B_, r4_B_;
	double		RotMatrixWall_B_[3][3], WallVert_B_, WallHoriz_B_, Path_B_, thetaC_B_[2], thetaCSM_B_[2], RthetaCSM_B_[2], mued_B_[4], mrangh_B_, mrangv_B_; int Wallonoff_B_=0;

	VectorType	WallOffset_C_, WallNormal_C_, r1_C_, r2_C_, r3_C_, r4_C_;
	double		RotMatrixWall_C_[3][3], WallVert_C_, WallHoriz_C_, Path_C_, thetaC_C_[2], thetaCSM_C_[2], RthetaCSM_C_[2], mued_C_[4], mrangh_C_, mrangv_C_; int Wallonoff_C_=0;

	VectorType	WallOffset_D_, WallNormal_D_, r1_D_, r2_D_, r3_D_, r4_D_;
	double		RotMatrixWall_D_[3][3], WallVert_D_, WallHoriz_D_, Path_D_, thetaC_D_[2], thetaCSM_D_[2], RthetaCSM_D_[2], mued_D_[4], mrangh_D_, mrangv_D_; int Wallonoff_D_=0;

	VectorType	WallOffset_E_, WallNormal_E_, r1_E_, r2_E_, r3_E_, r4_E_;
	double		RotMatrixWall_E_[3][3], WallVert_E_, WallHoriz_E_, Path_E_, thetaC_E_[2], thetaCSM_E_[2], RthetaCSM_E_[2], mued_E_[4], mrangh_E_, mrangv_E_; int Wallonoff_E_=0;

	VectorType	WallOffset_F_, WallNormal_F_, r1_F_, r2_F_, r3_F_, r4_F_;
	double		RotMatrixWall_F_[3][3], WallVert_F_, WallHoriz_F_, Path_F_, thetaC_F_[2], thetaCSM_F_[2], RthetaCSM_F_[2], mued_F_[4], mrangh_F_, mrangv_F_; int Wallonoff_F_=0;

	VectorType	WallOffset_G_, WallNormal_G_, r1_G_, r2_G_, r3_G_, r4_G_;
	double		RotMatrixWall_G_[3][3], WallVert_G_, WallHoriz_G_, Path_G_, thetaC_G_[2], thetaCSM_G_[2], RthetaCSM_G_[2], mued_G_[4], mrangh_G_, mrangv_G_; int Wallonoff_G_=0;

	VectorType	WallOffset_H_, WallNormal_H_, r1_H_, r2_H_, r3_H_, r4_H_;
	double		RotMatrixWall_H_[3][3], WallVert_H_, WallHoriz_H_, Path_H_, thetaC_H_[2], thetaCSM_H_[2], RthetaCSM_H_[2], mued_H_[4], mrangh_H_, mrangv_H_; int Wallonoff_H_=0;

	VectorType	WallOffset_I_, WallNormal_I_, r1_I_, r2_I_, r3_I_, r4_I_;
	double		RotMatrixWall_I_[3][3], WallVert_I_, WallHoriz_I_, Path_I_, thetaC_I_[2], thetaCSM_I_[2], RthetaCSM_I_[2], mued_I_[4], mrangh_I_, mrangv_I_; int Wallonoff_I_=0;

	VectorType	WallOffset_J_, WallNormal_J_, r1_J_, r2_J_, r3_J_, r4_J_;
	double		RotMatrixWall_J_[3][3], WallVert_J_, WallHoriz_J_, Path_J_, thetaC_J_[2], thetaCSM_J_[2], RthetaCSM_J_[2], mued_J_[4], mrangh_J_, mrangv_J_; int Wallonoff_J_=0;


	int		    hittriangle(VectorType r1, VectorType r2, VectorType rt);
	int		    hitwall(VectorType r1, VectorType r2, VectorType r3, VectorType r4, VectorType rt);
	double		CollideWall(double *prob, VectorType pos,VectorType dir,VectorType spin,VectorType WallOffset,VectorType WallNormal, double  RotMatrixWall[3][3],VectorType r1,VectorType r2,VectorType r3,VectorType r4, double thetaC[2], double thetaCSM[2], double RthetaCSM[2], double mued[4], double mrangh, double mrangv);
	double		reflectivity(double angle, double CritUp, double WL, double smearf, double ReflUp);
	void		OutputTransformations(double *tof, double *wl, double *prob, VectorType Pos, VectorType Dir, VectorType SpinVector);
	void		ReadParameterFile();
	void		OwnInit(int argc, char *argv[]);
	void		OwnCleanup();
	void	    CartesianToSpherical2(VectorType Vector, double *Theta, double *Phi);


/* FINISH HEADER STORY */


int main(int argc, char **argv)
{

 /* Initialize the program according to the parameters given  */ 

	Init(argc, argv);  
	OwnInit(argc, argv); 

	strcpy(COLLFILEName, "collisions.dat");  
	COLLFILE=fopen(COLLFILEName, "w");
	if(p==1)fprintf(COLLFILE, "   trj sp  wall    x/cm       y/cm        z/cm        dir y/°   dir z/° \n\n");


 /* Get the neutrons from the file */

  while((ReadNeutrons())!= 0)
  {
	/* here is what happens to the neutron */


for(i=0;i<NumNeutGot;i++)

{ 



	InputNeutrons[i].Vector[0]	= (double) sqrt(1 - sq(InputNeutrons[i].Vector[1]) - sq(InputNeutrons[i].Vector[2]));

	TOF = InputNeutrons[i].Time;

	WL = InputNeutrons[i].Wavelength;

	Prob = InputNeutrons[i].Probability; /*Prob = 1.;*/

	CopyVector(InputNeutrons[i].Position, Pos);

	CopyVector(InputNeutrons[i].Vector, Dir);

	CopyVector(InputNeutrons[i].Spin, SpinVector); 

	

/*  computes hitting positions on the walls */

	Path0 = 0.; m=0; nocol =0;
	{VectorType pos_A_, pos_B_, pos_C_, pos_D_, pos_E_, pos_F_, pos_G_, pos_H_, pos_I_, pos_J_,
				dir_A_, dir_B_ , dir_C_ , dir_D_, dir_E_, dir_F_ , dir_G_ , dir_H_, dir_I_, dir_J_,
				 spin_A_, spin_B_ , spin_C_ , spin_D_, spin_E_, spin_F_ , spin_G_ , spin_H_, spin_I_, spin_J_/**/; 
	 
	if(p==1)fprintf(COLLFILE, "    %ld  %d  0     %f  %f  %f     %f  %f \n", i, ((int) SpinVector[2]), Pos[0], Pos[1], Pos[2], 180./M_PI * atan2(Dir[1],Dir[0]), 180./M_PI * atan2(Dir[2],Dir[0]));


	for (j=0;j<1000;j++)
	{

	CopyVector(Pos, pos_A_); CopyVector(Dir, dir_A_); CopyVector(SpinVector, spin_A_); prob_A_= Prob; 
	if((m!= 1)&&(Wallonoff_A_==1))Path_A_ = CollideWall(&prob_A_, pos_A_, dir_A_, spin_A_, WallOffset_A_, WallNormal_A_, RotMatrixWall_A_, r1_A_, r2_A_, r3_A_, r4_A_, thetaC_A_, thetaCSM_A_, RthetaCSM_A_, mued_A_, mrangh_A_ , mrangv_A_); else Path_A_ = 99999;

	CopyVector(Pos, pos_B_); CopyVector(Dir, dir_B_);  CopyVector(SpinVector, spin_B_); prob_B_= Prob; 
	if((m!= 2)&&(Wallonoff_B_==1))Path_B_ = CollideWall(&prob_B_, pos_B_, dir_B_, spin_B_, WallOffset_B_, WallNormal_B_, RotMatrixWall_B_, r1_B_, r2_B_, r3_B_, r4_B_, thetaC_B_, thetaCSM_B_, RthetaCSM_B_, mued_B_, mrangh_B_ , mrangv_B_); else Path_B_ = 99999;

	CopyVector(Pos, pos_C_); CopyVector(Dir, dir_C_);  CopyVector(SpinVector, spin_C_); prob_C_= Prob; 
	if((m!= 3)&&(Wallonoff_C_==1))Path_C_ = CollideWall(&prob_C_, pos_C_, dir_C_, spin_C_, WallOffset_C_, WallNormal_C_, RotMatrixWall_C_, r1_C_, r2_C_, r3_C_, r4_C_, thetaC_C_, thetaCSM_C_, RthetaCSM_C_, mued_C_, mrangh_C_ , mrangv_C_); else Path_C_ = 99999;

	CopyVector(Pos, pos_D_); CopyVector(Dir, dir_D_);  CopyVector(SpinVector, spin_D_); prob_D_= Prob; 
	if((m!= 4)&&(Wallonoff_D_==1))Path_D_ = CollideWall(&prob_D_, pos_D_, dir_D_, spin_D_, WallOffset_D_, WallNormal_D_, RotMatrixWall_D_, r1_D_, r2_D_, r3_D_, r4_D_, thetaC_D_, thetaCSM_D_, RthetaCSM_D_, mued_D_, mrangh_D_ , mrangv_D_); else Path_D_ = 99999;

	CopyVector(Pos, pos_E_); CopyVector(Dir, dir_E_);  CopyVector(SpinVector, spin_E_); prob_E_= Prob; 
	if((m!= 5)&&(Wallonoff_E_==1))Path_E_ = CollideWall(&prob_E_, pos_E_, dir_E_, spin_E_, WallOffset_E_, WallNormal_E_, RotMatrixWall_E_, r1_E_, r2_E_, r3_E_, r4_E_, thetaC_E_, thetaCSM_E_, RthetaCSM_E_, mued_E_, mrangh_E_ , mrangv_E_); else Path_E_ = 99999;

	CopyVector(Pos, pos_F_); CopyVector(Dir, dir_F_);  CopyVector(SpinVector, spin_F_); prob_F_= Prob; 
	if((m!= 6)&&(Wallonoff_F_==1))Path_F_ = CollideWall(&prob_F_, pos_F_, dir_F_, spin_F_, WallOffset_F_, WallNormal_F_, RotMatrixWall_F_, r1_F_, r2_F_, r3_F_, r4_F_, thetaC_F_, thetaCSM_F_, RthetaCSM_F_, mued_F_, mrangh_F_ , mrangv_F_); else Path_F_ = 99999;

	CopyVector(Pos, pos_G_); CopyVector(Dir, dir_G_);  CopyVector(SpinVector, spin_G_); prob_G_= Prob; 
	if((m!= 7)&&(Wallonoff_G_==1))Path_G_ = CollideWall(&prob_G_, pos_G_, dir_G_, spin_G_, WallOffset_G_, WallNormal_G_, RotMatrixWall_G_, r1_G_, r2_G_, r3_G_, r4_G_, thetaC_G_, thetaCSM_G_, RthetaCSM_G_, mued_G_, mrangh_G_ , mrangv_G_); else Path_G_ = 99999;

	CopyVector(Pos, pos_H_); CopyVector(Dir, dir_H_);  CopyVector(SpinVector, spin_H_); prob_H_= Prob; 
	if((m!= 8)&&(Wallonoff_H_==1))Path_H_ = CollideWall(&prob_H_, pos_H_, dir_H_, spin_H_, WallOffset_H_, WallNormal_H_, RotMatrixWall_H_, r1_H_, r2_H_, r3_H_, r4_H_, thetaC_H_, thetaCSM_H_, RthetaCSM_H_, mued_H_, mrangh_H_ , mrangv_H_); else Path_H_ = 99999;

	CopyVector(Pos, pos_I_); CopyVector(Dir, dir_I_);  CopyVector(SpinVector, spin_I_); prob_I_= Prob; 
	if((m!= 9)&&(Wallonoff_I_==1))Path_I_ = CollideWall(&prob_I_, pos_I_, dir_I_, spin_I_, WallOffset_I_, WallNormal_I_, RotMatrixWall_I_, r1_I_, r2_I_, r3_I_, r4_I_, thetaC_I_, thetaCSM_I_, RthetaCSM_I_, mued_I_, mrangh_I_ , mrangv_I_); else Path_I_ = 99999;

	CopyVector(Pos, pos_J_); CopyVector(Dir, dir_J_);  CopyVector(SpinVector, spin_J_); prob_J_= Prob; 
	if((m!=10)&&(Wallonoff_J_==1))Path_J_ = CollideWall(&prob_J_, pos_J_, dir_J_, spin_J_, WallOffset_J_, WallNormal_J_, RotMatrixWall_J_, r1_J_, r2_J_, r3_J_, r4_J_, thetaC_J_, thetaCSM_J_, RthetaCSM_J_, mued_J_, mrangh_J_ , mrangv_J_); else Path_J_ = 99999;


	if((Path_A_ == 99999.0)&&(Path_B_ == 99999.0)&&(Path_C_ == 99999.0)&&(Path_D_ == 99999.0)&&(Path_E_ == 99999.0)&&(Path_F_ == 99999.0)&&(Path_G_ == 99999.0)&&(Path_H_ == 99999.0)&&(Path_I_ == 99999.0)&&(Path_J_ == 99999.0))
	{ Path = 99999.0; goto conti; if(p==1)fprintf(COLLFILE,"no collision\n");}


	if((m!=1)&&(Path_A_ <= Path_B_)&&(Path_A_ <= Path_C_)&&(Path_A_ <= Path_D_)&&(Path_A_ <= Path_E_)&&(Path_A_ <= Path_F_)&&(Path_A_ <= Path_G_)&&(Path_A_ <= Path_H_)&&(Path_A_ <= Path_I_)&&(Path_A_ <= Path_J_)) 
	{ CopyVector(pos_A_, Pos); CopyVector(dir_A_, Dir); Path = Path_A_; Prob = prob_A_; m=1; nocol +=1; if(p==1)fprintf(COLLFILE, "    %ld  %d  %ld     %f  %f  %f     %f  %f \n", i, ((int) SpinVector[2]) , m, Pos[0], Pos[1], Pos[2], 180./M_PI * atan2(Dir[1],Dir[0]), 180./M_PI * atan2(Dir[2],Dir[0]));/**/ }

	if((m!=2)&&(Path_B_ <= Path_C_)&&(Path_B_ <= Path_D_)&&(Path_B_ <= Path_E_)&&(Path_B_ <= Path_F_)&&(Path_B_ <= Path_G_)&&(Path_B_ <= Path_H_)&&(Path_B_ <= Path_I_)&&(Path_B_ <= Path_J_)&&(Path_B_ <= Path_A_)) 
	{ CopyVector(pos_B_, Pos); CopyVector(dir_B_, Dir); Path = Path_B_; Prob = prob_B_; m=2; nocol +=1; if(p==1)fprintf(COLLFILE, "    %ld  %d  %ld     %f  %f  %f     %f  %f \n", i, ((int) SpinVector[2]) , m, Pos[0], Pos[1], Pos[2], 180./M_PI * atan2(Dir[1],Dir[0]), 180./M_PI * atan2(Dir[2],Dir[0]));/**/ }

	if((m!=3)&&(Path_C_ <= Path_D_)&&(Path_C_ <= Path_E_)&&(Path_C_ <= Path_F_)&&(Path_C_ <= Path_G_)&&(Path_C_ <= Path_H_)&&(Path_C_ <= Path_I_)&&(Path_C_ <= Path_J_)&&(Path_C_ <= Path_A_)&&(Path_C_ <= Path_B_)) 
	{ CopyVector(pos_C_, Pos); CopyVector(dir_C_, Dir); Path = Path_C_; Prob = prob_C_; m=3; nocol +=1; if(p==1)fprintf(COLLFILE, "    %ld  %d  %ld     %f  %f  %f     %f  %f \n", i, ((int) SpinVector[2]) , m, Pos[0], Pos[1], Pos[2], 180./M_PI * atan2(Dir[1],Dir[0]), 180./M_PI * atan2(Dir[2],Dir[0]));/**/ }

	if((m!=4)&&(Path_D_ <= Path_E_)&&(Path_D_ <= Path_F_)&&(Path_D_ <= Path_G_)&&(Path_D_ <= Path_H_)&&(Path_D_ <= Path_I_)&&(Path_D_ <= Path_J_)&&(Path_D_ <= Path_A_)&&(Path_D_ <= Path_B_)&&(Path_D_ <= Path_C_)) 
	{ CopyVector(pos_D_, Pos); CopyVector(dir_D_, Dir); Path = Path_D_; Prob = prob_D_; m=4; nocol +=1; if(p==1)fprintf(COLLFILE, "    %ld  %d  %ld     %f  %f  %f     %f  %f \n", i, ((int) SpinVector[2]) , m, Pos[0], Pos[1], Pos[2], 180./M_PI * atan2(Dir[1],Dir[0]), 180./M_PI * atan2(Dir[2],Dir[0]));/**/ } 

	if((m!=5)&&(Path_E_ <= Path_F_)&&(Path_E_ <= Path_G_)&&(Path_E_ <= Path_H_)&&(Path_E_ <= Path_I_)&&(Path_E_ <= Path_J_)&&(Path_E_ <= Path_A_)&&(Path_E_ <= Path_B_)&&(Path_E_ <= Path_C_)&&(Path_E_ <= Path_D_)) 
	{ CopyVector(pos_E_, Pos); CopyVector(dir_E_, Dir); Path = Path_E_; Prob = prob_E_; m=5; nocol +=1; if(p==1)fprintf(COLLFILE, "    %ld  %d  %ld     %f  %f  %f     %f  %f \n", i, ((int) SpinVector[2]) , m, Pos[0], Pos[1], Pos[2], 180./M_PI * atan2(Dir[1],Dir[0]), 180./M_PI * atan2(Dir[2],Dir[0]));/**/ } 

	if((m!=6)&&(Path_F_ <= Path_G_)&&(Path_F_ <= Path_H_)&&(Path_F_ <= Path_I_)&&(Path_F_ <= Path_J_)&&(Path_F_ <= Path_A_)&&(Path_F_ <= Path_B_)&&(Path_F_ <= Path_C_)&&(Path_F_ <= Path_D_)&&(Path_F_ <= Path_E_)) 
	{ CopyVector(pos_F_, Pos); CopyVector(dir_F_, Dir); Path = Path_F_; Prob = prob_F_; m=6; nocol +=1; if(p==1)fprintf(COLLFILE, "    %ld  %d  %ld     %f  %f  %f     %f  %f \n", i, ((int) SpinVector[2]) , m, Pos[0], Pos[1], Pos[2], 180./M_PI * atan2(Dir[1],Dir[0]), 180./M_PI * atan2(Dir[2],Dir[0]));/**/ } 

	if((m!=7)&&(Path_G_ <= Path_H_)&&(Path_G_ <= Path_I_)&&(Path_G_ <= Path_J_)&&(Path_G_ <= Path_A_)&&(Path_G_ <= Path_B_)&&(Path_G_ <= Path_C_)&&(Path_G_ <= Path_D_)&&(Path_G_ <= Path_E_)&&(Path_G_ <= Path_F_)) 
	{ CopyVector(pos_G_, Pos); CopyVector(dir_G_, Dir); Path = Path_G_; Prob = prob_G_; m=7; nocol +=1; if(p==1)fprintf(COLLFILE, "    %ld  %d  %ld     %f  %f  %f     %f  %f \n", i, ((int) SpinVector[2]) , m, Pos[0], Pos[1], Pos[2], 180./M_PI * atan2(Dir[1],Dir[0]), 180./M_PI * atan2(Dir[2],Dir[0]));/**/ } 

	if((m!=8)&&(Path_H_ <= Path_I_)&&(Path_H_ <= Path_J_)&&(Path_H_ <= Path_A_)&&(Path_H_ <= Path_B_)&&(Path_H_ <= Path_C_)&&(Path_H_ <= Path_D_)&&(Path_H_ <= Path_E_)&&(Path_H_ <= Path_F_)&&(Path_H_ <= Path_G_)) 
	{ CopyVector(pos_H_, Pos); CopyVector(dir_H_, Dir); Path = Path_H_; Prob = prob_H_; m=8; nocol +=1; if(p==1)fprintf(COLLFILE, "    %ld  %d  %ld     %f  %f  %f     %f  %f \n", i, ((int) SpinVector[2]) , m, Pos[0], Pos[1], Pos[2], 180./M_PI * atan2(Dir[1],Dir[0]), 180./M_PI * atan2(Dir[2],Dir[0]));/**/ } 

	if((m!=9)&&(Path_I_ <= Path_J_)&&(Path_I_ <= Path_A_)&&(Path_I_ <= Path_B_)&&(Path_I_ <= Path_C_)&&(Path_I_ <= Path_D_)&&(Path_I_ <= Path_E_)&&(Path_I_ <= Path_F_)&&(Path_I_ <= Path_G_)&&(Path_I_ <= Path_H_)) 
	{ CopyVector(pos_I_, Pos); CopyVector(dir_I_, Dir); Path = Path_I_; Prob = prob_I_; m=9; nocol +=1; if(p==1)fprintf(COLLFILE, "    %ld  %d  %ld     %f  %f  %f     %f  %f \n", i, ((int) SpinVector[2]) , m, Pos[0], Pos[1], Pos[2], 180./M_PI * atan2(Dir[1],Dir[0]), 180./M_PI * atan2(Dir[2],Dir[0]));/**/ } 

	if((m!=10)&&(Path_J_ <= Path_A_)&&(Path_J_ <= Path_B_)&&(Path_J_ <= Path_C_)&&(Path_J_ <= Path_D_)&&(Path_J_ <= Path_E_)&&(Path_J_ <= Path_F_)&&(Path_J_ <= Path_G_)&&(Path_J_ <= Path_H_)&&(Path_J_ <= Path_I_)) 
	{ CopyVector(pos_J_, Pos); CopyVector(dir_J_, Dir); Path = Path_J_; Prob = prob_J_; m=10; nocol +=1; if(p==1)fprintf(COLLFILE, "    %ld  %d  %ld     %f  %f  %f     %f  %f \n", i, ((int) SpinVector[2]) , m, Pos[0], Pos[1], Pos[2], 180./M_PI * atan2(Dir[1],Dir[0]), 180./M_PI * atan2(Dir[2],Dir[0]));/**/ } 

	if(nocol == nocolM) goto conti; 
	
		if(Prob < wei_min) goto getlost; 

	Path0 += Path;

	}
conti:;

	}
	 	if(Prob < wei_min) goto getlost; 


/* transforms into output frame  */
	
	SubVector(Pos, TranslOutput);

	RotVector(RotMatrixOut, Pos);

	RotVector(RotMatrixOut, Dir);


	/* translates neutron variables for output - X'=0. */

	{

	VectorType Path ;

	TOF += - Pos[0] / fabs(Dir[0]) / V_FROM_LAMBDA(WL) ;
		
	CopyVector(Dir, Path) ;

	MultiplyByScalar(Path, - Pos[0]/ Dir[0] ) ;

	AddVector(Pos, Path) ;  
	
	}			/* Path = displacement vector */

	if(p==1)/* transforms into input frame to give here the exit positions */
	{
	VectorType posex, direx ;
	
	CopyVector(Pos, posex) ;

	CopyVector(Dir, direx) ;

	RotBackVector(RotMatrixOut, direx);	/**/

	RotBackVector(RotMatrixOut, posex);

	AddVector(posex, TranslOutput);
	
		fprintf(COLLFILE, "    %ld  %d  0     %f  %f  %f     %f  %f \n", i, ((int) SpinVector[2]), posex[0], posex[1], posex[2], 180./M_PI * atan2(direx[1],direx[0]), 180./M_PI * atan2(direx[2], direx[0]));
	}


	/* transmit coordinates which were not changed, the rest overwrite below */
	Neutrons = InputNeutrons[i]; 

	Neutrons.Time = TOF + Path0 / V_FROM_LAMBDA(WL);

	Neutrons.Probability = Prob;

	CopyVector(Pos, Neutrons.Position);

	CopyVector(Dir, Neutrons.Vector);

	Neutrons.Vector[0]	= (double) sqrt(1 - sq(Neutrons.Vector[1]) - sq(Neutrons.Vector[2]));

	CopyVector(SpinVector, Neutrons.Spin);


	/* writes output binary file ps(nocol);*/

	WriteNeutron(&Neutrons);  

getlost:;
		if(p==1) fprintf(COLLFILE, " \n");

}
  }
   
 /* Do the general cleanup */

	fclose(COLLFILE);	
	
	OwnCleanup(); 

	Cleanup();	

	fprintf(LogFilePtr," \n");


  return 0;
}




/* own initialization of the ensemble module */

void OwnInit(int argc, char *argv[])
{
	fprintf(LogFilePtr," \n");
	print_module_name("supermirror_ensemble");

/*    INPUT  */
	
	for(j=0;j<3;j++) TranslOutput[j]=0.;OutputAngleHoriz=OutputAngleVert=0.;


	while(argc>1)
	{
		switch(argv[1][1])
		{
				
			  case 'P':
					if((Par_Field = fopen(&argv[1][2],"r"))==NULL)
					{
						fprintf(LogFilePtr,"\nParameter file '%s' not found\n",&argv[1][2]);
						exit(0);
					}
					ParameterFileName=&argv[1][2];
					break;
		
			  case 'M':
					sscanf(&argv[1][2], "%ld", &nocolM); fprintf(LogFilePtr,"collisions: %ld\n", nocolM);
					break;

			  case 'T':
					sscanf(&argv[1][2], "%d", &p); 
					if (p==1)
						fprintf(LogFilePtr,"prints the coordinates\n");
					break;

			  case 'r':
					sscanf(&argv[1][2], "%lf", &TranslOutput[0]); 
					break;

			  case 's':
					sscanf(&argv[1][2], "%lf", &TranslOutput[1]); 
					break;

			  case 't':
					sscanf(&argv[1][2], "%lf", &TranslOutput[2]); 
					break;

			  case 'h':
					sscanf(&argv[1][2], "%lf", &OutputAngleHoriz); 
					break;

			  case 'v':
					sscanf(&argv[1][2], "%lf", &OutputAngleVert); 
					break;

				}
				argc--;
				argv++;
	}
	

/*	 init for ASCII output */


	NumOut=0;

	IntegralIntensity = 0.; Path = 0.; 

	OutputAngleHoriz	*= M_PI/180.;
	OutputAngleVert	*= M_PI/180.;
	FillRotMatrixZY(RotMatrixOut, OutputAngleVert, OutputAngleHoriz);

	

	/* reads parameter file */

	ReadParameterFile(); 


/**/

	if(Par_Field != NULL)fclose(Par_Field);

/*	 checks some values */




}/* End OwnInit */


/* own cleanup of the monochromator/analyser module */

void OwnCleanup()
{

}/* End OwnCleanup */


/* ReadParameterFile() reads the parameters from file */

void ReadParameterFile()
{

		/*ReadParComment(Par_Field);  parameter names*/

		r1_A_[0] = r2_A_[0] = r3_A_[0] = r4_A_[0] = 0;
		Wallonoff_A_=ReadParI(Par_Field);/* */
		r1_A_[1]=ReadParF(Par_Field); r1_A_[2]=ReadParF(Par_Field);
		r2_A_[1]=ReadParF(Par_Field); r2_A_[2]=ReadParF(Par_Field);
		r3_A_[1]=ReadParF(Par_Field); r3_A_[2]=ReadParF(Par_Field);
		r4_A_[1]=ReadParF(Par_Field); r4_A_[2]=ReadParF(Par_Field); 
		WallOffset_A_[0]=ReadParF(Par_Field); WallOffset_A_[1]=ReadParF(Par_Field); WallOffset_A_[2]=ReadParF(Par_Field); 
		WallHoriz_A_=ReadParF(Par_Field); WallVert_A_=ReadParF(Par_Field); 
		mrangh_A_=ReadParF(Par_Field); mrangv_A_=ReadParF(Par_Field); 
		thetaC_A_[0]=ReadParF(Par_Field); thetaCSM_A_[0]=ReadParF(Par_Field); RthetaCSM_A_[0]=ReadParF(Par_Field); mued_A_[0]=ReadParF(Par_Field); mued_A_[1]=ReadParF(Par_Field); 
		thetaC_A_[1]=ReadParF(Par_Field); thetaCSM_A_[1]=ReadParF(Par_Field); RthetaCSM_A_[1]=ReadParF(Par_Field); mued_A_[2]=ReadParF(Par_Field); mued_A_[3]=ReadParF(Par_Field);  
		if(Wallonoff_A_ == 1)
		{
		fprintf(LogFilePtr,"\nA:   %f %f   %f %f   %f %f   %f %f   %f %f %f   %f %f   %f %f   %f %f  %f  %f  %f  %f  %f  %f  %f %f \n", r1_A_[1],r1_A_[2],r2_A_[1],r2_A_[2],r3_A_[1],r3_A_[2],r4_A_[1],r4_A_[2],WallOffset_A_[0],WallOffset_A_[1],WallOffset_A_[2], WallHoriz_A_,WallVert_A_, mrangh_A_, mrangv_A_, thetaC_A_[0], thetaCSM_A_[0], RthetaCSM_A_[0], mued_A_[0], mued_A_[1], thetaC_A_[1], thetaCSM_A_[1], RthetaCSM_A_[1], mued_A_[2], mued_A_[3]);
		}
		ReadParComment(Par_Field); 
		WallHoriz_A_	*= M_PI/180.;
		WallVert_A_	*= M_PI/180.;
		mrangh_A_	*= M_PI/180.;
		mrangv_A_	*= M_PI/180.;
		FillRotMatrixZY(RotMatrixWall_A_, WallVert_A_, WallHoriz_A_);
		EulerToCartesianZY(WallNormal_A_, &WallVert_A_, &WallHoriz_A_); 


		r1_B_[0] = r2_B_[0] = r3_B_[0] = r4_B_[0] = 0;
		Wallonoff_B_=ReadParI(Par_Field);/* */
		r1_B_[1]=ReadParF(Par_Field); r1_B_[2]=ReadParF(Par_Field);
		r2_B_[1]=ReadParF(Par_Field); r2_B_[2]=ReadParF(Par_Field);
		r3_B_[1]=ReadParF(Par_Field); r3_B_[2]=ReadParF(Par_Field);
		r4_B_[1]=ReadParF(Par_Field); r4_B_[2]=ReadParF(Par_Field); 
		WallOffset_B_[0]=ReadParF(Par_Field); WallOffset_B_[1]=ReadParF(Par_Field); WallOffset_B_[2]=ReadParF(Par_Field); 
		WallHoriz_B_=ReadParF(Par_Field); WallVert_B_=ReadParF(Par_Field); 
		mrangh_B_=ReadParF(Par_Field); mrangv_B_=ReadParF(Par_Field); 
		thetaC_B_[0]=ReadParF(Par_Field); thetaCSM_B_[0]=ReadParF(Par_Field); RthetaCSM_B_[0]=ReadParF(Par_Field); mued_B_[0]=ReadParF(Par_Field);  mued_B_[1]=ReadParF(Par_Field); 
		thetaC_B_[1]=ReadParF(Par_Field); thetaCSM_B_[1]=ReadParF(Par_Field); RthetaCSM_B_[1]=ReadParF(Par_Field); mued_B_[2]=ReadParF(Par_Field);  mued_B_[3]=ReadParF(Par_Field); 
		if(Wallonoff_B_ == 1)
		{
		fprintf(LogFilePtr,"\nB:   %f %f   %f %f   %f %f   %f %f   %f %f %f   %f %f   %f %f   %f %f  %f  %f  %f  %f  %f  %f  %f %f \n", r1_B_[1],r1_B_[2],r2_B_[1],r2_B_[2],r3_B_[1],r3_B_[2],r4_B_[1],r4_B_[2],WallOffset_B_[0],WallOffset_B_[1],WallOffset_B_[2], WallHoriz_B_,WallVert_B_, mrangh_B_, mrangv_B_, thetaC_B_[0], thetaCSM_B_[0], RthetaCSM_B_[0], mued_B_[0], mued_B_[1], thetaC_B_[1], thetaCSM_B_[1], RthetaCSM_B_[1], mued_B_[2], mued_B_[3]);
		}
		ReadParComment(Par_Field); 
		WallHoriz_B_	*= M_PI/180.;
		WallVert_B_	*= M_PI/180.;
		mrangh_B_	*= M_PI/180.;
		mrangv_B_	*= M_PI/180.;
		FillRotMatrixZY(RotMatrixWall_B_, WallVert_B_, WallHoriz_B_);
		EulerToCartesianZY(WallNormal_B_, &WallVert_B_, &WallHoriz_B_); 


		r1_C_[0] = r2_C_[0] = r3_C_[0] = r4_C_[0] = 0;
		Wallonoff_C_=ReadParI(Par_Field); /**/
		r1_C_[1]=ReadParF(Par_Field); r1_C_[2]=ReadParF(Par_Field);
		r2_C_[1]=ReadParF(Par_Field); r2_C_[2]=ReadParF(Par_Field);
		r3_C_[1]=ReadParF(Par_Field); r3_C_[2]=ReadParF(Par_Field);
		r4_C_[1]=ReadParF(Par_Field); r4_C_[2]=ReadParF(Par_Field); 
		WallOffset_C_[0]=ReadParF(Par_Field); WallOffset_C_[1]=ReadParF(Par_Field); WallOffset_C_[2]=ReadParF(Par_Field); 
		WallHoriz_C_=ReadParF(Par_Field); WallVert_C_=ReadParF(Par_Field); 
		mrangh_C_=ReadParF(Par_Field); mrangv_C_=ReadParF(Par_Field); 
		thetaC_C_[0]=ReadParF(Par_Field); thetaCSM_C_[0]=ReadParF(Par_Field); RthetaCSM_C_[0]=ReadParF(Par_Field); mued_C_[0]=ReadParF(Par_Field);  mued_C_[1]=ReadParF(Par_Field); 
		thetaC_C_[1]=ReadParF(Par_Field); thetaCSM_C_[1]=ReadParF(Par_Field); RthetaCSM_C_[1]=ReadParF(Par_Field); mued_C_[2]=ReadParF(Par_Field);  mued_C_[3]=ReadParF(Par_Field); 
		if(Wallonoff_C_ == 1)
		{
		fprintf(LogFilePtr,"\nC:   %f %f   %f %f   %f %f   %f %f   %f %f %f   %f %f   %f %f   %f %f  %f  %f  %f  %f  %f  %f  %f %f \n", r1_C_[1],r1_C_[2],r2_C_[1],r2_C_[2],r3_C_[1],r3_C_[2],r4_C_[1],r4_C_[2],WallOffset_C_[0],WallOffset_C_[1],WallOffset_C_[2], WallHoriz_C_,WallVert_C_, mrangh_C_, mrangv_C_, thetaC_C_[0], thetaCSM_C_[0], RthetaCSM_C_[0], mued_C_[0], mued_C_[1], thetaC_C_[1], thetaCSM_C_[1], RthetaCSM_C_[1], mued_C_[2], mued_C_[3]);
		}
		ReadParComment(Par_Field); 
		WallHoriz_C_	*= M_PI/180.;
		WallVert_C_	*= M_PI/180.;
		mrangh_C_	*= M_PI/180.;
		mrangv_C_	*= M_PI/180.;
		FillRotMatrixZY(RotMatrixWall_C_, WallVert_C_, WallHoriz_C_);
		EulerToCartesianZY(WallNormal_C_, &WallVert_C_, &WallHoriz_C_); 

		r1_D_[0] = r2_D_[0] = r3_D_[0] = r4_D_[0] = 0;
		Wallonoff_D_=ReadParI(Par_Field);/**/
		r1_D_[1]=ReadParF(Par_Field); r1_D_[2]=ReadParF(Par_Field);
		r2_D_[1]=ReadParF(Par_Field); r2_D_[2]=ReadParF(Par_Field);
		r3_D_[1]=ReadParF(Par_Field); r3_D_[2]=ReadParF(Par_Field);
		r4_D_[1]=ReadParF(Par_Field); r4_D_[2]=ReadParF(Par_Field); 
		WallOffset_D_[0]=ReadParF(Par_Field); WallOffset_D_[1]=ReadParF(Par_Field); WallOffset_D_[2]=ReadParF(Par_Field); 
		WallHoriz_D_=ReadParF(Par_Field); WallVert_D_=ReadParF(Par_Field); 
		mrangh_D_=ReadParF(Par_Field); mrangv_D_=ReadParF(Par_Field); 
		thetaC_D_[0]=ReadParF(Par_Field); thetaCSM_D_[0]=ReadParF(Par_Field); RthetaCSM_D_[0]=ReadParF(Par_Field); mued_D_[0]=ReadParF(Par_Field);  mued_D_[1]=ReadParF(Par_Field); 
		thetaC_D_[1]=ReadParF(Par_Field); thetaCSM_D_[1]=ReadParF(Par_Field); RthetaCSM_D_[1]=ReadParF(Par_Field); mued_D_[2]=ReadParF(Par_Field);  mued_D_[3]=ReadParF(Par_Field); 
		 if(Wallonoff_D_ == 1)
		{
		fprintf(LogFilePtr,"\nD:   %f %f   %f %f   %f %f   %f %f   %f %f %f   %f %f   %f %f   %f %f  %f  %f  %f  %f  %f  %f  %f %f \n", r1_D_[1],r1_D_[2],r2_D_[1],r2_D_[2],r3_D_[1],r3_D_[2],r4_D_[1],r4_D_[2],WallOffset_D_[0],WallOffset_D_[1],WallOffset_D_[2], WallHoriz_D_,WallVert_D_, mrangh_D_, mrangv_D_, thetaC_D_[0], thetaCSM_D_[0], RthetaCSM_D_[0], mued_D_[0], mued_D_[1], thetaC_D_[1], thetaCSM_D_[1], RthetaCSM_D_[1], mued_D_[2], mued_D_[3]);
		}
		ReadParComment(Par_Field); 
		WallHoriz_D_	*= M_PI/180.;
		WallVert_D_	*= M_PI/180.;
		mrangh_D_	*= M_PI/180.;
		mrangv_D_	*= M_PI/180.;
		FillRotMatrixZY(RotMatrixWall_D_, WallVert_D_, WallHoriz_D_);
		EulerToCartesianZY(WallNormal_D_, &WallVert_D_, &WallHoriz_D_); 

		r1_E_[0] = r2_E_[0] = r3_E_[0] = r4_E_[0] = 0;
		Wallonoff_E_=ReadParI(Par_Field); /**/
		r1_E_[1]=ReadParF(Par_Field); r1_E_[2]=ReadParF(Par_Field);
		r2_E_[1]=ReadParF(Par_Field); r2_E_[2]=ReadParF(Par_Field);
		r3_E_[1]=ReadParF(Par_Field); r3_E_[2]=ReadParF(Par_Field);
		r4_E_[1]=ReadParF(Par_Field); r4_E_[2]=ReadParF(Par_Field); 
		WallOffset_E_[0]=ReadParF(Par_Field); WallOffset_E_[1]=ReadParF(Par_Field); WallOffset_E_[2]=ReadParF(Par_Field); 
		WallHoriz_E_=ReadParF(Par_Field); WallVert_E_=ReadParF(Par_Field); 
		mrangh_E_=ReadParF(Par_Field); mrangv_E_=ReadParF(Par_Field); 
		thetaC_E_[0]=ReadParF(Par_Field); thetaCSM_E_[0]=ReadParF(Par_Field); RthetaCSM_E_[0]=ReadParF(Par_Field); mued_E_[0]=ReadParF(Par_Field);  mued_E_[1]=ReadParF(Par_Field); 
		thetaC_E_[1]=ReadParF(Par_Field); thetaCSM_E_[1]=ReadParF(Par_Field); RthetaCSM_E_[1]=ReadParF(Par_Field); mued_E_[2]=ReadParF(Par_Field);  mued_E_[3]=ReadParF(Par_Field); 
		if(Wallonoff_E_ == 1)
		{
		fprintf(LogFilePtr,"\nE:   %f %f   %f %f   %f %f   %f %f   %f %f %f   %f %f   %f %f   %f %f  %f  %f  %f  %f  %f  %f  %f %f \n", r1_E_[1],r1_E_[2],r2_E_[1],r2_E_[2],r3_E_[1],r3_E_[2],r4_E_[1],r4_E_[2],WallOffset_E_[0],WallOffset_E_[1],WallOffset_E_[2], WallHoriz_E_,WallVert_E_, mrangh_E_, mrangv_E_, thetaC_E_[0], thetaCSM_E_[0], RthetaCSM_E_[0], mued_E_[0], mued_E_[1], thetaC_E_[1], thetaCSM_E_[1], RthetaCSM_E_[1], mued_E_[2], mued_E_[3]);
		} 
		ReadParComment(Par_Field); 
		WallHoriz_E_	*= M_PI/180.;
		WallVert_E_	*= M_PI/180.;
		mrangh_E_	*= M_PI/180.;
		mrangv_E_	*= M_PI/180.;
		FillRotMatrixZY(RotMatrixWall_E_, WallVert_E_, WallHoriz_E_);
		EulerToCartesianZY(WallNormal_E_, &WallVert_E_, &WallHoriz_E_); 

		r1_F_[0] = r2_F_[0] = r3_F_[0] = r4_F_[0] = 0;
		Wallonoff_F_=ReadParI(Par_Field); /**/
		r1_F_[1]=ReadParF(Par_Field); r1_F_[2]=ReadParF(Par_Field);
		r2_F_[1]=ReadParF(Par_Field); r2_F_[2]=ReadParF(Par_Field);
		r3_F_[1]=ReadParF(Par_Field); r3_F_[2]=ReadParF(Par_Field);
		r4_F_[1]=ReadParF(Par_Field); r4_F_[2]=ReadParF(Par_Field); 
		WallOffset_F_[0]=ReadParF(Par_Field); WallOffset_F_[1]=ReadParF(Par_Field); WallOffset_F_[2]=ReadParF(Par_Field); 
		WallHoriz_F_=ReadParF(Par_Field); WallVert_F_=ReadParF(Par_Field); 
		mrangh_F_=ReadParF(Par_Field); mrangv_F_=ReadParF(Par_Field); 
		thetaC_F_[0]=ReadParF(Par_Field); thetaCSM_F_[0]=ReadParF(Par_Field); RthetaCSM_F_[0]=ReadParF(Par_Field); mued_F_[0]=ReadParF(Par_Field);  mued_F_[1]=ReadParF(Par_Field); 
		thetaC_F_[1]=ReadParF(Par_Field); thetaCSM_F_[1]=ReadParF(Par_Field); RthetaCSM_F_[1]=ReadParF(Par_Field); mued_F_[2]=ReadParF(Par_Field);  mued_F_[3]=ReadParF(Par_Field); 
		if(Wallonoff_F_ == 1)
		{
		fprintf(LogFilePtr,"\nF:   %f %f   %f %f   %f %f   %f %f   %f %f %f   %f %f   %f %f   %f %f  %f  %f  %f  %f  %f  %f  %f %f \n", r1_F_[1],r1_F_[2],r2_F_[1],r2_F_[2],r3_F_[1],r3_F_[2],r4_F_[1],r4_F_[2],WallOffset_F_[0],WallOffset_F_[1],WallOffset_F_[2], WallHoriz_F_,WallVert_F_, mrangh_F_, mrangv_F_, thetaC_F_[0], thetaCSM_F_[0], RthetaCSM_F_[0], mued_F_[0], mued_F_[1], thetaC_F_[1], thetaCSM_F_[1], RthetaCSM_F_[1], mued_F_[2], mued_F_[3]);
		}
		ReadParComment(Par_Field); 
		WallHoriz_F_	*= M_PI/180.;
		WallVert_F_	*= M_PI/180.;
		mrangh_F_	*= M_PI/180.;
		mrangv_F_	*= M_PI/180.;
		FillRotMatrixZY(RotMatrixWall_F_, WallVert_F_, WallHoriz_F_);
		EulerToCartesianZY(WallNormal_F_, &WallVert_F_, &WallHoriz_F_); 

		r1_G_[0] = r2_G_[0] = r3_G_[0] = r4_G_[0] = 0;
		Wallonoff_G_=ReadParI(Par_Field); /**/
		r1_G_[1]=ReadParF(Par_Field); r1_G_[2]=ReadParF(Par_Field);
		r2_G_[1]=ReadParF(Par_Field); r2_G_[2]=ReadParF(Par_Field);
		r3_G_[1]=ReadParF(Par_Field); r3_G_[2]=ReadParF(Par_Field);
		r4_G_[1]=ReadParF(Par_Field); r4_G_[2]=ReadParF(Par_Field); 
		WallOffset_G_[0]=ReadParF(Par_Field); WallOffset_G_[1]=ReadParF(Par_Field); WallOffset_G_[2]=ReadParF(Par_Field); 
		WallHoriz_G_=ReadParF(Par_Field); WallVert_G_=ReadParF(Par_Field); 
		mrangh_G_=ReadParF(Par_Field); mrangv_G_=ReadParF(Par_Field); 
		thetaC_G_[0]=ReadParF(Par_Field); thetaCSM_G_[0]=ReadParF(Par_Field); RthetaCSM_G_[0]=ReadParF(Par_Field); mued_G_[0]=ReadParF(Par_Field);  mued_G_[1]=ReadParF(Par_Field); 
		thetaC_G_[1]=ReadParF(Par_Field); thetaCSM_G_[1]=ReadParF(Par_Field); RthetaCSM_G_[1]=ReadParF(Par_Field); mued_G_[2]=ReadParF(Par_Field);  mued_G_[3]=ReadParF(Par_Field); 
		if(Wallonoff_G_ == 1)
		{
		fprintf(LogFilePtr,"\nG:   %f %f   %f %f   %f %f   %f %f   %f %f %f   %f %f   %f %f   %f %f  %f  %f  %f  %f  %f  %f  %f %f \n", r1_G_[1],r1_G_[2],r2_G_[1],r2_G_[2],r3_G_[1],r3_G_[2],r4_G_[1],r4_G_[2],WallOffset_G_[0],WallOffset_G_[1],WallOffset_G_[2], WallHoriz_G_,WallVert_G_, mrangh_G_, mrangv_G_, thetaC_G_[0], thetaCSM_G_[0], RthetaCSM_G_[0], mued_G_[0], mued_G_[1], thetaC_G_[1], thetaCSM_G_[1], RthetaCSM_G_[1], mued_G_[2], mued_G_[3]);
		} 
		ReadParComment(Par_Field); 
		WallHoriz_G_	*= M_PI/180.;
		WallVert_G_	*= M_PI/180.;
		mrangh_G_	*= M_PI/180.;
		mrangv_G_	*= M_PI/180.;
		FillRotMatrixZY(RotMatrixWall_G_, WallVert_G_, WallHoriz_G_);
		EulerToCartesianZY(WallNormal_G_, &WallVert_G_, &WallHoriz_G_); 

		r1_H_[0] = r2_H_[0] = r3_H_[0] = r4_H_[0] = 0;
		Wallonoff_H_=ReadParI(Par_Field); /**/
		r1_H_[1]=ReadParF(Par_Field); r1_H_[2]=ReadParF(Par_Field);
		r2_H_[1]=ReadParF(Par_Field); r2_H_[2]=ReadParF(Par_Field);
		r3_H_[1]=ReadParF(Par_Field); r3_H_[2]=ReadParF(Par_Field);
		r4_H_[1]=ReadParF(Par_Field); r4_H_[2]=ReadParF(Par_Field); 
		WallOffset_H_[0]=ReadParF(Par_Field); WallOffset_H_[1]=ReadParF(Par_Field); WallOffset_H_[2]=ReadParF(Par_Field); 
		WallHoriz_H_=ReadParF(Par_Field); WallVert_H_=ReadParF(Par_Field); 
		mrangh_H_=ReadParF(Par_Field); mrangv_H_=ReadParF(Par_Field); 
		thetaC_H_[0]=ReadParF(Par_Field); thetaCSM_H_[0]=ReadParF(Par_Field); RthetaCSM_H_[0]=ReadParF(Par_Field); mued_H_[0]=ReadParF(Par_Field); mued_H_[1]=ReadParF(Par_Field); 
		thetaC_H_[1]=ReadParF(Par_Field); thetaCSM_H_[1]=ReadParF(Par_Field); RthetaCSM_H_[1]=ReadParF(Par_Field); mued_H_[2]=ReadParF(Par_Field); mued_H_[3]=ReadParF(Par_Field); 
		if(Wallonoff_H_ == 1)
		{
		fprintf(LogFilePtr,"\nH:   %f %f   %f %f   %f %f   %f %f   %f %f %f   %f %f   %f %f   %f %f  %f  %f  %f  %f  %f  %f  %f %f \n", r1_H_[1],r1_H_[2],r2_H_[1],r2_H_[2],r3_H_[1],r3_H_[2],r4_H_[1],r4_H_[2],WallOffset_H_[0],WallOffset_H_[1],WallOffset_H_[2], WallHoriz_H_,WallVert_H_, mrangh_H_, mrangv_H_, thetaC_H_[0], thetaCSM_H_[0], RthetaCSM_H_[0], mued_H_[0], mued_H_[1], thetaC_H_[1], thetaCSM_H_[1], RthetaCSM_H_[1], mued_H_[2], mued_H_[3]);
		} 
		ReadParComment(Par_Field); 
		WallHoriz_H_	*= M_PI/180.;
		WallVert_H_	*= M_PI/180.;
		mrangh_H_	*= M_PI/180.;
		mrangv_H_	*= M_PI/180.;
		FillRotMatrixZY(RotMatrixWall_H_, WallVert_H_, WallHoriz_H_);
		EulerToCartesianZY(WallNormal_H_, &WallVert_H_, &WallHoriz_H_); 

		r1_I_[0] = r2_I_[0] = r3_I_[0] = r4_I_[0] = 0;
		Wallonoff_I_=ReadParI(Par_Field);/**/
		r1_I_[1]=ReadParF(Par_Field); r1_I_[2]=ReadParF(Par_Field);
		r2_I_[1]=ReadParF(Par_Field); r2_I_[2]=ReadParF(Par_Field);
		r3_I_[1]=ReadParF(Par_Field); r3_I_[2]=ReadParF(Par_Field);
		r4_I_[1]=ReadParF(Par_Field); r4_I_[2]=ReadParF(Par_Field); 
		WallOffset_I_[0]=ReadParF(Par_Field); WallOffset_I_[1]=ReadParF(Par_Field); WallOffset_I_[2]=ReadParF(Par_Field); 
		WallHoriz_I_=ReadParF(Par_Field); WallVert_I_=ReadParF(Par_Field); 
		mrangh_I_=ReadParF(Par_Field); mrangv_I_=ReadParF(Par_Field); 
		thetaC_I_[0]=ReadParF(Par_Field); thetaCSM_I_[0]=ReadParF(Par_Field); RthetaCSM_I_[0]=ReadParF(Par_Field); mued_I_[0]=ReadParF(Par_Field);  mued_I_[1]=ReadParF(Par_Field); 
		thetaC_I_[1]=ReadParF(Par_Field); thetaCSM_I_[1]=ReadParF(Par_Field); RthetaCSM_I_[1]=ReadParF(Par_Field); mued_I_[2]=ReadParF(Par_Field);  mued_I_[3]=ReadParF(Par_Field); 
		if(Wallonoff_I_ == 1)
		{
		fprintf(LogFilePtr,"\nI:   %f %f   %f %f   %f %f   %f %f   %f %f %f   %f %f   %f %f   %f %f  %f  %f  %f  %f  %f  %f  %f %f \n", r1_I_[1],r1_I_[2],r2_I_[1],r2_I_[2],r3_I_[1],r3_I_[2],r4_I_[1],r4_I_[2],WallOffset_I_[0],WallOffset_I_[1],WallOffset_I_[2], WallHoriz_I_,WallVert_I_, mrangh_I_, mrangv_I_, thetaC_I_[0], thetaCSM_I_[0], RthetaCSM_I_[0], mued_I_[0], mued_I_[1], thetaC_I_[1], thetaCSM_I_[1], RthetaCSM_I_[1], mued_I_[2], mued_I_[3]);
		} 
		ReadParComment(Par_Field); 
		WallHoriz_I_	*= M_PI/180.;
		WallVert_I_	*= M_PI/180.;
		mrangh_I_	*= M_PI/180.;
		mrangv_I_	*= M_PI/180.;
		FillRotMatrixZY(RotMatrixWall_I_, WallVert_I_, WallHoriz_I_);
		EulerToCartesianZY(WallNormal_I_, &WallVert_I_, &WallHoriz_I_); 

		r1_J_[0] = r2_J_[0] = r3_J_[0] = r4_J_[0] = 0;
		Wallonoff_J_=ReadParI(Par_Field);/**/
		r1_J_[1]=ReadParF(Par_Field); r1_J_[2]=ReadParF(Par_Field);
		r2_J_[1]=ReadParF(Par_Field); r2_J_[2]=ReadParF(Par_Field);
		r3_J_[1]=ReadParF(Par_Field); r3_J_[2]=ReadParF(Par_Field);
		r4_J_[1]=ReadParF(Par_Field); r4_J_[2]=ReadParF(Par_Field); 
		WallOffset_J_[0]=ReadParF(Par_Field); WallOffset_J_[1]=ReadParF(Par_Field); WallOffset_J_[2]=ReadParF(Par_Field); 
		WallHoriz_J_=ReadParF(Par_Field); WallVert_J_=ReadParF(Par_Field); 
		mrangh_J_=ReadParF(Par_Field); mrangv_J_=ReadParF(Par_Field); 
		thetaC_J_[0]=ReadParF(Par_Field); thetaCSM_J_[0]=ReadParF(Par_Field); RthetaCSM_J_[0]=ReadParF(Par_Field); mued_J_[0]=ReadParF(Par_Field);  mued_J_[1]=ReadParF(Par_Field); 
		thetaC_J_[1]=ReadParF(Par_Field); thetaCSM_J_[1]=ReadParF(Par_Field); RthetaCSM_J_[1]=ReadParF(Par_Field); mued_J_[2]=ReadParF(Par_Field);  mued_J_[3]=ReadParF(Par_Field); 
		if(Wallonoff_J_ == 1)
		{
		fprintf(LogFilePtr,"\nJ:   %f %f   %f %f   %f %f   %f %f   %f %f %f   %f %f   %f %f   %f %f  %f  %f  %f  %f  %f  %f  %f %f \n", r1_J_[1],r1_J_[2],r2_J_[1],r2_J_[2],r3_J_[1],r3_J_[2],r4_J_[1],r4_J_[2],WallOffset_J_[0],WallOffset_J_[1],WallOffset_J_[2], WallHoriz_J_,WallVert_J_, mrangh_J_, mrangv_J_, thetaC_J_[0], thetaCSM_J_[0], RthetaCSM_J_[0], mued_J_[0], mued_J_[1], thetaC_J_[1], thetaCSM_J_[1], RthetaCSM_J_[1], mued_J_[2], mued_J_[3]);
		} 
		ReadParComment(Par_Field); 
		WallHoriz_J_	*= M_PI/180.;
		WallVert_J_	*= M_PI/180.;
		mrangh_J_	*= M_PI/180.;
		mrangv_J_	*= M_PI/180.;
		FillRotMatrixZY(RotMatrixWall_J_, WallVert_J_, WallHoriz_J_);
		EulerToCartesianZY(WallNormal_J_, &WallVert_J_, &WallHoriz_J_); 

		fprintf(LogFilePtr,"\nABCDEFGHIJ: %d %d %d %d %d %d %d %d %d %d\n", Wallonoff_A_,  Wallonoff_B_,  Wallonoff_C_,  Wallonoff_D_,  Wallonoff_E_,  Wallonoff_F_,  Wallonoff_G_,  Wallonoff_H_,  Wallonoff_I_, Wallonoff_J_);   
		/**/

		


		/**/

		
}/* End ReadParameterFile  */





/* Theta and Phi of a unit vector if Theta is the angle with the axis of lowest index  */

void CartesianToSpherical2(VectorType Vector, double *Theta, double *Phi)
{
	*Theta	= (double) acos(Vector[0]);

	if(Vector[2]>=0) *Phi	= (double) atan2(Vector[2], Vector[1]);
	if(Vector[2]< 0) *Phi	= 2. * M_PI + (double) atan2(Vector[2], Vector[1]);

}



/* hittriangle */

int hittriangle(VectorType r1, VectorType r2, VectorType rt)
{
	double the1, the2, thet, phi1, phi2, phit;

	CartesianToSpherical2(r1, &the1, &phi1);
	CartesianToSpherical2(r2, &the2, &phi2);
	CartesianToSpherical2(rt, &thet, &phit);

	if(phi1 < phi2) {
		if((phi1 < phit)&&(phit < phi2)&&(Area(r1,r2) > (Area(r1,rt) + Area(r2, rt)))) return 1; 
		else return 0;
	}
	if(phi1 > phi2) {
		if(((phi1 < phit)&&(phit < 2*M_PI)||(0. < phit)&&(phit < phi2))&&(Area(r1,r2) > (Area(r1,rt) + Area(r2, rt)))) return 1; 
		else return 0;
	}

	else return 0;
}

/* hitwall */

int hitwall(VectorType r1, VectorType r2, VectorType r3, VectorType r4, VectorType rt)
{
	if((hittriangle(r1, r2, rt) == 1)||(hittriangle(r2, r3, rt) == 1)||(hittriangle(r3, r4, rt) == 1)||(hittriangle(r4, r1, rt) == 1)) return 1;
	else return 0;
}

/* rutine which computes the collision with a wall */

double CollideWall(double *prob, VectorType pos,VectorType dir,VectorType spin,VectorType WallOffset,VectorType WallNormal, double  RotMatrixWall[3][3],VectorType r1,VectorType r2,VectorType r3,VectorType r4, double thetaC[2], double thetaCSM[2], double RthetaCSM[2], double mued[4], double mrangh, double mrangv)
{
	VectorType rt; double path;  double rangh=0., rangv=0., RotMatrixRang[3][3];	

	/* random angular spread */ 
	{
/*	rangh=MonteCarlo(0., mrangh);
	rangv=MonteCarlo(0., mrangv);
*/	rangh=MonteCarlo(- mrangh/2., mrangh/2.);
	rangv=MonteCarlo(- mrangv/2., mrangv/2.);
	FillRotMatrixZY(RotMatrixRang, rangv, rangh); /*fprintf(LogFilePtr,"%e  %e \n", rangh, rangv);*/
	}
	
	if(PlaneLineIntersect(pos, dir, WallNormal, (double) ScalarProduct(WallOffset, WallNormal), rt)== 0) return 99999;

	{ VectorType replacement;

	CopyVector(rt, replacement);

	SubVector(replacement, pos);

	if((ScalarProduct(replacement, dir)< 0.)/*||(ScalarProduct(replacement, WallNormal)< 0.)*/) return 99999;	
	}


	/* transforms into frame of the wall  */
	
	SubVector(rt, WallOffset);

	RotVector(RotMatrixWall, rt);

	RotVector(RotMatrixWall, dir);

	if((mrangh!=0.)||(mrangv!=0.))
	{
	RotVector(RotMatrixRang, rt);

	RotVector(RotMatrixRang, dir);
	}



	/* here comes to collision etc. */
	{double the, Choise, Refl[2], expon[2];
		
		Choise = MonteCarlo(0,1);
		the = M_PI_2 - (double) acos(fabs(dir[0])); 

		if(hitwall(r1, r2, r3, r4, rt)==0) 
		{		
		RotBackVector(RotMatrixWall, dir);

		return 99999;
		}
		
		/* here if spin up */
		if(SpinVector[2]==1.)
		{
			if(the <= thetaC[0] * WL) 
			{	dir[0] *= -1.;
			}
			else
			{	expon[0] = (mued[0] * WL + mued[1]) /(double) sqrt(sq((double) sin(the)) - sq((double) sin(thetaC[0] * WL)));
				if(expon[0] > 500) expon[0] = 500;
				if((the > thetaC[0] * WL)&&(the <= thetaCSM[0] * WL))
				{
					Refl[0] = RthetaCSM[0] + (1. - RthetaCSM[0])/(thetaCSM[0] * WL - thetaC[0] * WL)*(thetaCSM[0] * WL - the);
					if(Choise < Refl[0]) dir[0] *= -1.; 
					else *prob *= (double) exp(- expon[0]);
				}
				if(the >  thetaCSM[0] * WL) *prob *= (double) exp(- expon[0]);
			}
		}


		/* here if spin down */
		if(SpinVector[2]==-1.)
		{

			if(the <= thetaC[1] * WL) 
			{	dir[0] *= -1.;
			}
			else
			{	expon[1] = (mued[2] * WL + mued[3]) /(double) sqrt(sq((double) sin(the)) - sq((double) sin(thetaC[1] * WL)));
				if(expon[1] > 500) expon[1] = 500;
				if((the > thetaC[1] * WL)&&(the <= thetaCSM[1] * WL))
				{
					Refl[1] = RthetaCSM[1] + (1. - RthetaCSM[1])/(thetaCSM[1] * WL - thetaC[1] * WL)*(thetaCSM[1] * WL - the);
					if(Choise < Refl[1]) dir[0] *= -1.; 
					else *prob *= (double) exp(- expon[1]);
				}
				if(the >  thetaCSM[1] * WL) *prob *= (double) exp(- expon[1]);
			}
		}

	}
/* transforms back into original frame  */
	
	if((mrangh!=0.)||(mrangv!=0.))
	{
	RotBackVector(RotMatrixRang, rt);

	RotBackVector(RotMatrixRang, dir);
	}

	RotBackVector(RotMatrixWall, rt);

	AddVector(rt, WallOffset);

	RotBackVector(RotMatrixWall, dir);

	dir[0]	= (double) sqrt(1 - sq(dir[1]) - sq(dir[2]));


/* computes pathlength until collision */

	path = DistVector(rt, pos); 
	
	CopyVector(rt, pos); 
	return path; 

}



