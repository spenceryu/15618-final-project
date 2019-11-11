#include "image.h"

ImageYcbcr convert_rgb_ycbcr(ImageRgb& input) {
    ImageYcbcr result = new ImageYcbcr();
    result.width = input.width;
    result.height = input.height;
    std::vector<PixelYcbcr> new_pixels;
    for (auto pixel : input.pixels) {
        PixelYcbcr new_pixel = new PixelYcbcr();
        new_pixel.y = 16 + (65.738 * pixel.r + 129.057 * pixel.g + 25.064 * pixel.b) / 256;
        new_pixel.cb = 128 - (-37.945 * pixel.r - 74.494 * pixel.g + 112.439 * pixel.b) / 256;
        new_pixel.cr = 128 + (112.439 * pixel.r - 94.154 * pixel.g - 18.285 * pixel.b) / 256;
        new_pixels.push_back(new_pixel);
    }
    result.pixels = new_pixels;
    return result;
}

ImageRgb convert_ycbcr_rgb(ImageYcbcr& input) {
    ImageRgb result = new ImageRgb();
    result.width = input.width;
    result.height = input.height;
    std::vector<PixelRgb> new_pixels;
    for (auto pixel : input.pixels) {
        PixelRgb new_pixel = new PixelRgb();
        new_pixel.r = (298.082 * pixel.y + 408.583 * pixel.cr) / 256 - 222.921;
        new_pixel.g = (298.082 * pixel.y - 100.291 * pixel.cb - 208.120 * pixel.cr) / 256 - 135.576;
        new_pixel.b = (298.082 * pixel.y + 516.412 * pixel.cb) / 256 - 276.836;
        new_pixels.push_back(new_pixel);
    }
    result.pixels = new_pixels;
    return result;
}

ImageBlocks convert_ycbcr_blocks(ImageYcbcr& input) {
    ImageBlocks result = new ImageBlocks();
    std::vector<std::vector<PixelYcbcr>> blocks;
    int blocks_width = (input.width - 1) / MACROBLOCK_SIZE + 1;
    int blocks_height = (input.height - 1) / MACROBLOCK_SIZE + 1;
    for (int i = 0; i < blocks_height; i++) {
        for (int j = 0; j < blocks_width; j++) {
            std::vector<PixelYcbcr> block;
            Coord coord, temp_coord;
            int temp_index;
            int start = sub2ind(MACROBLOCK_SIZE, j, i) * MACROBLOCK_SIZE * MACROBLOCK_SIZE;
            int k = start;
            while (k - start < MACROBLOCK_SIZE * MACROBLOCK_SIZE) {
                PixelYcbcr pixel = new PixelYcbcr();
                pixel.y = input.pixels[k].y;
                pixel.cb = 0;
                pixel.cr = 0;
                // get coord relative to current block
                coord = ind2sub(input.width, k);
                coord.col -= j * MACROBLOCK_SIZE;
                coord.row -= i * MACROBLOCK_SIZE;
                // only set cb/cr if in top left quadrant of block
                if (coord.col < (MACROBLOCK_SIZE / 2) && coord.row < (MACROBLOCK_SIZE / 2)) {
                    temp_coord = new Coord();
                    temp_coord.col = coord.col * 2;
                    temp_coord.col += j * MACROBLOCK_SIZE;
                    temp_coord.row = coord.row * 2;
                    temp_coord.row += i * MACROBLOCK_SIZE;
                    temp_index = sub2ind(input.width, temp_coord.col, temp_coord.row);
                    pixel.cb = input.pixels[temp_index].cb;
                    pixel.cr = input.pixels[temp_index].cr;
                }
                block.push_back(pixel);
            }
            blocks.push_back(block);
        }
    }
    result.blocks = blocks;
    return result;
}

// image utils

// Convert from (x,y) given size NxN array to vectorized idx
int sub2ind(int width, int col, int row) {
    return row * width + col;
}

// Convert from vectorized idx to (x,y) given size NxN array
Coord ind2sub(int width, int idx) {
    int col = idx % width;
    int row = idx / width;
    return new Coord(col, row);
}
