#include <iostream>
#include <fstream>
#include "getopt.h"
#include "stdio.h"
#include "lodepng/lodepng.h"
#include "dct.h"
#include "quantize.h"
#include "dpcm.h"
#include "rle.h"

#define MACROBLOCK_SIZE 8

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

void encodeSeq(const char* infile, const char* outfile, const char* compressedFile) {
    std::vector<unsigned char> bytes; //the raw pixels
    unsigned int width, height;

    //decode
    unsigned int error = lodepng::decode(bytes, width, height, infile);

    //if there's an error, display it
    if(error) {
      std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;
    } else {
      fprintf(stdout, "success decoding %s!\n", infile);
    }

    //the pixels are now in the vector "image", 4 bytes per pixel, ordered RGBARGBA..., use it as texture, draw it, ...
    fprintf(stdout, "convertBytesToImage()...\n");
    std::shared_ptr<ImageRgb> imageRgb = convertBytesToImage(bytes, width, height);

    fprintf(stdout, "convertRgbToYcbcr()...\n");
    std::shared_ptr<ImageYcbcr> imageYcbcr = convertRgbToYcbcr(imageRgb);

    fprintf(stdout, "convertYcbcrToBlocks()...\n");
    std::shared_ptr<ImageBlocks> imageBlocks = convertYcbcrToBlocks(imageYcbcr, MACROBLOCK_SIZE);
    width = imageBlocks->width;
    height = imageBlocks->height;

    fprintf(stdout, "DCT()...\n");
    std::vector<std::vector<std::shared_ptr<PixelYcbcr>>> dcts;
    for (auto block : imageBlocks->blocks) {
        dcts.push_back(DCT(block, MACROBLOCK_SIZE, true));
    }

    fprintf(stdout, "quantize()...\n");
    std::vector<std::vector<std::shared_ptr<PixelYcbcr>>> quantizedBlocks;
    for (auto dct : dcts) {
        quantizedBlocks.push_back(quantize(dct, MACROBLOCK_SIZE, true));
    }

    fprintf(stdout, "DPCM()...\n");
    DPCM(quantizedBlocks);

    fprintf(stdout, "RLE()...\n");
    std::vector<std::shared_ptr<EncodedBlock>> encodedBlocks;
    for (auto quantizedBlock : quantizedBlocks) {
        encodedBlocks.push_back(RLE(quantizedBlock, MACROBLOCK_SIZE));
    }

    fprintf(stdout, "done encoding!\n");
    fprintf(stdout, "writing to file...\n");
    std::ofstream jpegFile(compressedFile);
    for (const auto &block : encodedBlocks) {
        jpegFile << block;
    }
    fprintf(stdout, "jpeg stored!\n");
    fprintf(stdout, "==============\n");
    fprintf(stdout, "now let's undo the process...\n");

    fprintf(stdout, "undoing RLE()...\n");
    std::vector<std::vector<std::shared_ptr<PixelYcbcr>>> decodedQuantizedBlocks;
    for (auto encodedBlock : encodedBlocks) {
        decodedQuantizedBlocks.push_back(decodeRLE(encodedBlock, MACROBLOCK_SIZE));
    }

    fprintf(stdout, "undoing DPCM()...\n");
    unDPCM(decodedQuantizedBlocks);

    fprintf(stdout, "undoing quantize()...\n");
    std::vector<std::vector<std::shared_ptr<PixelYcbcr>>> unquantizedBlocks;
    for (auto decodedQuantizedBlock : decodedQuantizedBlocks) {
        unquantizedBlocks.push_back(unquantize(decodedQuantizedBlock, MACROBLOCK_SIZE, true));
    }

    fprintf(stdout, "undoing DCT()...\n");
    std::vector<std::vector<std::shared_ptr<PixelYcbcr>>> idcts;
    for (auto unquantized : unquantizedBlocks) {
        idcts.push_back(IDCT(unquantized, MACROBLOCK_SIZE, true));
    }

    fprintf(stdout, "undoing convertYcbcrToBlocks()...\n");
    std::shared_ptr<ImageBlocks> imageBlocksIdct(new ImageBlocks);
    imageBlocksIdct->blocks = idcts;
    imageBlocksIdct->width = width;
    imageBlocksIdct->height = height;
    std::shared_ptr<ImageYcbcr> imgFromBlocks = convertBlocksToYcbcr(imageBlocksIdct, MACROBLOCK_SIZE);

    fprintf(stdout, "undoing convertRgbToYcbcr()...\n");
    std::shared_ptr<ImageRgb> imageRgbRecovered = convertYcbcrToRgb(imgFromBlocks);

    fprintf(stdout, "undoing convertBytesToImage()...\n");
    std::vector<unsigned char> imgRecovered = convertImageToBytes(imageRgbRecovered);

    error = lodepng::encode(outfile, imgRecovered, width, height);

    //if there's an error, display it
    if(error) {
        std::cout << "encoder error " << error << ": "<< lodepng_error_text(error) << std::endl;
    } else {
        fprintf(stdout, "success encoding to %s!\n", outfile);
    }
}

void encodePar(const char* infile, const char* outfile, const char* compressedFile) {
    // TODO change vectors to array or thrust
    std::vector<unsigned char> bytes; //the raw pixels
    unsigned int width, height;

    //decode
    unsigned int error = lodepng::decode(bytes, width, height, infile);

    //if there's an error, display it
    if(error) {
      std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;
    } else {
      fprintf(stdout, "success decoding %s!\n", infile);
    }

    //TODO copy to device

    //the pixels are now in the vector "image", 4 bytes per pixel, ordered RGBARGBA..., use it as texture, draw it, ...
    fprintf(stdout, "convertBytesToImage()...\n");
    std::shared_ptr<ImageRgb> imageRgb = cudaConvertBytesToImage(bytes, width, height);

    fprintf(stdout, "convertRgbToYcbcr()...\n");
    std::shared_ptr<ImageYcbcr> imageYcbcr = cudaConvertRgbToYcbcr(imageRgb);

    fprintf(stdout, "convertYcbcrToBlocks()...\n");
    std::shared_ptr<ImageBlocks> imageBlocks = cudaConvertYcbcrToBlocks(imageYcbcr, MACROBLOCK_SIZE);
    width = imageBlocks->width;
    height = imageBlocks->height;

    fprintf(stdout, "DCT()...\n");
    std::vector<std::vector<std::shared_ptr<PixelYcbcr>>> dcts;
    for (auto block : imageBlocks->blocks) {
        dcts.push_back(cudaDCT(block, MACROBLOCK_SIZE, true));
    }

    fprintf(stdout, "quantize()...\n");
    std::vector<std::vector<std::shared_ptr<PixelYcbcr>>> quantizedBlocks;
    for (auto dct : dcts) {
        quantizedBlocks.push_back(cudaQuantize(dct, MACROBLOCK_SIZE, true));
    }

    fprintf(stdout, "DPCM()...\n");
    cudaDPCM(quantizedBlocks);

    fprintf(stdout, "RLE()...\n");
    std::vector<std::shared_ptr<EncodedBlock>> encodedBlocks;
    for (auto quantizedBlock : quantizedBlocks) {
        encodedBlocks.push_back(cudaRLE(quantizedBlock, MACROBLOCK_SIZE));
    }
    //TODO copy back to host

    fprintf(stdout, "done encoding!\n");
    fprintf(stdout, "writing to file...\n");
    std::ofstream jpegFile(compressedFile);
    for (const auto &block : encodedBlocks) {
        jpegFile << block;
    }
    fprintf(stdout, "jpeg stored!\n");
    fprintf(stdout, "==============\n");
    fprintf(stdout, "now let's undo the process...\n");
    //TODO copy back to device

    fprintf(stdout, "undoing RLE()...\n");
    std::vector<std::vector<std::shared_ptr<PixelYcbcr>>> decodedQuantizedBlocks;
    for (auto encodedBlock : encodedBlocks) {
        decodedQuantizedBlocks.push_back(cudaDecodeRLE(encodedBlock, MACROBLOCK_SIZE));
    }

    fprintf(stdout, "undoing DPCM()...\n");
    cudaUnDPCM(decodedQuantizedBlocks);

    fprintf(stdout, "undoing quantize()...\n");
    std::vector<std::vector<std::shared_ptr<PixelYcbcr>>> unquantizedBlocks;
    for (auto decodedQuantizedBlock : decodedQuantizedBlocks) {
        unquantizedBlocks.push_back(cudaUnquantize(decodedQuantizedBlock, MACROBLOCK_SIZE, true));
    }

    fprintf(stdout, "undoing DCT()...\n");
    std::vector<std::vector<std::shared_ptr<PixelYcbcr>>> idcts;
    for (auto unquantized : unquantizedBlocks) {
        idcts.push_back(cudaIDCT(unquantized, MACROBLOCK_SIZE, true));
    }

    fprintf(stdout, "undoing convertYcbcrToBlocks()...\n");
    std::shared_ptr<ImageBlocks> imageBlocksIdct(new ImageBlocks);
    imageBlocksIdct->blocks = idcts;
    imageBlocksIdct->width = width;
    imageBlocksIdct->height = height;
    std::shared_ptr<ImageYcbcr> imgFromBlocks = cudaConvertBlocksToYcbcr(imageBlocksIdct, MACROBLOCK_SIZE);

    fprintf(stdout, "undoing convertRgbToYcbcr()...\n");
    std::shared_ptr<ImageRgb> imageRgbRecovered = cudaConvertYcbcrToRgb(imgFromBlocks);

    fprintf(stdout, "undoing convertBytesToImage()...\n");
    std::vector<unsigned char> imgRecovered = cudaConvertImageToBytes(imageRgbRecovered);

    //TODO copy back to host

    error = lodepng::encode(outfile, imgRecovered, width, height);

    //if there's an error, display it
    if(error) {
        std::cout << "encoder error " << error << ": "<< lodepng_error_text(error) << std::endl;
    } else {
        fprintf(stdout, "success encoding to %s!\n", outfile);
    }
}

int main(int argc, char** argv) {
    int opt;
    int parallel = 0;
    while ((opt = getopt(argc, argv, ":p")) != -1) {
        switch (opt) {
            case 'p':
                parallel = 1;
                break;
            default:
                fprintf(stderr, "Usage: ./compress [p]\n");
                exit(EXIT_FAILURE);
        }
    }

    if (parallel) {
        fprintf(stdout, "running parallel version\n");
    } else {
        fprintf(stdout, "running sequential version\n");
        encodeSeq("raw_images/cookie2.png", "images/cookie2.png", "compressed/cookie2.jpeg");
    }

    exit(EXIT_SUCCESS);
}
