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

/**
 * \file
 * \brief Declaration of TComInterpolationFilter class
 */

#ifndef __TCOMINTERPOLATIONFILTER__
#define __TCOMINTERPOLATIONFILTER__

#include "CommonDef.h"

//! \ingroup TLibCommon
//! \{

#define NTAPS_LUMA        8 ///< Number of taps for luma
#define NTAPS_CHROMA      4 ///< Number of taps for chroma
#if VCEG_AZ07_FRUC_MERGE
#define NTAPS_LUMA_FRUC   2
#endif
#define IF_INTERNAL_PREC 14 ///< Number of bits for internal precision
#define IF_FILTER_PREC    6 ///< Log2 of sum of filter taps
#define IF_INTERNAL_OFFS (1<<(IF_INTERNAL_PREC-1)) ///< Offset used internally

#if COM16_C1016_AFFINE
#define IF_FILTER_PREC_AFFINE    8  ///< Log2 of sum of affine filter taps
#define NFRACS_LUMA_AFFINE       64 ///< Number of fraction positions for luma affine MCP
#define NFRACS_CHROMA_AFFINE     64 ///< Number of fraction positions for chroma affine MCP
#endif

/**
 * \brief Interpolation filter class
 */
class TComInterpolationFilter
{
#if VCEG_AZ07_MV_ADD_PRECISION_BIT_FOR_STORE == 1
  static const Short m_lumaFilter[8][NTAPS_LUMA];     ///< Luma filter taps
  static const Short m_chromaFilter[16][NTAPS_CHROMA]; ///< Chroma filter taps
#else
  static const TFilterCoeff m_lumaFilter[LUMA_INTERPOLATION_FILTER_SUB_SAMPLE_POSITIONS][NTAPS_LUMA];     ///< Luma filter taps
  static const TFilterCoeff m_chromaFilter[CHROMA_INTERPOLATION_FILTER_SUB_SAMPLE_POSITIONS][NTAPS_CHROMA]; ///< Chroma filter taps
#endif
#if VCEG_AZ07_FRUC_MERGE
#if VCEG_AZ07_MV_ADD_PRECISION_BIT_FOR_STORE == 1
  static const Short m_lumaFilterBilinear[8][NTAPS_LUMA_FRUC];     ///< Luma filter taps
#else
  static const Short m_lumaFilterBilinear[4][NTAPS_LUMA_FRUC];     ///< Luma filter taps
#endif
#endif

#if COM16_C1016_AFFINE
  static const Short m_lumaFilterAffine[(NFRACS_LUMA_AFFINE)*NTAPS_LUMA];
  static const Short m_chromaFilterAffine[(NFRACS_CHROMA_AFFINE)*NTAPS_CHROMA];
#endif

  static Void filterCopy(Int bitDepth, const Pel *src, Int srcStride, Pel *dst, Int dstStride, Int width, Int height, Bool isFirst, Bool isLast);

  template<Int N, Bool isVertical, Bool isFirst, Bool isLast>
  static Void filter(Int bitDepth, Pel const *src, Int srcStride, Pel *dst, Int dstStride, Int width, Int height, TFilterCoeff const *coeff);

  template<Int N>
  static Void filterHor(Int bitDepth, Pel *src, Int srcStride, Pel *dst, Int dstStride, Int width, Int height,               Bool isLast, TFilterCoeff const *coeff);
  template<Int N>
  static Void filterVer(Int bitDepth, Pel *src, Int srcStride, Pel *dst, Int dstStride, Int width, Int height, Bool isFirst, Bool isLast, TFilterCoeff const *coeff);

#if COM16_C1016_AFFINE
  template<Int N, Bool isVertical, Bool isFirst, Bool isLast>
  static Void filterAffine(Int bitDepth, Pel const *src, Int srcStride, Short *dst, Int dstStride, Int width, Int height, Short const *coeff);

  template<Int N>
  static Void filterHorAffine(Int bitDepth, Pel *src, Int srcStride, Short *dst, Int dstStride, Int width, Int height,               Bool isLast, Short const *coeff);
  template<Int N>
  static Void filterVerAffine(Int bitDepth, Pel *src, Int srcStride, Short *dst, Int dstStride, Int width, Int height, Bool isFirst, Bool isLast, Short const *coeff);
#endif

public:
  TComInterpolationFilter() {}
  ~TComInterpolationFilter() {}

  Void filterHor(const ComponentID compID, Pel *src, Int srcStride, Pel *dst, Int dstStride, Int width, Int height, Int frac,               Bool isLast, const ChromaFormat fmt, const Int bitDepth 
#if VCEG_AZ07_FRUC_MERGE
    , Int nFilterIdx = 0
#endif
    );
  Void filterVer(const ComponentID compID, Pel *src, Int srcStride, Pel *dst, Int dstStride, Int width, Int height, Int frac, Bool isFirst, Bool isLast, const ChromaFormat fmt, const Int bitDepth 
#if VCEG_AZ07_FRUC_MERGE
    , Int nFilterIdx = 0
#endif
    );

#if COM16_C1016_AFFINE
  Void filterHorAffine(const ComponentID compID, Pel *src, Int srcStride, Pel *dst, Int dstStride, Int width, Int height, Int frac,               Bool isLast, const ChromaFormat fmt, const Int bitDepth );
  Void filterVerAffine(const ComponentID compID, Pel *src, Int srcStride, Pel *dst, Int dstStride, Int width, Int height, Int frac, Bool isFirst, Bool isLast, const ChromaFormat fmt, const Int bitDepth );
#endif
};

//! \}

#endif