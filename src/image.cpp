#include "image.h"

#define DEFAULT_ALPHA 1

std::shared_ptr<ImageRgb> convertBytesToImage(std::vector<unsigned char> bytes, unsigned int width, unsigned int height) {
    std::shared_ptr<ImageRgb> image(new ImageRgb());
    std::vector<std::shared_ptr<PixelRgba>> pixels;
    image->width = width;
    image->height = height;
    for (unsigned int i = 0; i < bytes.size(); i += 4) {
        std::shared_ptr<PixelRgba> pixel(new PixelRgba());
        pixel->r = bytes[i];
        pixel->g = bytes[i + 1];
        pixel->b = bytes[i + 2];
        pixel->a = bytes[i + 3];
        pixels.push_back(pixel);
    }
    image->pixels = pixels;
    return image;
}

std::vector<unsigned char> convertImageToBytes(std::shared_ptr<ImageRgb> image) {
    std::vector<unsigned char> bytes;
    for (auto pixel : image->pixels) {
        bytes.push_back(pixel->r);
        bytes.push_back(pixel->g);
        bytes.push_back(pixel->b);
        // ignore reconstructed alpha
        bytes.push_back(DEFAULT_ALPHA);
    }
    return bytes;
}

std::shared_ptr<ImageYcbcr> convertRgbToYcbcr(std::shared_ptr<ImageRgb> input) {
    std::shared_ptr<ImageYcbcr> result(new ImageYcbcr());
    result->width = input->width;
    result->height = input->height;
    std::vector<std::shared_ptr<PixelYcbcr>> new_pixels;
    for (auto pixel : input->pixels) {
        std::shared_ptr<PixelYcbcr> new_pixel(new PixelYcbcr());
        new_pixel->y = 16 + (65.738 * pixel->r + 129.057 * pixel->g + 25.064 * pixel->b) / 256;
        new_pixel->cb = 128 - (-37.945 * pixel->r - 74.494 * pixel->g + 112.439 * pixel->b) / 256;
        new_pixel->cr = 128 + (112.439 * pixel->r - 94.154 * pixel->g - 18.285 * pixel->b) / 256;
        new_pixels.push_back(new_pixel);
    }
    result->pixels = new_pixels;
    return result;
}

std::shared_ptr<ImageRgb> convertYcbcrToRgb(std::shared_ptr<ImageYcbcr> input) {
    std::shared_ptr<ImageRgb> result(new ImageRgb());
    result->width = input->width;
    result->height = input->height;
    std::vector<std::shared_ptr<PixelRgba>> new_pixels;
    for (auto pixel : input->pixels) {
        std::shared_ptr<PixelRgba> new_pixel(new PixelRgba());
        new_pixel->r = (298.082 * pixel->y + 408.583 * pixel->cr) / 256 - 222.921;
        new_pixel->g = (298.082 * pixel->y - 100.291 * pixel->cb - 208.120 * pixel->cr) / 256 - 135.576;
        new_pixel->b = (298.082 * pixel->y + 516.412 * pixel->cb) / 256 - 276.836;
        new_pixels.push_back(new_pixel);
    }
    result->pixels = new_pixels;
    return result;
}

std::shared_ptr<ImageBlocks> convertYcbcrToBlocks(std::shared_ptr<ImageYcbcr> input, int block_size) {
    std::shared_ptr<ImageBlocks> result(new ImageBlocks());
    std::vector<std::vector<std::shared_ptr<PixelYcbcr>>> blocks;
    int blocks_width = (input->width + block_size - 1) / block_size;
    int blocks_height = (input->height + block_size - 1) / block_size;
    for (int i = 0; i < blocks_height; i++) {
        for (int j = 0; j < blocks_width; j++) {
            std::vector<std::shared_ptr<PixelYcbcr>> block;
            Coord coord, temp_coord;
            int temp_index;
            int start = sub2ind(block_size, j, i) * block_size * block_size;
            int k = start;
            while (k - start < block_size * block_size && k < (input->width * input->height)) {
                std::shared_ptr<PixelYcbcr> pixel(new PixelYcbcr());
                pixel->y = input->pixels[k]->y;
                pixel->cb = 0;
                pixel->cr = 0;
                // get coord relative to current block
                coord = ind2sub(block_size, k - start);
                // only set cb/cr if in top left quadrant of block
                if (coord.col < (block_size / 2) && coord.row < (block_size / 2)) {
                    // get index into actual image
                    temp_coord.col = coord.col * 2;
                    temp_coord.col += j * block_size;
                    temp_coord.row = coord.row * 2;
                    temp_coord.row += i * block_size;
                    temp_index = sub2ind(input->width, temp_coord);
                    pixel->cb = input->pixels[temp_index]->cb;
                    pixel->cr = input->pixels[temp_index]->cr;
                }
                block.push_back(pixel);
                k++;
            }
            blocks.push_back(block);
        }
    }
    result->blocks = blocks;
    return result;
}

/*
std::shared_ptr<ImageYcbcr> convertBlocksToYcbcr(std::shared_ptr<ImageBlocks> input, int block_size) {
    std::shared_ptr<ImageYcbcr> result(new ImageYcbcr());
    std::vector<std::vector<std::shared_ptr<PixelYcbcr>>> blocks;
    int blocks_width = (input->width + block_size - 1) / block_size;
    int blocks_height = (input->height + block_size - 1) / block_size;
    for (int i = 0; i < blocks_height; i++) {
        for (int j = 0; j < blocks_width; j++) {
            std::vector<std::shared_ptr<PixelYcbcr>> block;
            Coord coord, temp_coord;
            int temp_index;
            int start = sub2ind(block_size, j, i) * block_size * block_size;
            int k = start;
            while (k - start < block_size * block_size && k < (input->width * input->height)) {
                std::shared_ptr<PixelYcbcr> pixel(new PixelYcbcr());
                pixel->y = input->pixels[k]->y;
                pixel->cb = 0;
                pixel->cr = 0;
                // get coord relative to current block
                coord = ind2sub(input->width, k);
                coord.col += j * block_size;
                coord.row += i * block_size;
                // only set cb/cr if in top left quadrant of block
                if (coord.col < (block_size / 2) && coord.row < (block_size / 2)) {
                    temp_coord.col = coord.col * 2;
                    temp_coord.col += j * block_size;
                    temp_coord.row = coord.row * 2;
                    temp_coord.row += i * block_size;
                    temp_index = sub2ind(input->width, temp_coord.col, temp_coord.row);
                    pixel->cb = input->pixels[temp_index]->cb;
                    pixel->cr = input->pixels[temp_index]->cr;
                }
                block.push_back(pixel);
                k++;
            }
            blocks.push_back(block);
        }
    }
    result->blocks = blocks;
    return result;
}
*/

// image utils

// Convert from (x,y) given size NxN array to vectorized idx
int sub2ind(int width, int col, int row) {
    return row * width + col;
}

int sub2ind(int width, Coord coord) {
    return coord.row * width + coord.col;
}

// Convert from vectorized idx to (x,y) given size NxN array
Coord ind2sub(int width, int idx) {
    Coord result;
    result.col = idx % width;
    result.row = idx / width;
    return result;
}
