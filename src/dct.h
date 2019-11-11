#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES // Pi

#include "image.h"
#include "math.h"

// Forward DCT operation for NxN block
// If <all> set, then YCbCr each have DCT performed on them.
// Else, only Y has DCT performed on it.
std::vector<PixelYcbcr> DCT(std::vector<PixelYcbcr> pixels, N, bool all);

// Inverse DCT operation for NxN block
// If <all> set, then YCbCr each have IDCT performed on them.
// Else, only Y has IDCT performed on it.
std::vector<pixel_ycbcr> IDCT(std::vector<pixel_ycbcr> pixels, N, bool all);

