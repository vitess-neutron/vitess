#ifndef SOURCEVAE_CPP 
#define SOURCEVAE_CPP

/********************************************************************************************/
/*  VITESS module 'source_vae.cpp'                                                          */
/*                                                                                          */
/* The free non-commercial use of these routines is granted                                 */
/* providing due credit is given to the authors.                                            */
/* 2 Aug 2024  J. Robledo  initial version                                                  */
/********************************************************************************************/

/**************************************************/
/** Definitions, Global Variables and Prototypes **/
/**************************************************/
#include "source_vae.h"
#include <cmath>
#include <torch/script.h>

/******************************/
/** Program                  **/
/******************************/
int main(int argc, char *argv[]){

  Neutron InNeutron;
  _eModule=MCN_SOURCE_VAE;

  Init(argc, argv, _eModule);
  PrintModuleName(_eModule, "1.0");
  OwnInit(argc, argv);

  InitNeutron(&InNeutron);
  try {
		model = torch::jit::load(ModelFileName, device);
		// model = torch::jit::optimize_for_inference(model);
    fprintf(LogFilePtr,"Loaded Correctly\n");
	}
	catch (const c10::Error& e) {
		fprintf(LogFilePtr,"Error loading the model\n");
		return -1;
	}

  
  int latentDims = model.attr("latent_dim").toInt();
  torch::Tensor x_hat = torch::zeros({nNeut,latentDims});
  double lda=0.1;


  torch::jit::Module decoder = model.attr("decoder").toModule();
  torch::Tensor maxT = model.attr("dataset_maxvals").toTensor();
  torch::Tensor minT = model.attr("dataset_minvals").toTensor();


  for (int iBnch=0; iBnch < nBunches; iBnch++)   // std::cout << x_hat << std::endl;
  {
    // sampling
    std::vector<torch::jit::IValue> inputs;
  	inputs.push_back(torch::randn({nNeut,latentDims}));
    x_hat = decoder.forward(inputs).toTensor();
    x_hat = (x_hat) * (maxT - minT) + minT;

    #pragma omp parallel for 
    // saving and outputting
    for (int nAccepted=0; nAccepted < nNeut; ++nAccepted){
      InNeutron.Wavelength = LAMBDA_FROM_ENERGY(pow(10,x_hat[nAccepted][6].item<float>())*1e12);
      InNeutron.Time = pow(10,x_hat[nAccepted][5].item<float>());
      InNeutron.Probability = sin(x_hat[nAccepted][4].item<float>()*M_PI/180.0); 
      InNeutron.Position[1] = x_hat[nAccepted][0].item<float>();
      InNeutron.Position[2] = x_hat[nAccepted][1].item<float>();
      float theta = M_PI/180.0*(pow(lda * x_hat[nAccepted][3].item<float>()+1.0,1.0/lda) -1.0);
      float phi = x_hat[nAccepted][2].item<float>();
      InNeutron.Vector[0] = cos(theta);
      InNeutron.Vector[1] = sin(theta) * cos(phi);
      InNeutron.Vector[2] =  sin(theta) * sin(phi);
      WriteNeutron(&InNeutron);
    }
    if (iBnch < nBunches)
      WriteEOB();
    }
   return 0;
}

/***************************************************************/
/* OwnInit: Reads input parameters and sets global parameters  */
/***************************************************************/
void  OwnInit(int argc, char *argv[])
{
  while(argc>1)
  {
    if(argv[1][0]!='+')
    {
      switch(argv[1][1])
      {
        case 'M':
	        ModelFileName=&argv[1][2];
           break;

        case 'n':
            nNeut = atoi(&argv[1][2]);
           break;
        case 't':
            _eTraceMode = (VtTrace) atoi(&argv[1][2]);
            break;

        case 'b':
            nBunches = atoi(&argv[1][2]);
            break;
        case 'T':
            _sTraceFileName = &argv[1][2];
            break;
        default:
            Error2("unkown command option", argv[1]);
            exit(-1);
      }
    }
    argc--;
    argv++;
  }
  return;
}


/*******************************************************/
/** Does module specific cleanup                      **/
/*******************************************************/
void OwnCleanup()
{
  // do not free(&Neutrons); because Neutrons is a static variable
    if (_aTrace!=NULL) free(_aTrace);
}


#endif
