#include <iostream>
#include <fstream>
#include "stdio.h"
#include "lodepng/lodepng.h"
#include "dct.h"
#include "quantize.h"
#include "dpcm.h"
#include "rle.h"

#define MACROBLOCK_SIZE 8

void testFnInv(const char* infile, const char* outfile) {
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
    
    fprintf(stdout, "starting encryption\n");

    std::shared_ptr<ImageRgb> imageRgb = convertBytesToImage(bytes, width, height);
    std::shared_ptr<ImageYcbcr> imageYcbcr = convertRgbToYcbcr(imageRgb);
    std::shared_ptr<ImageBlocks> imageBlocks = convertYcbcrToBlocks(imageYcbcr, MACROBLOCK_SIZE);

    fprintf(stdout, "starting decryption\n");

    std::shared_ptr<ImageYcbcr> imgFromBlocks = convertBlocksToYcbcr(imageBlocks, MACROBLOCK_SIZE);
    std::shared_ptr<ImageRgb> imageRgbRecovered = convertYcbcrToRgb(imgFromBlocks);
    bytes = convertImageToBytes(imageRgbRecovered);

    error = lodepng::encode(outfile, bytes, width, height);

    //if there's an error, display it
    if(error) {
        std::cout << "encoder error " << error << ": "<< lodepng_error_text(error) << std::endl;
    } else {
        fprintf(stdout, "success encoding to %s!\n", outfile);
    }
}

void decodeOneStep(const char* infile, const char* outfile, const char* compressedFile) {
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
        decodedQuantizedBlocks.push_back(DecodeRLE(encodedBlock, MACROBLOCK_SIZE));
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

int main() {
    /* decodeOneStep("raw_images/cookie.png", "images/cookie.png", "compressed/cookie.jpeg"); */
    testFnInv("raw_images/cookie.png", "images/cookie.png");
    /* testFnInv("raw_images/cookie_small.png", "images/cookie_small.png"); */
    return 0;
}
