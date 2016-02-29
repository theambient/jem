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

/** \file     TDecCu.cpp
    \brief    CU decoder class
*/

#include "TDecCu.h"
#include "TLibCommon/TComTU.h"
#include "TLibCommon/TComPrediction.h"

//! \ingroup TLibDecoder
//! \{
#if INTER_KLT
extern UInt g_uiDepth2InterTempSize[5];
#endif
#if INTRA_KLT
extern UInt g_uiDepth2IntraTempSize[5];
#endif
// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TDecCu::TDecCu()
{
  m_ppcYuvResi = NULL;
  m_ppcYuvReco = NULL;
  m_ppcCU      = NULL;
#if COM16_C806_VCEG_AZ10_SUB_PU_TMVP
  m_pMvFieldSP[0] = new TComMvField[MAX_NUM_PART_IDXS_IN_CTU_WIDTH*MAX_NUM_PART_IDXS_IN_CTU_WIDTH*2];
  m_pMvFieldSP[1] = new TComMvField[MAX_NUM_PART_IDXS_IN_CTU_WIDTH*MAX_NUM_PART_IDXS_IN_CTU_WIDTH*2];
  m_phInterDirSP[0] = new UChar[MAX_NUM_PART_IDXS_IN_CTU_WIDTH*MAX_NUM_PART_IDXS_IN_CTU_WIDTH];
  m_phInterDirSP[1] = new UChar[MAX_NUM_PART_IDXS_IN_CTU_WIDTH*MAX_NUM_PART_IDXS_IN_CTU_WIDTH];
  assert( m_pMvFieldSP[0] != NULL && m_phInterDirSP[0] != NULL );
  assert( m_pMvFieldSP[1] != NULL && m_phInterDirSP[1] != NULL );
#endif
#if COM16_C806_OBMC
  m_ppcTmpYuv1 = NULL;
  m_ppcTmpYuv2 = NULL;
#endif
}

TDecCu::~TDecCu()
{
#if COM16_C806_VCEG_AZ10_SUB_PU_TMVP
  for (UInt ui=0;ui<2;ui++)
  {
    if( m_pMvFieldSP[ui] != NULL )
    {
      delete [] m_pMvFieldSP[ui];
      m_pMvFieldSP[ui] = NULL;
    }
    if( m_phInterDirSP[ui] != NULL )
    {
      delete [] m_phInterDirSP[ui];
      m_phInterDirSP[ui] = NULL;
    }
  }
#endif
}

Void TDecCu::init( TDecEntropy* pcEntropyDecoder, TComTrQuant* pcTrQuant, TComPrediction* pcPrediction)
{
  m_pcEntropyDecoder  = pcEntropyDecoder;
  m_pcTrQuant         = pcTrQuant;
  m_pcPrediction      = pcPrediction;
}

/**
 \param    uiMaxDepth      total number of allowable depth
 \param    uiMaxWidth      largest CU width
 \param    uiMaxHeight     largest CU height
 \param    chromaFormatIDC chroma format
 */
Void TDecCu::create( UInt uiMaxDepth, UInt uiMaxWidth, UInt uiMaxHeight, ChromaFormat chromaFormatIDC )
{
  m_uiMaxDepth = uiMaxDepth+1;

  m_ppcYuvResi = new TComYuv*[m_uiMaxDepth-1];
  m_ppcYuvReco = new TComYuv*[m_uiMaxDepth-1];
  m_ppcCU      = new TComDataCU*[m_uiMaxDepth-1];
#if COM16_C806_OBMC
  m_ppcTmpYuv1 = new TComYuv*[m_uiMaxDepth-1];
  m_ppcTmpYuv2 = new TComYuv*[m_uiMaxDepth-1];
#endif

  for ( UInt ui = 0; ui < m_uiMaxDepth-1; ui++ )
  {
    UInt uiNumPartitions = 1<<( ( m_uiMaxDepth - ui - 1 )<<1 );
    UInt uiWidth  = uiMaxWidth  >> ui;
    UInt uiHeight = uiMaxHeight >> ui;

    // The following arrays (m_ppcYuvResi, m_ppcYuvReco and m_ppcCU) are only required for CU depths
    // although data is allocated for all possible depths of the CU/TU tree except the last.
    // Since the TU tree will always include at least one additional depth greater than the CU tree,
    // there will be enough entries for these arrays.
    // (Section 7.4.3.2: "The CVS shall not contain data that result in (Log2MinTrafoSize) MinTbLog2SizeY
    //                    greater than or equal to MinCbLog2SizeY")
    // TODO: tidy the array allocation given the above comment.

    m_ppcYuvResi[ui] = new TComYuv;    m_ppcYuvResi[ui]->create( uiWidth, uiHeight, chromaFormatIDC );
    m_ppcYuvReco[ui] = new TComYuv;    m_ppcYuvReco[ui]->create( uiWidth, uiHeight, chromaFormatIDC );
    m_ppcCU     [ui] = new TComDataCU; m_ppcCU     [ui]->create( chromaFormatIDC, uiNumPartitions, uiWidth, uiHeight, true, uiMaxWidth >> (m_uiMaxDepth - 1) );
#if COM16_C806_OBMC
    m_ppcTmpYuv1[ui] = new TComYuv;    m_ppcTmpYuv1[ui]->create( uiWidth, uiHeight, chromaFormatIDC );
    m_ppcTmpYuv2[ui] = new TComYuv;    m_ppcTmpYuv2[ui]->create( uiWidth, uiHeight, chromaFormatIDC );
#endif
  }

  m_bDecodeDQP = false;
  m_IsChromaQpAdjCoded = false;

  // initialize partition order.
  UInt* piTmp = &g_auiZscanToRaster[0];
  initZscanToRaster(m_uiMaxDepth, 1, 0, piTmp);
  initRasterToZscan( uiMaxWidth, uiMaxHeight, m_uiMaxDepth );

  // initialize conversion matrix from partition index to pel
  initRasterToPelXY( uiMaxWidth, uiMaxHeight, m_uiMaxDepth );
}

Void TDecCu::destroy()
{
  for ( UInt ui = 0; ui < m_uiMaxDepth-1; ui++ )
  {
    m_ppcYuvResi[ui]->destroy(); delete m_ppcYuvResi[ui]; m_ppcYuvResi[ui] = NULL;
    m_ppcYuvReco[ui]->destroy(); delete m_ppcYuvReco[ui]; m_ppcYuvReco[ui] = NULL;
    m_ppcCU     [ui]->destroy(); delete m_ppcCU     [ui]; m_ppcCU     [ui] = NULL;
#if COM16_C806_OBMC
    m_ppcTmpYuv1[ui]->destroy(); delete m_ppcTmpYuv1[ui]; m_ppcTmpYuv1[ui] = NULL;
    m_ppcTmpYuv2[ui]->destroy(); delete m_ppcTmpYuv2[ui]; m_ppcTmpYuv2[ui] = NULL;
#endif
  }

  delete [] m_ppcYuvResi; m_ppcYuvResi = NULL;
  delete [] m_ppcYuvReco; m_ppcYuvReco = NULL;
  delete [] m_ppcCU     ; m_ppcCU      = NULL;
#if COM16_C806_OBMC
  delete [] m_ppcTmpYuv1; m_ppcTmpYuv1 = NULL;
  delete [] m_ppcTmpYuv2; m_ppcTmpYuv2 = NULL;
#endif
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

/** 
 Parse a CTU.
 \param    pCtu                      [in/out] pointer to CTU data structure
 \param    isLastCtuOfSliceSegment   [out]    true, if last CTU of the slice segment
 */
Void TDecCu::decodeCtu( TComDataCU* pCtu, Bool& isLastCtuOfSliceSegment )
{
  if ( pCtu->getSlice()->getPPS()->getUseDQP() )
  {
    setdQPFlag(true);
  }

  if ( pCtu->getSlice()->getUseChromaQpAdj() )
  {
    setIsChromaQpAdjCoded(true);
  }

  // start from the top level CU
  xDecodeCU( pCtu, 0, 0, isLastCtuOfSliceSegment);
}

/** 
 Decoding process for a CTU.
 \param    pCtu                      [in/out] pointer to CTU data structure
 */
Void TDecCu::decompressCtu( TComDataCU* pCtu )
{
  xDecompressCU( pCtu, 0,  0 );
}

// ====================================================================================================================
// Protected member functions
// ====================================================================================================================

//! decode end-of-slice flag
Bool TDecCu::xDecodeSliceEnd( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  UInt uiIsLastCtuOfSliceSegment;

  if (pcCU->isLastSubCUOfCtu(uiAbsPartIdx))
  {
    m_pcEntropyDecoder->decodeTerminatingBit( uiIsLastCtuOfSliceSegment );
  }
  else
  {
    uiIsLastCtuOfSliceSegment=0;
  }

  return uiIsLastCtuOfSliceSegment>0;
}

//! decode CU block recursively
Void TDecCu::xDecodeCU( TComDataCU*const pcCU, const UInt uiAbsPartIdx, const UInt uiDepth, Bool &isLastCtuOfSliceSegment)
{
  TComPic* pcPic        = pcCU->getPic();
  const TComSPS &sps    = pcPic->getPicSym()->getSPS();
  const TComPPS &pps    = pcPic->getPicSym()->getPPS();
  const UInt maxCuWidth = sps.getMaxCUWidth();
  const UInt maxCuHeight= sps.getMaxCUHeight();
  UInt uiCurNumParts    = pcPic->getNumPartitionsInCtu() >> (uiDepth<<1);
  UInt uiQNumParts      = uiCurNumParts>>2;


  Bool bBoundary = false;
  UInt uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
  UInt uiRPelX   = uiLPelX + (maxCuWidth>>uiDepth)  - 1;
  UInt uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ];
  UInt uiBPelY   = uiTPelY + (maxCuHeight>>uiDepth) - 1;

  if( ( uiRPelX < sps.getPicWidthInLumaSamples() ) && ( uiBPelY < sps.getPicHeightInLumaSamples() ) )
  {
    m_pcEntropyDecoder->decodeSplitFlag( pcCU, uiAbsPartIdx, uiDepth );
  }
  else
  {
    bBoundary = true;
  }
  if( ( ( uiDepth < pcCU->getDepth( uiAbsPartIdx ) ) && ( uiDepth < sps.getLog2DiffMaxMinCodingBlockSize() ) ) || bBoundary )
  {
    UInt uiIdx = uiAbsPartIdx;
    if( uiDepth == pps.getMaxCuDQPDepth() && pps.getUseDQP())
    {
      setdQPFlag(true);
      pcCU->setQPSubParts( pcCU->getRefQP(uiAbsPartIdx), uiAbsPartIdx, uiDepth ); // set QP to default QP
    }

    if( uiDepth == pps.getPpsRangeExtension().getDiffCuChromaQpOffsetDepth() && pcCU->getSlice()->getUseChromaQpAdj() )
    {
      setIsChromaQpAdjCoded(true);
    }

    for ( UInt uiPartUnitIdx = 0; uiPartUnitIdx < 4; uiPartUnitIdx++ )
    {
      uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiIdx] ];
      uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiIdx] ];

      if ( !isLastCtuOfSliceSegment && ( uiLPelX < sps.getPicWidthInLumaSamples() ) && ( uiTPelY < sps.getPicHeightInLumaSamples() ) )
      {
        xDecodeCU( pcCU, uiIdx, uiDepth+1, isLastCtuOfSliceSegment );
      }
      else
      {
        pcCU->setOutsideCUPart( uiIdx, uiDepth+1 );
      }

      uiIdx += uiQNumParts;
    }
    if( uiDepth == pps.getMaxCuDQPDepth() && pps.getUseDQP())
    {
      if ( getdQPFlag() )
      {
        UInt uiQPSrcPartIdx = uiAbsPartIdx;
        pcCU->setQPSubParts( pcCU->getRefQP( uiQPSrcPartIdx ), uiAbsPartIdx, uiDepth ); // set QP to default QP
      }
    }
    return;
  }

  if( uiDepth <= pps.getMaxCuDQPDepth() && pps.getUseDQP())
  {
    setdQPFlag(true);
    pcCU->setQPSubParts( pcCU->getRefQP(uiAbsPartIdx), uiAbsPartIdx, uiDepth ); // set QP to default QP
  }

  if( uiDepth <= pps.getPpsRangeExtension().getDiffCuChromaQpOffsetDepth() && pcCU->getSlice()->getUseChromaQpAdj() )
  {
    setIsChromaQpAdjCoded(true);
  }

  if (pps.getTransquantBypassEnableFlag())
  {
    m_pcEntropyDecoder->decodeCUTransquantBypassFlag( pcCU, uiAbsPartIdx, uiDepth );
  }

  // decode CU mode and the partition size
  if( !pcCU->getSlice()->isIntra())
  {
    m_pcEntropyDecoder->decodeSkipFlag( pcCU, uiAbsPartIdx, uiDepth );
  }
#if COM16_C806_OBMC
  pcCU->setOBMCFlagSubParts( true, uiAbsPartIdx, uiDepth );
#endif

  if( pcCU->isSkipped(uiAbsPartIdx) )
  {
    m_ppcCU[uiDepth]->copyInterPredInfoFrom( pcCU, uiAbsPartIdx, REF_PIC_LIST_0 );
    m_ppcCU[uiDepth]->copyInterPredInfoFrom( pcCU, uiAbsPartIdx, REF_PIC_LIST_1 );
#if VCEG_AZ07_FRUC_MERGE
    m_pcEntropyDecoder->decodeFRUCMgrMode( pcCU , uiAbsPartIdx , uiDepth , 0 );
    if( !pcCU->getFRUCMgrMode( uiAbsPartIdx ) )
    {
#endif
#if !VCEG_AZ07_FRUC_MERGE
    TComMvField cMvFieldNeighbours[MRG_MAX_NUM_CANDS << 1]; // double length for mv of both lists
#if COM16_C1016_AFFINE
    TComMvField cAffineMvField[2][3]; // double length for mv of both lists, 3 mv for affine
#endif

    UChar uhInterDirNeighbours[MRG_MAX_NUM_CANDS];
#if COM16_C806_VCEG_AZ10_SUB_PU_TMVP 
    UChar eMergeCandTypeNieghors[MRG_MAX_NUM_CANDS];
    memset ( eMergeCandTypeNieghors, MGR_TYPE_DEFAULT_N, sizeof(UChar)*MRG_MAX_NUM_CANDS );
#endif
#if VCEG_AZ06_IC
    Bool abICFlag[MRG_MAX_NUM_CANDS];
#endif
    Int numValidMergeCand = 0;
    for( UInt ui = 0; ui < m_ppcCU[uiDepth]->getSlice()->getMaxNumMergeCand(); ++ui )
    {
      uhInterDirNeighbours[ui] = 0;
    }
#endif

#if COM16_C1016_AFFINE
    if ( pcCU->isAffineMrgFlagCoded(uiAbsPartIdx, 0) )
    {
      m_pcEntropyDecoder->decodeAffineFlag( pcCU, uiAbsPartIdx, uiDepth, 0 );
    }

    if ( !pcCU->isAffine(uiAbsPartIdx) )
    {
      m_pcEntropyDecoder->decodeMergeIndex( pcCU, 0, uiAbsPartIdx, uiDepth );
    }
#else
    m_pcEntropyDecoder->decodeMergeIndex( pcCU, 0, uiAbsPartIdx, uiDepth );
#endif

#if !VCEG_AZ07_FRUC_MERGE
    UInt uiMergeIndex = pcCU->getMergeIndex(uiAbsPartIdx);

#if COM16_C1016_AFFINE
    if ( pcCU->isAffine( uiAbsPartIdx ) )
    {
      m_ppcCU[uiDepth]->getAffineMergeCandidates( 0, 0, cAffineMvField, uhInterDirNeighbours, numValidMergeCand );
      pcCU->setInterDirSubParts( uhInterDirNeighbours[uiMergeIndex], uiAbsPartIdx, 0, uiDepth );

      TComMv cTmpMv( 0, 0 );
      for ( UInt uiRefListIdx = 0; uiRefListIdx < 2; uiRefListIdx++ )
      {
        if ( pcCU->getSlice()->getNumRefIdx( RefPicList( uiRefListIdx ) ) > 0 )
        {
          TComMvField* pcMvField = cAffineMvField[ 2 * uiMergeIndex + uiRefListIdx ];

          pcCU->setMVPIdxSubParts( 0, RefPicList( uiRefListIdx ), uiAbsPartIdx, 0, uiDepth);
          pcCU->setMVPNumSubParts( 0, RefPicList( uiRefListIdx ), uiAbsPartIdx, 0, uiDepth);
          pcCU->getCUMvField( RefPicList( uiRefListIdx ) )->setAllMvd( cTmpMv, SIZE_2Nx2N, uiAbsPartIdx, uiDepth );
          pcCU->setAllAffineMvField( uiAbsPartIdx, 0, pcMvField, RefPicList(uiRefListIdx), uiDepth );
        }
      }
      xFinishDecodeCU( pcCU, uiAbsPartIdx, uiDepth, isLastCtuOfSliceSegment );
      return;
    }
#endif

    m_ppcCU[uiDepth]->getInterMergeCandidates( 0, 0, cMvFieldNeighbours, uhInterDirNeighbours, numValidMergeCand
#if VCEG_AZ06_IC
      , abICFlag
#endif
#if COM16_C806_VCEG_AZ10_SUB_PU_TMVP
      , eMergeCandTypeNieghors
      , m_pMvFieldSP
      , m_phInterDirSP
      , uiAbsPartIdx
      , pcCU
#endif
      , uiMergeIndex );

#if COM16_C806_VCEG_AZ10_SUB_PU_TMVP
   pcCU->setMergeTypeSubParts( eMergeCandTypeNieghors[uiMergeIndex] , uiAbsPartIdx, 0, uiDepth ); 
   if( eMergeCandTypeNieghors[uiMergeIndex] == MGR_TYPE_DEFAULT_N )
#endif
    pcCU->setInterDirSubParts( uhInterDirNeighbours[uiMergeIndex], uiAbsPartIdx, 0, uiDepth );
#if VCEG_AZ06_IC
   pcCU->setICFlagSubParts( pcCU->getSlice()->getApplyIC() ? abICFlag[uiMergeIndex] : 0, uiAbsPartIdx, uiDepth );
#endif
    TComMv cTmpMv( 0, 0 );
    for ( UInt uiRefListIdx = 0; uiRefListIdx < 2; uiRefListIdx++ )
    {
      if ( pcCU->getSlice()->getNumRefIdx( RefPicList( uiRefListIdx ) ) > 0 )
      {
        pcCU->setMVPIdxSubParts( 0, RefPicList( uiRefListIdx ), uiAbsPartIdx, 0, uiDepth);
        pcCU->setMVPNumSubParts( 0, RefPicList( uiRefListIdx ), uiAbsPartIdx, 0, uiDepth);
        pcCU->getCUMvField( RefPicList( uiRefListIdx ) )->setAllMvd( cTmpMv, SIZE_2Nx2N, uiAbsPartIdx, uiDepth );
#if COM16_C806_VCEG_AZ10_SUB_PU_TMVP
        if( eMergeCandTypeNieghors[uiMergeIndex] == MGR_TYPE_DEFAULT_N )
        {
#endif
        pcCU->getCUMvField( RefPicList( uiRefListIdx ) )->setAllMvField( cMvFieldNeighbours[ 2*uiMergeIndex + uiRefListIdx ], SIZE_2Nx2N, uiAbsPartIdx, uiDepth );
#if COM16_C806_VCEG_AZ10_SUB_PU_TMVP
        }
#endif
      }
    }
#endif
#if VCEG_AZ07_FRUC_MERGE
    }
#if VCEG_AZ06_IC
    m_pcEntropyDecoder->decodeICFlag( pcCU, uiAbsPartIdx, uiDepth );
#endif
#endif
    xFinishDecodeCU( pcCU, uiAbsPartIdx, uiDepth, isLastCtuOfSliceSegment );
    return;
  }

  m_pcEntropyDecoder->decodePredMode( pcCU, uiAbsPartIdx, uiDepth );
#if VCEG_AZ05_INTRA_MPI
  m_pcEntropyDecoder->decodeMPIIdx(pcCU, uiAbsPartIdx, uiDepth);
#endif
#if COM16_C1046_PDPC_INTRA
  m_pcEntropyDecoder->decodePDPCIdx(pcCU, uiAbsPartIdx, uiDepth);
#endif
  m_pcEntropyDecoder->decodePartSize( pcCU, uiAbsPartIdx, uiDepth );

  if (pcCU->isIntra( uiAbsPartIdx ) && pcCU->getPartitionSize( uiAbsPartIdx ) == SIZE_2Nx2N )
  {
    m_pcEntropyDecoder->decodeIPCMInfo( pcCU, uiAbsPartIdx, uiDepth );

    if(pcCU->getIPCMFlag(uiAbsPartIdx))
    {
      xFinishDecodeCU( pcCU, uiAbsPartIdx, uiDepth, isLastCtuOfSliceSegment );
      return;
    }
  }

  // prediction mode ( Intra : direction mode, Inter : Mv, reference idx )
  m_pcEntropyDecoder->decodePredInfo( pcCU, uiAbsPartIdx, uiDepth, m_ppcCU[uiDepth]);
#if COM16_C806_OBMC
  m_pcEntropyDecoder->decodeOBMCFlag( pcCU, uiAbsPartIdx, uiDepth );
#endif
#if VCEG_AZ06_IC
  m_pcEntropyDecoder->decodeICFlag( pcCU, uiAbsPartIdx, uiDepth );
#endif
  // Coefficient decoding
  Bool bCodeDQP = getdQPFlag();
  Bool isChromaQpAdjCoded = getIsChromaQpAdjCoded();
  m_pcEntropyDecoder->decodeCoeff( pcCU, uiAbsPartIdx, uiDepth, bCodeDQP, isChromaQpAdjCoded );
  setIsChromaQpAdjCoded( isChromaQpAdjCoded );
  setdQPFlag( bCodeDQP );
  xFinishDecodeCU( pcCU, uiAbsPartIdx, uiDepth, isLastCtuOfSliceSegment );
}

Void TDecCu::xFinishDecodeCU( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, Bool &isLastCtuOfSliceSegment)
{
  if(  pcCU->getSlice()->getPPS()->getUseDQP())
  {
    pcCU->setQPSubParts( getdQPFlag()?pcCU->getRefQP(uiAbsPartIdx):pcCU->getCodedQP(), uiAbsPartIdx, uiDepth ); // set QP
  }

  if (pcCU->getSlice()->getUseChromaQpAdj() && !getIsChromaQpAdjCoded())
  {
    pcCU->setChromaQpAdjSubParts( pcCU->getCodedChromaQpAdj(), uiAbsPartIdx, uiDepth ); // set QP
  }

  isLastCtuOfSliceSegment = xDecodeSliceEnd( pcCU, uiAbsPartIdx );
}

Void TDecCu::xDecompressCU( TComDataCU* pCtu, UInt uiAbsPartIdx,  UInt uiDepth )
{
  TComPic* pcPic = pCtu->getPic();
  TComSlice * pcSlice = pCtu->getSlice();
  const TComSPS &sps=*(pcSlice->getSPS());

  Bool bBoundary = false;
  UInt uiLPelX   = pCtu->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
  UInt uiRPelX   = uiLPelX + (sps.getMaxCUWidth()>>uiDepth)  - 1;
  UInt uiTPelY   = pCtu->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ];
  UInt uiBPelY   = uiTPelY + (sps.getMaxCUHeight()>>uiDepth) - 1;

  if( ( uiRPelX >= sps.getPicWidthInLumaSamples() ) || ( uiBPelY >= sps.getPicHeightInLumaSamples() ) )
  {
    bBoundary = true;
  }

  if( ( ( uiDepth < pCtu->getDepth( uiAbsPartIdx ) ) && ( uiDepth < sps.getLog2DiffMaxMinCodingBlockSize() ) ) || bBoundary )
  {
    UInt uiNextDepth = uiDepth + 1;
    UInt uiQNumParts = pCtu->getTotalNumPart() >> (uiNextDepth<<1);
    UInt uiIdx = uiAbsPartIdx;
    for ( UInt uiPartIdx = 0; uiPartIdx < 4; uiPartIdx++ )
    {
      uiLPelX = pCtu->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiIdx] ];
      uiTPelY = pCtu->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiIdx] ];

      if( ( uiLPelX < sps.getPicWidthInLumaSamples() ) && ( uiTPelY < sps.getPicHeightInLumaSamples() ) )
      {
        xDecompressCU(pCtu, uiIdx, uiNextDepth );
      }

      uiIdx += uiQNumParts;
    }
    return;
  }

  // Residual reconstruction
  m_ppcYuvResi[uiDepth]->clear();

  m_ppcCU[uiDepth]->copySubCU( pCtu, uiAbsPartIdx );

  switch( m_ppcCU[uiDepth]->getPredictionMode(0) )
  {
    case MODE_INTER:
#if VCEG_AZ07_FRUC_MERGE
      xDeriveCUMV( pCtu , uiAbsPartIdx , uiDepth );
#endif
      xReconInter( m_ppcCU[uiDepth], uiDepth );
      break;
    case MODE_INTRA:
      xReconIntraQT( m_ppcCU[uiDepth], uiDepth );
      break;
    default:
      assert(0);
      break;
  }

#if DEBUG_STRING
  const PredMode predMode=m_ppcCU[uiDepth]->getPredictionMode(0);
  if (DebugOptionList::DebugString_Structure.getInt()&DebugStringGetPredModeMask(predMode))
  {
    PartSize eSize=m_ppcCU[uiDepth]->getPartitionSize(0);
    std::ostream &ss(std::cout);

    ss <<"###: " << (predMode==MODE_INTRA?"Intra   ":"Inter   ") << partSizeToString[eSize] << " CU at " << m_ppcCU[uiDepth]->getCUPelX() << ", " << m_ppcCU[uiDepth]->getCUPelY() << " width=" << UInt(m_ppcCU[uiDepth]->getWidth(0)) << std::endl;
  }
#endif

  if ( m_ppcCU[uiDepth]->isLosslessCoded(0) && (m_ppcCU[uiDepth]->getIPCMFlag(0) == false))
  {
    xFillPCMBuffer(m_ppcCU[uiDepth], uiDepth);
  }

  xCopyToPic( m_ppcCU[uiDepth], pcPic, uiAbsPartIdx, uiDepth );
}

Void TDecCu::xReconInter( TComDataCU* pcCU, UInt uiDepth )
{

  // inter prediction
  m_pcPrediction->motionCompensation( pcCU, m_ppcYuvReco[uiDepth] );
#if COM16_C806_OBMC
  m_pcPrediction->subBlockOBMC( pcCU, 0, m_ppcYuvReco[uiDepth], m_ppcTmpYuv1[uiDepth], m_ppcTmpYuv2[uiDepth] );
#endif
#if DEBUG_STRING
  const Int debugPredModeMask=DebugStringGetPredModeMask(MODE_INTER);
  if (DebugOptionList::DebugString_Pred.getInt()&debugPredModeMask)
  {
    printBlockToStream(std::cout, "###inter-pred: ", *(m_ppcYuvReco[uiDepth]));
  }
#endif

  // inter recon
  xDecodeInterTexture( pcCU, uiDepth );

#if DEBUG_STRING
  if (DebugOptionList::DebugString_Resi.getInt()&debugPredModeMask)
  {
    printBlockToStream(std::cout, "###inter-resi: ", *(m_ppcYuvResi[uiDepth]));
  }
#endif

  // clip for only non-zero cbp case
  if  ( pcCU->getQtRootCbf( 0) )
  {
    m_ppcYuvReco[uiDepth]->addClip( m_ppcYuvReco[uiDepth], m_ppcYuvResi[uiDepth], 0, pcCU->getWidth( 0 ), pcCU->getSlice()->getSPS()->getBitDepths() );
  }
  else
  {
    m_ppcYuvReco[uiDepth]->copyPartToPartYuv( m_ppcYuvReco[uiDepth],0, pcCU->getWidth( 0 ),pcCU->getHeight( 0 ));
  }
#if DEBUG_STRING
  if (DebugOptionList::DebugString_Reco.getInt()&debugPredModeMask)
  {
    printBlockToStream(std::cout, "###inter-reco: ", *(m_ppcYuvReco[uiDepth]));
  }
#endif

}


Void
TDecCu::xIntraRecBlk(       TComYuv*    pcRecoYuv,
                            TComYuv*    pcPredYuv,
                            TComYuv*    pcResiYuv,
                      const ComponentID compID,
                            TComTU     &rTu)
{
  if (!rTu.ProcessComponentSection(compID))
  {
    return;
  }
  const Bool       bIsLuma = isLuma(compID);


  TComDataCU *pcCU = rTu.getCU();
  const TComSPS &sps=*(pcCU->getSlice()->getSPS());
  const UInt uiAbsPartIdx=rTu.GetAbsPartIdxTU();

  const TComRectangle &tuRect  =rTu.getRect(compID);
  const UInt uiWidth           = tuRect.width;
  const UInt uiHeight          = tuRect.height;
  const UInt uiStride          = pcRecoYuv->getStride (compID);
        Pel* piPred            = pcPredYuv->getAddr( compID, uiAbsPartIdx );
  const ChromaFormat chFmt     = rTu.GetChromaFormat();

  if (uiWidth != uiHeight)
  {
    //------------------------------------------------

    //split at current level if dividing into square sub-TUs

    TComTURecurse subTURecurse(rTu, false, TComTU::VERTICAL_SPLIT, true, compID);

    //recurse further
    do
    {
      xIntraRecBlk(pcRecoYuv, pcPredYuv, pcResiYuv, compID, subTURecurse);
    } while (subTURecurse.nextSection(rTu));

    //------------------------------------------------

    return;
  }

  const UInt uiChPredMode  = pcCU->getIntraDir( toChannelType(compID), uiAbsPartIdx );
  const UInt partsPerMinCU = 1<<(2*(sps.getMaxTotalCUDepth() - sps.getLog2DiffMaxMinCodingBlockSize()));
  const UInt uiChCodedMode = (uiChPredMode==DM_CHROMA_IDX && !bIsLuma) ? pcCU->getIntraDir(CHANNEL_TYPE_LUMA, getChromasCorrespondingPULumaIdx(uiAbsPartIdx, chFmt, partsPerMinCU)) : uiChPredMode;
  const UInt uiChFinalMode = ((chFmt == CHROMA_422)       && !bIsLuma) ? g_chroma422IntraAngleMappingTable[uiChCodedMode] : uiChCodedMode;

  //===== init availability pattern =====
#if !COM16_C983_RSAF
  const 
#endif
        Bool bUseFilteredPredictions=TComPrediction::filteringIntraReferenceSamples(compID, 
                                                                                    uiChFinalMode, 
                                                                                    uiWidth, 
                                                                                    uiHeight, 
                                                                                    chFmt, 
                                                                                    pcCU->getSlice()->getSPS()->getSpsRangeExtension().getIntraSmoothingDisabledFlag()
#if COM16_C983_RSAF_PREVENT_OVERSMOOTHING
                                                                                  , sps.getUseRSAF()
#endif
                                                                                   );

#if DEBUG_STRING
  std::ostream &ss(std::cout);
#endif

#if COM16_C983_RSAF
  Bool bFilter = false;
#if COM16_C1046_PDPC_RSAF_HARMONIZATION
  if (compID == COMPONENT_Y && sps.getUseRSAF() && pcCU->getPDPCIdx(uiAbsPartIdx) == 0)
#else
  if (compID==COMPONENT_Y && sps.getUseRSAF())
#endif
  {
    bFilter = (pcCU->getLumaIntraFilter( uiAbsPartIdx )) != 0;
    bFilter &= !(pcCU->getWidth(0)>32);
  }
#endif


  DEBUG_STRING_NEW(sTemp)
  m_pcPrediction->initIntraPatternChType( rTu, compID, bUseFilteredPredictions  
#if COM16_C983_RSAF
                                        , (compID==COMPONENT_Y) ? bFilter : false
#endif
                                          DEBUG_STRING_PASS_INTO(sTemp) );


  //===== get prediction signal =====

#if COM16_C806_LMCHROMA
  if( uiChFinalMode == LM_CHROMA_IDX )
  {
    m_pcPrediction->getLumaRecPixels( rTu, uiWidth, uiHeight );
    m_pcPrediction->predLMIntraChroma( rTu, compID, piPred, uiStride, uiWidth, uiHeight );
  }
  else
  {
#endif 
#if COM16_C983_RSAF
  if (compID==COMPONENT_Y && sps.getUseRSAF()) 
  {
    bUseFilteredPredictions = (bFilter != false);
  }
#endif
    m_pcPrediction->predIntraAng( compID,   uiChFinalMode, 0 /* Decoder does not have an original image */, 0, piPred, uiStride, rTu, bUseFilteredPredictions );

#if COM16_C806_LMCHROMA
    if( compID == COMPONENT_Cr && pcCU->getSlice()->getSPS()->getUseLMChroma() )
    { 
      m_pcPrediction->addCrossColorResi( rTu, compID, piPred, uiStride, uiWidth, uiHeight, pcResiYuv->getAddr( COMPONENT_Cb, uiAbsPartIdx ), pcResiYuv->getStride(COMPONENT_Cb) );
    }
  }
#endif

#if DEBUG_STRING
  ss << sTemp;
#endif

  //===== inverse transform =====
  Pel*      piResi            = pcResiYuv->getAddr( compID, uiAbsPartIdx );
  TCoeff*   pcCoeff           = pcCU->getCoeff(compID) + rTu.getCoefficientOffset(compID);//( uiNumCoeffInc * uiAbsPartIdx );

  const QpParam cQP(*pcCU, compID);


  DEBUG_STRING_NEW(sDebug);
#if DEBUG_STRING
  const Int debugPredModeMask=DebugStringGetPredModeMask(MODE_INTRA);
  std::string *psDebug=(DebugOptionList::DebugString_InvTran.getInt()&debugPredModeMask) ? &sDebug : 0;
#endif

  if (pcCU->getCbf(uiAbsPartIdx, compID, rTu.GetTransformDepthRel()) != 0)
  {
    m_pcTrQuant->invTransformNxN( rTu, compID, piResi, uiStride, pcCoeff, cQP DEBUG_STRING_PASS_INTO(psDebug) );
  }
  else
  {
    for (UInt y = 0; y < uiHeight; y++)
    {
      for (UInt x = 0; x < uiWidth; x++)
      {
        piResi[(y * uiStride) + x] = 0;
      }
    }
  }

#if DEBUG_STRING
  if (psDebug)
  {
    ss << (*psDebug);
  }
#endif

  //===== reconstruction =====
  const UInt uiRecIPredStride  = pcCU->getPic()->getPicYuvRec()->getStride(compID);

  const Bool useCrossComponentPrediction = isChroma(compID) && (pcCU->getCrossComponentPredictionAlpha(uiAbsPartIdx, compID) != 0);
  const Pel* pResiLuma  = pcResiYuv->getAddr( COMPONENT_Y, uiAbsPartIdx );
  const Int  strideLuma = pcResiYuv->getStride( COMPONENT_Y );

        Pel* pPred      = piPred;
        Pel* pResi      = piResi;
        Pel* pReco      = pcRecoYuv->getAddr( compID, uiAbsPartIdx );
        Pel* pRecIPred  = pcCU->getPic()->getPicYuvRec()->getAddr( compID, pcCU->getCtuRsAddr(), pcCU->getZorderIdxInCtu() + uiAbsPartIdx );


#if DEBUG_STRING
  const Bool bDebugPred=((DebugOptionList::DebugString_Pred.getInt()&debugPredModeMask) && DEBUG_STRING_CHANNEL_CONDITION(compID));
  const Bool bDebugResi=((DebugOptionList::DebugString_Resi.getInt()&debugPredModeMask) && DEBUG_STRING_CHANNEL_CONDITION(compID));
  const Bool bDebugReco=((DebugOptionList::DebugString_Reco.getInt()&debugPredModeMask) && DEBUG_STRING_CHANNEL_CONDITION(compID));
  if (bDebugPred || bDebugResi || bDebugReco)
  {
    ss << "###: " << "CompID: " << compID << " pred mode (ch/fin): " << uiChPredMode << "/" << uiChFinalMode << " absPartIdx: " << rTu.GetAbsPartIdxTU() << std::endl;
  }
#endif

  const Int clipbd = sps.getBitDepth(toChannelType(compID));
#if O0043_BEST_EFFORT_DECODING
  const Int bitDepthDelta = sps.getStreamBitDepth(toChannelType(compID)) - clipbd;
#endif

  if( useCrossComponentPrediction )
  {
    TComTrQuant::crossComponentPrediction( rTu, compID, pResiLuma, piResi, piResi, uiWidth, uiHeight, strideLuma, uiStride, uiStride, true );
  }

  for( UInt uiY = 0; uiY < uiHeight; uiY++ )
  {
#if DEBUG_STRING
    if (bDebugPred || bDebugResi || bDebugReco)
    {
      ss << "###: ";
    }

    if (bDebugPred)
    {
      ss << " - pred: ";
      for( UInt uiX = 0; uiX < uiWidth; uiX++ )
      {
        ss << pPred[ uiX ] << ", ";
      }
    }
    if (bDebugResi)
    {
      ss << " - resi: ";
    }
#endif

    for( UInt uiX = 0; uiX < uiWidth; uiX++ )
    {
#if DEBUG_STRING
      if (bDebugResi)
      {
        ss << pResi[ uiX ] << ", ";
      }
#endif
#if O0043_BEST_EFFORT_DECODING
      pReco    [ uiX ] = ClipBD( rightShiftEvenRounding<Pel>(pPred[ uiX ] + pResi[ uiX ], bitDepthDelta), clipbd );
#else
      pReco    [ uiX ] = ClipBD( pPred[ uiX ] + pResi[ uiX ], clipbd );
#endif
      pRecIPred[ uiX ] = pReco[ uiX ];
    }
#if DEBUG_STRING
    if (bDebugReco)
    {
      ss << " - reco: ";
      for( UInt uiX = 0; uiX < uiWidth; uiX++ )
      {
        ss << pReco[ uiX ] << ", ";
      }
    }

    if (bDebugPred || bDebugResi || bDebugReco)
    {
      ss << "\n";
    }
#endif
    pPred     += uiStride;
    pResi     += uiStride;
    pReco     += uiStride;
    pRecIPred += uiRecIPredStride;
  }
}

#if INTRA_KLT
Void
TDecCu::xIntraRecBlkTM( TComYuv*    pcRecoYuv,
                      TComYuv*    pcPredYuv,
                      TComYuv*    pcResiYuv,
                      const ComponentID compID,
                      TComTU     &rTu,
                      Int tmpred0_tmpredklt1_ori2
                      )
{
    if (!rTu.ProcessComponentSection(compID))
    {
        return;
    }

    TComDataCU *pcCU = rTu.getCU();
    const TComSPS &sps = *(pcCU->getSlice()->getSPS());
    const UInt uiAbsPartIdx = rTu.GetAbsPartIdxTU();

    const TComRectangle &tuRect = rTu.getRect(compID);
    const UInt uiWidth = tuRect.width;
    const UInt uiHeight = tuRect.height;
    const UInt uiStride = pcRecoYuv->getStride(compID);
    Pel* piPred = pcPredYuv->getAddr(compID, uiAbsPartIdx);
    assert(uiWidth == uiHeight);

    Bool useKLT = false;
    UInt uiBlkSize = uiWidth;
    UInt uiTarDepth = g_aucConvertToBit[uiBlkSize];
    UInt uiTempSize = g_uiDepth2IntraTempSize[uiTarDepth];
    m_pcTrQuant->getTargetTemplate(pcCU, uiAbsPartIdx, uiBlkSize, uiTempSize);
    m_pcTrQuant->candidateSearchIntra(pcCU, uiAbsPartIdx, uiBlkSize, uiTempSize);
    Int foundCandiNum;
    Bool bSuccessful = m_pcTrQuant->generateTMPrediction(piPred, uiStride, uiBlkSize, uiTempSize, foundCandiNum);
    if (1 == tmpred0_tmpredklt1_ori2 && bSuccessful)
    {
        useKLT = m_pcTrQuant->calcKLTIntra(piPred, uiStride, uiBlkSize);
    }
    assert(foundCandiNum >= 1);

    //===== inverse transform =====
    Pel*      piResi = pcResiYuv->getAddr(compID, uiAbsPartIdx);
    TCoeff*   pcCoeff = pcCU->getCoeff(compID) + rTu.getCoefficientOffset(compID);

    const QpParam cQP(*pcCU, compID);


    DEBUG_STRING_NEW(sDebug);
#if DEBUG_STRING
    const Int debugPredModeMask = DebugStringGetPredModeMask(MODE_INTRA);
    std::string *psDebug = (DebugOptionList::DebugString_InvTran.getInt()&debugPredModeMask) ? &sDebug : 0;
#endif

    if (pcCU->getCbf(uiAbsPartIdx, compID, rTu.GetTransformDepthRel()) != 0)
    {
        const UInt *scan = NULL;
        if (useKLT)
        {
            TUEntropyCodingParameters codingParameters;
            getTUEntropyCodingParameters(codingParameters, rTu, compID);
            scan = codingParameters.scan;
            recoverOrderCoeff(pcCoeff, scan, uiWidth, uiHeight);
        }
        m_pcTrQuant->invTransformNxN(rTu, compID, piResi, uiStride, pcCoeff, cQP, useKLT DEBUG_STRING_PASS_INTO(psDebug));
        if (useKLT)
        {
            reOrderCoeff(pcCoeff, scan, uiWidth, uiHeight);
        }
    }
    else
    {
        for (UInt y = 0; y < uiHeight; y++)
        {
            for (UInt x = 0; x < uiWidth; x++)
            {
                piResi[(y * uiStride) + x] = 0;
            }
        }
    }

    //===== reconstruction =====
    const UInt uiRecIPredStride = pcCU->getPic()->getPicYuvRec()->getStride(compID);

    const Bool useCrossComponentPrediction = isChroma(compID) && (pcCU->getCrossComponentPredictionAlpha(uiAbsPartIdx, compID) != 0);
    assert(useCrossComponentPrediction == false);
    Pel* pPred = piPred;
    Pel* pResi = piResi;
    Pel* pReco = pcRecoYuv->getAddr(compID, uiAbsPartIdx);
    Pel* pRecIPred = pcCU->getPic()->getPicYuvRec()->getAddr(compID, pcCU->getCtuRsAddr(), pcCU->getZorderIdxInCtu() + uiAbsPartIdx);

#if DEBUG_STRING
    const Bool bDebugPred = ((DebugOptionList::DebugString_Pred.getInt()&debugPredModeMask) && DEBUG_STRING_CHANNEL_CONDITION(compID));
    const Bool bDebugResi = ((DebugOptionList::DebugString_Resi.getInt()&debugPredModeMask) && DEBUG_STRING_CHANNEL_CONDITION(compID));
    const Bool bDebugReco = ((DebugOptionList::DebugString_Reco.getInt()&debugPredModeMask) && DEBUG_STRING_CHANNEL_CONDITION(compID));
    if (bDebugPred || bDebugResi || bDebugReco)
    {
        ss << "###: " << "CompID: " << compID << " pred mode (ch/fin): " << uiChPredMode << "/" << uiChFinalMode << " absPartIdx: " << rTu.GetAbsPartIdxTU() << std::endl;
    }
#endif

    const Int clipbd = sps.getBitDepth(toChannelType(compID));
#if O0043_BEST_EFFORT_DECODING
    const Int bitDepthDelta = sps.getStreamBitDepth(toChannelType(compID)) - clipbd;
#endif

    for (UInt uiY = 0; uiY < uiHeight; uiY++)
    {
#if DEBUG_STRING
        if (bDebugPred || bDebugResi || bDebugReco)
        {
            ss << "###: ";
        }

        if (bDebugPred)
        {
            ss << " - pred: ";
            for (UInt uiX = 0; uiX < uiWidth; uiX++)
            {
                ss << pPred[uiX] << ", ";
            }
        }
        if (bDebugResi)
        {
            ss << " - resi: ";
        }
#endif

        for (UInt uiX = 0; uiX < uiWidth; uiX++)
        {
#if DEBUG_STRING
            if (bDebugResi)
            {
                ss << pResi[uiX] << ", ";
            }
#endif
#if O0043_BEST_EFFORT_DECODING
            pReco[uiX] = ClipBD(rightShiftEvenRounding<Pel>(pPred[uiX] + pResi[uiX], bitDepthDelta), clipbd);
#else
            pReco[uiX] = ClipBD(pPred[uiX] + pResi[uiX], clipbd);
#endif
            pRecIPred[uiX] = pReco[uiX];
        }
#if DEBUG_STRING
        if (bDebugReco)
        {
            ss << " - reco: ";
            for (UInt uiX = 0; uiX < uiWidth; uiX++)
            {
                ss << pReco[uiX] << ", ";
            }
        }

        if (bDebugPred || bDebugResi || bDebugReco)
        {
            ss << "\n";
        }
#endif
        pPred += uiStride;
        pResi += uiStride;
        pReco += uiStride;
        pRecIPred += uiRecIPredStride;
    }
}
#endif


Void
TDecCu::xReconIntraQT( TComDataCU* pcCU, UInt uiDepth )
{
  if (pcCU->getIPCMFlag(0))
  {
    xReconPCM( pcCU, uiDepth );
    return;
  }
  const UInt numChType = pcCU->getPic()->getChromaFormat()!=CHROMA_400 ? 2 : 1;
  for (UInt chType=CHANNEL_TYPE_LUMA; chType<numChType; chType++)
  {
    const ChannelType chanType=ChannelType(chType);
    const Bool NxNPUHas4Parts = ::isChroma(chanType) ? enable4ChromaPUsInIntraNxNCU(pcCU->getPic()->getChromaFormat()) : true;
    const UInt uiInitTrDepth = ( pcCU->getPartitionSize(0) != SIZE_2Nx2N && NxNPUHas4Parts ? 1 : 0 );

    TComTURecurse tuRecurseCU(pcCU, 0);
    TComTURecurse tuRecurseWithPU(tuRecurseCU, false, (uiInitTrDepth==0)?TComTU::DONT_SPLIT : TComTU::QUAD_SPLIT);

    do
    {
      xIntraRecQT( m_ppcYuvReco[uiDepth], m_ppcYuvReco[uiDepth], m_ppcYuvResi[uiDepth], chanType, tuRecurseWithPU );
    } while (tuRecurseWithPU.nextSection(tuRecurseCU));
  }
}



/** Function for deriving reconstructed PU/CU chroma samples with QTree structure
 * \param pcRecoYuv pointer to reconstructed sample arrays
 * \param pcPredYuv pointer to prediction sample arrays
 * \param pcResiYuv pointer to residue sample arrays
 * \param chType    texture channel type (luma/chroma)
 * \param rTu       reference to transform data
 *
 \ This function derives reconstructed PU/CU chroma samples with QTree recursive structure
 */

Void
TDecCu::xIntraRecQT(TComYuv*    pcRecoYuv,
                    TComYuv*    pcPredYuv,
                    TComYuv*    pcResiYuv,
                    const ChannelType chType,
                    TComTU     &rTu)
{
  UInt uiTrDepth    = rTu.GetTransformDepthRel();
  TComDataCU *pcCU  = rTu.getCU();
  UInt uiAbsPartIdx = rTu.GetAbsPartIdxTU();
  UInt uiTrMode     = pcCU->getTransformIdx( uiAbsPartIdx );
  if( uiTrMode == uiTrDepth )
  {
    if (isLuma(chType))
    {
#if INTRA_KLT
#if USE_KLT
        if (pcCU->getSlice()->getSPS()->getUseIntraKLT() && (Bool)pcCU->getKLTFlag(uiAbsPartIdx, COMPONENT_Y))
#else
        Bool bTMFlag = (Bool)pcCU->getKLTFlag(uiAbsPartIdx, COMPONENT_Y);
        if (bTMFlag)
#endif
        {
            xIntraRecBlkTM(pcRecoYuv, pcPredYuv, pcResiYuv, COMPONENT_Y, rTu, TMPRED0_TMPREDKLT1_ORI2);
        }
        else
        {
            xIntraRecBlk(pcRecoYuv, pcPredYuv, pcResiYuv, COMPONENT_Y, rTu);
        }
#else
      xIntraRecBlk( pcRecoYuv, pcPredYuv, pcResiYuv, COMPONENT_Y,  rTu );
#endif
    }
    else
    {
      const UInt numValidComp=getNumberValidComponents(rTu.GetChromaFormat());
      for(UInt compID=COMPONENT_Cb; compID<numValidComp; compID++)
      {
        xIntraRecBlk( pcRecoYuv, pcPredYuv, pcResiYuv, ComponentID(compID), rTu );
      }
    }
  }
  else
  {
    TComTURecurse tuRecurseChild(rTu, false);
    do
    {
      xIntraRecQT( pcRecoYuv, pcPredYuv, pcResiYuv, chType, tuRecurseChild );
    } while (tuRecurseChild.nextSection(rTu));
  }
}

Void TDecCu::xCopyToPic( TComDataCU* pcCU, TComPic* pcPic, UInt uiZorderIdx, UInt uiDepth )
{
  UInt uiCtuRsAddr = pcCU->getCtuRsAddr();

  m_ppcYuvReco[uiDepth]->copyToPicYuv  ( pcPic->getPicYuvRec (), uiCtuRsAddr, uiZorderIdx );

  return;
}

Void TDecCu::xDecodeInterTexture ( TComDataCU* pcCU, UInt uiDepth )
{

  TComTURecurse tuRecur(pcCU, 0, uiDepth);

#if INTER_KLT
  TComYuv* pcPred = m_ppcYuvReco[uiDepth];
#endif
  for(UInt ch=0; ch<pcCU->getPic()->getNumberValidComponents(); ch++)
  {
    const ComponentID compID=ComponentID(ch);
    DEBUG_STRING_OUTPUT(std::cout, debug_reorder_data_inter_token[compID])
#if INTER_KLT
    m_pcTrQuant->invRecurTransformNxN ( compID, m_ppcYuvResi[uiDepth], tuRecur, pcPred);
#else
    m_pcTrQuant->invRecurTransformNxN ( compID, m_ppcYuvResi[uiDepth], tuRecur );
#endif
  }

  DEBUG_STRING_OUTPUT(std::cout, debug_reorder_data_inter_token[MAX_NUM_COMPONENT])
}

/** Function for deriving reconstructed luma/chroma samples of a PCM mode CU.
 * \param pcCU pointer to current CU
 * \param uiPartIdx part index
 * \param piPCM pointer to PCM code arrays
 * \param piReco pointer to reconstructed sample arrays
 * \param uiStride stride of reconstructed sample arrays
 * \param uiWidth CU width
 * \param uiHeight CU height
 * \param compID colour component ID
 * \returns Void
 */
Void TDecCu::xDecodePCMTexture( TComDataCU* pcCU, const UInt uiPartIdx, const Pel *piPCM, Pel* piReco, const UInt uiStride, const UInt uiWidth, const UInt uiHeight, const ComponentID compID)
{
        Pel* piPicReco         = pcCU->getPic()->getPicYuvRec()->getAddr(compID, pcCU->getCtuRsAddr(), pcCU->getZorderIdxInCtu()+uiPartIdx);
  const UInt uiPicStride       = pcCU->getPic()->getPicYuvRec()->getStride(compID);
  const TComSPS &sps           = *(pcCU->getSlice()->getSPS());
  const UInt uiPcmLeftShiftBit = sps.getBitDepth(toChannelType(compID)) - sps.getPCMBitDepth(toChannelType(compID));

  for(UInt uiY = 0; uiY < uiHeight; uiY++ )
  {
    for(UInt uiX = 0; uiX < uiWidth; uiX++ )
    {
      piReco[uiX] = (piPCM[uiX] << uiPcmLeftShiftBit);
      piPicReco[uiX] = piReco[uiX];
    }
    piPCM += uiWidth;
    piReco += uiStride;
    piPicReco += uiPicStride;
  }
}

/** Function for reconstructing a PCM mode CU.
 * \param pcCU pointer to current CU
 * \param uiDepth CU Depth
 * \returns Void
 */
Void TDecCu::xReconPCM( TComDataCU* pcCU, UInt uiDepth )
{
  const UInt maxCuWidth     = pcCU->getSlice()->getSPS()->getMaxCUWidth();
  const UInt maxCuHeight    = pcCU->getSlice()->getSPS()->getMaxCUHeight();
  for (UInt ch=0; ch < pcCU->getPic()->getNumberValidComponents(); ch++)
  {
    const ComponentID compID = ComponentID(ch);
    const UInt width  = (maxCuWidth >>(uiDepth+m_ppcYuvResi[uiDepth]->getComponentScaleX(compID)));
    const UInt height = (maxCuHeight>>(uiDepth+m_ppcYuvResi[uiDepth]->getComponentScaleY(compID)));
    const UInt stride = m_ppcYuvResi[uiDepth]->getStride(compID);
    Pel * pPCMChannel = pcCU->getPCMSample(compID);
    Pel * pRecChannel = m_ppcYuvReco[uiDepth]->getAddr(compID);
    xDecodePCMTexture( pcCU, 0, pPCMChannel, pRecChannel, stride, width, height, compID );
  }
}

/** Function for filling the PCM buffer of a CU using its reconstructed sample array
 * \param pCU   pointer to current CU
 * \param depth CU Depth
 */
Void TDecCu::xFillPCMBuffer(TComDataCU* pCU, UInt depth)
{
  const ChromaFormat format = pCU->getPic()->getChromaFormat();
  const UInt numValidComp   = getNumberValidComponents(format);
  const UInt maxCuWidth     = pCU->getSlice()->getSPS()->getMaxCUWidth();
  const UInt maxCuHeight    = pCU->getSlice()->getSPS()->getMaxCUHeight();

  for (UInt componentIndex = 0; componentIndex < numValidComp; componentIndex++)
  {
    const ComponentID component = ComponentID(componentIndex);

    const UInt width  = maxCuWidth  >> (depth + getComponentScaleX(component, format));
    const UInt height = maxCuHeight >> (depth + getComponentScaleY(component, format));

    Pel *source      = m_ppcYuvReco[depth]->getAddr(component, 0, width);
    Pel *destination = pCU->getPCMSample(component);

    const UInt sourceStride = m_ppcYuvReco[depth]->getStride(component);

    for (Int line = 0; line < height; line++)
    {
      for (Int column = 0; column < width; column++)
      {
        destination[column] = source[column];
      }

      source      += sourceStride;
      destination += width;
    }
  }
}

#if VCEG_AZ07_FRUC_MERGE
Void TDecCu::xDeriveCUMV( TComDataCU * pcCU , UInt uiAbsPartIdx , UInt uiDepth )
{
  PartSize ePartSize = pcCU->getPartitionSize( uiAbsPartIdx );
  UInt uiNumPU = ( ePartSize == SIZE_2Nx2N ? 1 : ( ePartSize == SIZE_NxN ? 4 : 2 ) );
  UInt uiPUOffset = ( g_auiPUOffset[UInt( ePartSize )] << ( ( pcCU->getSlice()->getSPS()->getMaxTotalCUDepth() - uiDepth ) << 1 ) ) >> 4;
  TComDataCU * pcSubCU = m_ppcCU[uiDepth];

  TComMvField cMvFieldNeighbours[MRG_MAX_NUM_CANDS << 1]; // double length for mv of both lists
  UChar uhInterDirNeighbours[MRG_MAX_NUM_CANDS];
  memset( uhInterDirNeighbours , 0 , sizeof( uhInterDirNeighbours ) );
  Int numValidMergeCand = 0;
  Bool isMerged = false;

#if COM16_C806_VCEG_AZ10_SUB_PU_TMVP
  UChar    eMergeCandTypeNieghors[MRG_MAX_NUM_CANDS];
  memset( eMergeCandTypeNieghors, MGR_TYPE_DEFAULT_N, sizeof(UChar)*MRG_MAX_NUM_CANDS);
#endif
#if VCEG_AZ06_IC
  Bool abICFlag[MRG_MAX_NUM_CANDS];
#endif
  for ( UInt uiPartIdx = 0, uiSubPartIdx = uiAbsPartIdx; uiPartIdx < uiNumPU; uiPartIdx++, uiSubPartIdx += uiPUOffset )
  {
    if ( pcCU->getMergeFlag( uiSubPartIdx ) )
    {
      if( pcCU->getFRUCMgrMode( uiSubPartIdx ) )
      {
        Bool bAvailable = m_pcPrediction->deriveFRUCMV( pcSubCU , uiDepth , uiSubPartIdx - pcSubCU->getZorderIdxInCtu() , uiPartIdx );
        assert( bAvailable );
      }
      else
      {
#if COM16_C1016_AFFINE
        if ( pcCU->isAffine( uiSubPartIdx ) )
        {
          TComMvField  cAffineMvField[2][3]; // double length for mv of both lists, 3 mv for affine
          UInt uiMergeIndex = 0;

          m_ppcCU[uiDepth]->getAffineMergeCandidates( uiSubPartIdx-uiAbsPartIdx, uiPartIdx,cAffineMvField, uhInterDirNeighbours, numValidMergeCand );
          pcCU->setInterDirSubParts( uhInterDirNeighbours[uiMergeIndex], uiSubPartIdx, uiPartIdx, uiDepth );

          TComMv cTmpMv( 0, 0 );
          for ( UInt uiRefListIdx = 0; uiRefListIdx < 2; uiRefListIdx++ )
          {
            if ( pcCU->getSlice()->getNumRefIdx( RefPicList( uiRefListIdx ) ) > 0 )
            {
              TComMvField* pcMvField = cAffineMvField[ 2 * uiMergeIndex + uiRefListIdx ];

              pcCU->setMVPIdxSubParts( 0, RefPicList( uiRefListIdx ), uiSubPartIdx, uiPartIdx, uiDepth);
              pcCU->setMVPNumSubParts( 0, RefPicList( uiRefListIdx ), uiSubPartIdx , uiPartIdx, uiDepth);
              pcCU->getCUMvField( RefPicList( uiRefListIdx ) )->setAllMvd( cTmpMv, SIZE_2Nx2N, uiSubPartIdx, uiDepth );
              pcCU->setAllAffineMvField( uiSubPartIdx , uiPartIdx, pcMvField, RefPicList(uiRefListIdx), uiDepth );
            }
          }
        }
        else
        {
#endif
        UInt uiMergeIndex = pcCU->getMergeIndex(uiSubPartIdx);
        if ( pcCU->getSlice()->getPPS()->getLog2ParallelMergeLevelMinus2() && ePartSize != SIZE_2Nx2N && pcSubCU->getWidth( 0 ) <= 8 ) 
        {
          pcSubCU->setPartSizeSubParts( SIZE_2Nx2N, 0, uiDepth );
          if ( !isMerged )
          {
            pcSubCU->getInterMergeCandidates( 0, 0, cMvFieldNeighbours, uhInterDirNeighbours, numValidMergeCand 
#if VCEG_AZ06_IC
              , abICFlag
#endif
#if COM16_C806_VCEG_AZ10_SUB_PU_TMVP
              , eMergeCandTypeNieghors , m_pMvFieldSP , m_phInterDirSP
#endif
              );
            isMerged = true;
          }
          pcSubCU->setPartSizeSubParts( ePartSize, 0, uiDepth );
        }
        else
        {
          uiMergeIndex = pcCU->getMergeIndex(uiSubPartIdx);
          pcSubCU->getInterMergeCandidates( uiSubPartIdx-uiAbsPartIdx, uiPartIdx, cMvFieldNeighbours, uhInterDirNeighbours, numValidMergeCand, 
#if VCEG_AZ06_IC
            abICFlag,
#endif
#if COM16_C806_VCEG_AZ10_SUB_PU_TMVP
            eMergeCandTypeNieghors , m_pMvFieldSP , m_phInterDirSP  
#endif
            );
        }
        pcCU->setInterDirSubParts( uhInterDirNeighbours[uiMergeIndex], uiSubPartIdx, uiPartIdx, uiDepth );
#if VCEG_AZ06_IC
        if( ePartSize == SIZE_2Nx2N )
        {
          pcCU->setICFlagSubParts( pcCU->getSlice()->getApplyIC() ? abICFlag[uiMergeIndex] : 0, uiSubPartIdx, uiDepth );
        }
#endif
        TComMv cTmpMv( 0, 0 );
        for ( UInt uiRefListIdx = 0; uiRefListIdx < 2; uiRefListIdx++ )
        {        
          if ( pcCU->getSlice()->getNumRefIdx( RefPicList( uiRefListIdx ) ) > 0 )
          {
            pcCU->setMVPIdxSubParts( 0, RefPicList( uiRefListIdx ), uiSubPartIdx, uiPartIdx, uiDepth);
            pcCU->setMVPNumSubParts( 0, RefPicList( uiRefListIdx ), uiSubPartIdx, uiPartIdx, uiDepth);
            pcCU->getCUMvField( RefPicList( uiRefListIdx ) )->setAllMvd( cTmpMv, ePartSize, uiSubPartIdx, uiDepth, uiPartIdx );
            pcCU->getCUMvField( RefPicList( uiRefListIdx ) )->setAllMvField( cMvFieldNeighbours[ 2*uiMergeIndex + uiRefListIdx ], ePartSize, uiSubPartIdx, uiDepth, uiPartIdx );
          }
        }
#if COM16_C806_VCEG_AZ10_SUB_PU_TMVP
        pcCU->setMergeTypeSubParts( eMergeCandTypeNieghors[uiMergeIndex], uiSubPartIdx, uiPartIdx, uiDepth ); 
        if( eMergeCandTypeNieghors[uiMergeIndex]==MGR_TYPE_SUBPU_TMVP || eMergeCandTypeNieghors[uiMergeIndex]==MGR_TYPE_SUBPU_TMVP_EXT )
        {
          Int iWidth, iHeight;
          UInt uiIdx;
          pcCU->getPartIndexAndSize( uiPartIdx, uiIdx, iWidth, iHeight, uiSubPartIdx, true );

          UInt uiSPAddr;

          Int iNumSPInOneLine, iNumSP, iSPWidth, iSPHeight;
          UInt uiSPListIndex =  eMergeCandTypeNieghors[uiMergeIndex]==MGR_TYPE_SUBPU_TMVP ? 0:1;
          pcCU->getSPPara(iWidth, iHeight, iNumSP, iNumSPInOneLine, iSPWidth, iSPHeight);

          for (Int iPartitionIdx = 0; iPartitionIdx < iNumSP; iPartitionIdx++)
          {
            pcCU->getSPAbsPartIdx(uiSubPartIdx, iSPWidth, iSPHeight, iPartitionIdx, iNumSPInOneLine, uiSPAddr);
            pcCU->setInterDirSP(m_phInterDirSP[uiSPListIndex][iPartitionIdx], uiSPAddr, iSPWidth, iSPHeight);
            pcCU->getCUMvField( REF_PIC_LIST_0 )->setMvFieldSP(pcCU, uiSPAddr, m_pMvFieldSP[uiSPListIndex][2*iPartitionIdx], iSPWidth, iSPHeight);
            pcCU->getCUMvField( REF_PIC_LIST_1 )->setMvFieldSP(pcCU, uiSPAddr, m_pMvFieldSP[uiSPListIndex][2*iPartitionIdx + 1], iSPWidth, iSPHeight);
          }
        }
#endif
#if COM16_C1016_AFFINE
        }
#endif
      }
    }
    else
    {
      for ( UInt uiRefListIdx = 0; uiRefListIdx < 2; uiRefListIdx++ )
      {
        RefPicList eRefList = RefPicList( uiRefListIdx );
        if ( pcCU->getSlice()->getNumRefIdx( eRefList ) > 0 && ( pcCU->getInterDir( uiSubPartIdx ) & ( 1 << uiRefListIdx ) ) )
        {
#if COM16_C1016_AFFINE
          if ( pcCU->isAffine(uiSubPartIdx) )
          {
            TComMv acMv[3];
            memset( acMv, 0, sizeof(TComMv) * 3 );
            UInt uiSubCUPartIdx = uiSubPartIdx - uiAbsPartIdx;
            TComCUMvField* pcSubCUMvField = pcSubCU->getCUMvField( eRefList );
            Int iRefIdx = pcSubCUMvField->getRefIdx( uiSubCUPartIdx ); 

            AffineAMVPInfo* pAffineAMVPInfo = pcSubCUMvField->getAffineAMVPInfo();
            pcSubCU->fillAffineMvpCand( uiPartIdx, uiSubCUPartIdx, eRefList, iRefIdx, pAffineAMVPInfo );
            m_pcPrediction->getMvPredAffineAMVP( pcSubCU, uiPartIdx, uiSubCUPartIdx, eRefList, acMv );

            UInt uiPartIdxLT, uiPartIdxRT, uiPartIdxLB, uiAbsIndexInLCU;
            uiAbsIndexInLCU = pcSubCU->getZorderIdxInCtu();
            pcSubCU->deriveLeftRightTopIdx( uiPartIdx, uiPartIdxLT, uiPartIdxRT );
            pcSubCU->deriveLeftBottomIdx( uiPartIdx, uiPartIdxLB );

            acMv[0] += pcSubCUMvField->getMvd( uiPartIdxLT - uiAbsIndexInLCU );
            acMv[1] += pcSubCUMvField->getMvd( uiPartIdxRT - uiAbsIndexInLCU );

            Int iWidth = pcSubCU->getWidth(uiSubCUPartIdx);
            Int iHeight = pcSubCU->getHeight(uiSubCUPartIdx);
            Int vx2 =  - ( acMv[1].getVer() - acMv[0].getVer() ) * iHeight / iWidth + acMv[0].getHor();
            Int vy2 =    ( acMv[1].getHor() - acMv[0].getHor() ) * iHeight / iWidth + acMv[0].getVer();
            acMv[2].set( vx2, vy2 );

            pcSubCU->clipMv(acMv[0]);
            pcSubCU->clipMv(acMv[1]);
            pcSubCU->clipMv(acMv[2]);
            pcSubCU->setAllAffineMv( uiSubCUPartIdx, uiPartIdx, acMv, eRefList, 0 );
          }
          else
          {
#endif

          Short iMvdHor = pcCU->getCUMvField( eRefList )->getMvd( uiSubPartIdx ).getHor();
          Short iMvdVer = pcCU->getCUMvField( eRefList )->getMvd( uiSubPartIdx ).getVer();
#if VCEG_AZ07_IMV
          if( pcCU->getiMVFlag( uiSubPartIdx ) )
          {
            iMvdHor <<= 2;
            iMvdVer <<= 2;
          }
#endif
          TComMv cMv( iMvdHor, iMvdVer );

          pcCU->getCUMvField( RefPicList( uiRefListIdx ) )->setAllMvd( cMv, ePartSize, uiSubPartIdx, uiDepth, uiPartIdx );

          AMVPInfo* pAMVPInfo = pcSubCU->getCUMvField( eRefList )->getAMVPInfo();
          pcSubCU->fillMvpCand(uiPartIdx, uiSubPartIdx - uiAbsPartIdx, eRefList, pcSubCU->getCUMvField( eRefList )->getRefIdx( uiSubPartIdx - uiAbsPartIdx ), pAMVPInfo, m_pcPrediction );
          m_pcPrediction->getMvPredAMVP( pcSubCU, uiPartIdx, uiSubPartIdx - uiAbsPartIdx, RefPicList( uiRefListIdx ), cMv );
          cMv += TComMv( iMvdHor, iMvdVer );
          pcSubCU->getCUMvField( RefPicList( uiRefListIdx ) )->setAllMv( cMv, ePartSize, uiSubPartIdx - uiAbsPartIdx, 0, uiPartIdx);
#if COM16_C1016_AFFINE
          }
#endif
        }
      }
    }

    if ( (pcCU->getInterDir(uiSubPartIdx) == 3) && pcSubCU->isBipredRestriction(uiPartIdx) && !pcCU->getFRUCMgrMode( uiSubPartIdx ) ) 
    {
      pcCU->getCUMvField( REF_PIC_LIST_1 )->setAllMv( TComMv(0,0), ePartSize, uiSubPartIdx, uiDepth, uiPartIdx);
      pcCU->getCUMvField( REF_PIC_LIST_1 )->setAllRefIdx( -1, ePartSize, uiSubPartIdx, uiDepth, uiPartIdx);
      pcCU->setInterDirSubParts( 1, uiSubPartIdx, uiPartIdx, uiDepth);
    }
  }
}
#endif
//! \}
