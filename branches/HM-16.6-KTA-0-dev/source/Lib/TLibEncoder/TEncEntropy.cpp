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

/** \file     TEncEntropy.cpp
    \brief    entropy encoder class
*/

#include "TEncEntropy.h"
#include "TLibCommon/CommonDef.h"
#include "TLibCommon/TComSampleAdaptiveOffset.h"
#include "TLibCommon/TComTU.h"
#if ALF_HM3_REFACTOR
#include "TLibCommon/TComAdaptiveLoopFilter.h"
#include <cmath>
#endif

#if ENVIRONMENT_VARIABLE_DEBUG_AND_TEST
#include "../TLibCommon/Debug.h"
static const Bool bDebugPredEnabled = DebugOptionList::DebugPred.getInt()!=0;
#endif

//! \ingroup TLibEncoder
//! \{
#if VCEG_AZ07_BAC_ADAPT_WDOW
Void TEncEntropy::encodeCtxUpdateInfo( TComSlice* pcSlice,  TComStats* apcStats )
{
  m_pcEntropyCoderIf->codeCtxUpdateInfo( pcSlice, apcStats );
  return;
}
#endif

Void TEncEntropy::setEntropyCoder ( TEncEntropyIf* e )
{
  m_pcEntropyCoderIf = e;
}

Void TEncEntropy::encodeSliceHeader ( TComSlice* pcSlice )
{
  m_pcEntropyCoderIf->codeSliceHeader( pcSlice );
  return;
}

Void  TEncEntropy::encodeTilesWPPEntryPoint( TComSlice* pSlice )
{
  m_pcEntropyCoderIf->codeTilesWPPEntryPoint( pSlice );
}

Void TEncEntropy::encodeTerminatingBit      ( UInt uiIsLast )
{
  m_pcEntropyCoderIf->codeTerminatingBit( uiIsLast );

  return;
}

Void TEncEntropy::encodeSliceFinish()
{
  m_pcEntropyCoderIf->codeSliceFinish();
}

Void TEncEntropy::encodePPS( const TComPPS* pcPPS )
{
  m_pcEntropyCoderIf->codePPS( pcPPS );
  return;
}

Void TEncEntropy::encodeSPS( const TComSPS* pcSPS )
{
  m_pcEntropyCoderIf->codeSPS( pcSPS );
  return;
}

Void TEncEntropy::encodeCUTransquantBypassFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD )
{
  if( bRD )
  {
    uiAbsPartIdx = 0;
  }
  m_pcEntropyCoderIf->codeCUTransquantBypassFlag( pcCU, uiAbsPartIdx );
}

Void TEncEntropy::encodeVPS( const TComVPS* pcVPS )
{
  m_pcEntropyCoderIf->codeVPS( pcVPS );
  return;
}

Void TEncEntropy::encodeSkipFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD )
{
  if ( pcCU->getSlice()->isIntra() )
  {
    return;
  }
  if( bRD )
  {
    uiAbsPartIdx = 0;
  }
  m_pcEntropyCoderIf->codeSkipFlag( pcCU, uiAbsPartIdx );
}

#if VCEG_AZ07_IMV
Void TEncEntropy::encodeiMVFlag( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  assert( pcCU->getSlice()->getSPS()->getIMV() );
  if ( pcCU->getSlice()->isIntra() )
  {
    return;
  }
  else if( pcCU->getMergeFlag( uiAbsPartIdx ) && pcCU->getPartitionSize( uiAbsPartIdx ) == SIZE_2Nx2N )
  {
    assert( pcCU->getiMVFlag( uiAbsPartIdx ) == 0 );
    return;
  }
  m_pcEntropyCoderIf->codeiMVFlag( pcCU, uiAbsPartIdx );
}
#endif

#if COM16_C806_OBMC
Void TEncEntropy::encodeOBMCFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD )
{
  if( bRD )
  {
    uiAbsPartIdx = 0;
  }

  if ( !pcCU->getSlice()->getSPS()->getOBMC() || !pcCU->isOBMCFlagCoded( uiAbsPartIdx ) )
  {
    return;
  }

  m_pcEntropyCoderIf->codeOBMCFlag( pcCU, uiAbsPartIdx );
}
#endif

#if VCEG_AZ06_IC
Void TEncEntropy::encodeICFlag( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  if( pcCU->isICFlagCoded( uiAbsPartIdx ) )
  {
    m_pcEntropyCoderIf->codeICFlag( pcCU, uiAbsPartIdx );
  }
}
#endif

#if VCEG_AZ05_INTRA_MPI
Void TEncEntropy::encodeMPIIdx(TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD)
{
  if (bRD)
  {
    uiAbsPartIdx = 0;
  }
  // at least one merge candidate existsput return when not encoded
  m_pcEntropyCoderIf->codeMPIIdx(pcCU, uiAbsPartIdx);
}
#endif

//! encode merge flag
Void TEncEntropy::encodeMergeFlag( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  // at least one merge candidate exists
  m_pcEntropyCoderIf->codeMergeFlag( pcCU, uiAbsPartIdx );
}

//! encode merge index
Void TEncEntropy::encodeMergeIndex( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD )
{
  if( bRD )
  {
    uiAbsPartIdx = 0;
    assert( pcCU->getPartitionSize(uiAbsPartIdx) == SIZE_2Nx2N );
  }
  m_pcEntropyCoderIf->codeMergeIndex( pcCU, uiAbsPartIdx );
}

#if VCEG_AZ07_FRUC_MERGE
Void TEncEntropy::encodeFRUCMgrMode( TComDataCU* pcCU, UInt uiAbsPartIdx , UInt uiPUIdx )
{ 
  m_pcEntropyCoderIf->codeFRUCMgrMode( pcCU, uiAbsPartIdx , uiPUIdx );
}
#endif

//! encode prediction mode
Void TEncEntropy::encodePredMode( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD )
{
  if( bRD )
  {
    uiAbsPartIdx = 0;
  }

  if ( pcCU->getSlice()->isIntra() )
  {
    return;
  }

  m_pcEntropyCoderIf->codePredMode( pcCU, uiAbsPartIdx );
}

//! encode split flag
Void TEncEntropy::encodeSplitFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, Bool bRD )
{
  if( bRD )
  {
    uiAbsPartIdx = 0;
  }

  m_pcEntropyCoderIf->codeSplitFlag( pcCU, uiAbsPartIdx, uiDepth );
}

#if COM16_C806_EMT
Void TEncEntropy::encodeEmtCuFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, Bool bCodeCuFlag )
{
  m_pcEntropyCoderIf->codeEmtCuFlag( pcCU, uiAbsPartIdx, uiDepth, bCodeCuFlag );
}
#endif

//! encode partition size
Void TEncEntropy::encodePartSize( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, Bool bRD )
{
  if( bRD )
  {
    uiAbsPartIdx = 0;
  }

  m_pcEntropyCoderIf->codePartSize( pcCU, uiAbsPartIdx, uiDepth );
}


/** Encode I_PCM information.
 * \param pcCU          pointer to CU
 * \param uiAbsPartIdx  CU index
 * \param bRD           flag indicating estimation or encoding
 */
Void TEncEntropy::encodeIPCMInfo( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD )
{
  if(!pcCU->getSlice()->getSPS()->getUsePCM()
    || pcCU->getWidth(uiAbsPartIdx) > (1<<pcCU->getSlice()->getSPS()->getPCMLog2MaxSize())
    || pcCU->getWidth(uiAbsPartIdx) < (1<<pcCU->getSlice()->getSPS()->getPCMLog2MinSize()))
  {
    return;
  }

  if( bRD )
  {
    uiAbsPartIdx = 0;
  }

  m_pcEntropyCoderIf->codeIPCMInfo ( pcCU, uiAbsPartIdx );

}

Void TEncEntropy::xEncodeTransform( Bool& bCodeDQP, Bool& codeChromaQpAdj, TComTU &rTu
#if VCEG_AZ05_INTRA_MPI
  , Int& bCbfCU
#endif
  )
{
//pcCU, absPartIdxCU, uiAbsPartIdx, uiDepth+1, uiTrIdx+1, quadrant,
  TComDataCU *pcCU=rTu.getCU();
  const UInt uiAbsPartIdx=rTu.GetAbsPartIdxTU();
  const UInt numValidComponent = pcCU->getPic()->getNumberValidComponents();
  const Bool bChroma = isChromaEnabled(pcCU->getPic()->getChromaFormat());
  const UInt uiTrIdx = rTu.GetTransformDepthRel();
  const UInt uiDepth = rTu.GetTransformDepthTotal();
#if ENVIRONMENT_VARIABLE_DEBUG_AND_TEST
  const Bool bDebugRQT=pcCU->getSlice()->getFinalized() && DebugOptionList::DebugRQT.getInt()!=0;
  if (bDebugRQT)
  {
    printf("x..codeTransform: offsetLuma=%d offsetChroma=%d absPartIdx=%d, uiDepth=%d\n width=%d, height=%d, uiTrIdx=%d, uiInnerQuadIdx=%d\n",
           rTu.getCoefficientOffset(COMPONENT_Y), rTu.getCoefficientOffset(COMPONENT_Cb), uiAbsPartIdx, uiDepth, rTu.getRect(COMPONENT_Y).width, rTu.getRect(COMPONENT_Y).height, rTu.GetTransformDepthRel(), rTu.GetSectionNumber());
  }
#endif
  const UInt uiSubdiv = pcCU->getTransformIdx( uiAbsPartIdx ) > uiTrIdx;// + pcCU->getDepth( uiAbsPartIdx ) > uiDepth;
  const UInt uiLog2TrafoSize = rTu.GetLog2LumaTrSize();


  UInt cbf[MAX_NUM_COMPONENT] = {0,0,0};
  Bool bHaveACodedBlock       = false;
  Bool bHaveACodedChromaBlock = false;

  for(UInt ch=0; ch<numValidComponent; ch++)
  {
    const ComponentID compID = ComponentID(ch);

    cbf[compID] = pcCU->getCbf( uiAbsPartIdx, compID , uiTrIdx );
    
    if (cbf[ch] != 0)
    {
      bHaveACodedBlock = true;
      if (isChroma(compID))
      {
        bHaveACodedChromaBlock = true;
      }
    }
  }

  if( pcCU->isIntra(uiAbsPartIdx) && pcCU->getPartitionSize(uiAbsPartIdx) == SIZE_NxN && uiDepth == pcCU->getDepth(uiAbsPartIdx) )
  {
    assert( uiSubdiv );
  }
  else if( pcCU->isInter(uiAbsPartIdx) && (pcCU->getPartitionSize(uiAbsPartIdx) != SIZE_2Nx2N) && uiDepth == pcCU->getDepth(uiAbsPartIdx) &&  (pcCU->getSlice()->getSPS()->getQuadtreeTUMaxDepthInter() == 1) )
  {
    if ( uiLog2TrafoSize > pcCU->getQuadtreeTULog2MinSizeInCU(uiAbsPartIdx) )
    {
      assert( uiSubdiv );
    }
    else
    {
      assert(!uiSubdiv );
    }
  }
  else if( uiLog2TrafoSize > pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() )
  {
    assert( uiSubdiv );
  }
  else if( uiLog2TrafoSize == pcCU->getSlice()->getSPS()->getQuadtreeTULog2MinSize() )
  {
    assert( !uiSubdiv );
  }
  else if( uiLog2TrafoSize == pcCU->getQuadtreeTULog2MinSizeInCU(uiAbsPartIdx) )
  {
    assert( !uiSubdiv );
  }
  else
  {
    assert( uiLog2TrafoSize > pcCU->getQuadtreeTULog2MinSizeInCU(uiAbsPartIdx) );
#if COM16_C806_T64
    m_pcEntropyCoderIf->codeTransformSubdivFlag( uiSubdiv, pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrafoSize );
#else
    m_pcEntropyCoderIf->codeTransformSubdivFlag( uiSubdiv, 5 - uiLog2TrafoSize );
#endif
  }

  const UInt uiTrDepthCurr = uiDepth - pcCU->getDepth( uiAbsPartIdx );
  const Bool bFirstCbfOfCU = uiTrDepthCurr == 0;

  for(UInt ch=COMPONENT_Cb; ch<numValidComponent; ch++)
  {
    const ComponentID compID=ComponentID(ch);
    if( bFirstCbfOfCU || rTu.ProcessingAllQuadrants(compID) )
    {
      if( bFirstCbfOfCU || pcCU->getCbf( uiAbsPartIdx, compID, uiTrDepthCurr - 1 ) )
      {
        m_pcEntropyCoderIf->codeQtCbf( rTu, compID, (uiSubdiv == 0) );
      }
    }
    else
    {
      assert( pcCU->getCbf( uiAbsPartIdx, compID, uiTrDepthCurr ) == pcCU->getCbf( uiAbsPartIdx, compID, uiTrDepthCurr - 1 ) );
    }
  }

  if( uiSubdiv )
  {
    TComTURecurse tuRecurseChild(rTu, true);
#if COM16_C806_EMT
    if( bFirstCbfOfCU )
    {
      m_pcEntropyCoderIf->codeEmtCuFlag( pcCU, uiAbsPartIdx, uiDepth, true );
    }
#endif
    do
    {
      xEncodeTransform( bCodeDQP, codeChromaQpAdj, tuRecurseChild 
#if VCEG_AZ05_INTRA_MPI
        , bCbfCU
#endif
        );
    } while (tuRecurseChild.nextSection(rTu));
  }
  else
  {
    {
      DTRACE_CABAC_VL( g_nSymbolCounter++ );
      DTRACE_CABAC_T( "\tTrIdx: abspart=" );
      DTRACE_CABAC_V( uiAbsPartIdx );
      DTRACE_CABAC_T( "\tdepth=" );
      DTRACE_CABAC_V( uiDepth );
      DTRACE_CABAC_T( "\ttrdepth=" );
      DTRACE_CABAC_V( pcCU->getTransformIdx( uiAbsPartIdx ) );
      DTRACE_CABAC_T( "\n" );
    }

    if( !pcCU->isIntra(uiAbsPartIdx) && uiDepth == pcCU->getDepth( uiAbsPartIdx ) && (!bChroma || (!pcCU->getCbf( uiAbsPartIdx, COMPONENT_Cb, 0 ) && !pcCU->getCbf( uiAbsPartIdx, COMPONENT_Cr, 0 ) ) ) )
    {
      assert( pcCU->getCbf( uiAbsPartIdx, COMPONENT_Y, 0 ) );
      //      printf( "saved one bin! " );
    }
    else
    {
      m_pcEntropyCoderIf->codeQtCbf( rTu, COMPONENT_Y, true ); //luma CBF is always at the lowest level
    }

#if COM16_C806_EMT
    if( bFirstCbfOfCU )
    {
      m_pcEntropyCoderIf->codeEmtCuFlag( pcCU, uiAbsPartIdx, uiDepth, cbf[COMPONENT_Y] ? true : false );
    }
#endif

    if ( bHaveACodedBlock )
    {
      // dQP: only for CTU once
      if ( pcCU->getSlice()->getPPS()->getUseDQP() )
      {
        if ( bCodeDQP )
        {
          encodeQP( pcCU, rTu.GetAbsPartIdxCU() );
          bCodeDQP = false;
        }
      }

      if ( pcCU->getSlice()->getUseChromaQpAdj() )
      {
        if ( bHaveACodedChromaBlock && codeChromaQpAdj && !pcCU->getCUTransquantBypass(rTu.GetAbsPartIdxCU()) )
        {
          encodeChromaQpAdjustment( pcCU, rTu.GetAbsPartIdxCU() );
          codeChromaQpAdj = false;
        }
      }

      const UInt numValidComp=pcCU->getPic()->getNumberValidComponents();

      for(UInt ch=COMPONENT_Y; ch<numValidComp; ch++)
      {
        const ComponentID compID=ComponentID(ch);

        if (rTu.ProcessComponentSection(compID))
        {
#if ENVIRONMENT_VARIABLE_DEBUG_AND_TEST
          if (bDebugRQT)
          {
            printf("Call NxN for chan %d width=%d height=%d cbf=%d\n", compID, rTu.getRect(compID).width, rTu.getRect(compID).height, 1);
          }
#endif

          if (rTu.getRect(compID).width != rTu.getRect(compID).height)
          {
            //code two sub-TUs
            TComTURecurse subTUIterator(rTu, false, TComTU::VERTICAL_SPLIT, true, compID);

            do
            {
              const UChar subTUCBF = pcCU->getCbf(subTUIterator.GetAbsPartIdxTU(compID), compID, (uiTrIdx + 1));

              if (subTUCBF != 0)
              {
#if ENVIRONMENT_VARIABLE_DEBUG_AND_TEST
                if (bDebugRQT)
                {
                  printf("Call NxN for chan %d width=%d height=%d cbf=%d\n", compID, subTUIterator.getRect(compID).width, subTUIterator.getRect(compID).height, 1);
                }
#endif
                m_pcEntropyCoderIf->codeCoeffNxN( subTUIterator, (pcCU->getCoeff(compID) + subTUIterator.getCoefficientOffset(compID)), compID 
#if VCEG_AZ05_INTRA_MPI
                  , bCbfCU
#endif
                  );
              }
#if COM16_C806_EMT
              else if ( isLuma(compID) && cbf[COMPONENT_Y] != 0 )
              {
                pcCU->setEmtTuIdxSubParts( DCT2_EMT, uiAbsPartIdx, uiDepth );
              }
#endif
            }
            while (subTUIterator.nextSection(rTu));
          }
          else
          {
            if (isChroma(compID) && (cbf[COMPONENT_Y] != 0))
            {
              m_pcEntropyCoderIf->codeCrossComponentPrediction( rTu, compID );
            }

            if (cbf[compID] != 0)
            {
              m_pcEntropyCoderIf->codeCoeffNxN( rTu, (pcCU->getCoeff(compID) + rTu.getCoefficientOffset(compID)), compID
#if VCEG_AZ05_INTRA_MPI
                , bCbfCU
#endif
                );
            }
#if COM16_C806_EMT
            else if ( isLuma(compID) && cbf[COMPONENT_Y] != 0 )
            {
              pcCU->setEmtTuIdxSubParts( DCT2_EMT, uiAbsPartIdx, uiDepth );
            }
#endif
          }
        }
      }
    }
  }
}


//! encode intra direction for luma
Void TEncEntropy::encodeIntraDirModeLuma  ( TComDataCU* pcCU, UInt absPartIdx, Bool isMultiplePU 
#if VCEG_AZ07_INTRA_65ANG_MODES
                                           , Int* piModes, Int  iAboveLeftCase
#endif
                                           )
{
  m_pcEntropyCoderIf->codeIntraDirLumaAng( pcCU, absPartIdx , isMultiplePU
#if VCEG_AZ07_INTRA_65ANG_MODES
    , piModes, iAboveLeftCase
#endif
    );
}


//! encode intra direction for chroma
Void TEncEntropy::encodeIntraDirModeChroma( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  m_pcEntropyCoderIf->codeIntraDirChroma( pcCU, uiAbsPartIdx );

#if ENVIRONMENT_VARIABLE_DEBUG_AND_TEST
  if (bDebugPredEnabled && pcCU->getSlice()->getFinalized())
  {
    UInt cdir=pcCU->getIntraDir(CHANNEL_TYPE_CHROMA, uiAbsPartIdx);
    if (cdir==36)
    {
      cdir=pcCU->getIntraDir(CHANNEL_TYPE_LUMA, uiAbsPartIdx);
    }
    printf("coding chroma Intra dir: %d, uiAbsPartIdx: %d, luma dir: %d\n", cdir, uiAbsPartIdx, pcCU->getIntraDir(CHANNEL_TYPE_LUMA, uiAbsPartIdx));
  }
#endif
}


Void TEncEntropy::encodePredInfo( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  if( pcCU->isIntra( uiAbsPartIdx ) )                                 // If it is Intra mode, encode intra prediction mode.
  {
    encodeIntraDirModeLuma  ( pcCU, uiAbsPartIdx,true );
    if (pcCU->getPic()->getChromaFormat()!=CHROMA_400)
    {
      encodeIntraDirModeChroma( pcCU, uiAbsPartIdx );

      if (enable4ChromaPUsInIntraNxNCU(pcCU->getPic()->getChromaFormat()) && pcCU->getPartitionSize( uiAbsPartIdx )==SIZE_NxN)
      {
        UInt uiPartOffset = ( pcCU->getPic()->getNumPartitionsInCtu() >> ( pcCU->getDepth(uiAbsPartIdx) << 1 ) ) >> 2;
        encodeIntraDirModeChroma( pcCU, uiAbsPartIdx + uiPartOffset   );
        encodeIntraDirModeChroma( pcCU, uiAbsPartIdx + uiPartOffset*2 );
        encodeIntraDirModeChroma( pcCU, uiAbsPartIdx + uiPartOffset*3 );
      }
    }
  }
  else                                                                // if it is Inter mode, encode motion vector and reference index
  {
    encodePUWise( pcCU, uiAbsPartIdx );
  }
}

Void TEncEntropy::encodeCrossComponentPrediction( TComTU &rTu, ComponentID compID )
{
  m_pcEntropyCoderIf->codeCrossComponentPrediction( rTu, compID );
}

//! encode motion information for every PU block
Void TEncEntropy::encodePUWise( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
#if ENVIRONMENT_VARIABLE_DEBUG_AND_TEST
  const Bool bDebugPred = bDebugPredEnabled && pcCU->getSlice()->getFinalized();
#endif

  PartSize ePartSize = pcCU->getPartitionSize( uiAbsPartIdx );
  UInt uiNumPU = ( ePartSize == SIZE_2Nx2N ? 1 : ( ePartSize == SIZE_NxN ? 4 : 2 ) );
  UInt uiDepth = pcCU->getDepth( uiAbsPartIdx );
  UInt uiPUOffset = ( g_auiPUOffset[UInt( ePartSize )] << ( ( pcCU->getSlice()->getSPS()->getMaxTotalCUDepth() - uiDepth ) << 1 ) ) >> 4;
#if VCEG_AZ07_IMV
  Bool bNonZeroMvd = false;
#endif

  for ( UInt uiPartIdx = 0, uiSubPartIdx = uiAbsPartIdx; uiPartIdx < uiNumPU; uiPartIdx++, uiSubPartIdx += uiPUOffset )
  {
    encodeMergeFlag( pcCU, uiSubPartIdx );
    if ( pcCU->getMergeFlag( uiSubPartIdx ) )
    {
#if VCEG_AZ07_FRUC_MERGE
      encodeFRUCMgrMode( pcCU, uiSubPartIdx , uiPartIdx );
      if( !pcCU->getFRUCMgrMode( uiSubPartIdx ) )
#endif
      encodeMergeIndex( pcCU, uiSubPartIdx );
#if ENVIRONMENT_VARIABLE_DEBUG_AND_TEST
      if (bDebugPred)
      {
        std::cout << "Coded merge flag, CU absPartIdx: " << uiAbsPartIdx << " PU(" << uiPartIdx << ") absPartIdx: " << uiSubPartIdx;
        std::cout << " merge index: " << (UInt)pcCU->getMergeIndex(uiSubPartIdx) << std::endl;
      }
#endif
    }
    else
    {
      encodeInterDirPU( pcCU, uiSubPartIdx );
      for ( UInt uiRefListIdx = 0; uiRefListIdx < 2; uiRefListIdx++ )
      {
        if ( pcCU->getSlice()->getNumRefIdx( RefPicList( uiRefListIdx ) ) > 0 )
        {
          encodeRefFrmIdxPU ( pcCU, uiSubPartIdx, RefPicList( uiRefListIdx ) );
          encodeMvdPU       ( pcCU, uiSubPartIdx, RefPicList( uiRefListIdx ) );
#if VCEG_AZ07_IMV
          bNonZeroMvd |= ( pcCU->getCUMvField( RefPicList( uiRefListIdx ) )->getMvd( uiSubPartIdx ).getHor() != 0 );
          bNonZeroMvd |= ( pcCU->getCUMvField( RefPicList( uiRefListIdx ) )->getMvd( uiSubPartIdx ).getVer() != 0 );
#endif
          encodeMVPIdxPU    ( pcCU, uiSubPartIdx, RefPicList( uiRefListIdx ) );
#if ENVIRONMENT_VARIABLE_DEBUG_AND_TEST
          if (bDebugPred)
          {
            std::cout << "refListIdx: " << uiRefListIdx << std::endl;
            std::cout << "MVD horizontal: " << pcCU->getCUMvField(RefPicList(uiRefListIdx))->getMvd( uiAbsPartIdx ).getHor() << std::endl;
            std::cout << "MVD vertical:   " << pcCU->getCUMvField(RefPicList(uiRefListIdx))->getMvd( uiAbsPartIdx ).getVer() << std::endl;
            std::cout << "MVPIdxPU: " << pcCU->getMVPIdx(RefPicList( uiRefListIdx ), uiSubPartIdx) << std::endl;
            std::cout << "InterDir: " << (UInt)pcCU->getInterDir(uiSubPartIdx) << std::endl;
          }
#endif
        }
      }
    }
  }

#if VCEG_AZ07_IMV
  if( bNonZeroMvd && pcCU->getSlice()->getSPS()->getIMV() )
  {
    encodeiMVFlag( pcCU , uiAbsPartIdx );
  }
  if( !bNonZeroMvd )
  {
    assert( pcCU->getiMVFlag( uiAbsPartIdx ) == 0 );
  }
#endif
  return;
}

Void TEncEntropy::encodeInterDirPU( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  if ( !pcCU->getSlice()->isInterB() )
  {
    return;
  }

  m_pcEntropyCoderIf->codeInterDir( pcCU, uiAbsPartIdx );

  return;
}

//! encode reference frame index for a PU block
Void TEncEntropy::encodeRefFrmIdxPU( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList )
{
  assert( pcCU->isInter( uiAbsPartIdx ) );

  if ( ( pcCU->getSlice()->getNumRefIdx( eRefList ) == 1 ) )
  {
    return;
  }

  if ( pcCU->getInterDir( uiAbsPartIdx ) & ( 1 << eRefList ) )
  {
    m_pcEntropyCoderIf->codeRefFrmIdx( pcCU, uiAbsPartIdx, eRefList );
  }

  return;
}

//! encode motion vector difference for a PU block
Void TEncEntropy::encodeMvdPU( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList )
{
  assert( pcCU->isInter( uiAbsPartIdx ) );

  if ( pcCU->getInterDir( uiAbsPartIdx ) & ( 1 << eRefList ) )
  {
    m_pcEntropyCoderIf->codeMvd( pcCU, uiAbsPartIdx, eRefList );
  }
  return;
}

Void TEncEntropy::encodeMVPIdxPU( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList )
{
  if ( (pcCU->getInterDir( uiAbsPartIdx ) & ( 1 << eRefList )) )
  {
    m_pcEntropyCoderIf->codeMVPIdx( pcCU, uiAbsPartIdx, eRefList );
  }

  return;
}

Void TEncEntropy::encodeQtCbf( TComTU &rTu, const ComponentID compID, const Bool lowestLevel )
{
  m_pcEntropyCoderIf->codeQtCbf( rTu, compID, lowestLevel );
}

Void TEncEntropy::encodeTransformSubdivFlag( UInt uiSymbol, UInt uiCtx )
{
  m_pcEntropyCoderIf->codeTransformSubdivFlag( uiSymbol, uiCtx );
}

Void TEncEntropy::encodeQtRootCbf( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  m_pcEntropyCoderIf->codeQtRootCbf( pcCU, uiAbsPartIdx );
}

Void TEncEntropy::encodeQtCbfZero( TComTU &rTu, const ChannelType chType )
{
  m_pcEntropyCoderIf->codeQtCbfZero( rTu, chType );
}

Void TEncEntropy::encodeQtRootCbfZero( )
{
  m_pcEntropyCoderIf->codeQtRootCbfZero( );
}

// dQP
Void TEncEntropy::encodeQP( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD )
{
  if( bRD )
  {
    uiAbsPartIdx = 0;
  }

  if ( pcCU->getSlice()->getPPS()->getUseDQP() )
  {
    m_pcEntropyCoderIf->codeDeltaQP( pcCU, uiAbsPartIdx );
  }
}

//! encode chroma qp adjustment
Void TEncEntropy::encodeChromaQpAdjustment( TComDataCU* cu, UInt absPartIdx, Bool inRd )
{
  if( inRd )
  {
    absPartIdx = 0;
  }

  m_pcEntropyCoderIf->codeChromaQpAdjustment( cu, absPartIdx );
}

// texture

//! encode coefficients
Void TEncEntropy::encodeCoeff( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, Bool& bCodeDQP, Bool& codeChromaQpAdj
#if VCEG_AZ05_INTRA_MPI
  , Int& bNonZeroCoeff 
#endif
  )
{
#if ENVIRONMENT_VARIABLE_DEBUG_AND_TEST
  const Bool bDebugRQT=pcCU->getSlice()->getFinalized() && DebugOptionList::DebugRQT.getInt()!=0;
#endif

  if( pcCU->isIntra(uiAbsPartIdx) )
  {
    if (false)
    {
      DTRACE_CABAC_VL( g_nSymbolCounter++ )
      DTRACE_CABAC_T( "\tdecodeTransformIdx()\tCUDepth=" )
      DTRACE_CABAC_V( uiDepth )
      DTRACE_CABAC_T( "\n" )
    }
  }
  else
  {
    if( !(pcCU->getMergeFlag( uiAbsPartIdx ) && pcCU->getPartitionSize(uiAbsPartIdx) == SIZE_2Nx2N ) )
    {
      m_pcEntropyCoderIf->codeQtRootCbf( pcCU, uiAbsPartIdx );
    }
    if ( !pcCU->getQtRootCbf( uiAbsPartIdx ) )
    {
      return;
    }
  }

  TComTURecurse tuRecurse(pcCU, uiAbsPartIdx, uiDepth);
#if ENVIRONMENT_VARIABLE_DEBUG_AND_TEST
  if (bDebugRQT)
  {
    printf("..codeCoeff: uiAbsPartIdx=%d, PU format=%d, 2Nx2N=%d, NxN=%d\n", uiAbsPartIdx, pcCU->getPartitionSize(uiAbsPartIdx), SIZE_2Nx2N, SIZE_NxN);
  }
#endif


#if VCEG_AZ05_INTRA_MPI 
  Int  bCbfCU = false;
#endif   
  xEncodeTransform( bCodeDQP, codeChromaQpAdj, tuRecurse
#if VCEG_AZ05_INTRA_MPI
    , bCbfCU
#endif
    );
#if VCEG_AZ05_INTRA_MPI
  bNonZeroCoeff = bCbfCU;
#endif
}

Void TEncEntropy::encodeCoeffNxN( TComTU &rTu, TCoeff* pcCoef, const ComponentID compID)
{
  TComDataCU *pcCU = rTu.getCU();

#if VCEG_AZ05_INTRA_MPI 
  Int  bCbfCU = false;
#endif

  if (pcCU->getCbf(rTu.GetAbsPartIdxTU(), compID, rTu.GetTransformDepthRel()) != 0)
  {
    if (rTu.getRect(compID).width != rTu.getRect(compID).height)
    {
      //code two sub-TUs
      TComTURecurse subTUIterator(rTu, false, TComTU::VERTICAL_SPLIT, true, compID);

      const UInt subTUSize = subTUIterator.getRect(compID).width * subTUIterator.getRect(compID).height;

      do
      {
        const UChar subTUCBF = pcCU->getCbf(subTUIterator.GetAbsPartIdxTU(compID), compID, (subTUIterator.GetTransformDepthRel() + 1));

        if (subTUCBF != 0)
        {
          m_pcEntropyCoderIf->codeCoeffNxN( subTUIterator, (pcCoef + (subTUIterator.GetSectionNumber() * subTUSize)), compID
#if VCEG_AZ05_INTRA_MPI
            , bCbfCU
#endif
            );
        }
      }
      while (subTUIterator.nextSection(rTu));
    }
    else
    {
      m_pcEntropyCoderIf->codeCoeffNxN(rTu, pcCoef, compID
#if VCEG_AZ05_INTRA_MPI
        , bCbfCU
#endif
        );
    }
  }
}

Void TEncEntropy::estimateBit (estBitsSbacStruct* pcEstBitsSbac, Int width, Int height, const ChannelType chType)
{
  const UInt heightAtEntropyCoding = (width != height) ? (height >> 1) : height;

  m_pcEntropyCoderIf->estBit ( pcEstBitsSbac, width, heightAtEntropyCoding, chType );
}

Int TEncEntropy::countNonZeroCoeffs( TCoeff* pcCoef, UInt uiSize )
{
  Int count = 0;

  for ( Int i = 0; i < uiSize; i++ )
  {
    count += pcCoef[i] != 0;
  }

  return count;
}

#if ALF_HM3_REFACTOR
Void TEncEntropy::codeFiltCountBit(ALFParam* pAlfParam, Int64* ruiRate, const TComSlice * pSlice)
{
  resetEntropy(pSlice);
  resetBits();
  codeFilt(pAlfParam);
  *ruiRate = getNumberOfWrittenBits();
  resetEntropy(pSlice);
  resetBits();
}

Void TEncEntropy::codeAuxCountBit(ALFParam* pAlfParam, Int64* ruiRate, const TComSlice * pSlice)
{
  resetEntropy(pSlice);
  resetBits();
  codeAux(pAlfParam);
  *ruiRate = getNumberOfWrittenBits();
  resetEntropy(pSlice);
  resetBits();
}

Void TEncEntropy::codeAux(ALFParam* pAlfParam)
{
  Int FiltTab[3] = {9, 7, 5};
  Int Tab = FiltTab[pAlfParam->realfiltNo];
  //  m_pcEntropyCoderIf->codeAlfUvlc(pAlfParam->realfiltNo); 

  m_pcEntropyCoderIf->codeAlfUvlc((Tab-5)/2); 

  if (pAlfParam->filtNo>=0)
  {
    if(pAlfParam->realfiltNo >= 0)
    {
      // filters_per_fr
      m_pcEntropyCoderIf->codeAlfUvlc(pAlfParam->noFilters);

      if(pAlfParam->noFilters == 1)
      {
        m_pcEntropyCoderIf->codeAlfUvlc(pAlfParam->startSecondFilter);
      }
      else if (pAlfParam->noFilters == 2)
      {
        for (int i=1; i<TComAdaptiveLoopFilter::m_NO_VAR_BINS; i++) m_pcEntropyCoderIf->codeAlfFlag (pAlfParam->filterPattern[i]);
      }
    }
  }
}

Int TEncEntropy::lengthGolomb(int coeffVal, int k)
{
  int m = 2 << (k - 1);
  int q = coeffVal / m;
  if(coeffVal != 0)
    return(q + 2 + k);
  else
    return(q + 1 + k);
}

Int TEncEntropy::codeFilterCoeff(ALFParam* ALFp)
{
  int filters_per_group = ALFp->filters_per_group_diff;
  int sqrFiltLength = ALFp->num_coeff;
  int filtNo = ALFp->realfiltNo;
  int flTab[]={9/2, 7/2, 5/2};
  int fl = flTab[filtNo];
  int i, k, kMin, kStart, minBits, ind, scanPos, maxScanVal, coeffVal, len = 0,
    kMinTab[TComAdaptiveLoopFilter::m_MAX_SQR_FILT_LENGTH], bitsCoeffScan[TComAdaptiveLoopFilter::m_MAX_SCAN_VAL][TComAdaptiveLoopFilter::m_MAX_EXP_GOLOMB],
    minKStart, minBitsKStart, bitsKStart;

  const Int * pDepthInt = TComAdaptiveLoopFilter::m_pDepthIntTab[fl-2];

  maxScanVal = 0;
  for(i = 0; i < sqrFiltLength; i++)
    maxScanVal = max(maxScanVal, pDepthInt[i]);

  // vlc for all
  memset(bitsCoeffScan, 0, TComAdaptiveLoopFilter::m_MAX_SCAN_VAL * TComAdaptiveLoopFilter::m_MAX_EXP_GOLOMB * sizeof(int));
  for(ind=0; ind<filters_per_group; ++ind)
  {
    for(i = 0; i < sqrFiltLength; i++)
    {
      scanPos=pDepthInt[i]-1;
      coeffVal=abs(ALFp->coeffmulti[ind][i]);
      for (k=1; k<15; k++)
      {
        bitsCoeffScan[scanPos][k]+=lengthGolomb(coeffVal, k);
      }
    }
  }

  minBitsKStart = 0;
  minKStart = -1;
  for(k = 1; k < 8; k++)
  { 
    bitsKStart = 0; 
    kStart = k;
    for(scanPos = 0; scanPos < maxScanVal; scanPos++)
    {
      kMin = kStart; 
      minBits = bitsCoeffScan[scanPos][kMin];

      if(bitsCoeffScan[scanPos][kStart+1] < minBits)
      {
        kMin = kStart + 1; 
        minBits = bitsCoeffScan[scanPos][kMin];
      }
      kStart = kMin;
      bitsKStart += minBits;
    }
    if((bitsKStart < minBitsKStart) || (k == 1))
    {
      minBitsKStart = bitsKStart;
      minKStart = k;
    }
  }

  kStart = minKStart; 
  for(scanPos = 0; scanPos < maxScanVal; scanPos++)
  {
    kMin = kStart; 
    minBits = bitsCoeffScan[scanPos][kMin];

    if(bitsCoeffScan[scanPos][kStart+1] < minBits)
    {
      kMin = kStart + 1; 
      minBits = bitsCoeffScan[scanPos][kMin];
    }

    kMinTab[scanPos] = kMin;
    kStart = kMin;
  }

  // Coding parameters
  ALFp->minKStart = minKStart;
  ALFp->maxScanVal = maxScanVal;
  for(scanPos = 0; scanPos < maxScanVal; scanPos++)
  {
    ALFp->kMinTab[scanPos] = kMinTab[scanPos];
  }
  len += writeFilterCodingParams(minKStart, maxScanVal, kMinTab);

  // Filter coefficients
  len += writeFilterCoeffs(sqrFiltLength, filters_per_group, pDepthInt, ALFp->coeffmulti, kMinTab);

  return len;
}

Int TEncEntropy::writeFilterCodingParams(int minKStart, int maxScanVal, int kMinTab[])
{
  int scanPos;
  int golombIndexBit;
  int kMin;

  // Golomb parameters
  m_pcEntropyCoderIf->codeAlfUvlc(minKStart - 1);

  kMin = minKStart; 
  for(scanPos = 0; scanPos < maxScanVal; scanPos++)
  {
    golombIndexBit = (kMinTab[scanPos] != kMin)? 1: 0;

    assert(kMinTab[scanPos] <= kMin + 1);

    m_pcEntropyCoderIf->codeAlfFlag(golombIndexBit);
    kMin = kMinTab[scanPos];
  }    

  return 0;
}

Int TEncEntropy::writeFilterCoeffs(int sqrFiltLength, int filters_per_group, const int pDepthInt[], 
  int **FilterCoeff, int kMinTab[])
{
  int ind, scanPos, i;

  for(ind = 0; ind < filters_per_group; ++ind)
  {
    for(i = 0; i < sqrFiltLength; i++)
    {
      scanPos = pDepthInt[i] - 1;
      golombEncode(FilterCoeff[ind][i], kMinTab[scanPos]);
    }
  }
  return 0;
}

Int TEncEntropy::golombEncode(int coeff, int k)
{
  int q, i, m;
  int symbol = abs(coeff);

  m = (int)pow(2.0, k);
  q = symbol / m;

  for (i = 0; i < q; i++)
    m_pcEntropyCoderIf->codeAlfFlag(1);
  m_pcEntropyCoderIf->codeAlfFlag(0);
  // write one zero

  for(i = 0; i < k; i++)
  {
    m_pcEntropyCoderIf->codeAlfFlag(symbol & 0x01);
    symbol >>= 1;
  }

  if(coeff != 0)
  {
    int sign = (coeff > 0)? 1: 0;
    m_pcEntropyCoderIf->codeAlfFlag(sign);
  }
  return 0;
}

Void TEncEntropy::codeFilt(ALFParam* pAlfParam)
{
  if(pAlfParam->filters_per_group > 1)
  {
    m_pcEntropyCoderIf->codeAlfFlag (pAlfParam->predMethod);
  }
  codeFilterCoeff (pAlfParam);
}

Void  print(ALFParam* pAlfParam)
{
  Int i=0;
  Int ind=0;
  Int FiltLengthTab[] = {22, 14, 8}; //0:9tap
  Int FiltLength = FiltLengthTab[pAlfParam->realfiltNo];

  printf("set of params\n");
  printf("realfiltNo:%d\n", pAlfParam->realfiltNo);
  printf("filtNo:%d\n", pAlfParam->filtNo);
  printf("filterPattern:");
  for (i=0; i<TComAdaptiveLoopFilter::m_NO_VAR_BINS; i++) printf("%d ", pAlfParam->filterPattern[i]);
  printf("\n");

  printf("startSecondFilter:%d\n", pAlfParam->startSecondFilter);
  printf("noFilters:%d\n", pAlfParam->noFilters);
  printf("varIndTab:");
  for (i=0; i<TComAdaptiveLoopFilter::m_NO_VAR_BINS; i++) printf("%d ", pAlfParam->varIndTab[i]);
  printf("\n");
  printf("filters_per_group_diff:%d\n", pAlfParam->filters_per_group_diff);
  printf("filters_per_group:%d\n", pAlfParam->filters_per_group);
  printf("codedVarBins:");
  for (i=0; i<TComAdaptiveLoopFilter::m_NO_VAR_BINS; i++) printf("%d ", pAlfParam->codedVarBins[i]);
  printf("\n");
  printf("forceCoeff0:%d\n", pAlfParam->forceCoeff0);
  printf("predMethod:%d\n", pAlfParam->predMethod);

  for (ind=0; ind<pAlfParam->filters_per_group_diff; ind++)
  {
    printf("coeffmulti(%d):", ind);
    for (i=0; i<FiltLength; i++) printf("%d ", pAlfParam->coeffmulti[ind][i]);
    printf("\n");
  }

  printf("minKStart:%d\n", pAlfParam->minKStart);  
  printf("maxScanVal:%d\n", pAlfParam->maxScanVal);  
  printf("kMinTab:");
  for(Int scanPos = 0; scanPos < pAlfParam->maxScanVal; scanPos++)
  {
    printf("%d ", pAlfParam->kMinTab[scanPos]);
  }
  printf("\n");

  printf("chroma_idc:%d\n", pAlfParam->chroma_idc);  
  printf("tap_chroma:%d\n", pAlfParam->tap_chroma);  
  printf("chroma_coeff:");
  for(Int scanPos = 0; scanPos < pAlfParam->num_coeff_chroma; scanPos++)
  {
    printf("%d ", pAlfParam->coeff_chroma[scanPos]);
  }
  printf("\n");
}

Void TEncEntropy::encodeAlfParam(ALFParam* pAlfParam , UInt uiMaxTotalCUDepth )
{
  m_pcEntropyCoderIf->codeAlfFlag(pAlfParam->alf_flag);
  if (!pAlfParam->alf_flag)
    return;
  Int pos;
#if COM16_C806_ALF_TEMPPRED_NUM
  //encode temporal prediction flag and index
  m_pcEntropyCoderIf->codeAlfFlag( pAlfParam->temproalPredFlag ? 1 : 0 );
  if( pAlfParam->temproalPredFlag )
  {
    m_pcEntropyCoderIf->codeAlfUvlc( pAlfParam->prevIdx );
  }
  else
  {
#endif
    codeAux(pAlfParam);
    codeFilt(pAlfParam);
#if COM16_C806_ALF_TEMPPRED_NUM
  }
#endif
  // filter parameters for chroma
  m_pcEntropyCoderIf->codeAlfUvlc(pAlfParam->chroma_idc);
#if COM16_C806_ALF_TEMPPRED_NUM
  if( !pAlfParam->temproalPredFlag && pAlfParam->chroma_idc )
#else
  if(pAlfParam->chroma_idc)
#endif
  {
    m_pcEntropyCoderIf->codeAlfUvlc((pAlfParam->tap_chroma-5)/2);

    // filter coefficients for chroma
    for(pos=0; pos<pAlfParam->num_coeff_chroma; pos++)
    {
      m_pcEntropyCoderIf->codeAlfSvlc(pAlfParam->coeff_chroma[pos]);
    }
  }

  // region control parameters for luma
  m_pcEntropyCoderIf->codeAlfFlag(pAlfParam->cu_control_flag);
  if (pAlfParam->cu_control_flag)
  {
    assert( (pAlfParam->cu_control_flag && m_pcEntropyCoderIf->getAlfCtrl()) || (!pAlfParam->cu_control_flag && !m_pcEntropyCoderIf->getAlfCtrl()));
    m_pcEntropyCoderIf->codeAlfCtrlDepth( uiMaxTotalCUDepth );
  }
}

Void TEncEntropy::encodeAlfCtrlFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD )
{
  if( bRD )
    uiAbsPartIdx = 0;

  m_pcEntropyCoderIf->codeAlfCtrlFlag( pcCU, uiAbsPartIdx );
}

Void TEncEntropy::encodeAlfCtrlParam( ALFParam* pAlfParam )
{
  m_pcEntropyCoderIf->codeAlfFlagNum( pAlfParam->num_alf_cu_flag, pAlfParam->num_cus_in_frame );

  for(UInt i=0; i<pAlfParam->num_alf_cu_flag; i++)
  {
    m_pcEntropyCoderIf->codeAlfCtrlFlag( pAlfParam->alf_cu_flag[i] );
  }
}
#endif
//! \}
