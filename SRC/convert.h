#ifndef CONVERT_H
#define CONVERT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "defines.h"

/******************************/
/** Prototypes               **/
/******************************/
// General
// -------
void      RndGen_ID2Txt(char* sText, const VtRndGen eID);
VtRndGen  RndGen_Txt2ID(const char* sText);

void      CompID2Name(char* sName, const McCompID eComp);
McCompID  Name2CompID(const char* sName);

void      Reason_ID2Txt(char* sText, const VtReason eID);
VtReason  Reason_Txt2ID(const char* sText);

void      DirType_ID2Txt(char* sText, const VtDirType eID);
VtDirType DirType_Txt2ID(const char* sText);

void      DirInOut_ID2Txt(char* sText, const VtDir eID);
VtDir     DirInOut_Txt2ID(const char* sText);

void      Axis_ID2Txt(char* sText, const VtAxis eID);
VtAxis    Axis_Txt2ID(const char* sText);

void      RotAxis_ID2Txt(char* sText, const VtRotAxis eID);
VtRotAxis RotAxis_Txt2ID(const char* sText);

void      Orient_ID2Txt(char* sText, const VtOrient eID);
VtOrient  Orient_Txt2ID(const char* sText);

void     FrameGen_ID2Txt(char* sText, const VtFrameGen eID);
VtFrameGen FrameGen_Txt2ID(const char* sText);

void      Shape_ID2Txt(char* sText, const VtShape eID);
VtShape   Shape_Txt2ID(const char* sText);

void      CompAct_ID2Txt(char* sText, const VtCompAct eID);
VtCompAct CompAct_Txt2ID(const char* sText);

void      Distr_ID2Txt(char* sText, const VtDistr eID);
VtDistr   Distr_Txt2ID(const char* sText);

void       InstGeom_ID2Txt(char* sText, const VtInstGeom eID);
VtInstGeom InstGeom_Txt2ID(const char* sText);

// Source
// ------
void      SrcName_ID2Txt(char* sText, const VtSrcName eID);
VtSrcName SrcName_Txt2ID(const char* sText);

void      SrcKind_ID2Txt(char* sText, const VtSrcKind eID);
VtSrcKind SrcKind_Txt2ID(const char* sText);

void      SrcType_ID2Txt(char* sText, const VtSrcType eID);
VtSrcType SrcType_Txt2ID(const char* sText);

void      TS_ID2Txt(char* sText, const VtTS eID);
VtTS      TS_Txt2ID(const char* sText);

void      ModType_ID2Txt(char* sText, const VtModType eID);
VtModType ModType_Txt2ID(const char* sText);

void       ModShape_ID2Txt(char* sText, const VtModShape eID);
VtModShape ModShape_Txt2ID(const char* sText);

void       Direct_ID2Txt(char* sText, const VtDirect eID);
VtDirect   Direct_Txt2ID(const char* sText);

void      ModVsn_ID2Txt(char* sText, const EssModVsn eID);
EssModVsn ModVsn_Txt2ID(const char* sText);

// Readin + WriteOut
// -----------------
void      Trace_ID2Txt(char* sText, const VtTrace eID);
VtTrace   Trace_Txt2ID(const char* sText);

void         PrgFormat_ID2Txt(char* sText, const VtPrgFormat eID);
VtPrgFormat  PrgFormat_Txt2ID(const char* sText);

void         DataFormat_ID2Txt(char* sText, const VtDataFormat eID);
VtDataFormat DataFormat_Txt2ID(const char* sText);

void        Separator_ID2Txt(char* sText, const VtSeparator eID);
VtSeparator Separator_Txt2ID(const char* sText);


// Frame
// -----
void        TfmnSeq_ID2Txt(char* sText, const VtTfmnSeq eID);
VtTfmnSeq   TfmnSeq_Txt2ID(const char* sText);

// Windows and Collimators
// -----------------------
void        AbsMat_ID2Txt(char* sText, const VtAbsMat eID);
VtAbsMat    AbsMat_Txt2ID(const char* sText);

void        WndAbs_ID2Txt(char* sText, const VtWndAbs eID);
VtWndAbs    WndAbs_Txt2ID(const char* sText);

void        Oscill_ID2Txt(char* sText, const VtOscill eID);
VtOscill    Oscill_Txt2ID(const char* sText);

void           MultWndShape_ID2Txt(char* sText, const VtMultWndShape eID);
VtMultWndShape MultWndShape_Txt2ID(const char* sText);

// Guides
// ------
void      GdeWall_ID2Txt(char* sText, const VtGdeWall eID);
VtGdeWall GdeWall_Txt2ID(const char* sText);

void       GdeShape_ID2Txt(char* sText, const VtGdeShape eID);
VtGdeShape GdeShape_Txt2ID(const char* sText);

void        WaviDistr_ID2Txt(char* sText, const VtWaviDistr eID);
VtWaviDistr WaviDistr_Txt2ID(const char* sText);

void        MirrMat_ID2Txt(char* sText, const VtMirrMat eID);
VtMirrMat   MirrMat_Txt2ID(const char* sText);

void        ListPar_ID2Txt(char* sText, const VtListPar eID);
VtListPar   ListPar_Txt2ID(const char* sText);

void        ListVbs_ID2Txt(char* sText, const VtListVbs eID);
VtListVbs   ListVbs_Txt2ID(const char* sText);

void        PlotPar_ID2Txt(char* sText, const VtPlotPar eID);
VtPlotPar   PlotPar_Txt2ID(const char* sText);

void        PlotFilt_ID2Txt(char* sText, const VtPlotFilt eID);
VtPlotFilt  PlotFilt_Txt2ID(const char* sText);

// Monochromators and choppers
// ---------------------------
void          MonoArrange_ID2Txt(char* sText, const VtMonoArrange eID);
VtMonoArrange MonoArrange_Txt2ID(const char* sText);

void        MonoType_ID2Txt(char* sText, const VtMonoType eID);
VtMonoType  MonoType_Txt2ID(const char* sText);

void        MonoMove_ID2Txt(char* sText, const VtMonoMove eID);
VtMonoMove  MonoMove_Txt2ID(const char* sText);

void        MonoFocus_ID2Txt(char* sText, const VtMonoFocus eID);
VtMonoFocus MonoFocus_Txt2ID(const char* sText);

void        ChnlShape_ID2Txt(char* sText, const VtChnlShape eID);
VtChnlShape ChnlShape_Txt2ID(const char* sText);

// Samples
// -------
void        SmplGeom_ID2Txt(char* sText, const VtSmplGeom eID);
VtSmplGeom  SmplGeom_Txt2ID(const char* sText);

void        PtclGeom_ID2Txt (char* sText, const VtPtclGeom eID);
VtPtclGeom  PtclGeom_Char2ID(const char cID);
VtPtclGeom  PtclGeom_Txt2ID (const char* sText);

void        DataSrc_ID2Txt(char* sText, const VtDataSrc eID);
VtDataSrc   DataSrc_Txt2ID(const char* sText);

void        MeasMode_ID2Txt(char* sText, const VtMeasMode eID);
VtMeasMode  MeasMode_Txt2ID(const char* sText);

// Detectors
// ---------
void        DetGeom_ID2Txt(char* sText, const VtDetGeom eID);
VtDetGeom   DetGeom_Txt2ID(const char* sText);

void        DetType_ID2Txt(char* sText, const VtDetType eID);
VtDetType   DetType_Txt2ID(const char* sText);

void        TubeShape_ID2Txt(char* sText, const VtTubeShape eID);
VtTubeShape TubeShape_Txt2ID(const char* sText);

void       DetUse_ID2Txt(char* sText, const VtDetUse eID);
VtDetUse   DetUse_Txt2ID(const char* sText);

void       DetAbs_ID2Txt(char* sText, const VtDetAbs eID);
VtDetAbs   DetAbs_Txt2ID(const char* sText);

// Monitors
// --------
void      Mon1Par_ID2Txt(char* sText, const VtMon1Par eID);
VtMon1Par Mon1Par_Txt2ID(const char* sText);

void      Mon2Par_ID2Txt(char* sText, const VtMon2Par eID);
VtMon2Par Mon2Par_Txt2ID(const char* sText);

void      MonPar_ID2Txt(char* sText, const VtMonPar eID);
VtMonPar  MonPar_Txt2ID(const char* sText);

void      MonNorm_ID2Txt(char* sText, const VtMonNorm eID);
VtMonNorm MonNorm_Txt2ID(const char* sText);

void      BrlPar_ID2Txt(char* sText, const VtBrlPar eID);
VtBrlPar  BrlPar_Txt2ID(const char* sText);

void      BrlNorm_ID2Txt(char* sText, const VtBrlNorm eID);
VtBrlNorm BrlNorm_Txt2ID(const char* sText);

void       Format2D_ID2Txt(char* sText, const VtFormat2D eID);
VtFormat2D Format2D_Txt2ID(const char* sText);

// Evaluation + Filter
// -------------------
void       FiltComb_ID2Txt(char* sText, const VtFiltComb eID);
VtFiltComb FiltComb_Txt2ID(const char* sText);

void       EvalPar_ID2Txt(char* sText, const VtEvalPar eID);
VtEvalPar  EvalPar_Txt2ID(const char* sText);

void       EvalComb_ID2Txt(char* sText, const VtEvalComb eID);
VtEvalComb EvalComb_Txt2ID(const char* sText);

void       EvalSort_ID2Txt(char* sText, const VtEvalSort eID);
VtEvalSort EvalSort_Txt2ID(const char* sText);

void       AngleSel_ID2Txt(char* sText, const VtAngleSel eID);
VtAngleSel AngleSel_Txt2ID(const char* sText);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

