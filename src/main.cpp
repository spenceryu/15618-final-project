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

std::shared_ptr<JpegEncoded> jpegSeq(const char* infile, const char* outfile, const char* compressedFile) {

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

    std::shared_ptr<JpegEncoded> result = std::make_shared<JpegEncoded>();
    result->encodedBlocks = encodedBlocks;
    result->width = width;
    result->height = height;

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

    // start parallel area
    MPI_Status mpiStatus;
    int numTasks, rank;
    /* MPI_Init(&argc, &argv); */
    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &numTasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Begin setup MPI structs

    // set up mpi pixelycbcr
    // float y, cb, cr
    MPI_Datatype MPI_PixelYcbcr;
    MPI_Type_contiguous(3, MPI_DOUBLE, &MPI_PixelYcbcr);
    MPI_Type_commit(&MPI_PixelYcbcr);

    // set up mpi imageycbr
    // int numPixels, width, height
    MPI_Datatype MPI_ImageYcbcr;
    MPI_Type_contiguous(3, MPI_INT, &MPI_ImageYcbcr);
    MPI_Type_commit(&MPI_ImageYcbcr);

    // Set up RleTuple datatype
    MPI_Datatype MPI_RleTuple;
    MPI_Type_contiguous(2, MPI_CHAR, &MPI_RleTuple);
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
    MPI_Datatype MPI_RleTupleVector;
    MPI_Type_contiguous(MACROBLOCK_SIZE * MACROBLOCK_SIZE, MPI_RleTuple, &MPI_RleTupleVector);
    MPI_Type_commit(&MPI_RleTupleVector);

    // 2. Set up vector for map: char values
    MPI_Datatype MPI_CharVector;
    MPI_Type_contiguous(MACROBLOCK_SIZE * MACROBLOCK_SIZE, MPI_CHAR, &MPI_CharVector);
    MPI_Type_commit(&MPI_CharVector);

    // 3. Set up vector for map: double values
    MPI_Datatype MPI_DoubleVector;
    MPI_Type_contiguous(MACROBLOCK_SIZE * MACROBLOCK_SIZE, MPI_DOUBLE, &MPI_DoubleVector);
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
    MPI_Datatype MPI_EncodedBlock;
    MPI_Type_contiguous(3, MPI_EncodedBlockColor, &MPI_EncodedBlock);
    MPI_Type_commit(&MPI_EncodedBlock);

    // End setup for MPI Structs

    // SCATTER
    // give each thread a section of bytes to convert to an image
    fprintf(stdout, "convertBytesToImage()...\n");
    std::shared_ptr<ImageRgb> imageRgb = convertBytesToImage(bytes, width, height);

    // each thread will continue and convert its image to ycbcr
    fprintf(stdout, "convertRgbToYcbcr()...\n");
    std::shared_ptr<ImageYcbcr> imageYcbcr = convertRgbToYcbcr(imageRgb);

    // GATHER
    // collect all images into the full image in master
    fprintf(stdout, "convertYcbcrToBlocks()...\n");
    std::shared_ptr<ImageBlocks> imageBlocks = convertYcbcrToBlocks(imageYcbcr, MACROBLOCK_SIZE);
    width = imageBlocks->width;
    height = imageBlocks->height;

    /*
     * SCATTER
     */
    int numPixels;
    int tag = 1;
    std::vector<std::vector<std::shared_ptr<PixelYcbcr>>> imageBlocksWorker;

    if (rank == 0) {
        int numBlocks = (imageBlocks->blocks.size() + numTasks - 1) / numTasks; // round up
        numPixels = numBlocks * MACROBLOCK_SIZE * MACROBLOCK_SIZE;

        PixelYcbcr buffer[numPixels];

        // Iterate through each task instance
        for (int i = 1; i < numTasks; i++) { // do not send pixels to master
            int offset = numTasks * numBlocks;
            int j;
            // Iterate through number of blocks assigned to each task
            int totalNumBlocks = imageBlocks->blocks.size();
            for (j = 0; ((j < numBlocks) && (offset + j < totalNumBlocks)); j++) {
                std::vector<std::shared_ptr<PixelYcbcr>> px = imageBlocks->blocks[offset + j];
                // Copy each pixel into the buffer
                for (unsigned int k = 0; k < px.size(); k++) {
                    buffer[j].y = px[k]->y;
                    buffer[j].cb = px[k]->cb;
                    buffer[j].cr = px[k]->cr;
                }
            }
            // Send the header information (num pixels) to the worker task
            MPI_Send(&numPixels, 1, MPI_INT, j, tag, MPI_COMM_WORLD);
            // Send that number of blocks as a task
            MPI_Send(buffer, numPixels, MPI_PixelYcbcr, j, tag, MPI_COMM_WORLD);
        }

        // Set blocks of pixels for the master thread
        for (unsigned int i = 0; ((i < (unsigned int)numBlocks) &&
            (i < imageBlocks->blocks.size())); i++) {
            std::vector<std::shared_ptr<PixelYcbcr>> px = imageBlocks->blocks[i];
            // Copy each pixel into the buffer
            for (unsigned int k = 0; k < px.size(); k++) {
                buffer[i].y = px[k]->y;
                buffer[i].cb = px[k]->cb;
                buffer[i].cr = px[k]->cr;
            }
        }

        // Convert the buffer back to shared ptrs inside of blocks to continue
        // The blocks in imageBlocksWorker are a subset of the set of blocks
        numBlocks = numPixels / (MACROBLOCK_SIZE * MACROBLOCK_SIZE);
        for (int i = 0; i < numBlocks; i++) {
            int offsetIndex = (i * MACROBLOCK_SIZE * MACROBLOCK_SIZE);
            for (int j = 0; j < (MACROBLOCK_SIZE * MACROBLOCK_SIZE); j++) {
                imageBlocksWorker[i].push_back(
                    std::make_shared<PixelYcbcr>(buffer[offsetIndex + j]
                ));
            }
        }
    } else {
        // Get number of pixels to recv from master
        MPI_Recv(&numPixels, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &mpiStatus);
        PixelYcbcr buffer[numPixels];
        MPI_Recv(buffer, numPixels, MPI_PixelYcbcr, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &mpiStatus);

        // Convert the buffer back to shared ptrs inside of blocks to continue
        // The blocks in imageBlocksWorker are a subset of the set of blocks
        int numBlocks = numPixels / (MACROBLOCK_SIZE * MACROBLOCK_SIZE);
        for (int i = 0; i < numBlocks; i++) {
            int offsetIndex = (i * MACROBLOCK_SIZE * MACROBLOCK_SIZE);
            for (int j = 0; j < (MACROBLOCK_SIZE * MACROBLOCK_SIZE); j++) {
                imageBlocksWorker[i].push_back(
                    std::make_shared<PixelYcbcr>(buffer[offsetIndex + j]
                ));
            }
        }
    }

    /*
     * END SCATTER
     * Purpose: send blocks out to threads
     */

    fprintf(stdout, "DCT()...\n");
    std::vector<std::vector<std::shared_ptr<PixelYcbcr>>> dcts;
    //for (auto block : imageBlocks->blocks) {
    for (auto block : imageBlocksWorker) {
        dcts.push_back(DCT(block, MACROBLOCK_SIZE, true));
    }

    // blocks stay in their threads
    fprintf(stdout, "quantize()...\n");
    std::vector<std::vector<std::shared_ptr<PixelYcbcr>>> quantizedBlocks;
    for (auto dct : dcts) {
        quantizedBlocks.push_back(quantize(dct, MACROBLOCK_SIZE, true));
    }

    // blocks stay in their threads
    fprintf(stdout, "DPCM()...\n");
    DPCM(quantizedBlocks);

    // blocks stay in their threads
    fprintf(stdout, "RLE()...\n");
    std::vector<std::shared_ptr<EncodedBlock>> encodedBlocks;
    for (auto quantizedBlock : quantizedBlocks) {
        encodedBlocks.push_back(RLE(quantizedBlock, MACROBLOCK_SIZE));
    }

    /*
     * BEGIN GATHER
     * Purpose: collect all blocks back into a vector of encoded blocks in master
     */

    // For master thread, collect encoded blocks in order from workers
    int numEncodedBlocks;
    if (rank == 0) {
        for (int i = 1; i < numTasks; i++) {
            // recv the number of encoded blocks
            MPI_Recv(&numEncodedBlocks, 1, MPI_INT, i, MPI_ANY_TAG,
                MPI_COMM_WORLD, &mpiStatus);
            // recv the encoded blocks
            EncodedBlockNoPtr encodedBlocksBuffer[numEncodedBlocks];
            MPI_Recv(encodedBlocksBuffer, numEncodedBlocks, MPI_EncodedBlock, i,
                MPI_ANY_TAG, MPI_COMM_WORLD, &mpiStatus);
            // TODO add the encoded blocks to encodedBlocks vector
        }
    } else {
        // For each worker, send its encoded blocks back to master
        numEncodedBlocks = encodedBlocks.size();
        // send number of encoded blocks
        MPI_Send(&numEncodedBlocks, 1, MPI_INT, 0, tag, MPI_COMM_WORLD);
        EncodedBlockNoPtr encodedBlockBuffer[numEncodedBlocks];
        // build the encoded blocks structure
        for (unsigned int i = 0; i < encodedBlocks.size(); i++) {
            writeToBuffer(encodedBlockBuffer, encodedBlocks, i, COLOR_Y);
            writeToBuffer(encodedBlockBuffer, encodedBlocks, i, COLOR_CR);
            writeToBuffer(encodedBlockBuffer, encodedBlocks, i, COLOR_CB);
        }
        // send the encoded blocks
        MPI_Send(&encodedBlockBuffer, numEncodedBlocks, MPI_EncodedBlock, 0,
            tag, MPI_COMM_WORLD);
    }

    /*
     * END GATHER
     * Purpose: collect all blocks back into a vector of encoded blocks in master
     */

    fprintf(stdout, "done encoding!\n");
    fprintf(stdout, "writing to file...\n");
    std::ofstream jpegFile(compressedFile);
    for (const auto &block : encodedBlocks) {
        jpegFile << block;
    }
    fprintf(stdout, "jpeg stored!\n");
    fprintf(stdout, "==============\n");
    fprintf(stdout, "now let's undo the process...\n");

    // SCATTER
    // send blocks out to threads
    fprintf(stdout, "undoing RLE()...\n");
    std::vector<std::vector<std::shared_ptr<PixelYcbcr>>> decodedQuantizedBlocks;
    for (auto encodedBlock : encodedBlocks) {
        decodedQuantizedBlocks.push_back(decodeRLE(encodedBlock, MACROBLOCK_SIZE));
    }

    // blocks stay in their threads
    fprintf(stdout, "undoing DPCM()...\n");
    unDPCM(decodedQuantizedBlocks);

    // blocks stay in their threads
    fprintf(stdout, "undoing quantize()...\n");
    std::vector<std::vector<std::shared_ptr<PixelYcbcr>>> unquantizedBlocks;
    for (auto decodedQuantizedBlock : decodedQuantizedBlocks) {
        unquantizedBlocks.push_back(unquantize(decodedQuantizedBlock, MACROBLOCK_SIZE, true));
    }

    // blocks stay in their threads
    fprintf(stdout, "undoing DCT()...\n");
    std::vector<std::vector<std::shared_ptr<PixelYcbcr>>> idcts;
    for (auto unquantized : unquantizedBlocks) {
        idcts.push_back(IDCT(unquantized, MACROBLOCK_SIZE, true));
    }

    // GATHER
    // collect all blocks back into a vector of blocks in master
    fprintf(stdout, "undoing convertYcbcrToBlocks()...\n");
    std::shared_ptr<ImageBlocks> imageBlocksIdct(new ImageBlocks);
    imageBlocksIdct->blocks = idcts;
    imageBlocksIdct->width = width;
    imageBlocksIdct->height = height;
    std::shared_ptr<ImageYcbcr> imgFromBlocks = convertBlocksToYcbcr(imageBlocksIdct, MACROBLOCK_SIZE);

    // SCATTER
    // give each thread an image with a section of the pixels to convert to rgb
    fprintf(stdout, "undoing convertRgbToYcbcr()...\n");
    std::shared_ptr<ImageRgb> imageRgbRecovered = convertYcbcrToRgb(imgFromBlocks);

    // each thread will continue and convert its image back to bytes
    fprintf(stdout, "undoing convertImageToBytes()...\n");
    std::vector<unsigned char> imgRecovered = convertImageToBytes(imageRgbRecovered);

    // GATHER
    // collect all bytes back into the full bytes vector for the image in master
    if (rank == 0) {
        error = lodepng::encode(outfile, imgRecovered, width, height);

        //if there's an error, display it
        if(error) {
            std::cout << "encoder error " << error << ": "<< lodepng_error_text(error) << std::endl;
        } else {
            fprintf(stdout, "success encoding to %s!\n", outfile);
        }
    }

    // Free derived types
    MPI_Type_free(&MPI_PixelYcbcr);
    MPI_Type_free(&MPI_ImageYcbcr);
    MPI_Type_free(&MPI_RleTuple);
    MPI_Type_free(&MPI_RleTupleVector);
    MPI_Type_free(&MPI_CharVector);
    MPI_Type_free(&MPI_DoubleVector);
    MPI_Type_free(&MPI_EncodedBlockColor);
    MPI_Type_free(&MPI_EncodedBlock);
    // end parallel area
    MPI_Finalize();
}

int main(int argc, char** argv) {
    //TODO insert timing code
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
