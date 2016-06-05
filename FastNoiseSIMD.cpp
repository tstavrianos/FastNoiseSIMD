// FastNoiseSIMD.cpp
//
// MIT License
//
// Copyright(c) 2016 Jordan Peck
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// The developer's email is jorzixdan.me2@gzixmail.com (for great email, take
// off every 'zix'.)
//

#include "FastNoiseSIMD.h"

// Intrisic headers retroactively include others
//#include <immintrin.h> //AVX FN_AVX2 FMA3
#include <smmintrin.h> //SSE4.1
//#include <emmintrin.h> //SSE2

#include <algorithm> // CPUid

// Macro redefinition warning
#ifdef _MSC_VER
#pragma warning(disable : 4005)
#endif

// Compile once for each instruction set
#ifdef FN_COMPILE_NO_SIMD_FALLBACK
#define SIMD_LEVEL FN_NO_SIMD_FALLBACK
#include "FastNoiseSIMD_internal.cpp"
#endif

#ifdef FN_COMPILE_SSE2
#define SIMD_LEVEL FN_SSE2
#include "FastNoiseSIMD_internal.cpp"
#endif

#ifdef FN_COMPILE_SSE41
#define SIMD_LEVEL FN_SSE41
#include "FastNoiseSIMD_internal.cpp"
#endif

// FN_AVX2 compiled directly through FastNoiseSIMD_internal.cpp to allow arch:AVX flag

int FastNoiseSIMD::s_currentSIMDLevel = -1;

int GetFastestSIMD()
{
	// https://github.com/Mysticial/FeatureDetector

	int cpuInfo[4];

	__cpuidex(cpuInfo, 0, 0);
	int nIds = cpuInfo[0];

	if (nIds < 0x00000001)
		return FN_NO_SIMD_FALLBACK;

	__cpuidex(cpuInfo, 0x00000001, 0);

	// FN_SSE2
	if ((cpuInfo[3] & 1 << 26) == 0)
		return FN_NO_SIMD_FALLBACK;

	// FN_SSE41
	if ((cpuInfo[2] & 1 << 19) == 0)
		return FN_SSE2;

	// AVX
	bool osAVXSuport = (cpuInfo[2] & 1 << 27) != 0;
	bool cpuAVXSuport = (cpuInfo[2] & 1 << 28) != 0;

	if (osAVXSuport && cpuAVXSuport)
	{
		unsigned long long xcrFeatureMask = _xgetbv(_XCR_XFEATURE_ENABLED_MASK);
		if ((xcrFeatureMask & 0x6) == 0)
			return FN_SSE41;
	}
	else
		return FN_SSE41;

	// FN_AVX2 FMA3
	if (nIds < 0x00000007)
		return FN_SSE41;

	bool cpuFMA3Support = (cpuInfo[2] & 1 << 12) != 0;

	__cpuidex(cpuInfo, 0x00000007, 0);

	bool cpuAVX2Support = (cpuInfo[1] & 1 << 5) != 0;

	if (cpuFMA3Support && cpuAVX2Support)
		return FN_AVX2;
	else
		return FN_SSE41;
}

FastNoiseSIMD* FastNoiseSIMD::NewFastNoiseSIMD(int seed)
{
	if (s_currentSIMDLevel < 0)
		s_currentSIMDLevel = GetFastestSIMD();

#ifdef FN_COMPILE_AVX2
	if (s_currentSIMDLevel >= FN_AVX2)
		return new FastNoiseSIMD_internal::FASTNOISE_SIMD_CLASS(FN_AVX2)(seed);
#endif

#ifdef FN_COMPILE_SSE41
	if (s_currentSIMDLevel >= FN_SSE41)
		return new FastNoiseSIMD_internal::FASTNOISE_SIMD_CLASS(FN_SSE41)(seed);
#endif

#ifdef FN_COMPILE_SSE2
	if (s_currentSIMDLevel >= FN_SSE2)
		return new FastNoiseSIMD_internal::FASTNOISE_SIMD_CLASS(FN_SSE2)(seed);
#endif

#ifdef FN_COMPILE_NO_SIMD_FALLBACK
	return new FastNoiseSIMD_internal::FASTNOISE_SIMD_CLASS(FN_NO_SIMD_FALLBACK)(seed);
#else
	return nullptr;
#endif
}

void FastNoiseSIMD::FreeNoiseSet(float* floatArray)
{
	if (s_currentSIMDLevel > FN_NO_SIMD_FALLBACK)
		_aligned_free(floatArray);
	else
		delete[] floatArray;
}

float* FastNoiseSIMD::GetNoiseSet(int xStart, int yStart, int zStart, int xSize, int ySize, int zSize, float stepDistance)
{
	float* floatSet = GetEmptySet(xSize, ySize, zSize);

	FillNoiseSet(floatSet, xStart,  yStart,  zStart,  xSize,  ySize,  zSize, stepDistance);

	return floatSet;
}

void FastNoiseSIMD::FillNoiseSet(float* floatSet, int xStart, int yStart, int zStart, int xSize, int ySize, int zSize, float stepDistance)
{
	switch (m_noiseType)
	{
	case Value:
		FillValueSet(floatSet, xStart, yStart, zStart, xSize, ySize, zSize, stepDistance);
		break;
	case ValueFractal:
		FillValueFractalSet(floatSet, xStart, yStart, zStart, xSize, ySize, zSize, stepDistance);
		break;
	case Gradient:
		FillGradientSet(floatSet, xStart, yStart, zStart, xSize, ySize, zSize, stepDistance); 
		break;
	case GradientFractal:
		FillGradientFractalSet(floatSet, xStart, yStart, zStart, xSize, ySize, zSize, stepDistance); 
		break;
	case Simplex: 
		FillSimplexSet(floatSet, xStart, yStart, zStart, xSize, ySize, zSize, stepDistance);
		break;
	case SimplexFractal: 
		FillSimplexFractalSet(floatSet, xStart, yStart, zStart, xSize, ySize, zSize, stepDistance); 
		break;
	case WhiteNoise: 
		break;
	default:
		break;
	}
}
