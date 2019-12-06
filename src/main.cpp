#include <iostream>
#include <fstream>
#include "getopt.h"
#include "stdio.h"
#include "lodepng/lodepng.h"
#include "dct.h"
#include "quantize.h"
#include "dpcm.h"
#include "rle.h"
#include "mpi.h"

#define MACROBLOCK_SIZE 8

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

    // start parallel area
    MPI_Status mpiStatus;
    int numTasks, rank;
    /* MPI_Init(&argc, &argv); */
    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &numTasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    /*
     * Begin setup MPI structs
     */

    // Set up RleTuple datatype
    int rleTupleLen = 1;
    MPI_Datatype MPI_RleTuple, rleTupleTypes[rleTupleLen];
    int rleTupleBlocks[rleTupleLen];
    MPI_Aint rleTupleOffsets[rleTupleLen];
    rleTupleOffsets[0] = 0;
    rleTupleTypes[0] = MPI_CHAR;
    rleTupleBlocks[0] = 2;
    MPI_Type_create_struct(rleTupleLen, rleTupleBlocks, rleTupleOffsets,
        rleTupleTypes, &MPI_RleTuple);
    MPI_Type_commit(&MPI_RleTuple);

    // Set up EncodedBlockColor
    // Change to data structure: convert std::vector to {char size, [RleTuple]}
    // Change to data stucture: convert std::map to
    //      {char num_entries, [char], [double]}
    // New struct:
    //      - double dc_val
    //      - char encoded_len
    //      - rleTupleVector encoded
    //      - char table_size
    //      - charVector char_vals
    //      - doubleVector double_vals

    // 1. Set up encoded[RleTuple]
    int rleTupleVectorLen = 1;
    MPI_Datatype MPI_RleTupleVector, rleTupleVectorTypes[rleTupleVectorLen];
    int rleTupleVectorBlocks[rleTupleVectorLen];
    MPI_Aint rleTupleVectorOffsets[rleTupleVectorLen];
    rleTupleVectorOffsets[0] = 0;
    rleTupleVectorTypes[0] = MPI_RleTuple;
    rleTupleVectorBlocks[0] = MACROBLOCK_SIZE * MACROBLOCK_SIZE;
    MPI_Type_create_struct(rleTupleVectorLen, rleTupleVectorBlocks,
        rleTupleVectorOffsets, rleTupleVectorTypes, &MPI_RleTupleVector);
    MPI_Type_commit(&MPI_RleTupleVector);

    // 2. Set up vector for map: char values
    int charVectorLen = 1;
    MPI_Datatype MPI_CharVector, charVectorTypes[charVectorLen];
    int charVectorBlocks[charVectorLen];
    MPI_Aint charVectorOffsets[charVectorLen];
    charVectorOffsets[0] = 0;
    charVectorTypes[0] = MPI_CHAR;
    charVectorBlocks[0] = MACROBLOCK_SIZE * MACROBLOCK_SIZE;
    MPI_Type_create_struct(charVectorLen, charVectorBlocks,
        charVectorOffsets, charVectorTypes, &MPI_CharVector);
    MPI_Type_commit(&MPI_CharVector);

    // 3. Set up vector for map: double values
    int doubleVectorLen = 1;
    MPI_Datatype MPI_DoubleVector, doubleVectorTypes[doubleVectorLen];
    int doubleVectorBlocks[doubleVectorLen];
    MPI_Aint doubleVectorOffsets[doubleVectorLen];
    doubleVectorOffsets[0] = 0;
    doubleVectorTypes[0] = MPI_DOUBLE;
    doubleVectorBlocks[0] = MACROBLOCK_SIZE * MACROBLOCK_SIZE;
    MPI_Type_create_struct(doubleVectorLen, doubleVectorBlocks,
        doubleVectorOffsets, doubleVectorTypes, &MPI_DoubleVector);
    MPI_Type_commit(&MPI_DoubleVector);

    // 4. Set up structure for EncodedBlockColor
    int encodedBlockColorLen = 6;
    MPI_Datatype MPI_EncodedBlockColor, encodedBlockColorTypes[encodedBlockColorLen];
    int encodedBlockColorBlocks[encodedBlockColorLen];
    MPI_Aint encodedBlockColorOffsets[encodedBlockColorLen], lb, extent;
    // double dc_val
    encodedBlockColorOffsets[0] = 0;
    encodedBlockColorTypes[0] = MPI_DOUBLE;
    encodedBlockColorBlocks[0] = 1;
    // char encoded_len
    MPI_Type_get_extent(MPI_DOUBLE, &lb, &extent);
    encodedBlockColorOffsets[1] = encodedBlockColorOffsets[0] + extent;
    encodedBlockColorTypes[1] = MPI_CHAR;
    encodedBlockColorBlocks[1] = 1;
    // rleTupleVector encoded
    MPI_Type_get_extent(MPI_CHAR, &lb, &extent);
    encodedBlockColorOffsets[2] = encodedBlockColorOffsets[1] + extent;
    encodedBlockColorTypes[2] = MPI_RleTupleVector;
    encodedBlockColorBlocks[2] = 1;
    // char table_size
    MPI_Type_get_extent(MPI_RleTupleVector, &lb, &extent);
    encodedBlockColorOffsets[3] = encodedBlockColorOffsets[2] + extent;
    encodedBlockColorTypes[3] = MPI_CHAR;
    encodedBlockColorBlocks[3] = 1;
    // charVector char_vals
    MPI_Type_get_extent(MPI_CHAR, &lb, &extent);
    encodedBlockColorOffsets[4] = encodedBlockColorOffsets[3] + extent;
    encodedBlockColorTypes[4] = MPI_CharVector;
    encodedBlockColorBlocks[4] = 1;
    // doubleVector double_vals
    MPI_Type_get_extent(MPI_CharVector, &lb, &extent);
    encodedBlockColorOffsets[5] = encodedBlockColorOffsets[4] + extent;
    encodedBlockColorTypes[5] = MPI_DoubleVector;
    encodedBlockColorBlocks[5] = 1;
    MPI_Type_create_struct(encodedBlockColorLen, encodedBlockColorBlocks,
        encodedBlockColorOffsets, encodedBlockColorTypes, &MPI_EncodedBlockColor);
    MPI_Type_commit(&MPI_EncodedBlockColor);

    // Set up EncodedBlock
    // Change to struct: change it to array of EncodedBlockColor[3] instead
    //      of y, cr, cb fields
    int encodedBlockLen = 1;
    MPI_Datatype MPI_EncodedBlock, encodedBlockTypes[encodedBlockLen];
    int encodedBlockBlocks[encodedBlockLen];
    MPI_Aint encodedBlockOffsets[encodedBlockLen];
    encodedBlockOffsets[0] = 0;
    encodedBlockTypes[0] = MPI_EncodedBlockColor;
    encodedBlockBlocks[0] = 3;
    MPI_Type_create_struct(encodedBlockLen, encodedBlockBlocks,
        encodedBlockOffsets, encodedBlockTypes, &MPI_EncodedBlock);
    MPI_Type_commit(&MPI_EncodedBlock);

    /*
     * End setup for MPI Structs
     */

    fprintf(stdout, "rank %d out of %d\n", rank, numTasks);

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

    // end parallel area
    MPI_Finalize();

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
        encodePar("raw_images/cookie2.png", "images/cookie2.png", "compressed/cookie2.jpeg");
    } else {
        fprintf(stdout, "running sequential version\n");
        encodeSeq("raw_images/cookie2.png", "images/cookie2.png", "compressed/cookie2.jpeg");
    }

    exit(EXIT_SUCCESS);
}
