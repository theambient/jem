/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.
 *
 * Copyright (c) 2010-2015, ITU/ISO/IEC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *  * Neither the name of the ITU/ISO/IEC nor the names of its contributors may
 *    be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/** \file     TEncEntropy.h
    \brief    entropy encoder class (header)
*/

#ifndef __TENCENTROPY__
#define __TENCENTROPY__

#include "TLibCommon/TComSlice.h"
#include "TLibCommon/TComDataCU.h"
#include "TLibCommon/TComBitStream.h"
#include "TLibCommon/ContextModel.h"
#include "TLibCommon/TComPic.h"
#include "TLibCommon/TComTrQuant.h"
#include "TLibCommon/TComSampleAdaptiveOffset.h"
#include "TLibCommon/TComChromaFormat.h"
#if ALF_HM3_REFACTOR
#include "TLibCommon//TComAdaptiveLoopFilter.h"
#endif

class TEncSbac;
class TEncCavlc;
class SEI;

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// entropy encoder pure class
class TEncEntropyIf
{
public:
  virtual Void  resetEntropy          (const TComSlice *pSlice)                = 0;
  virtual SliceType determineCabacInitIdx (const TComSlice *pSlice)                = 0;
  virtual Void  setBitstream          ( TComBitIf* p )  = 0;
  virtual Void  resetBits             ()                = 0;
  virtual UInt  getNumberOfWrittenBits()                = 0;

  virtual Void  codeVPS                 ( const TComVPS* pcVPS )                                      = 0;
  virtual Void  codeSPS                 ( const TComSPS* pcSPS )                                      = 0;
  virtual Void  codePPS                 ( const TComPPS* pcPPS )                                      = 0;
  virtual Void  codeSliceHeader         ( TComSlice* pcSlice )                                  = 0;

  virtual Void  codeTilesWPPEntryPoint  ( TComSlice* pSlice )     = 0;
  virtual Void  codeTerminatingBit      ( UInt uilsLast )                                       = 0;
  virtual Void  codeSliceFinish         ()                                                      = 0;
  virtual Void  codeMVPIdx ( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList ) = 0;

public:
  virtual Void codeCUTransquantBypassFlag( TComDataCU* pcCU, UInt uiAbsPartIdx ) = 0;
  virtual Void codeSkipFlag      ( TComDataCU* pcCU, UInt uiAbsPartIdx ) = 0;
#if VCEG_AZ05_INTRA_MPI
  virtual Void codeMPIIdx        (TComDataCU* pcCU, UInt uiAbsPartIdx)   = 0;
#endif
#if COM16_C1046_PDPC_INTRA
  virtual Void codePDPCIdx       (TComDataCU* pcCU, UInt uiAbsPartIdx) = 0;
#endif
#if VCEG_AZ05_ROT_TR || COM16_C1044_NSST
   virtual Void codeROTIdx     ( TComDataCU* pcCU, UInt uiAbsPartIdx,UInt uiDepth ) = 0;
#if CU_TU_NSST
   virtual Void codeTuROTIdx     ( TComDataCU* pcCU, UInt uiAbsPartIdx,UInt uiDepth ) = 0;
#endif
#endif
#if VCEG_AZ07_IMV
  virtual Void codeiMVFlag       ( TComDataCU* pcCU, UInt uiAbsPartIdx ) = 0;
#endif
#if COM16_C806_OBMC
  virtual Void codeOBMCFlag      ( TComDataCU* pcCU, UInt uiAbsPartIdx ) = 0;
#endif
#if VCEG_AZ06_IC
  virtual Void codeICFlag        ( TComDataCU* pcCU, UInt uiAbsPartIdx ) = 0;
#endif
  virtual Void codeMergeFlag     ( TComDataCU* pcCU, UInt uiAbsPartIdx ) = 0;
  virtual Void codeMergeIndex    ( TComDataCU* pcCU, UInt uiAbsPartIdx ) = 0;
#if VCEG_AZ07_FRUC_MERGE
  virtual Void codeFRUCMgrMode   ( TComDataCU* pcCU, UInt uiAbsPartIdx , UInt uiPUIdx ) = 0;
#endif
  virtual Void codeSplitFlag     ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth ) = 0;

  virtual Void codePartSize      ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth ) = 0;
  virtual Void codePredMode      ( TComDataCU* pcCU, UInt uiAbsPartIdx ) = 0;

  virtual Void codeIPCMInfo      ( TComDataCU* pcCU, UInt uiAbsPartIdx ) = 0;

  virtual Void codeTransformSubdivFlag( UInt uiSymbol, UInt uiCtx ) = 0;
  virtual Void codeQtCbf         ( TComTU &rTu, const ComponentID compID, const Bool lowestLevel ) = 0;
  virtual Void codeQtRootCbf     ( TComDataCU* pcCU, UInt uiAbsPartIdx ) = 0;
  virtual Void codeQtCbfZero     ( TComTU &rTu, const ChannelType chType ) = 0;
  virtual Void codeQtRootCbfZero ( ) = 0;
  virtual Void codeIntraDirLumaAng( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool isMultiplePU 
#if VCEG_AZ07_INTRA_65ANG_MODES
    , Int* piModes = NULL, Int iAboveLeftCase = -1
#endif
    ) = 0;

  virtual Void codeIntraDirChroma( TComDataCU* pcCU, UInt uiAbsPartIdx ) = 0;
  virtual Void codeInterDir      ( TComDataCU* pcCU, UInt uiAbsPartIdx ) = 0;
  virtual Void codeRefFrmIdx     ( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList )      = 0;
  virtual Void codeMvd           ( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList )      = 0;

  virtual Void codeCrossComponentPrediction( TComTU &rTu, ComponentID compID ) = 0;

  virtual Void codeDeltaQP       ( TComDataCU* pcCU, UInt uiAbsPartIdx ) = 0;
  virtual Void codeChromaQpAdjustment( TComDataCU* pcCU, UInt uiAbsPartIdx ) = 0;
  #if CU_TU_NSST
  virtual Int codeCoeffNxN      ( TComTU &rTu, TCoeff* pcCoef, const ComponentID compID 
#if VCEG_AZ05_ROT_TR    || VCEG_AZ05_INTRA_MPI || COM16_C1044_NSST || COM16_C1046_PDPC_INTRA
    , int& bCbfCU
#endif
) = 0;
  #else
  virtual Void codeCoeffNxN      ( TComTU &rTu, TCoeff* pcCoef, const ComponentID compID 
#if VCEG_AZ05_ROT_TR    || VCEG_AZ05_INTRA_MPI || COM16_C1044_NSST || COM16_C1046_PDPC_INTRA
    , int& bCbfCU
#endif
    ) = 0;
#endif
  virtual Void codeTransformSkipFlags ( TComTU &rTu, ComponentID component ) = 0;
#if VCEG_AZ08_KLT_COMMON
  virtual Void codeKLTFlags      (TComTU &rTu, ComponentID component) = 0;
#endif
  virtual Void codeSAOBlkParam   (SAOBlkParam& saoBlkParam, const BitDepths &bitDepths, Bool* sliceEnabled, Bool leftMergeAvail, Bool aboveMergeAvail, Bool onlyEstMergeInfo = false)    =0;
  virtual Void estBit               (estBitsSbacStruct* pcEstBitsSbac, Int width, Int height, ChannelType chType) = 0;

  virtual Void codeExplicitRdpcmMode ( TComTU &rTu, const ComponentID compID ) = 0;

  virtual ~TEncEntropyIf() {}

#if VCEG_AZ07_BAC_ADAPT_WDOW || VCEG_AZ07_INIT_PREVFRAME
  virtual TComStats* getStatesHandle () = 0;
  virtual Void setStatesHandle   ( TComStats* pcStats ) = 0;
#if VCEG_AZ07_BAC_ADAPT_WDOW 
  virtual Void codeCtxUpdateInfo ( TComSlice* pcSlice,  TComStats* apcStats ) = 0;
#endif
#endif

#if ALF_HM3_REFACTOR
  virtual Bool getAlfCtrl()                = 0;
  virtual UInt getMaxAlfCtrlDepth()                = 0;
  virtual Void setAlfCtrl(Bool bAlfCtrl)                = 0;
  virtual Void setMaxAlfCtrlDepth(UInt uiMaxAlfCtrlDepth)                = 0;
  virtual Void codeAlfCtrlDepth     ( UInt uiMaxTotalCUDepth ) = 0;
  virtual Void codeAlfCtrlFlag      ( TComDataCU* pcCU, UInt uiAbsPartIdx ) = 0;
  virtual Void codeAlfFlag          ( UInt uiCode ) = 0;
  virtual Void codeAlfUvlc          ( UInt uiCode ) = 0;
  virtual Void codeAlfSvlc          ( Int   iCode ) = 0;
  virtual Void codeAlfFlagNum       ( UInt uiCode, UInt minValue ) = 0;
  virtual Void codeAlfCtrlFlag      ( UInt uiSymbol ) = 0;
#endif

#if COM16_C806_EMT
  virtual Void codeEmtTuIdx      ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth ) = 0;
  virtual Void codeEmtCuFlag     ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, Bool bRootCbf ) = 0;
#endif

#if COM16_C1016_AFFINE
  virtual Void codeAffineFlag    ( TComDataCU* pcCU, UInt uiAbsPartIdx ) = 0;
#endif
};

/// entropy encoder class
class TEncEntropy
{
#if VCEG_AZ07_BAC_ADAPT_WDOW || VCEG_AZ07_INIT_PREVFRAME
private:
  TComStats*                      m_pcStats;
#endif

public:
  Void    setEntropyCoder           ( TEncEntropyIf* e );
  Void    setBitstream              ( TComBitIf* p )          { m_pcEntropyCoderIf->setBitstream(p);  }
  Void    resetBits                 ()                        { m_pcEntropyCoderIf->resetBits();      }
  UInt    getNumberOfWrittenBits    ()                        { return m_pcEntropyCoderIf->getNumberOfWrittenBits(); }
#if VCEG_AZ07_BAC_ADAPT_WDOW || VCEG_AZ07_INIT_PREVFRAME
  Void    resetEntropy              (const TComSlice *pSlice)                        
  {
    m_pcEntropyCoderIf->setStatesHandle (m_pcStats);
    m_pcEntropyCoderIf->resetEntropy(pSlice);  
  }
#else
  Void    resetEntropy              (const TComSlice *pSlice) { m_pcEntropyCoderIf->resetEntropy(pSlice);  }
#endif

  SliceType determineCabacInitIdx   (const TComSlice *pSlice) { return m_pcEntropyCoderIf->determineCabacInitIdx(pSlice); }

  Void    encodeSliceHeader         ( TComSlice* pcSlice );
  Void    encodeTilesWPPEntryPoint( TComSlice* pSlice );
  Void    encodeTerminatingBit      ( UInt uiIsLast );
  Void    encodeSliceFinish         ();
  TEncEntropyIf*      m_pcEntropyCoderIf;
#if VCEG_AZ07_BAC_ADAPT_WDOW
  TEncSbac* getCABACCoder  ( ) { return (TEncSbac*)m_pcEntropyCoderIf; }
  Void encodeCtxUpdateInfo ( TComSlice* pcSlice,  TComStats* apcStats );
#endif

public:
  Void encodeVPS               ( const TComVPS* pcVPS);
  // SPS
  Void encodeSPS               ( const TComSPS* pcSPS );
  Void encodePPS               ( const TComPPS* pcPPS );
  Void encodeSplitFlag         ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, Bool bRD = false );
  Void encodeCUTransquantBypassFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD = false );
  Void encodeSkipFlag          ( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD = false );
#if COM16_C806_OBMC
  Void encodeOBMCFlag          ( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD = false );
#endif
#if VCEG_AZ07_IMV
  Void encodeiMVFlag           ( TComDataCU* pcCU, UInt uiAbsPartIdx );
#endif
#if VCEG_AZ06_IC
  Void encodeICFlag            ( TComDataCU* pcCU, UInt uiAbsPartIdx );
#endif
  Void encodePUWise       ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  Void encodeInterDirPU   ( TComDataCU* pcSubCU, UInt uiAbsPartIdx  );
  Void encodeRefFrmIdxPU  ( TComDataCU* pcSubCU, UInt uiAbsPartIdx, RefPicList eRefList );
  Void encodeMvdPU        ( TComDataCU* pcSubCU, UInt uiAbsPartIdx, RefPicList eRefList );
  Void encodeMVPIdxPU     ( TComDataCU* pcSubCU, UInt uiAbsPartIdx, RefPicList eRefList );
  Void encodeMergeFlag    ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  Void encodeMergeIndex   ( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD = false );
#if VCEG_AZ05_INTRA_MPI
  Void encodeMPIIdx       ( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD = false);
#endif
#if COM16_C1046_PDPC_INTRA
  Void encodePDPCIdx      ( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD = false);
#endif
#if VCEG_AZ05_ROT_TR || COM16_C1044_NSST
    Void encodeROTIdx    ( TComDataCU* pcCU, UInt uiAbsPartIdx,UInt uiDepth, Bool bRD = false );
#endif
#if CU_TU_NSST
   Void encodeTuROTIdx    ( TComDataCU* pcCU, UInt uiAbsPartIdx,UInt uiDepth, Bool bRD = false );
#endif
#if VCEG_AZ07_FRUC_MERGE
  Void encodeFRUCMgrMode  ( TComDataCU* pcCU, UInt uiAbsPartIdx , UInt uiPUIdx );
#endif
  Void encodePredMode          ( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD = false );
  Void encodePartSize          ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, Bool bRD = false );
  Void encodeIPCMInfo          ( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD = false );
  Void encodePredInfo          ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  Void encodeIntraDirModeLuma  ( TComDataCU* pcCU, UInt absPartIdx, Bool isMultiplePU = false 
#if VCEG_AZ07_INTRA_65ANG_MODES
    , Int* piModes = NULL, Int iAboveLeftCase = -1
#endif
    );

  Void encodeIntraDirModeChroma( TComDataCU* pcCU, UInt uiAbsPartIdx );

  Void encodeTransformSubdivFlag( UInt uiSymbol, UInt uiCtx );
  Void encodeQtCbf             ( TComTU &rTu, const ComponentID compID, const Bool lowestLevel );

  Void encodeQtCbfZero         ( TComTU &rTu, const ChannelType chType );
  Void encodeQtRootCbfZero     ( );
  Void encodeQtRootCbf         ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  Void encodeQP                ( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD = false );
  Void encodeChromaQpAdjustment ( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD = false );

  Void encodeCrossComponentPrediction( TComTU &rTu, ComponentID compID );

private:
#if CU_TU_NSST
  Int xEncodeTransform        ( Bool& bCodeDQP, Bool& codeChromaQpAdj, TComTU &rTu
#if VCEG_AZ05_ROT_TR    || VCEG_AZ05_INTRA_MPI || COM16_C1044_NSST || COM16_C1046_PDPC_INTRA
    , Int& bCbfCU
#endif
#if CU_TU_NSST
, Int& TuCmap
, UInt& uiAbsTu
#endif
    );

#else
  Void xEncodeTransform        ( Bool& bCodeDQP, Bool& codeChromaQpAdj, TComTU &rTu
#if VCEG_AZ05_ROT_TR    || VCEG_AZ05_INTRA_MPI || COM16_C1044_NSST || COM16_C1046_PDPC_INTRA
    , Int& bCbfCU
#endif
    );
#endif
public:
  Void encodeCoeff             ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, Bool& bCodeDQP, Bool& codeChromaQpAdj
#if VCEG_AZ05_ROT_TR  || VCEG_AZ05_INTRA_MPI || COM16_C1044_NSST || COM16_C1046_PDPC_INTRA
    , Int& bNonZeroCoeff
#endif
#if CU_TU_NSST
#endif
    );

  Void encodeCoeffNxN         ( TComTU &rTu, TCoeff* pcCoef, const ComponentID compID );

  Void estimateBit             ( estBitsSbacStruct* pcEstBitsSbac, Int width, Int height, ChannelType chType );

  Void encodeSAOBlkParam(SAOBlkParam& saoBlkParam, const BitDepths &bitDepths, Bool* sliceEnabled, Bool leftMergeAvail, Bool aboveMergeAvail){m_pcEntropyCoderIf->codeSAOBlkParam(saoBlkParam, bitDepths, sliceEnabled, leftMergeAvail, aboveMergeAvail, false);}

  static Int countNonZeroCoeffs( TCoeff* pcCoef, UInt uiSize );

#if VCEG_AZ07_BAC_ADAPT_WDOW || VCEG_AZ07_INIT_PREVFRAME
  Void setStatsHandle       ( TComStats*  pcStats)  { m_pcStats=pcStats; }
  TComStats* getStatsHandle ()                      { return m_pcStats;  }
#endif 

#if ALF_HM3_REFACTOR
  Void encodeAlfParam(ALFParam* pAlfParam, UInt uiMaxTotalCUDepth
#if FIX_TICKET12
        ,const TComSlice * pSlice
#endif
    );
  Void encodeAlfCtrlFlag       ( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD = false );
  Void encodeAlfCtrlParam      ( ALFParam *pAlfParam );
  Bool getAlfCtrl() {return m_pcEntropyCoderIf->getAlfCtrl();}
  UInt getMaxAlfCtrlDepth() {return m_pcEntropyCoderIf->getMaxAlfCtrlDepth();}
  Void setAlfCtrl(Bool bAlfCtrl) {m_pcEntropyCoderIf->setAlfCtrl(bAlfCtrl);}
  Void setMaxAlfCtrlDepth(UInt uiMaxAlfCtrlDepth) {m_pcEntropyCoderIf->setMaxAlfCtrlDepth(uiMaxAlfCtrlDepth);}
  Void codeAuxCountBit(ALFParam* pAlfParam, Int64* ruiRate, const TComSlice * pSlice);
  Void codeFiltCountBit(ALFParam* pAlfParam, Int64* ruiRate, const TComSlice * pSlice);
  Void codeAux (ALFParam* pAlfParam);
  Void codeFilt (ALFParam* pAlfParam);
  Int codeFilterCoeff(ALFParam* ALFp);
  Int writeFilterCodingParams(int minKStart, int maxScanVal, int kMinTab[]);
  Int writeFilterCoeffs(int sqrFiltLength, int filters_per_group, const int pDepthInt[], 
    int **FilterCoeff, int kMinTab[]);
  Int golombEncode(int coeff, int k);
  Int lengthGolomb(int coeffVal, int k);
#endif
#if COM16_C806_EMT
  Void encodeEmtCuFlag       ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, Bool bCodeCuFlag );
#endif

#if COM16_C1016_AFFINE
  Void encodeAffineFlag      ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiPuIdx );
#endif
};// END CLASS DEFINITION TEncEntropy

//! \}

#endif // __TENCENTROPY__
