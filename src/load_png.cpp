#include <iostream>
#include "stdio.h"
#include "lodepng/lodepng.h"

void decodeOneStep(const char* filename) {
  std::vector<unsigned char> image; //the raw pixels
  unsigned width, height;

  //decode
  unsigned error = lodepng::decode(image, width, height, filename);

  //if there's an error, display it
  if(error) {
      std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;
  } else {
      fprintf(stdout, "success!\n");
  }

  //the pixels are now in the vector "image", 4 bytes per pixel, ordered RGBARGBA..., use it as texture, draw it, ...
}

int main() {
    decodeOneStep("raw_images/cookie.png");
    return 0;
}
