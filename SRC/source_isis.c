
typedef struct
{
  int nEnergy;        // Number of energy bins
  int nTime;          // number of time bins

  double* TimeBin;    // Time bins
  double* EnergyBin;  // Energy bins

  double** Flux;      // Flux per bin (integrated)
  double* EInt;       // Integrated Energy point
  double Total;       // Integrated Total

} ISource;

int cmdnumberD(char *,double*);
int cmdnumberI(char *,int*,const int);
double polInterp(double*,double*,int,double);
FILE *openFile(char*);
double LoadIsisDistrib(FILE*, double, double);
int timeStart(char*);
int timeEnd(char*);
int energyBin(char*,double,double,double*,double*);
int notComment(char*);
void ISISgetpoint(double*, double*);
double strArea(double,double,double,double,double);

static ISource TS;

int
cmdnumberD(char *mc,double* num)
/*!
  \returns 1 on success 0 on failure
*/
{
  int i,j;
  char* ss;
  char **endptr;
  double nmb;
  int len;

  len=strlen(mc);
  j=0;

  for(i=0;i<len && mc[i] &&
  (mc[i]=='\t' || mc[i]==' '  || mc[i]==',');i++);
  if(i==len || !mc[i]) return 0;
  ss=malloc(sizeof(char)*(len+1));

  for(;i<len && mc[i]!='\n' && mc[i]
  && mc[i]!='\t' && mc[i]!=' ' && mc[i]!=',';i++)
    {
      ss[j]=mc[i];
      j++;
    }
  if (!j)
    {
      free(ss);
      return 0;         //This should be impossible
    }
  ss[j]=0;
  endptr=malloc(sizeof(char*));
  nmb = strtod(ss,endptr);
  if (*endptr != ss+j)
    {
      free(endptr);
      free(ss);
      return 0;
    }
  *num = (double) nmb;
  for(j=0;j<i && mc[j];j++)
    mc[j]=' ';
  free(endptr);
  free(ss);
  return 1;
}

int
energyBin(char* Line,double Einit,double Eend,double* Ea,double* Eb)
/*!
  Search for a word "energy bin:" at the start of
  the line. Then separte off the energy bin values
  \param Line :: Line to search
  \param Ea :: first energy bin [meV]
  \param Eb :: second energy bin [meV]
  \returns 1 on success 0 on failure
*/
{
  int len,i;
  double A,B;

  len=strlen(Line);
  for(i=0;i<len && isspace(Line[i]);i++);
  if (len-i<11) return 0;


  if (strncmp(Line+i,"energy bin:",11))
    return 0;

  i+=11;
  if (!cmdnumberD(Line+i,&A))
    return 0;
  // remove 'to'
  for(;i<len-1 && Line[i]!='o';i++);
  i++;
  if (!cmdnumberD(Line+i,&B))
    return 0;
  A*=1e9;
  B*=1e9;
  *Ea=A;
  *Eb=B;
  if (*Eb>Einit && *Ea<Eend)
    return 1;
  return 0;
}

int
timeStart(char* Line)
/*!
  Search for a word time at the start of
  the line.
  \param Line :: Line to search
  \returns 1 on success 0 on failure
*/
{
  int len,i;

  len=strlen(Line);
  for(i=0;i<len && isspace(Line[i]);i++);
  if (len-i<4) return 0;
  return (strncmp(Line+i,"time",4)) ? 0 : 1;
}

int
timeEnd(char* Line)
/*!
  Search for a word time at the start of
  the line.
  \param Line :: Line to search
  \returns 1 on success 0 on failure
*/
{
  int len,i;

  len=strlen(Line);
  for(i=0;i<len && isspace(Line[i]);i++);
  if (len-i<5) return 0;
  return (strncmp(Line+i,"total",5)) ? 0 : 1;
}

double
polInterp(double* X,double* Y,int Psize,double Aim)
/*!
  returns the interpolated polynomial between Epnts
  and the integration
  \param X :: X coordinates
  \param Y :: Y coordinates
  \param Psize :: number of valid point in array to use
  \param Aim :: Aim point to intepolate result (X coordinate)
  \returns Energy point
*/
{
  double out,errOut;         /* out put variables */
  double testDiff,diff;

  double w,den,ho,hp;           /* intermediate variables */
  int i,m,ns;
#ifdef _MSC_VER
  double *C, *D;
  C = malloc(Psize*sizeof(double));
  D = malloc(Psize*sizeof(double));
#else
  double C[Psize], D[Psize];
#endif


  ns=0;
  diff=fabs(Aim-X[0]);
  C[0]=Y[0];
  D[0]=Y[0];
  for(i=1;i<Psize;i++)
    {
      testDiff=fabs(Aim-X[i]);
      if (diff>testDiff)
  {
    ns=i;
    diff=testDiff;
  }
      C[i]=Y[i];
      D[i]=Y[i];
    }

  out=Y[ns];
  ns--;              /* Now can be -1 !!!! */

  for(m=1;m<Psize;m++)
    {
      for(i=0;i<Psize-m;i++)
  {
    ho=X[i]-Aim;
    hp=X[i+m]-Aim;
    w=C[i+1]-D[i];
    /*    den=ho-hp;  -- test !=0.0 */
    den=w/(ho-hp);
    D[i]=hp*den;
    C[i]=ho*den;
  }

      errOut= (2*(ns+1)<(Psize-m)) ? C[ns+1] : D[ns--];
      out+=errOut;
    }

#ifdef _MSC_VER
  free(C);
  free(D);
#endif

  return out;
}

FILE* openFile(char* FileName)
{
  FILE* efile=0;

  /* Is the file located in working dir? */
  efile = OpenInputFile(FileName, FALSE, "r");
  if (!efile) {
    fprintf(LogFilePtr, "\nERROR: Can't open %s to read source file\n", FileName);
    exit (-1);
  }
  return efile;
}

double LoadIsisDistrib(FILE* TFile, double Einit, double Eend)
/*!
  Process a general h.o file to create an integrated
  table of results from Einit -> Eend
  \param Einit :: inital Energy
  \parma Eend  :: final energy
*/
{
  char ss[255];          /* BIG space for line */
  double Ea,Eb;
  double T,D,tmp;
  int Ftime;             // time Flag
  int eIndex;            // energy Index
  int tIndex;            // time Index
  double Tsum;           // Running integration
  double Efraction=0;    // Amount to use for an energy/time bin

  //  ISource TS;

  int DebugCnt;
  /*!
    Status Flag::
    Ftime=1 :: [time ] Reading Time : Data : Err [Exit on Total]

    Double Read File to determine how many bins and
    memory size
  */


  //convert Einit and Eend to MeV and reorder

  Einit=81.793936/(Einit*Einit);
  Eend=81.793936/(Eend*Eend);

  if (Einit>Eend)
    {
      tmp=Eend;
      Eend=Einit;
      Einit=tmp;
    }
  /////////////////////////

    //fprintf(stderr,"Einit %g :End : %g \n",Einit,Eend);

  Ea=0.0;
  Eb=0.0;

  eIndex= -1;
  DebugCnt=0;
  Ftime=0;
  tIndex=0;
  TS.nTime=0;
  TS.nEnergy=0;
  // Read file and get time bins
  while(fgets(ss,255,TFile) && Eend>Ea)
    {
      if (notComment(ss))
  {
    DebugCnt++;
          if (!Ftime)
      {
        if (energyBin(ss,Einit,Eend,&Ea,&Eb))
    {
      if (eIndex==0)
        TS.nTime=tIndex;
      eIndex++;
    }
        else if (timeStart(ss))
    {
      Ftime=1;
      tIndex=0;
    }
      }
    else  // In the time section
      {
        if (timeEnd(ss))     // Found "total"
    Ftime=0;
        else
    {
      // Need to read the line in the case of first run
      if (TS.nTime==0)
        {
          if (cmdnumberD(ss,&T) &&
        cmdnumberD(ss,&D))
      tIndex++;
        }
    }
      }
  }
    }
  // Plus 2 since we have a 0 counter and we have missed the last line.
  TS.nEnergy=eIndex+2;
  if (!TS.nTime && tIndex)
    TS.nTime=tIndex;
  // printf("tIndex %d %d %d %d \n",tIndex,eIndex,TS.nEnergy,TS.nTime);

  /* SECOND TIME THROUGH:: */
  rewind(TFile);

  TS.Flux=matrix(TS.nEnergy,TS.nTime);
  TS.EInt=(double*) malloc((TS.nEnergy+1)*sizeof(double));
  TS.TimeBin=(double*) malloc((TS.nTime+1)*sizeof(double));
  TS.EnergyBin=(double*) malloc((TS.nEnergy+1)*sizeof(double));

  Tsum=0.0;
  Ea=0.0;
  Eb=0.0;
  eIndex=-1;
  DebugCnt=0;
  Ftime=0;
  tIndex=0;
  TS.EInt[0]=0.0;
  // Read file and get time bins
  while(fgets(ss,255,TFile) && Eend>Ea)
    {
      if (notComment(ss))
  {
    DebugCnt++;
          if (!Ftime)
      {
        if (energyBin(ss,Einit,Eend,&Ea,&Eb))
    {
      eIndex++;
      TS.EnergyBin[eIndex]=(Einit>Ea) ? Einit : Ea;
      Efraction=calcFraction(Einit,Eend,Ea,Eb);
      Ftime++;
    }
      }
    else if (Ftime==1)
      {
        if (timeStart(ss))
    {
      Ftime=2;
      tIndex=0;
    }
      }

    else           // In the time section
      {
        if (timeEnd(ss))     // Found "total"
    {
      Ftime=0;
      TS.EInt[eIndex+1]=Tsum;
    }
        else
    {
      // Need to read the line in the case of first run
      if (cmdnumberD(ss,&T) &&
          cmdnumberD(ss,&D))
        {
          TS.TimeBin[tIndex]=T/1e8;     // convert Time into second (from shakes)
          Tsum+=D*Efraction;
          TS.Flux[eIndex][tIndex]=Tsum;
          tIndex++;
        }
    }
      }
  }
    }

  TS.EnergyBin[eIndex+1]=Eend;
  TS.Total=Tsum;

  //  printf("tIndex %d %d %d \n",tIndex,eIndex,TS.nTime);
  //fprintf(stderr,"Tsum %g \n",Tsum);
  //fprintf(stderr,"ebin1 ebinN %g %g\n",TS.EnergyBin[0],TS.EnergyBin[TS.nEnergy-1]);

  return TS.Total;
}

int
notComment(char* Line)
/*!
  \returns 0 on a comment, 1 on a non-comment
*/
{
  int len,i;

  len=strlen(Line);
  for(i=0;i<len && isspace(Line[i]);i++);

  if (!Line[i] || Line[i]=='c' || Line[i]=='C' ||
      Line[i]=='!' || Line[i]=='#')
    return 0;
  return 1;
}

void
ISISgetpoint(double* TV,double* EV)
/*!
  Calculate the Time and Energy
  by sampling the file.
  Uses TS table to find the point
  \param TV ::
  \param EV ::
  \param lim1 ::
  \param lim2 ::
*/
{
  //  ISource TS;
  double R0,R1,R,Rend;
  int Epnt;       ///< Points to the next higher index of the neutron integral
  int Tpnt;
  int iStart,iEnd;
  double TRange,Tspread;
  double Espread,Estart;

  // So that lowPoly+highPoly==maxPoly
  const int maxPoly=6;
  const int highPoly=maxPoly/2;
  const int lowPoly=maxPoly-highPoly;

  // static int testVar=0;

  //  R0=rand01();
  R0=MonteCarlo(0,1);
  /* if (testVar==0)
     {
     R0=1.0e-8;
     testVar=1;
     }
  */
  Rend=R=TS.Total*R0;
  // This gives Eint[Epnt-1] > R > Eint[Epnt]
  Epnt=binSearch(TS.nEnergy-1,TS.EInt,R);

  //      if (Epnt < 0)
  //   Epnt=1;
  Tpnt=binSearch(TS.nTime-1,TS.Flux[Epnt-1],R);
  //  fprintf(stderr,"TBoundaryX == %12.6e %12.6e \n",TS.TimeBin[Tpnt-1],TS.TimeBin[Tpnt]);
  //  fprintf(stderr,"TFlux == %12.6e %12.6e %12.6e \n\n",TS.Flux[Epnt-1][Tpnt-1],R,TS.Flux[Epnt-1][Tpnt]);
  //  if (Epnt == -1)
  //{
  //    Epnt=0;
  // fprintf(stderr,"\n Rvals == %g %d %d %g\n",R,Epnt,Tpnt,TS.TimeBin[0]);
  //  fprintf(stderr,"EInt == %d %12.6e %12.6e %12.6e %12.6e \n",Epnt,TS.EInt[Epnt-1],R,TS.EInt[Epnt],TS.EInt[Epnt+1]);
  // printf("EBoundary == %12.6e %12.6e \n",TS.EnergyBin[Epnt],TS.EnergyBin[Epnt+1]);

  //  fprintf(stderr,"TFlux == %12.6e %12.6e %12.6e \n\n",TS.Flux[Epnt+1][Tpnt],R,TS.Flux[Epnt+1][Tpnt+1]);
  // }

  if(R < TS.Flux[Epnt-1][Tpnt-1] || R >TS.Flux[Epnt-1][Tpnt] )
    {
      fprintf(stderr,"outside bin limits Tpnt/Epnt problem  %12.6e %12.6e %12.6e \n",TS.Flux[Epnt-1][Tpnt-1],R,TS.Flux[Epnt-1][Tpnt]);
      exit(-1);
    }


  if(Epnt == 0)
    {
      Estart=0.0;
      Espread=TS.EInt[0];
      *EV=TS.EnergyBin[1];
    }
  else
    {
      Estart=TS.EInt[Epnt-1];
      Espread=TS.EInt[Epnt]-TS.EInt[Epnt-1];
      *EV=TS.EnergyBin[Epnt+1];
    }

  if (Tpnt==0 || Epnt==0)
    {
      fprintf(stderr,"BIG ERROR WITH Tpnt: %d and Epnt: %d\n",Tpnt,Epnt);
      exit(-1);
    }
  if (Tpnt==TS.nTime)
    {
      fprintf(stderr,"BIG ERROR WITH Tpnt and Epnt\n");
      exit(-1);
      *TV=0.0;
      Tspread=TS.Flux[Epnt-1][0]-TS.EInt[Epnt-1];
      TRange=TS.TimeBin[0];
      R-=TS.EInt[Epnt-1];
    }
  else
    {
      *TV=TS.TimeBin[Tpnt-1];
      TRange=TS.TimeBin[Tpnt]-TS.TimeBin[Tpnt-1];
      Tspread=TS.Flux[Epnt-1][Tpnt]-TS.Flux[Epnt-1][Tpnt-1];
      R-=TS.Flux[Epnt-1][Tpnt-1];
    }
  //  printf("R == %12.6e\n",R);
  R/=Tspread;
  //  printf("R == %12.6e\n",R);
  *TV+=TRange*R;


  R1=TS.EInt[Epnt-1]+Espread*MonteCarlo(0,1);
  iStart=Epnt>lowPoly ? Epnt-lowPoly : 0;                  // max(Epnt-halfPoly,0)
  iEnd=TS.nEnergy>Epnt+highPoly ? Epnt+highPoly : TS.nEnergy-1;  // min(nEnergy-1,Epnt+highPoly

  *EV=polInterp(TS.EInt+iStart,TS.EnergyBin+iStart,1+iEnd-iStart,R1);

  //  fprintf(stderr,"Energy == %d %d %12.6e %12.6e \n",iStart,iEnd,R1,*EV);
  //  fprintf(stderr,"bins == %12.6e %12.6e %12.6e %12.6e \n",TS.EnergyBin[iStart],TS.EnergyBin[iEnd],
  //    TS.EInt[Epnt],TS.EInt[Epnt-1]);

  if(*TV < TS.TimeBin[Tpnt-1] || *TV > TS.TimeBin[Tpnt])
    {
      fprintf(stderr,"%d Tpnt %d Tval %g Epnt %d \n",TS.nTime,Tpnt,*TV,Epnt);
      fprintf(stderr,"TBoundary == %12.6e,%g , %12.6e \n\n",TS.TimeBin[Tpnt-1],*TV,TS.TimeBin[Tpnt]);
    }


  // need to return a value in wavelength

  *EV=9.044/sqrt(*EV);
  // and in milliseconds...
  *TV *=1000;
  return;
}

int
cmdnumberI(char *mc,int* num,const int len)
/*!
  \param mc == character string to use
  \param num :: Place to put output
  \param len == length of the character string to process
  returns 1 on success and 0 on failure
*/
{
  int i,j;
  char* ss;
  char **endptr;
  double nmb;

  if (len<1)
    return 0;
  j=0;

  for(i=0;i<len && mc[i] &&
  (mc[i]=='\t' || mc[i]==' '  || mc[i]==',');i++);
  if(i==len || !mc[i]) return 0;
  ss=malloc(sizeof(char)*(len+1));
  /*  char *ss=new char[len+1]; */
  for(;i<len && mc[i]!='\n' && mc[i]
  && mc[i]!='\t' && mc[i]!=' ' && mc[i]!=',';i++)
    {
      ss[j]=mc[i];
      j++;
    }
  if (!j)
    {
      free(ss);
      return 0;         //This should be impossible
    }
  ss[j]=0;
  endptr=malloc(sizeof(char*));
  nmb = strtod(ss,endptr);
  if (*endptr != ss+j)
    {
      free(endptr);
      free(ss);
      return 0;
    }
  *num = (double) nmb;
  for(j=0;j<i && mc[j];j++)
    mc[j]=' ';
  free(endptr);
  free(ss);
  return 1;
}

double strArea(double rtmodX,double rtmodY,double dist,double xw,double yh)
{
  /*
     Returns the mean Str view of the viewport
     This integrates over each point on the window xw to yh
     View port is symmetric so use only 1/4 of the view
     for the calcuation.
     Control Values rtmodY rtmodX xw yh
  */

  double A;
  double Vx,Vy;        // view temp points
  double Mx,My;        // moderator x,y
  double D2;           // Distance ^2
  int i,j,aa,bb;       // loop variables

  D2=dist*dist;
  A=0.0;

  for(i=0;i<50;i++)              // Mod X
    {
      Mx=i*rtmodX/100.0;
      for(j=0;j<50;j++)         // Mod Y
  {
    My=j*rtmodY/100.0;
    // Position on moderator == (Mx,My)
    for(aa=-50;aa<51;aa++)  //view port
      for(bb=-50;bb<51;bb++)
        {
    Vx=aa*xw/101.0;
    Vy=bb*yh/101.0;
    A+=1.0/((Mx-Vx)*(Mx-Vx)+(My-Vy)*(My-Vy)+D2);
        }
  }
    }
  //change to Mx*My
  A*=(rtmodY*rtmodX)/(10201.0*2500.0);
  // Correct for the area of the viewport. (tables are per cm^2)
  A*=xw*yh*10000;

  //  fprintf(stderr,"Viewport == %g %g Moderator size == (%g * %g) m^2 \n",xw,yh,rtmodX,rtmodY);
  //  fprintf(stderr,"Dist == %g (metres) \n",dist);
  //  fprintf(stderr,"Viewport Solid angle == %g str\n",A/(xw*yh*10000));
  //      fprintf(stderr,"Solid angle used == %g str\n",A);
  //return A;
  return A/(xw*yh*10000);
}
