#include <stdio.h>

#include <cuda.h>
#include <cuda_runtime.h>
#include <driver_functions.h>

#include "dct.h"
#include "quantize.h"
#include "dpcm.h"
#include "rle.h"

#ifndef MACROBLOCK_SIZE
#define MACROBLOCK_SIZE 8
#endif

#define CUDA_THREADS_PER_BLOCK 64

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

// Kernels
__global__ void cudaDctKernel(CudaImageBlocks* imageBlocks, int block_size, bool all, CudaImageBlocks* device_result);

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
// returns vector of blocks of pointers
void cudaDCT(CudaImageBlocks* imageBlocks, int block_size, bool all, CudaImageBlocks* device_result) {
    const int threadsPerBlock = CUDA_THREADS_PER_BLOCK;
    const int blocks = imageBlocks->numBlocks;
    cudaDctKernel<<<blocks, threadsPerBlock>>>(imageBlocks, block_size, all, device_result);
}

__global__ void cudaDctKernel(CudaImageBlocks* imageBlocks, int block_size, bool all, CudaImageBlocks* device_result) {
    PixelYcbcr** pixels = imageBlocks->blocks[blockIdx.x];
    PixelYcbcr** F = device_result->blocks[blockIdx.x];

    // Output: F(p, q)
    for (int p = 0; p < block_size; p++) {
        for (int q = 0; q < block_size; q++) {
            int vectorized_idx_pq = sub2ind(block_size, q, p);
            double ap = (p > 0) ? (sqrt(2/block_size)): (1/sqrt(block_size));
            double aq = (q > 0) ? (sqrt(2/block_size)) : (1/sqrt(block_size));

            double tmp_y, tmp_cb, tmp_cr;
            tmp_y = 0;
            tmp_cb = 0;
            tmp_cr = 0;

            // Input: f(m, n)
            for (int m = 0; m < block_size; m++) { // row
                for (int n = 0; n < block_size; n++) { // cow
                    int vectorized_idx_mn = sub2ind(block_size, n, m);
                    PixelYcbcr* f_mn = pixels[vectorized_idx_mn];

                    double xprod = cos((2*m + 1)*p*M_PI/(2*block_size));
                    double yprod = cos((2*n + 1)*q*M_PI/(2*block_size));

                    tmp_y += ((f_mn->y) * xprod * yprod);
                    if (all) {
                        tmp_cr += ((f_mn->cr) * xprod * yprod);
                        tmp_cb += ((f_mn->cb) * xprod * yprod);
                    }
                }
            }

            //F[vectorized_idx_pq] = std::make_shared<PixelYcbcr>();
            F[vectorized_idx_pq]->y = ap * aq * tmp_y;
            if (all) {
                F[vectorized_idx_pq]->cr = ap * aq * tmp_cr;
                F[vectorized_idx_pq]->cb = ap * aq * tmp_cb;
            } else {
                F[vectorized_idx_pq]->cr = 0;
                F[vectorized_idx_pq]->cb = 0;
            }
        }
    }
}

// returns vector of blocks of pointers
void cudaIDCT(CudaImageBlocks* imageBlocks, int block_size, bool all, CudaImageBlocks* device_result) {

}

//cuda quantize.h
void cudaQuantize(CudaImageBlocks* imageBlocks, int block_size, bool all, CudaImageBlocks* device_result) {

}
void cudaUnquantize(CudaImageBlocks* imageBlocks, int block_size, bool all, CudaImageBlocks* device_result) {

}

//cuda dpcm.h
void cudaDPCM(CudaImageBlocks* imageBlocks) {
    return;
}
void cudaUnDPCM(CudaImageBlocks* imageBlocks) {
    return;
}

//cuda rle.h
// block is an array of pointers
void cudaRLE(CudaImageBlocks* block, int block_size, EncodedBlock** device_result) {

}
// returns an array of pointers
void cudaDecodeRLE(EncodedBlock** encoded, int block_size, CudaImageBlocks* device_result) {

}

unsigned char* encodeParCuda(unsigned char* bytes, unsigned int width, unsigned int height) {

    //the pixels are now in the vector "image", 4 bytes per pixel, ordered RGBARGBA..., use it as texture, draw it, ...
    fprintf(stdout, "convertBytesToImage()...\n");
    CudaImageRgb* imageRgb = cudaConvertBytesToImage((unsigned char*) NULL, width, height);

    fprintf(stdout, "convertRgbToYcbcr()...\n");
    CudaImageYcbcr* imageYcbcr = cudaConvertRgbToYcbcr(imageRgb);

    fprintf(stdout, "convertYcbcrToBlocks()...\n");
    CudaImageBlocks* imageBlocks = cudaConvertYcbcrToBlocks(imageYcbcr, MACROBLOCK_SIZE);
    width = imageBlocks->width;
    height = imageBlocks->height;

    fprintf(stdout, "DCT()...\n");
    CudaImageBlocks* dcts = new CudaImageBlocks();
    dcts->width = imageBlocks->width;
    dcts->height = imageBlocks->height;
    cudaCheckError(cudaMalloc(&dcts, imageBlocks->numBlocks));
    cudaDCT(imageBlocks, MACROBLOCK_SIZE, true, dcts);

    fprintf(stdout, "quantize()...\n");
    CudaImageBlocks* quantizedBlocks = new CudaImageBlocks();
    quantizedBlocks->width = imageBlocks->width;
    quantizedBlocks->height = imageBlocks->height;
    cudaCheckError(cudaMalloc(&quantizedBlocks, imageBlocks->numBlocks));
    cudaQuantize(dcts, MACROBLOCK_SIZE, true, quantizedBlocks);

    fprintf(stdout, "DPCM()...\n");
    cudaDPCM(quantizedBlocks);

    fprintf(stdout, "RLE()...\n");
    // array of pointers
    EncodedBlock** encodedBlocks;
    cudaCheckError(cudaMalloc(&encodedBlocks, imageBlocks->numBlocks));

    //TODO copy back to host

    fprintf(stdout, "warning: jpeg not being written, copy back to host\n");
    /*
    fprintf(stdout, "done encoding!\n");
    fprintf(stdout, "writing to file...\n");
    std::ofstream jpegFile(compressedFile);
    // TODO make this work
    for (int i = 0; i < imageBlocks->numBlocks; i++) {
        jpegFile << encodedBlocks[i];
    }
    fprintf(stdout, "jpeg stored!\n");
    */
    fprintf(stdout, "==============\n");
    fprintf(stdout, "now let's undo the process...\n");
    //TODO copy back to device

    fprintf(stdout, "undoing RLE()...\n");
    // array of array of pointers
    CudaImageBlocks* decodedQuantizedBlocks;
    cudaCheckError(cudaMalloc(&decodedQuantizedBlocks, imageBlocks->numBlocks));
    cudaDecodeRLE(encodedBlocks, MACROBLOCK_SIZE, decodedQuantizedBlocks);

    fprintf(stdout, "undoing DPCM()...\n");
    cudaUnDPCM(decodedQuantizedBlocks);

    fprintf(stdout, "undoing quantize()...\n");
    // array of array of pointers
    PixelYcbcr*** unquantizedBlocks;
    unquantizedBlocks = (PixelYcbcr***) calloc(imageBlocks->numBlocks, sizeof(PixelYcbcr**));
    for (int i = 0; i < imageBlocks->numBlocks; i++) {
        unquantizedBlocks[i] = cudaUnquantize(decodedQuantizedBlocks[i], MACROBLOCK_SIZE, true);
    }

    fprintf(stdout, "undoing DCT()...\n");
    // array of array of pointers
    PixelYcbcr*** idcts;
    idcts = (PixelYcbcr***) calloc(imageBlocks->numBlocks, sizeof(PixelYcbcr**));
    for (int i = 0; i < imageBlocks->numBlocks; i++) {
        idcts[i] = cudaIDCT(unquantizedBlocks[i], MACROBLOCK_SIZE, true);
    }

    fprintf(stdout, "undoing convertYcbcrToBlocks()...\n");
    CudaImageBlocks imageBlocksIdct;
    imageBlocksIdct.blocks = idcts;
    imageBlocksIdct.width = width;
    imageBlocksIdct.height = height;
    CudaImageYcbcr* imgFromBlocks = cudaConvertBlocksToYcbcr(&imageBlocksIdct, MACROBLOCK_SIZE);

    fprintf(stdout, "undoing convertRgbToYcbcr()...\n");
    CudaImageRgb* imageRgbRecovered = cudaConvertYcbcrToRgb(imgFromBlocks);

    fprintf(stdout, "undoing convertBytesToImage()...\n");
    // array
    unsigned char* imgRecovered = cudaConvertImageToBytes(imageRgbRecovered);
    //TODO copy back to host
    return imgRecovered;
}
