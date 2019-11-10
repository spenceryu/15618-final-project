// Quantization per channel for NxN block
// If <all> set, then YCbCr each have Quantize operation performed on them.
// Else, only Y has Quantize operation performed on it.
std::vector<PixelYcbcr> Quantize(std::vector<PixelYcbcr> pixels, N, bool all) {

    if (N != 8) {
        fprintf(stderr, "Quantization only supports 8x8 blocks!\n");
        exit(1);
    }

    for (int i = 0; i < N*N; i++) {
        pixels[i].y = round(pixels[i].y / quant_matrix[i]);
        if (all) {
            pixels[i].cr = round(pixels[i].cr / quant_matrix[i]);
            pixels[i].cb = round(pixels[i].cb / quant_matrix[i]);
        }
    }
    return pixels;
}
