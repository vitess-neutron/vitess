#ifndef ROTATING_FIELD_H
#define ROTATING_FIELD_H

/************************************/
/** Definitions, structures, enums **/
/************************************/
/* Number of domains */
#define	FIELD_SIZE	3000	/* must be given by  user */
#define FIELD_SIZE_2	6000	/* must be multipled by 2 */
#define	FIELD_SIZE_FL	9000	/* must be multipled by 3 */
/* After changing this definitions, you must recompile the module */

#define	ENERGY_FROM_LAMBDA(x) ( 81805.048 / x / x ) /*[ueV]*/
#define FREQUENCY_FROM_FIELD(x)  ( 18.324282 * x ) /* rad*kHz from Oe=Gauss */ 


/**************************/
/** Functions prototypes **/
/**************************/
void   OwnInit(int argc, char *argv[]);   // Reads input parameters and sets global parameters
void   EvalInput();                       // Analyses input parameters
void   OwnCleanup();                      // Does module specific cleanup
void   SetGeometry(char* sColor);         // Fills the structure stGeometry for visualization 
/* Intersection with rectangular object */
long   IntersectionWithRectangularWallNumber(VectorType DimDomain, VectorType Pos, VectorType Dir, VectorType Pos1, VectorType Pos2, long *wall_1, long *wall_2) ;
/* Generate common rotating or oscillating field */
long   CalculateField(long l_keyrotos, long l_keyaxis, double l_FieldValue, double l_FieldValue0[3],
                      double l_Omega, double l_TimeR, double l_TOFP, double l_phi0, VectorType l_RR);		

void gsl_ran_dir_3d (const gsl_rng * r, double * x, double * y, double * z);
	

#endif


	




