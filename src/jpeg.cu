#include <stdio.h>

#include <cuda.h>
#include <cuda_runtime.h>
#include <driver_functions.h>

#include "dct.h"
#include "quantize.h"
#include "dpcm.h"
#include "rle.h"

#define DEBUG

#ifdef DEBUG
#define cudaCheckError(ans) cudaAssert((ans), __FILE__, __LINE__);

inline void cudaAssert(cudaError_t code, const char *file, int line,
                       bool abort=true) {
    if(code != cudaSuccess) {
        fprintf(stderr, "CUDA Error: %s at %s:%d\n", cudaGetErrorString(code), file,
                line);
        if(abort) {
            exit(code);
        }
    }
}

#else
#define cudaCheckError(ans) ans
#endif

//cuda image.h
// bytes is an array
CudaImageRgb* cudaConvertBytesToImage(unsigned char* bytes, unsigned int width, unsigned int height) {
    return NULL;
}
unsigned char* cudaConvertImageToBytes(CudaImageRgb* image) {
    return NULL;
}

CudaImageYcbcr* cudaConvertRgbToYcbcr(CudaImageRgb* input) {
    return NULL;
}
CudaImageRgb* cudaConvertYcbcrToRgb(CudaImageYcbcr* input) {
    return NULL;
}

CudaImageBlocks* cudaConvertYcbcrToBlocks(CudaImageYcbcr* input, int block_size) {
    return NULL;
}
CudaImageYcbcr* cudaConvertBlocksToYcbcr(CudaImageBlocks* input, int block_size) {
    return NULL;
}

//cuda dct.h
// returns array of pointers
// pixels is an array of pointers
PixelYcbcr** cudaDCT(PixelYcbcr** pixels, int block_size, bool all) {
    return NULL;
}
// returns array of pointers
// pixels is an array of pointers
PixelYcbcr** cudaIDCT(PixelYcbcr** pixels, int block_size, bool all) {
    return NULL;
}

//cuda quantize.h
// returns array of pointers
// pixels is an array of pointers
PixelYcbcr** cudaQuantize(PixelYcbcr** pixels, int block_size, bool all) {
    return NULL;
}
// returns array of pointers
// pixels is an array of pointers
PixelYcbcr** cudaUnquantize(PixelYcbcr** pixels, int block_size, bool all) {
    return NULL;
}

//cuda dpcm.h
void cudaDPCM(PixelYcbcr*** blocks) {
    return;
}
void cudaUnDPCM(PixelYcbcr*** blocks) {
    return;
}

//cuda rle.h
// block is an array of pointers
EncodedBlock* cudaRLE(PixelYcbcr** block, int block_size) {
    return NULL;
}
// returns an array of pointers
PixelYcbcr** cudaDecodeRLE(EncodedBlock* encoded, int block_size) {
    return NULL;
}
