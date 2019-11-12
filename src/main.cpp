#include <iostream>
#include "stdio.h"
#include "lodepng/lodepng.h"
#include "dct.h"
#include "quantize.h"
#include "dpcm.h"
#include "rle.h"

#define MACROBLOCK_SIZE 8

void decodeOneStep(const char* filename) {
    std::vector<unsigned char> bytes; //the raw pixels
    unsigned int width, height;

    //decode
    unsigned int error = lodepng::decode(bytes, width, height, filename);

    //if there's an error, display it
    if(error) {
      std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;
    } else {
      fprintf(stdout, "success decoding %s!\n", filename);
    }

    //the pixels are now in the vector "image", 4 bytes per pixel, ordered RGBARGBA..., use it as texture, draw it, ...
    std::shared_ptr<ImageRgb> imageRgb = convertBytesToImage(bytes, width, height);
    std::shared_ptr<ImageYcbcr> imageYcbcr = convertRgbToYcbcr(imageRgb);
    std::shared_ptr<ImageBlocks> imageBlocks = convertYcbcrToBlocks(imageYcbcr, MACROBLOCK_SIZE);
    std::vector<std::vector<std::shared_ptr<PixelYcbcr>>> dcts;
    for (auto block : imageBlocks->blocks) {
        dcts.push_back(DCT(block, MACROBLOCK_SIZE, true));
    }
    std::vector<std::vector<std::shared_ptr<PixelYcbcr>>> quantizedBlocks;
    for (auto dct : dcts) {
        quantizedBlocks.push_back(quantize(dct, MACROBLOCK_SIZE, true));
    }
    DPCM(quantizedBlocks);
    std::vector<std::shared_ptr<EncodedBlock>> encodedBlocks;
    for (auto quantizedBlock : quantizedBlocks) {
        encodedBlocks.push_back(RLE(quantizedBlock, MACROBLOCK_SIZE));
    }

    /* const char* outfile = "images/out.png"; */
    /* error = lodepng::encode(outfile, image, width, height); */

    /* //if there's an error, display it */
    /* if(error) { */
    /*   std::cout << "encoder error " << error << ": "<< lodepng_error_text(error) << std::endl; */
    /* } else { */
    /*   fprintf(stdout, "success encoding to %s!\n", outfile); */
    /* } */
}

int main() {
    decodeOneStep("raw_images/cookie.png");
    return 0;
}
