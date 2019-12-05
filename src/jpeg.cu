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
std::shared_ptr<ImageRgb> cudaConvertBytesToImage(std::vector<unsigned char> bytes, unsigned int width, unsigned int height);
std::vector<unsigned char> cudaConvertImageToBytes(std::shared_ptr<ImageRgb> image);

std::shared_ptr<ImageYcbcr> cudaConvertRgbToYcbcr(std::shared_ptr<ImageRgb> input);
std::shared_ptr<ImageRgb> cudaConvertYcbcrToRgb(std::shared_ptr<ImageYcbcr> input);

std::shared_ptr<ImageBlocks> cudaConvertYcbcrToBlocks(std::shared_ptr<ImageYcbcr> input, int block_size);
std::shared_ptr<ImageYcbcr> cudaConvertBlocksToYcbcr(std::shared_ptr<ImageBlocks> input, int block_size);

//cuda dct.h
std::vector<std::shared_ptr<PixelYcbcr>> cudaDCT(std::vector<std::shared_ptr<PixelYcbcr>> pixels, int block_size, bool all);
std::vector<std::shared_ptr<PixelYcbcr>> cudaIDCT(std::vector<std::shared_ptr<PixelYcbcr>> pixels, int block_size, bool all);

//cuda quantize.h
std::vector<std::shared_ptr<PixelYcbcr>> cudaQuantize(std::vector<std::shared_ptr<PixelYcbcr>> pixels, int block_size, bool all);
std::vector<std::shared_ptr<PixelYcbcr>> cudaUnquantize(std::vector<std::shared_ptr<PixelYcbcr>> pixels, int block_size, bool all);

//cuda dpcm.h
void cudaDPCM(std::vector<std::vector<std::shared_ptr<PixelYcbcr>>> blocks);
void cudaUnDPCM(std::vector<std::vector<std::shared_ptr<PixelYcbcr>>> blocks);

//cuda rle.h
std::shared_ptr<EncodedBlock> cudaRLE( std::vector<std::shared_ptr<PixelYcbcr>> block, int block_size);
std::vector<std::shared_ptr<PixelYcbcr>> cudaDecodeRLE( std::shared_ptr<EncodedBlock> encoded, int block_size);
