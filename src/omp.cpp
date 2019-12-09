#include <iostream>
#include <fstream>
#include <cstdarg>
#include "CycleTimer.h"
#include "getopt.h"
#include "stdio.h"
#include "lodepng/lodepng.h"
#include "dct.h"
#include "quantize.h"
#include "dpcm.h"
#include "rle.h"
#include <omp.h>

#define MACROBLOCK_SIZE 8

void log(int rank, const char* format, ...) {
    if (rank != 0) {
        return;
    }
    va_list args;
    va_start(args, format);
    fprintf(stdout, format, args);
    va_end(args);
}

std::shared_ptr<JpegEncoded> jpegSeq(const char* infile, const char* outfile, const char* compressedFile) {
    fprintf(stdout, "running sequential version\n");

    double startTime = CycleTimer::currentSeconds();

    std::vector<unsigned char> bytes; //the raw pixels
    unsigned int width, height;

    //decode
    double loadImageStartTime = CycleTimer::currentSeconds();
    unsigned int error = lodepng::decode(bytes, width, height, infile);
    double loadImageStopTime = CycleTimer::currentSeconds();

    //if there's an error, display it
    if(error) {
      std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;
    } else {
      fprintf(stdout, "success decoding %s!\n", infile);
    }

    //the pixels are now in the vector "image", 4 bytes per pixel, ordered RGBARGBA..., use it as texture, draw it, ...
    fprintf(stdout, "convertBytesToImage()...\n");
    double convertBytesToImageStartTime = CycleTimer::currentSeconds();
    std::shared_ptr<ImageRgb> imageRgb = convertBytesToImage(bytes, width, height);
    double convertBytesToImageEndTime = CycleTimer::currentSeconds();

    fprintf(stdout, "convertRgbToYcbcr()...\n");
    double convertRgbToYcbcrStartTime = CycleTimer::currentSeconds();
    std::shared_ptr<ImageYcbcr> imageYcbcr = convertRgbToYcbcr(imageRgb);
    double convertRgbToYcbcrEndTime = CycleTimer::currentSeconds();

    fprintf(stdout, "convertYcbcrToBlocks()...\n");
    double convertYcbcrToBlocksStartTime = CycleTimer::currentSeconds();
    std::shared_ptr<ImageBlocks> imageBlocks = convertYcbcrToBlocks(imageYcbcr, MACROBLOCK_SIZE);
    width = imageBlocks->width;
    height = imageBlocks->height;
    double convertYcbcrToBlocksEndTime = CycleTimer::currentSeconds();

    fprintf(stdout, "DCT()...\n");
    double dctStartTime = CycleTimer::currentSeconds();
    std::vector<std::vector<std::shared_ptr<PixelYcbcr>>> dcts;
    for (auto block : imageBlocks->blocks) {
        dcts.push_back(DCT(block, MACROBLOCK_SIZE, true));
    }
    double dctEndTime = CycleTimer::currentSeconds();

    fprintf(stdout, "quantize()...\n");
    double quantizeStartTime = CycleTimer::currentSeconds();
    std::vector<std::vector<std::shared_ptr<PixelYcbcr>>> quantizedBlocks;
    for (auto dct : dcts) {
        quantizedBlocks.push_back(quantize(dct, MACROBLOCK_SIZE, true));
    }
    double quantizeEndTime = CycleTimer::currentSeconds();

    fprintf(stdout, "DPCM()...\n");
    double dpcmStartTime = CycleTimer::currentSeconds();
    DPCM(quantizedBlocks);
    double dpcmEndTime = CycleTimer::currentSeconds();

    fprintf(stdout, "RLE()...\n");
    double rleStartTime = CycleTimer::currentSeconds();
    std::vector<std::shared_ptr<EncodedBlock>> encodedBlocks;
    for (auto quantizedBlock : quantizedBlocks) {
        encodedBlocks.push_back(RLE(quantizedBlock, MACROBLOCK_SIZE));
    }
    double rleEndTime = CycleTimer::currentSeconds();

    fprintf(stdout, "done encoding!\n");
    fprintf(stdout, "writing to file...\n");
    double writeCompressedImageStartTime = CycleTimer::currentSeconds();
    std::ofstream jpegFile(compressedFile);
    for (const auto &block : encodedBlocks) {
        jpegFile << block;
    }
    double writeCompressedImageEndTime = CycleTimer::currentSeconds();
    fprintf(stdout, "jpeg stored!\n");

    std::shared_ptr<JpegEncoded> result = std::make_shared<JpegEncoded>();
    result->encodedBlocks = encodedBlocks;
    result->width = width;
    result->height = height;

    double endTime = CycleTimer::currentSeconds();

    fprintf(stdout,
    "=======================================\n"
    "= Sequential encoding performance: \n"
    "=======================================\n"
    "Load Image: %.3fs\n"
    "Convert Bytes to Image: %.3fs\n"
    "Convert RGB to YCbCr: %.3fs\n"
    "Convert YCbCr to Blocks: %.3fs\n"
    "DCT: %.3fs\n"
    "Quantize: %.3fs\n"
    "DPCM: %.3fs\n"
    "RLE: %.3fs\n"
    "Encode Compressed Image: %.3fs\n"
    "Total time: %.3fs\n",
    loadImageStopTime - loadImageStartTime,
    convertBytesToImageEndTime - convertBytesToImageStartTime,
    convertRgbToYcbcrEndTime - convertRgbToYcbcrStartTime,
    convertYcbcrToBlocksEndTime - convertYcbcrToBlocksStartTime,
    dctEndTime - dctStartTime,
    quantizeEndTime - quantizeStartTime,
    dpcmEndTime - dpcmStartTime,
    rleEndTime - rleStartTime,
    writeCompressedImageEndTime - writeCompressedImageStartTime,
    endTime - startTime);

    return result;
}

std::vector<unsigned char> jpegDecodeSeq(std::shared_ptr<JpegEncoded> jpegEncoded, const char* outfile) {

    unsigned int width = jpegEncoded->width;
    unsigned int height = jpegEncoded->height;
    std::vector<std::shared_ptr<EncodedBlock>> encodedBlocks = jpegEncoded->encodedBlocks;

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

    unsigned int error = lodepng::encode(outfile, imgRecovered, width, height);

    //if there's an error, display it
    if(error) {
        std::cout << "encoder error " << error << ": "<< lodepng_error_text(error) << std::endl;
    } else {
        fprintf(stdout, "success encoding to %s!\n", outfile);
    }

    return imgRecovered;
}

void encodeSeq(const char* infile, const char* outfile, const char* compressedFile) {

    std::shared_ptr<JpegEncoded> jpegEncoded = jpegSeq(infile, outfile, compressedFile);
    std::vector<std::shared_ptr<EncodedBlock>> encodedBlocks = jpegEncoded->encodedBlocks;
    std::vector<unsigned char> imgRecovered = jpegDecodeSeq(jpegEncoded, outfile);

}

std::shared_ptr<JpegEncoded> jpegPar(const char* infile, const char* outfile, const char* compressedFile) {

    fprintf(stdout, "running OMP version\n");

    double startTime = CycleTimer::currentSeconds();

    std::vector<unsigned char> bytes; //the raw pixels
    unsigned int width, height;

    //decode
    double loadImageStartTime = CycleTimer::currentSeconds();
    unsigned int error = lodepng::decode(bytes, width, height, infile);
    double loadImageStopTime = CycleTimer::currentSeconds();

    //if there's an error, display it
    if(error) {
      std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;
    } else {
      fprintf(stdout, "success decoding %s!\n", infile);
    }

    //the pixels are now in the vector "image", 4 bytes per pixel, ordered RGBARGBA..., use it as texture, draw it, ...
    fprintf(stdout, "convertBytesToImage()...\n");
    double convertBytesToImageStartTime = CycleTimer::currentSeconds();
    std::shared_ptr<ImageRgb> imageRgb = convertBytesToImage(bytes, width, height);
    double convertBytesToImageEndTime = CycleTimer::currentSeconds();

    fprintf(stdout, "convertRgbToYcbcr()...\n");
    double convertRgbToYcbcrStartTime = CycleTimer::currentSeconds();
    std::shared_ptr<ImageYcbcr> imageYcbcr = convertRgbToYcbcr(imageRgb);
    double convertRgbToYcbcrEndTime = CycleTimer::currentSeconds();

    fprintf(stdout, "convertYcbcrToBlocks()...\n");
    double convertYcbcrToBlocksStartTime = CycleTimer::currentSeconds();
    std::shared_ptr<ImageBlocks> imageBlocks = convertYcbcrToBlocks(imageYcbcr, MACROBLOCK_SIZE);
    width = imageBlocks->width;
    height = imageBlocks->height;
    double convertYcbcrToBlocksEndTime = CycleTimer::currentSeconds();

    fprintf(stdout, "DCT()...\n");
    double dctStartTime = CycleTimer::currentSeconds();
    std::vector<std::vector<std::shared_ptr<PixelYcbcr>>> dcts(imageBlocks->blocks.size());
    #pragma omp parallel for
    for (unsigned int i = 0; i < imageBlocks->blocks.size(); i++) {
        auto block = imageBlocks->blocks[i];
        dcts[i] = DCT(block, MACROBLOCK_SIZE, true);
    }
    double dctEndTime = CycleTimer::currentSeconds();

    fprintf(stdout, "quantize()...\n");
    double quantizeStartTime = CycleTimer::currentSeconds();
    std::vector<std::vector<std::shared_ptr<PixelYcbcr>>> quantizedBlocks(dcts.size());
    #pragma omp parallel for
    for (unsigned int i = 0; i < dcts.size(); i++) {
        auto dct = dcts[i];
        quantizedBlocks[i] = quantize(dct, MACROBLOCK_SIZE, true);
    }
    double quantizeEndTime = CycleTimer::currentSeconds();

    fprintf(stdout, "DPCM()...\n");
    double dpcmStartTime = CycleTimer::currentSeconds();
    DPCM(quantizedBlocks);
    double dpcmEndTime = CycleTimer::currentSeconds();

    fprintf(stdout, "RLE()...\n");
    double rleStartTime = CycleTimer::currentSeconds();
    std::vector<std::shared_ptr<EncodedBlock>> encodedBlocks(quantizedBlocks.size());
    #pragma omp parallel for
    for (unsigned int i = 0; i < quantizedBlocks.size(); i++) {
        auto quantizedBlock = quantizedBlocks[i];
        encodedBlocks[i] = RLE(quantizedBlock, MACROBLOCK_SIZE);
    }
    double rleEndTime = CycleTimer::currentSeconds();

    fprintf(stdout, "done encoding!\n");
    fprintf(stdout, "writing to file...\n");
    double writeCompressedImageStartTime = CycleTimer::currentSeconds();
    std::ofstream jpegFile(compressedFile);
    for (const auto &block : encodedBlocks) {
        jpegFile << block;
    }
    double writeCompressedImageEndTime = CycleTimer::currentSeconds();
    fprintf(stdout, "jpeg stored!\n");

    std::shared_ptr<JpegEncoded> result = std::make_shared<JpegEncoded>();
    result->encodedBlocks = encodedBlocks;
    result->width = width;
    result->height = height;

    double endTime = CycleTimer::currentSeconds();

    fprintf(stdout,
    "=======================================\n"
    "= OMP encoding performance: \n"
    "=======================================\n"
    "Load Image: %.3fs\n"
    "Convert Bytes to Image: %.3fs\n"
    "Convert RGB to YCbCr: %.3fs\n"
    "Convert YCbCr to Blocks: %.3fs\n"
    "DCT: %.3fs\n"
    "Quantize: %.3fs\n"
    "DPCM: %.3fs\n"
    "RLE: %.3fs\n"
    "Encode Compressed Image: %.3fs\n"
    "Total time: %.3fs\n",
    loadImageStopTime - loadImageStartTime,
    convertBytesToImageEndTime - convertBytesToImageStartTime,
    convertRgbToYcbcrEndTime - convertRgbToYcbcrStartTime,
    convertYcbcrToBlocksEndTime - convertYcbcrToBlocksStartTime,
    dctEndTime - dctStartTime,
    quantizeEndTime - quantizeStartTime,
    dpcmEndTime - dpcmStartTime,
    rleEndTime - rleStartTime,
    writeCompressedImageEndTime - writeCompressedImageStartTime,
    endTime - startTime);

    return result;
}

void encodeOmp(const char* infile, const char* outfile, const char* compressedFile) {
    std::shared_ptr<JpegEncoded> jpegEncoded = jpegPar(infile, outfile, compressedFile);
    std::vector<std::shared_ptr<EncodedBlock>> encodedBlocks = jpegEncoded->encodedBlocks;
    std::vector<unsigned char> imgRecovered = jpegDecodeSeq(jpegEncoded, outfile);
}

int main(int argc, char** argv) {
    int opt;
    int omp = 0;
    while ((opt = getopt(argc, argv, ":o")) != -1) {
        switch (opt) {
            case 'o':
                omp = 1;
                break;
            default:
                fprintf(stderr, "Usage: %s [o]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (omp) {
        encodeOmp("raw_images/cookie.png", "images/cookie.png", "compressed/cookie.jpeg");
    } else {
        encodeSeq("raw_images/cookie.png", "images/cookie.png", "compressed/cookie.jpeg");
    }

    exit(EXIT_SUCCESS);
}