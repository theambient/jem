/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.  
 *
 * Copyright (c) 2010-2014, ITU/ISO/IEC
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
#if INTER_KLT
extern UInt g_uiDepth2TempSize[5];
#endif
#if INTRA_KLT
extern UInt g_uiDepth2IntraTempSize[5];
#endif
//! \ingroup TLibDecoder
//! \{

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TDecCu::TDecCu()
{
  m_ppcYuvResi = NULL;
  m_ppcYuvReco = NULL;
  m_ppcCU      = NULL;
#if QC_OBMC
  m_ppcTmpYuv1 = NULL;
  m_ppcTmpYuv2 = NULL;
#endif
#if QC_SUB_PU_TMVP
#if QC_SUB_PU_TMVP_EXT
  m_pMvFieldSP[0] = new TComMvField[MAX_NUM_SPU_W*MAX_NUM_SPU_W*2];
  m_pMvFieldSP[1] = new TComMvField[MAX_NUM_SPU_W*MAX_NUM_SPU_W*2];
  m_phInterDirSP[0] = new UChar[MAX_NUM_SPU_W*MAX_NUM_SPU_W];
  m_phInterDirSP[1] = new UChar[MAX_NUM_SPU_W*MAX_NUM_SPU_W];
#else
  m_pMvFieldSP = new TComMvField[MAX_NUM_SPU_W*MAX_NUM_SPU_W*2];
  m_phInterDirSP = new UChar[MAX_NUM_SPU_W*MAX_NUM_SPU_W];
  assert( m_pMvFieldSP != NULL && m_phInterDirSP != NULL );
#endif
#endif
}

TDecCu::~TDecCu()
{
#if QC_SUB_PU_TMVP
#if QC_SUB_PU_TMVP_EXT
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
#else
  if( m_pMvFieldSP != NULL )
  {
    delete [] m_pMvFieldSP;
    m_pMvFieldSP = NULL;
  }
  if( m_phInterDirSP != NULL )
  {
    delete [] m_phInterDirSP;
    m_phInterDirSP = NULL;
  }
#endif
#endif
}

Void TDecCu::init( TDecEntropy* pcEntropyDecoder, TComTrQuant* pcTrQuant, TComPrediction* pcPrediction)
{
  m_pcEntropyDecoder  = pcEntropyDecoder;
  m_pcTrQuant         = pcTrQuant;
  m_pcPrediction      = pcPrediction;
}

/**
 \param    uiMaxDepth    total number of allowable depth
 \param    uiMaxWidth    largest CU width
 \param    uiMaxHeight   largest CU height
 */
Void TDecCu::create( UInt uiMaxDepth, UInt uiMaxWidth, UInt uiMaxHeight )
{
  m_uiMaxDepth = uiMaxDepth+1;
  
  m_ppcYuvResi = new TComYuv*[m_uiMaxDepth-1];
  m_ppcYuvReco = new TComYuv*[m_uiMaxDepth-1];
  m_ppcCU      = new TComDataCU*[m_uiMaxDepth-1];
#if QC_OBMC
  m_ppcTmpYuv1 = new TComYuv*[m_uiMaxDepth-1];
  m_ppcTmpYuv2 = new TComYuv*[m_uiMaxDepth-1];
#endif
  UInt uiNumPartitions;
  for ( UInt ui = 0; ui < m_uiMaxDepth-1; ui++ )
  {
    uiNumPartitions = 1<<( ( m_uiMaxDepth - ui - 1 )<<1 );
    UInt uiWidth  = uiMaxWidth  >> ui;
    UInt uiHeight = uiMaxHeight >> ui;
    
    m_ppcYuvResi[ui] = new TComYuv;    m_ppcYuvResi[ui]->create( uiWidth, uiHeight );
    m_ppcYuvReco[ui] = new TComYuv;    m_ppcYuvReco[ui]->create( uiWidth, uiHeight );
    m_ppcCU     [ui] = new TComDataCU; m_ppcCU     [ui]->create( uiNumPartitions, uiWidth, uiHeight, true, uiMaxWidth >> (m_uiMaxDepth - 1) );
#if QC_OBMC
    m_ppcTmpYuv1[ui] = new TComYuv;    m_ppcTmpYuv1[ui]->create( uiWidth, uiHeight );
    m_ppcTmpYuv2[ui] = new TComYuv;    m_ppcTmpYuv2[ui]->create( uiWidth, uiHeight );
#endif
  }
  
  m_bDecodeDQP = false;

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
#if QC_OBMC
    m_ppcTmpYuv1[ui]->destroy(); delete m_ppcTmpYuv1[ui]; m_ppcTmpYuv1[ui] = NULL;
    m_ppcTmpYuv2[ui]->destroy(); delete m_ppcTmpYuv2[ui]; m_ppcTmpYuv2[ui] = NULL;
#endif
  }
  
  delete [] m_ppcYuvResi; m_ppcYuvResi = NULL;
  delete [] m_ppcYuvReco; m_ppcYuvReco = NULL;
  delete [] m_ppcCU     ; m_ppcCU      = NULL;
#if QC_OBMC
  delete [] m_ppcTmpYuv1; m_ppcTmpYuv1 = NULL;
  delete [] m_ppcTmpYuv2; m_ppcTmpYuv2 = NULL;
#endif
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

/** \param    pcCU        pointer of CU data
 \param    ruiIsLast   last data?
 */
Void TDecCu::decodeCU( TComDataCU* pcCU, UInt& ruiIsLast )
{
  if ( pcCU->getSlice()->getPPS()->getUseDQP() )
  {
    setdQPFlag(true);
  }
#if KLT_TRACE
  DTRACE_CABAC_T("\t ---- CUAddr=")
  DTRACE_CABAC_V(pcCU->getAddr())
  DTRACE_CABAC_T("\n")
#endif
  // start from the top level CU
  xDecodeCU( pcCU, 0, 0, ruiIsLast);
}

/** \param    pcCU        pointer of CU data
 */
Void TDecCu::decompressCU( TComDataCU* pcCU )
{
  xDecompressCU( pcCU, 0,  0 );
}

// ====================================================================================================================
// Protected member functions
// ====================================================================================================================

/**decode end-of-slice flag
 * \param pcCU
 * \param uiAbsPartIdx 
 * \param uiDepth 
 * \returns Bool
 */
Bool TDecCu::xDecodeSliceEnd( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth)
{
  UInt uiIsLast;
  TComPic* pcPic = pcCU->getPic();
  TComSlice * pcSlice = pcPic->getSlice(pcPic->getCurrSliceIdx());
  UInt uiCurNumParts    = pcPic->getNumPartInCU() >> (uiDepth<<1);
  UInt uiWidth = pcSlice->getSPS()->getPicWidthInLumaSamples();
  UInt uiHeight = pcSlice->getSPS()->getPicHeightInLumaSamples();
  UInt uiGranularityWidth = g_uiMaxCUWidth;
  UInt uiPosX = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
  UInt uiPosY = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ];

  if(((uiPosX+pcCU->getWidth(uiAbsPartIdx))%uiGranularityWidth==0||(uiPosX+pcCU->getWidth(uiAbsPartIdx)==uiWidth))
    &&((uiPosY+pcCU->getHeight(uiAbsPartIdx))%uiGranularityWidth==0||(uiPosY+pcCU->getHeight(uiAbsPartIdx)==uiHeight)))
  {
    m_pcEntropyDecoder->decodeTerminatingBit( uiIsLast );
  }
  else
  {
    uiIsLast=0;
  }
  
  if(uiIsLast) 
  {
    if(pcSlice->isNextSliceSegment()&&!pcSlice->isNextSlice()) 
    {
      pcSlice->setSliceSegmentCurEndCUAddr(pcCU->getSCUAddr()+uiAbsPartIdx+uiCurNumParts);
    }
    else 
    {
      pcSlice->setSliceCurEndCUAddr(pcCU->getSCUAddr()+uiAbsPartIdx+uiCurNumParts);
      pcSlice->setSliceSegmentCurEndCUAddr(pcCU->getSCUAddr()+uiAbsPartIdx+uiCurNumParts);
    }
  }

  return uiIsLast>0;
}

/** decode CU block recursively
 * \param pcCU
 * \param uiAbsPartIdx 
 * \param uiDepth 
 * \returns Void
 */

Void TDecCu::xDecodeCU( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt& ruiIsLast)
{
  TComPic* pcPic = pcCU->getPic();
  UInt uiCurNumParts    = pcPic->getNumPartInCU() >> (uiDepth<<1);
  UInt uiQNumParts      = uiCurNumParts>>2;
  
  Bool bBoundary = false;
  UInt uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
  UInt uiRPelX   = uiLPelX + (g_uiMaxCUWidth>>uiDepth)  - 1;
  UInt uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ];
  UInt uiBPelY   = uiTPelY + (g_uiMaxCUHeight>>uiDepth) - 1;
  
  TComSlice * pcSlice = pcCU->getPic()->getSlice(pcCU->getPic()->getCurrSliceIdx());
  Bool bStartInCU = pcCU->getSCUAddr()+uiAbsPartIdx+uiCurNumParts>pcSlice->getSliceSegmentCurStartCUAddr()&&pcCU->getSCUAddr()+uiAbsPartIdx<pcSlice->getSliceSegmentCurStartCUAddr();
  if((!bStartInCU) && ( uiRPelX < pcSlice->getSPS()->getPicWidthInLumaSamples() ) && ( uiBPelY < pcSlice->getSPS()->getPicHeightInLumaSamples() ) )
  {
    m_pcEntropyDecoder->decodeSplitFlag( pcCU, uiAbsPartIdx, uiDepth );
  }
  else
  {
    bBoundary = true;
  }
  
  if( ( ( uiDepth < pcCU->getDepth( uiAbsPartIdx ) ) && ( uiDepth < g_uiMaxCUDepth - g_uiAddCUDepth ) ) || bBoundary )
  {
    UInt uiIdx = uiAbsPartIdx;
    if( (g_uiMaxCUWidth>>uiDepth) == pcCU->getSlice()->getPPS()->getMinCuDQPSize() && pcCU->getSlice()->getPPS()->getUseDQP())
    {
      setdQPFlag(true);
      pcCU->setQPSubParts( pcCU->getRefQP(uiAbsPartIdx), uiAbsPartIdx, uiDepth ); // set QP to default QP
    }

    for ( UInt uiPartUnitIdx = 0; uiPartUnitIdx < 4; uiPartUnitIdx++ )
    {
      uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiIdx] ];
      uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiIdx] ];
      
      Bool bSubInSlice = pcCU->getSCUAddr()+uiIdx+uiQNumParts>pcSlice->getSliceSegmentCurStartCUAddr();
      if ( bSubInSlice )
      {
        if ( !ruiIsLast && ( uiLPelX < pcCU->getSlice()->getSPS()->getPicWidthInLumaSamples() ) && ( uiTPelY < pcCU->getSlice()->getSPS()->getPicHeightInLumaSamples() ) )
        {
          xDecodeCU( pcCU, uiIdx, uiDepth+1, ruiIsLast );
        }
        else
        {
          pcCU->setOutsideCUPart( uiIdx, uiDepth+1 );
        }
      }
      
      uiIdx += uiQNumParts;
    }
    if( (g_uiMaxCUWidth>>uiDepth) == pcCU->getSlice()->getPPS()->getMinCuDQPSize() && pcCU->getSlice()->getPPS()->getUseDQP())
    {
      if ( getdQPFlag() )
      {
        UInt uiQPSrcPartIdx;
        if ( pcPic->getCU( pcCU->getAddr() )->getSliceSegmentStartCU(uiAbsPartIdx) != pcSlice->getSliceSegmentCurStartCUAddr() )
        {
          uiQPSrcPartIdx = pcSlice->getSliceSegmentCurStartCUAddr() % pcPic->getNumPartInCU();
        }
        else
        {
          uiQPSrcPartIdx = uiAbsPartIdx;
        }
        pcCU->setQPSubParts( pcCU->getRefQP( uiQPSrcPartIdx ), uiAbsPartIdx, uiDepth ); // set QP to default QP
      }
    }
    return;
  }
  
  if( (g_uiMaxCUWidth>>uiDepth) >= pcCU->getSlice()->getPPS()->getMinCuDQPSize() && pcCU->getSlice()->getPPS()->getUseDQP())
  {
    setdQPFlag(true);
    pcCU->setQPSubParts( pcCU->getRefQP(uiAbsPartIdx), uiAbsPartIdx, uiDepth ); // set QP to default QP
  }

  if (pcCU->getSlice()->getPPS()->getTransquantBypassEnableFlag())
  {
    m_pcEntropyDecoder->decodeCUTransquantBypassFlag( pcCU, uiAbsPartIdx, uiDepth );
  }
  
  // decode CU mode and the partition size
  if( !pcCU->getSlice()->isIntra())
  {
    m_pcEntropyDecoder->decodeSkipFlag( pcCU, uiAbsPartIdx, uiDepth );
  }
#if QC_OBMC
  pcCU->setOBMCFlagSubParts( true, uiAbsPartIdx, uiDepth );
#endif
  if( pcCU->isSkipped(uiAbsPartIdx) )
  {
    m_ppcCU[uiDepth]->copyInterPredInfoFrom( pcCU, uiAbsPartIdx, REF_PIC_LIST_0 );
    m_ppcCU[uiDepth]->copyInterPredInfoFrom( pcCU, uiAbsPartIdx, REF_PIC_LIST_1 );
#if QC_FRUC_MERGE
    m_pcEntropyDecoder->decodeFRUCMgrMode( pcCU , uiAbsPartIdx , uiDepth , 0 );
    if( !pcCU->getFRUCMgrMode( uiAbsPartIdx ) )
    {
#endif
#if !QC_FRUC_MERGE
    TComMvField cMvFieldNeighbours[MRG_MAX_NUM_CANDS << 1]; // double length for mv of both lists
    UChar uhInterDirNeighbours[MRG_MAX_NUM_CANDS];
#if QC_SUB_PU_TMVP 
    UChar    eMergeCandTypeNieghors[MRG_MAX_NUM_CANDS];
    memset(eMergeCandTypeNieghors, MGR_TYPE_DEFAULT_N, sizeof(UChar)*MRG_MAX_NUM_CANDS);
#endif
#if QC_IC
    Bool abICFlag[MRG_MAX_NUM_CANDS];
#endif
    Int numValidMergeCand = 0;
    for( UInt ui = 0; ui < m_ppcCU[uiDepth]->getSlice()->getMaxNumMergeCand(); ++ui )
    {
      uhInterDirNeighbours[ui] = 0;
    }
#endif
    m_pcEntropyDecoder->decodeMergeIndex( pcCU, 0, uiAbsPartIdx, uiDepth );
#if !QC_FRUC_MERGE
    UInt uiMergeIndex = pcCU->getMergeIndex(uiAbsPartIdx);
    m_ppcCU[uiDepth]->getInterMergeCandidates( 0, 0, cMvFieldNeighbours, uhInterDirNeighbours, numValidMergeCand
#if QC_IC
      , abICFlag
#endif
#if QC_SUB_PU_TMVP
  , eMergeCandTypeNieghors
  , m_pMvFieldSP
  , m_phInterDirSP
  , uiAbsPartIdx
  , pcCU
#endif
     , uiMergeIndex  );
#if QC_SUB_PU_TMVP
   pcCU->setMergeTypeSubParts( eMergeCandTypeNieghors[uiMergeIndex] , uiAbsPartIdx, 0, uiDepth ); 
   if(eMergeCandTypeNieghors[uiMergeIndex] == MGR_TYPE_DEFAULT_N)
#endif
    pcCU->setInterDirSubParts( uhInterDirNeighbours[uiMergeIndex], uiAbsPartIdx, 0, uiDepth );
#if QC_IC
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
#if QC_SUB_PU_TMVP
        if(eMergeCandTypeNieghors[uiMergeIndex] == MGR_TYPE_DEFAULT_N)
#endif
        pcCU->getCUMvField( RefPicList( uiRefListIdx ) )->setAllMvField( cMvFieldNeighbours[ 2*uiMergeIndex + uiRefListIdx ], SIZE_2Nx2N, uiAbsPartIdx, uiDepth );
      }
    }   
#endif
#if QC_FRUC_MERGE
    }
#endif
#if QC_IC
#if QC_FRUC_MERGE
    m_pcEntropyDecoder->decodeICFlag( pcCU, uiAbsPartIdx, uiDepth );
#endif
#endif
    xFinishDecodeCU( pcCU, uiAbsPartIdx, uiDepth, ruiIsLast );
    return;
  }

  m_pcEntropyDecoder->decodePredMode( pcCU, uiAbsPartIdx, uiDepth );
#if CU_LEVEL_MPI
  m_pcEntropyDecoder->decodeMPIIdx( pcCU, uiAbsPartIdx, uiDepth );
#endif
  m_pcEntropyDecoder->decodePartSize( pcCU, uiAbsPartIdx, uiDepth );

  if (pcCU->isIntra( uiAbsPartIdx ) && pcCU->getPartitionSize( uiAbsPartIdx ) == SIZE_2Nx2N )
  {
    m_pcEntropyDecoder->decodeIPCMInfo( pcCU, uiAbsPartIdx, uiDepth );

    if(pcCU->getIPCMFlag(uiAbsPartIdx))
    {
#if HM14_CLEAN_UP
      pcCU->resetPCMSample( uiAbsPartIdx );
#endif
      xFinishDecodeCU( pcCU, uiAbsPartIdx, uiDepth, ruiIsLast );
      return;
    }
  }

  UInt uiCurrWidth      = pcCU->getWidth ( uiAbsPartIdx );
  UInt uiCurrHeight     = pcCU->getHeight( uiAbsPartIdx );
  
  // prediction mode ( Intra : direction mode, Inter : Mv, reference idx )
  m_pcEntropyDecoder->decodePredInfo( pcCU, uiAbsPartIdx, uiDepth, m_ppcCU[uiDepth]);
#if QC_OBMC
  m_pcEntropyDecoder->decodeOBMCFlag( pcCU, uiAbsPartIdx, uiDepth );
#endif
#if QC_IC
  m_pcEntropyDecoder->decodeICFlag( pcCU, uiAbsPartIdx, uiDepth );
#endif
  // Coefficient decoding
  Bool bCodeDQP = getdQPFlag();
  m_pcEntropyDecoder->decodeCoeff( pcCU, uiAbsPartIdx, uiDepth, uiCurrWidth, uiCurrHeight, bCodeDQP );
  setdQPFlag( bCodeDQP );
  xFinishDecodeCU( pcCU, uiAbsPartIdx, uiDepth, ruiIsLast );
}

Void TDecCu::xFinishDecodeCU( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt& ruiIsLast)
{
  if(  pcCU->getSlice()->getPPS()->getUseDQP())
  {
    pcCU->setQPSubParts( getdQPFlag()?pcCU->getRefQP(uiAbsPartIdx):pcCU->getCodedQP(), uiAbsPartIdx, uiDepth ); // set QP
  }

  ruiIsLast = xDecodeSliceEnd( pcCU, uiAbsPartIdx, uiDepth);
}

Void TDecCu::xDecompressCU( TComDataCU* pcCU, UInt uiAbsPartIdx,  UInt uiDepth )
{
  TComPic* pcPic = pcCU->getPic();
  
  Bool bBoundary = false;
  UInt uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
  UInt uiRPelX   = uiLPelX + (g_uiMaxCUWidth>>uiDepth)  - 1;
  UInt uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ];
  UInt uiBPelY   = uiTPelY + (g_uiMaxCUHeight>>uiDepth) - 1;
  
  UInt uiCurNumParts    = pcPic->getNumPartInCU() >> (uiDepth<<1);
  TComSlice * pcSlice = pcCU->getPic()->getSlice(pcCU->getPic()->getCurrSliceIdx());
  Bool bStartInCU = pcCU->getSCUAddr()+uiAbsPartIdx+uiCurNumParts>pcSlice->getSliceSegmentCurStartCUAddr()&&pcCU->getSCUAddr()+uiAbsPartIdx<pcSlice->getSliceSegmentCurStartCUAddr();
  if(bStartInCU||( uiRPelX >= pcSlice->getSPS()->getPicWidthInLumaSamples() ) || ( uiBPelY >= pcSlice->getSPS()->getPicHeightInLumaSamples() ) )
  {
    bBoundary = true;
  }

  if( ( ( uiDepth < pcCU->getDepth( uiAbsPartIdx ) ) && ( uiDepth < g_uiMaxCUDepth - g_uiAddCUDepth ) ) || bBoundary )
  {
    UInt uiNextDepth = uiDepth + 1;
    UInt uiQNumParts = pcCU->getTotalNumPart() >> (uiNextDepth<<1);
    UInt uiIdx = uiAbsPartIdx;
    for ( UInt uiPartIdx = 0; uiPartIdx < 4; uiPartIdx++ )
    {
      uiLPelX = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiIdx] ];
      uiTPelY = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiIdx] ];
      
      Bool binSlice = (pcCU->getSCUAddr()+uiIdx+uiQNumParts>pcSlice->getSliceSegmentCurStartCUAddr())&&(pcCU->getSCUAddr()+uiIdx<pcSlice->getSliceSegmentCurEndCUAddr());
      if(binSlice&&( uiLPelX < pcSlice->getSPS()->getPicWidthInLumaSamples() ) && ( uiTPelY < pcSlice->getSPS()->getPicHeightInLumaSamples() ) )
      {
        xDecompressCU(pcCU, uiIdx, uiNextDepth );
      }
      
      uiIdx += uiQNumParts;
    }
    return;
  }
  // Residual reconstruction
  m_ppcYuvResi[uiDepth]->clear();
  
  m_ppcCU[uiDepth]->copySubCU( pcCU, uiAbsPartIdx, uiDepth );
  switch( m_ppcCU[uiDepth]->getPredictionMode(0) )
  {
    case MODE_INTER:
#if QC_FRUC_MERGE
      xDeriveCUMV( pcCU , uiAbsPartIdx , uiDepth );
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
#if QC_OBMC
  m_pcPrediction->subBlockOBMC( pcCU, 0, m_ppcYuvReco[uiDepth], m_ppcTmpYuv1[uiDepth], m_ppcTmpYuv2[uiDepth] );
#endif
  // inter recon
  xDecodeInterTexture( pcCU, 0, uiDepth );
  
  // clip for only non-zero cbp case
  if  ( ( pcCU->getCbf( 0, TEXT_LUMA ) ) || ( pcCU->getCbf( 0, TEXT_CHROMA_U ) ) || ( pcCU->getCbf(0, TEXT_CHROMA_V ) ) )
  {
    m_ppcYuvReco[uiDepth]->addClip( m_ppcYuvReco[uiDepth], m_ppcYuvResi[uiDepth], 0, pcCU->getWidth( 0 ) );
  }
  else
  {
    m_ppcYuvReco[uiDepth]->copyPartToPartYuv( m_ppcYuvReco[uiDepth],0, pcCU->getWidth( 0 ),pcCU->getHeight( 0 ));
  }
}

Void
TDecCu::xIntraRecLumaBlk( TComDataCU* pcCU,
                         UInt        uiTrDepth,
                         UInt        uiAbsPartIdx,
                         TComYuv*    pcRecoYuv,
                         TComYuv*    pcPredYuv, 
                         TComYuv*    pcResiYuv )
{
  UInt    uiWidth           = pcCU     ->getWidth   ( 0 ) >> uiTrDepth;
  UInt    uiHeight          = pcCU     ->getHeight  ( 0 ) >> uiTrDepth;
  UInt    uiStride          = pcRecoYuv->getStride  ();
  Pel*    piReco            = pcRecoYuv->getLumaAddr( uiAbsPartIdx );
  Pel*    piPred            = pcPredYuv->getLumaAddr( uiAbsPartIdx );
  Pel*    piResi            = pcResiYuv->getLumaAddr( uiAbsPartIdx );
  
  UInt    uiNumCoeffInc     = ( pcCU->getSlice()->getSPS()->getMaxCUWidth() * pcCU->getSlice()->getSPS()->getMaxCUHeight() ) >> ( pcCU->getSlice()->getSPS()->getMaxCUDepth() << 1 );
  TCoeff* pcCoeff           = pcCU->getCoeffY() + ( uiNumCoeffInc * uiAbsPartIdx );
  
  UInt    uiLumaPredMode    = pcCU->getLumaIntraDir     ( uiAbsPartIdx );
  
  UInt    uiZOrder          = pcCU->getZorderIdxInCU() + uiAbsPartIdx;
  Pel*    piRecIPred        = pcCU->getPic()->getPicYuvRec()->getLumaAddr( pcCU->getAddr(), uiZOrder );
  UInt    uiRecIPredStride  = pcCU->getPic()->getPicYuvRec()->getStride  ();
  Bool    useTransformSkip  = pcCU->getTransformSkip(uiAbsPartIdx, TEXT_LUMA);
#if QC_USE_65ANG_MODES
  UInt  uiPUWidth = pcCU->getWidth(uiAbsPartIdx) >> ( pcCU->getPartitionSize(uiAbsPartIdx)==SIZE_NxN ? 1 : 0);
#endif
  //===== init availability pattern =====
  Bool  bAboveAvail = false;
  Bool  bLeftAvail  = false;
  pcCU->getPattern()->initPattern   ( pcCU, uiTrDepth, uiAbsPartIdx );
  pcCU->getPattern()->initAdiPattern( pcCU, uiAbsPartIdx, uiTrDepth, 
                                     m_pcPrediction->getPredicBuf       (),
                                     m_pcPrediction->getPredicBufWidth  (),
                                     m_pcPrediction->getPredicBufHeight (),
                                     bAboveAvail, bLeftAvail );
  
  //===== get prediction signal =====
  m_pcPrediction->predIntraLumaAng( pcCU->getPattern(), uiLumaPredMode, piPred, uiStride, uiWidth, uiHeight, bAboveAvail, bLeftAvail 
#if QC_INTRA_4TAP_FILTER
    , pcCU->getSlice()->getSPS()->getUse4TapIntraFilter()
#endif
#if INTRA_BOUNDARY_FILTER
    , pcCU->getSlice()->getSPS()->getUseBoundaryFilter()
#endif
#if QC_USE_65ANG_MODES
    , pcCU->getUseExtIntraAngModes(uiPUWidth)
#endif
#if CU_LEVEL_MPI
,  pcCU,  uiAbsPartIdx
#endif
    );
  
  if ( pcCU->getCbf( uiAbsPartIdx, TEXT_LUMA, uiTrDepth ) )
  {
    //===== inverse transform =====
    m_pcTrQuant->setQPforQuant  ( pcCU->getQP(0), TEXT_LUMA, pcCU->getSlice()->getSPS()->getQpBDOffsetY(), 0 );
    
    Int scalingListType = (pcCU->isIntra(uiAbsPartIdx) ? 0 : 3) + g_eTTable[(Int)TEXT_LUMA];
    assert(scalingListType < SCALING_LIST_NUM);
    m_pcTrQuant->invtransformNxN( pcCU->getCUTransquantBypass(uiAbsPartIdx), TEXT_LUMA, pcCU->getLumaIntraDir( uiAbsPartIdx ), piResi, uiStride, pcCoeff, uiWidth, uiHeight, scalingListType, useTransformSkip 
#if QC_EMT_INTRA
      , pcCU->getSlice()->getSPS()->getUseIntraEMT() ? pcCU->getEmtTuIdx(uiAbsPartIdx) : DCT2_HEVC
#endif
#if ROT_TR 
   , pcCU->getROTIdx(uiAbsPartIdx)
#endif
#if QC_USE_65ANG_MODES
    , pcCU->getUseExtIntraAngModes(uiPUWidth)
#endif
      );
    
    
    //===== reconstruction =====
    Pel* pPred      = piPred;
    Pel* pResi      = piResi;
    Pel* pReco      = piReco;
    Pel* pRecIPred  = piRecIPred;
    for( UInt uiY = 0; uiY < uiHeight; uiY++ )
    {
      for( UInt uiX = 0; uiX < uiWidth; uiX++ )
      {
        pReco    [ uiX ] = ClipY( pPred[ uiX ] + pResi[ uiX ] );
        pRecIPred[ uiX ] = pReco[ uiX ];
      }
      pPred     += uiStride;
      pResi     += uiStride;
      pReco     += uiStride;
      pRecIPred += uiRecIPredStride;
    }
  }
  else
  {
    //===== reconstruction =====
    Pel* pPred      = piPred;
    Pel* pReco      = piReco;
    Pel* pRecIPred  = piRecIPred;
    for ( Int y = 0; y < uiHeight; y++ )
    {
      for ( Int x = 0; x < uiWidth; x++ )
      {
        pReco    [ x ] = pPred[ x ];
        pRecIPred[ x ] = pReco[ x ];
      }
      pPred     += uiStride;
      pReco     += uiStride;
      pRecIPred += uiRecIPredStride;
    }
  }
}

#if INTRA_KLT
Void
TDecCu::xIntraRecLumaBlkTM( TComDataCU* pcCU,
              UInt        uiTrDepth,
              UInt        uiAbsPartIdx,
              TComYuv*    pcRecoYuv,
              TComYuv*    pcPredYuv,
              TComYuv*    pcResiYuv,
              Int        genPred0genPredAndtrainKLT1)
{
  UInt    uiWidth = pcCU->getWidth(0) >> uiTrDepth;
  UInt    uiHeight = pcCU->getHeight(0) >> uiTrDepth;
  UInt    uiStride = pcRecoYuv->getStride();
  Pel*    piReco = pcRecoYuv->getLumaAddr(uiAbsPartIdx);
  Pel*    piPred = pcPredYuv->getLumaAddr(uiAbsPartIdx);
  Pel*    piResi = pcResiYuv->getLumaAddr(uiAbsPartIdx);

  UInt    uiNumCoeffInc = (pcCU->getSlice()->getSPS()->getMaxCUWidth() * pcCU->getSlice()->getSPS()->getMaxCUHeight()) >> (pcCU->getSlice()->getSPS()->getMaxCUDepth() << 1);
  TCoeff* pcCoeff = pcCU->getCoeffY() + (uiNumCoeffInc * uiAbsPartIdx);

  // UInt    uiLumaPredMode = pcCU->getLumaIntraDir(uiAbsPartIdx);

  UInt    uiZOrder = pcCU->getZorderIdxInCU() + uiAbsPartIdx;
  Pel*    piRecIPred = pcCU->getPic()->getPicYuvRec()->getLumaAddr(pcCU->getAddr(), uiZOrder);
  UInt    uiRecIPredStride = pcCU->getPic()->getPicYuvRec()->getStride();
  Bool    useTransformSkip = pcCU->getTransformSkip(uiAbsPartIdx, TEXT_LUMA);
#if QC_USE_65ANG_MODES
  UInt  uiPUWidth = pcCU->getWidth(uiAbsPartIdx) >> (pcCU->getPartitionSize(uiAbsPartIdx) == SIZE_NxN ? 1 : 0);
#endif
  if (pcCU->getPic()->getPOC() == 30 && pcCU->getAddr() == 0 && pcCU->getZorderIdxInCU() + uiAbsPartIdx == 6)
  {
    printf("Come here\n");
  }
  UInt uiBlkSize = uiWidth;
  UInt uiTarDepth = g_aucConvertToBit[uiBlkSize];
  // Pel   *pCurrStart = pcCU->getPic()->getPicYuvRec()->getLumaAddr(pcCU->getAddr(), uiZOrder);
  // UInt  uiPicStride = pcCU->getPic()->getStride();

  UInt uiTempSize = g_uiDepth2IntraTempSize[uiTarDepth];
  m_pcTrQuant->getTargetTemplate(pcCU, uiAbsPartIdx, uiAbsPartIdx, pcPredYuv, uiBlkSize, uiTempSize);
  m_pcTrQuant->candidateSearchIntra(pcCU, uiAbsPartIdx, uiBlkSize, uiTempSize);

  Bool useKLT = false;
  Int foundCandiNum;
  Bool bSuccessful = m_pcTrQuant->generateTMPrediction(piPred, uiStride, uiBlkSize, uiTempSize, genPred0genPredAndtrainKLT1, foundCandiNum);
  if (1 == genPred0genPredAndtrainKLT1 && bSuccessful)
  {
    useKLT = m_pcTrQuant->calcKLTIntra(piPred, uiStride, uiBlkSize, uiTempSize);
  }
  assert(foundCandiNum >= 1);

  if (pcCU->getCbf(uiAbsPartIdx, TEXT_LUMA, uiTrDepth))
  {
    //===== inverse transform =====
    m_pcTrQuant->setQPforQuant(pcCU->getQP(0), TEXT_LUMA, pcCU->getSlice()->getSPS()->getQpBDOffsetY(), 0);

    Int scalingListType = (pcCU->isIntra(uiAbsPartIdx) ? 0 : 3) + g_eTTable[(Int)TEXT_LUMA];
    assert(scalingListType < SCALING_LIST_NUM);

    UInt *scan = 0;
    if (useKLT)
    {
      const UInt  uiLog2BlockSize = g_aucConvertToBit[uiWidth] + 2;
      UInt scanIdx = pcCU->getCoefScanIdx(uiAbsPartIdx, uiWidth, TEXT_LUMA == TEXT_LUMA, pcCU->isIntra(uiAbsPartIdx));
      scan = g_auiSigLastScan[scanIdx][uiLog2BlockSize - 1];
      recoverOrderCoeff(pcCoeff, scan, uiWidth, uiHeight);
    }

    m_pcTrQuant->invtransformNxN(pcCU->getCUTransquantBypass(uiAbsPartIdx), TEXT_LUMA, pcCU->getLumaIntraDir(uiAbsPartIdx), piResi, uiStride, pcCoeff, uiWidth, uiHeight, scalingListType, useTransformSkip
#if QC_EMT_INTRA
      , DCT2_HEVC
#endif
#if ROT_TR 
      , pcCU->getROTIdx(uiAbsPartIdx)
#endif
#if QC_USE_65ANG_MODES
      , pcCU->getUseExtIntraAngModes(uiPUWidth)
#endif
      , useKLT);

    if (useKLT)
    {
      reOrderCoeff(pcCoeff, scan, uiWidth, uiHeight);
    }
    //===== reconstruction =====
    Pel* pPred = piPred;
    Pel* pResi = piResi;
    Pel* pReco = piReco;
    Pel* pRecIPred = piRecIPred;
    for (UInt uiY = 0; uiY < uiHeight; uiY++)
    {
      for (UInt uiX = 0; uiX < uiWidth; uiX++)
      {
        pReco[uiX] = ClipY(pPred[uiX] + pResi[uiX]);
        pRecIPred[uiX] = pReco[uiX];
      }
      pPred += uiStride;
      pResi += uiStride;
      pReco += uiStride;
      pRecIPred += uiRecIPredStride;
    }
  }
  else
  {
    //===== reconstruction =====
    Pel* pPred = piPred;
    Pel* pReco = piReco;
    Pel* pRecIPred = piRecIPred;
    for (Int y = 0; y < uiHeight; y++)
    {
      for (Int x = 0; x < uiWidth; x++)
      {
        pReco[x] = pPred[x];
        pRecIPred[x] = pReco[x];
      }
      pPred += uiStride;
      pReco += uiStride;
      pRecIPred += uiRecIPredStride;
    }
  }
}
#endif


Void
TDecCu::xIntraRecChromaBlk( TComDataCU* pcCU,
                           UInt        uiTrDepth,
                           UInt        uiAbsPartIdx,
                           TComYuv*    pcRecoYuv,
                           TComYuv*    pcPredYuv, 
                           TComYuv*    pcResiYuv,
                           UInt        uiChromaId )
{
  UInt uiFullDepth  = pcCU->getDepth( 0 ) + uiTrDepth;
  UInt uiLog2TrSize = g_aucConvertToBit[ pcCU->getSlice()->getSPS()->getMaxCUWidth() >> uiFullDepth ] + 2;

  if( uiLog2TrSize == 2 )
  {
    assert( uiTrDepth > 0 );
    uiTrDepth--;
    UInt uiQPDiv = pcCU->getPic()->getNumPartInCU() >> ( ( pcCU->getDepth( 0 ) + uiTrDepth ) << 1 );
    Bool bFirstQ = ( ( uiAbsPartIdx % uiQPDiv ) == 0 );
    if( !bFirstQ )
    {
      return;
    }
  }
  
  TextType  eText             = ( uiChromaId > 0 ? TEXT_CHROMA_V : TEXT_CHROMA_U );
  UInt      uiWidth           = pcCU     ->getWidth   ( 0 ) >> ( uiTrDepth + 1 );
  UInt      uiHeight          = pcCU     ->getHeight  ( 0 ) >> ( uiTrDepth + 1 );
  UInt      uiStride          = pcRecoYuv->getCStride ();
  Pel*      piReco            = ( uiChromaId > 0 ? pcRecoYuv->getCrAddr( uiAbsPartIdx ) : pcRecoYuv->getCbAddr( uiAbsPartIdx ) );
  Pel*      piPred            = ( uiChromaId > 0 ? pcPredYuv->getCrAddr( uiAbsPartIdx ) : pcPredYuv->getCbAddr( uiAbsPartIdx ) );
  Pel*      piResi            = ( uiChromaId > 0 ? pcResiYuv->getCrAddr( uiAbsPartIdx ) : pcResiYuv->getCbAddr( uiAbsPartIdx ) );
  
  UInt      uiNumCoeffInc     = ( ( pcCU->getSlice()->getSPS()->getMaxCUWidth() * pcCU->getSlice()->getSPS()->getMaxCUHeight() ) >> ( pcCU->getSlice()->getSPS()->getMaxCUDepth() << 1 ) ) >> 2;
  TCoeff*   pcCoeff           = ( uiChromaId > 0 ? pcCU->getCoeffCr() : pcCU->getCoeffCb() ) + ( uiNumCoeffInc * uiAbsPartIdx );
  
  UInt      uiChromaPredMode  = pcCU->getChromaIntraDir( 0 );
  
  UInt      uiZOrder          = pcCU->getZorderIdxInCU() + uiAbsPartIdx;
  Pel*      piRecIPred        = ( uiChromaId > 0 ? pcCU->getPic()->getPicYuvRec()->getCrAddr( pcCU->getAddr(), uiZOrder ) : pcCU->getPic()->getPicYuvRec()->getCbAddr( pcCU->getAddr(), uiZOrder ) );
  UInt      uiRecIPredStride  = pcCU->getPic()->getPicYuvRec()->getCStride();
  Bool      useTransformSkipChroma = pcCU->getTransformSkip(uiAbsPartIdx,eText);
  //===== init availability pattern =====
  Bool  bAboveAvail = false;
  Bool  bLeftAvail  = false;
  pcCU->getPattern()->initPattern         ( pcCU, uiTrDepth, uiAbsPartIdx );

#if QC_LMCHROMA
  if( uiChromaPredMode == LM_CHROMA_IDX && uiChromaId == 0 )
  {
    pcCU->getPattern()->initAdiPattern( pcCU, uiAbsPartIdx, uiTrDepth, m_pcPrediction->getPredicBuf(), m_pcPrediction->getPredicBufWidth(), m_pcPrediction->getPredicBufHeight(), bAboveAvail, bLeftAvail, true);
    Bool bLeftAvailLM = ( (pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ]) != 0 );
    m_pcPrediction->getLumaRecPixels(pcCU->getPattern(), uiWidth, uiHeight, !bLeftAvailLM );
  }
#endif

  pcCU->getPattern()->initAdiPatternChroma( pcCU, uiAbsPartIdx, uiTrDepth,
                                           m_pcPrediction->getPredicBuf       (),
                                           m_pcPrediction->getPredicBufWidth  (),
                                           m_pcPrediction->getPredicBufHeight (),
                                           bAboveAvail, bLeftAvail );
  Int* pPatChroma   = ( uiChromaId > 0 ? pcCU->getPattern()->getAdiCrBuf( uiWidth, uiHeight, m_pcPrediction->getPredicBuf() ) : pcCU->getPattern()->getAdiCbBuf( uiWidth, uiHeight, m_pcPrediction->getPredicBuf() ) );
  
  //===== get prediction signal =====
#if QC_LMCHROMA
  if( uiChromaPredMode == LM_CHROMA_IDX )
  {
    m_pcPrediction->predLMIntraChroma( pcCU->getPattern(), uiChromaId, piPred, uiStride, uiWidth, uiHeight );
  }
  else
#endif  
  {
    if( uiChromaPredMode == DM_CHROMA_IDX )
    {
      uiChromaPredMode = pcCU->getLumaIntraDir( 0 );
    }
    m_pcPrediction->predIntraChromaAng( pPatChroma, uiChromaPredMode, piPred, uiStride, uiWidth, uiHeight, bAboveAvail, bLeftAvail 
#if QC_INTRA_4TAP_FILTER
      , pcCU->getSlice()->getSPS()->getUse4TapIntraFilter()
#endif
#if CU_LEVEL_MPI
,  pcCU,  uiAbsPartIdx, uiChromaId
#endif
);  

#if QC_LMCHROMA
    if( uiChromaId == 1 && pcCU->getSlice()->getSPS()->getUseLMChroma() )
    { 
      m_pcPrediction->addCrossColorResi( pcCU->getPattern(), piPred, uiStride, uiWidth, uiHeight, pcResiYuv->getCbAddr( uiAbsPartIdx ), pcResiYuv->getCStride() );
    }
#endif
  }

  if ( pcCU->getCbf( uiAbsPartIdx, eText, uiTrDepth ) )
  {
    //===== inverse transform =====
    Int curChromaQpOffset;
    if(eText == TEXT_CHROMA_U)
    {
      curChromaQpOffset = pcCU->getSlice()->getPPS()->getChromaCbQpOffset() + pcCU->getSlice()->getSliceQpDeltaCb();
    }
    else
    {
      curChromaQpOffset = pcCU->getSlice()->getPPS()->getChromaCrQpOffset() + pcCU->getSlice()->getSliceQpDeltaCr();
    }
    m_pcTrQuant->setQPforQuant  ( pcCU->getQP(0), eText, pcCU->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset );
    
    Int scalingListType = (pcCU->isIntra(uiAbsPartIdx) ? 0 : 3) + g_eTTable[(Int)eText];
    assert(scalingListType < SCALING_LIST_NUM);
    m_pcTrQuant->invtransformNxN( pcCU->getCUTransquantBypass(uiAbsPartIdx), eText, REG_DCT, piResi, uiStride, pcCoeff, uiWidth, uiHeight, scalingListType, useTransformSkipChroma 
#if QC_EMT_INTRA
      , pcCU->getSlice()->getSPS()->getUseIntraEMT() ? DCT2_EMT : DCT2_HEVC
#endif
#if ROT_TR 
   , pcCU->getROTIdx(uiAbsPartIdx)
#endif
      );
    
    //===== reconstruction =====
    Pel* pPred      = piPred;
    Pel* pResi      = piResi;
    Pel* pReco      = piReco;
    Pel* pRecIPred  = piRecIPred;
    for( UInt uiY = 0; uiY < uiHeight; uiY++ )
    {
      for( UInt uiX = 0; uiX < uiWidth; uiX++ )
      {
        pReco    [ uiX ] = ClipC( pPred[ uiX ] + pResi[ uiX ] );
        pRecIPred[ uiX ] = pReco[ uiX ];
      }
      pPred     += uiStride;
      pResi     += uiStride;
      pReco     += uiStride;
      pRecIPred += uiRecIPredStride;
    }
  }
  else
  {
    //===== reconstruction =====
    Pel* pPred      = piPred;
    Pel* pReco      = piReco;
    Pel* pRecIPred  = piRecIPred;
#if QC_LMCHROMA
    Pel* pResi      = piResi;
#endif
    for ( Int y = 0; y < uiHeight; y++ )
    {
      for ( Int x = 0; x < uiWidth; x++ )
      {
        pReco    [ x ] = pPred[ x ];
        pRecIPred[ x ] = pReco[ x ];
#if QC_LMCHROMA
        pResi    [ x ] = 0;
#endif
      }
      pPred     += uiStride;
      pReco     += uiStride;
#if QC_LMCHROMA
      pResi     += uiStride;
#endif
      pRecIPred += uiRecIPredStride;
    }    
  }
}


Void
TDecCu::xReconIntraQT( TComDataCU* pcCU, UInt uiDepth )
{
  UInt  uiInitTrDepth = ( pcCU->getPartitionSize(0) == SIZE_2Nx2N ? 0 : 1 );
  UInt  uiNumPart     = pcCU->getNumPartitions();
  UInt  uiNumQParts   = pcCU->getTotalNumPart() >> 2;
  
  if (pcCU->getIPCMFlag(0))
  {
    xReconPCM( pcCU, uiDepth );
    return;
  }

  for( UInt uiPU = 0; uiPU < uiNumPart; uiPU++ )
  {
    xIntraLumaRecQT( pcCU, uiInitTrDepth, uiPU * uiNumQParts, m_ppcYuvReco[uiDepth], m_ppcYuvReco[uiDepth], m_ppcYuvResi[uiDepth] );
  }  

  for( UInt uiPU = 0; uiPU < uiNumPart; uiPU++ )
  {
    xIntraChromaRecQT( pcCU, uiInitTrDepth, uiPU * uiNumQParts, m_ppcYuvReco[uiDepth], m_ppcYuvReco[uiDepth], m_ppcYuvResi[uiDepth] );
  }

}

/** Function for deriving recontructed PU/CU Luma sample with QTree structure
 * \param pcCU pointer of current CU
 * \param uiTrDepth current tranform split depth
 * \param uiAbsPartIdx  part index
 * \param pcRecoYuv pointer to reconstructed sample arrays
 * \param pcPredYuv pointer to prediction sample arrays
 * \param pcResiYuv pointer to residue sample arrays
 * 
 \ This function dervies recontructed PU/CU Luma sample with recursive QTree structure
 */
Void
TDecCu::xIntraLumaRecQT( TComDataCU* pcCU,
                     UInt        uiTrDepth,
                     UInt        uiAbsPartIdx,
                     TComYuv*    pcRecoYuv,
                     TComYuv*    pcPredYuv, 
                     TComYuv*    pcResiYuv )
{
  UInt uiFullDepth  = pcCU->getDepth(0) + uiTrDepth;
  UInt uiTrMode     = pcCU->getTransformIdx( uiAbsPartIdx );
  if( uiTrMode == uiTrDepth )
  {
#if INTRA_KLT
    Bool bTMFlag = (Bool)pcCU->getKLTFlag(uiAbsPartIdx, TEXT_LUMA);
    if (bTMFlag)
    {
      Int genPred0genPredAndtrainKLT1 = GENPRED0GENPREDANDTRAINKLT1;
      xIntraRecLumaBlkTM(pcCU, uiTrDepth, uiAbsPartIdx, pcRecoYuv, pcPredYuv, pcResiYuv, genPred0genPredAndtrainKLT1);
    }
    else
    {
      xIntraRecLumaBlk(pcCU, uiTrDepth, uiAbsPartIdx, pcRecoYuv, pcPredYuv, pcResiYuv);
    }
#else
    xIntraRecLumaBlk  ( pcCU, uiTrDepth, uiAbsPartIdx, pcRecoYuv, pcPredYuv, pcResiYuv );
#endif
  }
  else
  {
    UInt uiNumQPart  = pcCU->getPic()->getNumPartInCU() >> ( ( uiFullDepth + 1 ) << 1 );
    for( UInt uiPart = 0; uiPart < 4; uiPart++ )
    {
      xIntraLumaRecQT( pcCU, uiTrDepth + 1, uiAbsPartIdx + uiPart * uiNumQPart, pcRecoYuv, pcPredYuv, pcResiYuv );
    }
  }
}

/** Function for deriving recontructed PU/CU chroma samples with QTree structure
 * \param pcCU pointer of current CU
 * \param uiTrDepth current tranform split depth
 * \param uiAbsPartIdx  part index
 * \param pcRecoYuv pointer to reconstructed sample arrays
 * \param pcPredYuv pointer to prediction sample arrays
 * \param pcResiYuv pointer to residue sample arrays
 * 
 \ This function dervies recontructed PU/CU chroma samples with QTree recursive structure
 */
Void
TDecCu::xIntraChromaRecQT( TComDataCU* pcCU,
                     UInt        uiTrDepth,
                     UInt        uiAbsPartIdx,
                     TComYuv*    pcRecoYuv,
                     TComYuv*    pcPredYuv, 
                     TComYuv*    pcResiYuv )
{
  UInt uiFullDepth  = pcCU->getDepth(0) + uiTrDepth;
  UInt uiTrMode     = pcCU->getTransformIdx( uiAbsPartIdx );
  if( uiTrMode == uiTrDepth )
  {
    xIntraRecChromaBlk( pcCU, uiTrDepth, uiAbsPartIdx, pcRecoYuv, pcPredYuv, pcResiYuv, 0 );
    xIntraRecChromaBlk( pcCU, uiTrDepth, uiAbsPartIdx, pcRecoYuv, pcPredYuv, pcResiYuv, 1 );
  }
  else
  {
    UInt uiNumQPart  = pcCU->getPic()->getNumPartInCU() >> ( ( uiFullDepth + 1 ) << 1 );
    for( UInt uiPart = 0; uiPart < 4; uiPart++ )
    {
      xIntraChromaRecQT( pcCU, uiTrDepth + 1, uiAbsPartIdx + uiPart * uiNumQPart, pcRecoYuv, pcPredYuv, pcResiYuv );
    }
  }
}

Void TDecCu::xCopyToPic( TComDataCU* pcCU, TComPic* pcPic, UInt uiZorderIdx, UInt uiDepth )
{
  UInt uiCUAddr = pcCU->getAddr();
  
  m_ppcYuvReco[uiDepth]->copyToPicYuv  ( pcPic->getPicYuvRec (), uiCUAddr, uiZorderIdx );
  
  return;
}

Void TDecCu::xDecodeInterTexture ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt    uiWidth    = pcCU->getWidth ( uiAbsPartIdx );
  UInt    uiHeight   = pcCU->getHeight( uiAbsPartIdx );
  TCoeff* piCoeff;
  
  Pel*    pResi;
  UInt    trMode = pcCU->getTransformIdx( uiAbsPartIdx );
  
  // Y
  piCoeff = pcCU->getCoeffY();
  pResi = m_ppcYuvResi[uiDepth]->getLumaAddr();

  m_pcTrQuant->setQPforQuant( pcCU->getQP( uiAbsPartIdx ), TEXT_LUMA, pcCU->getSlice()->getSPS()->getQpBDOffsetY(), 0 );
#if INTER_KLT
  TComYuv* pcPred = m_ppcYuvReco[uiDepth];
  m_pcTrQuant->invRecurTransformNxN(pcCU, 0, TEXT_LUMA, pResi, 0, m_ppcYuvResi[uiDepth]->getStride(), uiWidth, uiHeight, trMode, 0, piCoeff, pcPred);
#else
  m_pcTrQuant->invRecurTransformNxN ( pcCU, 0, TEXT_LUMA, pResi, 0, m_ppcYuvResi[uiDepth]->getStride(), uiWidth, uiHeight, trMode, 0, piCoeff );
#endif
  // Cb and Cr
  Int curChromaQpOffset = pcCU->getSlice()->getPPS()->getChromaCbQpOffset() + pcCU->getSlice()->getSliceQpDeltaCb();
  m_pcTrQuant->setQPforQuant( pcCU->getQP( uiAbsPartIdx ), TEXT_CHROMA, pcCU->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset );

  uiWidth  >>= 1;
  uiHeight >>= 1;
  piCoeff = pcCU->getCoeffCb(); pResi = m_ppcYuvResi[uiDepth]->getCbAddr();
#if INTER_KLT
  m_pcTrQuant->invRecurTransformNxN(pcCU, 0, TEXT_CHROMA_U, pResi, 0, m_ppcYuvResi[uiDepth]->getCStride(), uiWidth, uiHeight, trMode, 0, piCoeff, pcPred);
#else
  m_pcTrQuant->invRecurTransformNxN ( pcCU, 0, TEXT_CHROMA_U, pResi, 0, m_ppcYuvResi[uiDepth]->getCStride(), uiWidth, uiHeight, trMode, 0, piCoeff );
#endif
  curChromaQpOffset = pcCU->getSlice()->getPPS()->getChromaCrQpOffset() + pcCU->getSlice()->getSliceQpDeltaCr();
  m_pcTrQuant->setQPforQuant( pcCU->getQP( uiAbsPartIdx ), TEXT_CHROMA, pcCU->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset );

  piCoeff = pcCU->getCoeffCr(); pResi = m_ppcYuvResi[uiDepth]->getCrAddr();
#if INTER_KLT
  m_pcTrQuant->invRecurTransformNxN(pcCU, 0, TEXT_CHROMA_V, pResi, 0, m_ppcYuvResi[uiDepth]->getCStride(), uiWidth, uiHeight, trMode, 0, piCoeff, pcPred);
#else
  m_pcTrQuant->invRecurTransformNxN ( pcCU, 0, TEXT_CHROMA_V, pResi, 0, m_ppcYuvResi[uiDepth]->getCStride(), uiWidth, uiHeight, trMode, 0, piCoeff );
#endif
}

/** Function for deriving reconstructed luma/chroma samples of a PCM mode CU.
 * \param pcCU pointer to current CU
 * \param uiPartIdx part index
 * \param piPCM pointer to PCM code arrays
 * \param piReco pointer to reconstructed sample arrays
 * \param uiStride stride of reconstructed sample arrays
 * \param uiWidth CU width
 * \param uiHeight CU height
 * \param ttText texture component type
 * \returns Void
 */
Void TDecCu::xDecodePCMTexture( TComDataCU* pcCU, UInt uiPartIdx, Pel *piPCM, Pel* piReco, UInt uiStride, UInt uiWidth, UInt uiHeight, TextType ttText)
{
  UInt uiX, uiY;
  Pel* piPicReco;
  UInt uiPicStride;
  UInt uiPcmLeftShiftBit; 

  if( ttText == TEXT_LUMA )
  {
    uiPicStride   = pcCU->getPic()->getPicYuvRec()->getStride();
    piPicReco = pcCU->getPic()->getPicYuvRec()->getLumaAddr(pcCU->getAddr(), pcCU->getZorderIdxInCU()+uiPartIdx);
    uiPcmLeftShiftBit = g_bitDepthY - pcCU->getSlice()->getSPS()->getPCMBitDepthLuma();
  }
  else
  {
    uiPicStride = pcCU->getPic()->getPicYuvRec()->getCStride();

    if( ttText == TEXT_CHROMA_U )
    {
      piPicReco = pcCU->getPic()->getPicYuvRec()->getCbAddr(pcCU->getAddr(), pcCU->getZorderIdxInCU()+uiPartIdx);
    }
    else
    {
      piPicReco = pcCU->getPic()->getPicYuvRec()->getCrAddr(pcCU->getAddr(), pcCU->getZorderIdxInCU()+uiPartIdx);
    }
    uiPcmLeftShiftBit = g_bitDepthC - pcCU->getSlice()->getSPS()->getPCMBitDepthChroma();
  }

  for( uiY = 0; uiY < uiHeight; uiY++ )
  {
    for( uiX = 0; uiX < uiWidth; uiX++ )
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
  // Luma
  UInt uiWidth  = (g_uiMaxCUWidth >> uiDepth);
  UInt uiHeight = (g_uiMaxCUHeight >> uiDepth);

  Pel* piPcmY = pcCU->getPCMSampleY();
  Pel* piRecoY = m_ppcYuvReco[uiDepth]->getLumaAddr(0, uiWidth);

  UInt uiStride = m_ppcYuvResi[uiDepth]->getStride();

  xDecodePCMTexture( pcCU, 0, piPcmY, piRecoY, uiStride, uiWidth, uiHeight, TEXT_LUMA);

  // Cb and Cr
  UInt uiCWidth  = (uiWidth>>1);
  UInt uiCHeight = (uiHeight>>1);

  Pel* piPcmCb = pcCU->getPCMSampleCb();
  Pel* piPcmCr = pcCU->getPCMSampleCr();
  Pel* pRecoCb = m_ppcYuvReco[uiDepth]->getCbAddr();
  Pel* pRecoCr = m_ppcYuvReco[uiDepth]->getCrAddr();

  UInt uiCStride = m_ppcYuvReco[uiDepth]->getCStride();

  xDecodePCMTexture( pcCU, 0, piPcmCb, pRecoCb, uiCStride, uiCWidth, uiCHeight, TEXT_CHROMA_U);
  xDecodePCMTexture( pcCU, 0, piPcmCr, pRecoCr, uiCStride, uiCWidth, uiCHeight, TEXT_CHROMA_V);
}

/** Function for filling the PCM buffer of a CU using its reconstructed sample array 
 * \param pcCU pointer to current CU
 * \param uiDepth CU Depth
 * \returns Void
 */
Void TDecCu::xFillPCMBuffer(TComDataCU* pCU, UInt depth)
{
  // Luma
  UInt width  = (g_uiMaxCUWidth >> depth);
  UInt height = (g_uiMaxCUHeight >> depth);

  Pel* pPcmY = pCU->getPCMSampleY();
  Pel* pRecoY = m_ppcYuvReco[depth]->getLumaAddr(0, width);

  UInt stride = m_ppcYuvReco[depth]->getStride();

  for(Int y = 0; y < height; y++ )
  {
    for(Int x = 0; x < width; x++ )
    {
      pPcmY[x] = pRecoY[x];
    }
    pPcmY += width;
    pRecoY += stride;
  }

  // Cb and Cr
  UInt widthC  = (width>>1);
  UInt heightC = (height>>1);

  Pel* pPcmCb = pCU->getPCMSampleCb();
  Pel* pPcmCr = pCU->getPCMSampleCr();
  Pel* pRecoCb = m_ppcYuvReco[depth]->getCbAddr();
  Pel* pRecoCr = m_ppcYuvReco[depth]->getCrAddr();

  UInt strideC = m_ppcYuvReco[depth]->getCStride();

  for(Int y = 0; y < heightC; y++ )
  {
    for(Int x = 0; x < widthC; x++ )
    {
      pPcmCb[x] = pRecoCb[x];
      pPcmCr[x] = pRecoCr[x];
    }
    pPcmCr += widthC;
    pPcmCb += widthC;
    pRecoCb += strideC;
    pRecoCr += strideC;
  }

}

#if QC_FRUC_MERGE
Void TDecCu::xDeriveCUMV( TComDataCU * pcCU , UInt uiAbsPartIdx , UInt uiDepth )
{
  PartSize ePartSize = pcCU->getPartitionSize( uiAbsPartIdx );
  UInt uiNumPU = ( ePartSize == SIZE_2Nx2N ? 1 : ( ePartSize == SIZE_NxN ? 4 : 2 ) );
  UInt uiPUOffset = ( g_auiPUOffset[UInt( ePartSize )] << ( ( pcCU->getSlice()->getSPS()->getMaxCUDepth() - uiDepth ) << 1 ) ) >> 4;
  TComDataCU * pcSubCU = m_ppcCU[uiDepth];

  TComMvField cMvFieldNeighbours[MRG_MAX_NUM_CANDS << 1]; // double length for mv of both lists
  UChar uhInterDirNeighbours[MRG_MAX_NUM_CANDS];
  memset( uhInterDirNeighbours , 0 , sizeof( uhInterDirNeighbours ) );
  Int numValidMergeCand = 0;
  Bool isMerged = false;

#if QC_SUB_PU_TMVP
  UChar    eMergeCandTypeNieghors[MRG_MAX_NUM_CANDS];
  memset( eMergeCandTypeNieghors, MGR_TYPE_DEFAULT_N, sizeof(UChar)*MRG_MAX_NUM_CANDS);
#endif
#if QC_IC
  Bool abICFlag[MRG_MAX_NUM_CANDS];
#endif
  for ( UInt uiPartIdx = 0, uiSubPartIdx = uiAbsPartIdx; uiPartIdx < uiNumPU; uiPartIdx++, uiSubPartIdx += uiPUOffset )
  {
    if ( pcCU->getMergeFlag( uiSubPartIdx ) )
    {
      if( pcCU->getFRUCMgrMode( uiSubPartIdx ) )
      {
        Bool bAvailable = m_pcPrediction->deriveFRUCMV( pcSubCU , uiDepth , uiSubPartIdx - pcSubCU->getZorderIdxInCU() , uiPartIdx );
        assert( bAvailable );
      }
      else
      {
        UInt uiMergeIndex = pcCU->getMergeIndex(uiSubPartIdx);
        if ( pcCU->getSlice()->getPPS()->getLog2ParallelMergeLevelMinus2() && ePartSize != SIZE_2Nx2N && pcSubCU->getWidth( 0 ) <= 8 ) 
        {
          pcSubCU->setPartSizeSubParts( SIZE_2Nx2N, 0, uiDepth );
          if ( !isMerged )
          {
            pcSubCU->getInterMergeCandidates( 0, 0, cMvFieldNeighbours, uhInterDirNeighbours, numValidMergeCand 
#if QC_IC
              , abICFlag
#endif
#if QC_SUB_PU_TMVP
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
#if QC_IC
            abICFlag,
#endif
#if QC_SUB_PU_TMVP
            eMergeCandTypeNieghors , m_pMvFieldSP , m_phInterDirSP ,
#endif
            uiMergeIndex );
        }
        pcCU->setInterDirSubParts( uhInterDirNeighbours[uiMergeIndex], uiSubPartIdx, uiPartIdx, uiDepth );
#if QC_IC
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
#if QC_SUB_PU_TMVP
        pcCU->setMergeTypeSubParts( eMergeCandTypeNieghors[uiMergeIndex], uiSubPartIdx, uiPartIdx, uiDepth ); 
#if QC_SUB_PU_TMVP_EXT
        if( eMergeCandTypeNieghors[uiMergeIndex]==MGR_TYPE_SUBPU_TMVP || eMergeCandTypeNieghors[uiMergeIndex]==MGR_TYPE_SUBPU_TMVP_EXT )
#else
        if( eMergeCandTypeNieghors[uiMergeIndex]==MGR_TYPE_SUBPU_TMVP )
#endif
        {
          Int iWidth, iHeight;
          UInt uiIdx;
          pcCU->getPartIndexAndSize( uiPartIdx, uiIdx, iWidth, iHeight, uiSubPartIdx, true );

          UInt uiSPAddr;

          Int iNumSPInOneLine, iNumSP, iSPWidth, iSPHeight;
#if QC_SUB_PU_TMVP_EXT
          UInt uiSPListIndex =  eMergeCandTypeNieghors[uiMergeIndex]==MGR_TYPE_SUBPU_TMVP ? 0:1;
#endif
          pcCU->getSPPara(iWidth, iHeight, iNumSP, iNumSPInOneLine, iSPWidth, iSPHeight);

          for (Int iPartitionIdx = 0; iPartitionIdx < iNumSP; iPartitionIdx++)
          {
            pcCU->getSPAbsPartIdx(uiSubPartIdx, iSPWidth, iSPHeight, iPartitionIdx, iNumSPInOneLine, uiSPAddr);
#if QC_SUB_PU_TMVP_EXT
            pcCU->setInterDirSP(m_phInterDirSP[uiSPListIndex][iPartitionIdx], uiSPAddr, iSPWidth, iSPHeight);
            pcCU->getCUMvField( REF_PIC_LIST_0 )->setMvFieldSP(pcCU, uiSPAddr, m_pMvFieldSP[uiSPListIndex][2*iPartitionIdx], iSPWidth, iSPHeight);
            pcCU->getCUMvField( REF_PIC_LIST_1 )->setMvFieldSP(pcCU, uiSPAddr, m_pMvFieldSP[uiSPListIndex][2*iPartitionIdx + 1], iSPWidth, iSPHeight);
#else
            pcCU->setInterDirSP(m_phInterDirSP[iPartitionIdx], uiSPAddr, iSPWidth, iSPHeight);
            pcCU->getCUMvField( REF_PIC_LIST_0 )->setMvFieldSP(pcCU, uiSPAddr, m_pMvFieldSP[2*iPartitionIdx], iSPWidth, iSPHeight);
            pcCU->getCUMvField( REF_PIC_LIST_1 )->setMvFieldSP(pcCU, uiSPAddr, m_pMvFieldSP[2*iPartitionIdx + 1], iSPWidth, iSPHeight);
#endif
          }
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
          Short iMvdHor = pcCU->getCUMvField( eRefList )->getMvd( uiSubPartIdx ).getHor();
          Short iMvdVer = pcCU->getCUMvField( eRefList )->getMvd( uiSubPartIdx ).getVer();
#if QC_IMV
          if( pcCU->getiMVFlag( uiSubPartIdx ) )
          {
#if QC_MV_STORE_PRECISION_BIT
            iMvdHor <<= QC_MV_SIGNAL_PRECISION_BIT;
            iMvdVer <<= QC_MV_SIGNAL_PRECISION_BIT;
#else
            iMvdHor <<= 2;
            iMvdVer <<= 2;
#endif
          }
#endif
          TComMv cMv( iMvdHor, iMvdVer );

          pcCU->getCUMvField( RefPicList( uiRefListIdx ) )->setAllMvd( cMv, ePartSize, uiSubPartIdx, uiDepth, uiPartIdx );

          AMVPInfo* pAMVPInfo = pcSubCU->getCUMvField( eRefList )->getAMVPInfo();
          pcSubCU->fillMvpCand(uiPartIdx, uiSubPartIdx - uiAbsPartIdx, eRefList, pcSubCU->getCUMvField( eRefList )->getRefIdx( uiSubPartIdx - uiAbsPartIdx ), pAMVPInfo, m_pcPrediction );
          m_pcPrediction->getMvPredAMVP( pcSubCU, uiPartIdx, uiSubPartIdx - uiAbsPartIdx, RefPicList( uiRefListIdx ), cMv );
          cMv += TComMv( iMvdHor, iMvdVer );
          pcSubCU->getCUMvField( RefPicList( uiRefListIdx ) )->setAllMv( cMv, ePartSize, uiSubPartIdx - uiAbsPartIdx, 0, uiPartIdx);
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
