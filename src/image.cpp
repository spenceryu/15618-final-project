#include "stdio.h"
#include "image.h"

#define DEFAULT_ALPHA 255

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
        new_pixel->cb = 128 - (37.945 * pixel->r + 74.494 * pixel->g - 112.439 * pixel->b) / 256;
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
        new_pixel->g = (298.082 * pixel->y - 100.291 * pixel->cb - 208.120 * pixel->cr) / 256 + 135.576;
        new_pixel->b = (298.082 * pixel->y + 516.412 * pixel->cb) / 256 - 276.836;
        new_pixels.push_back(new_pixel);
    }
    result->pixels = new_pixels;
    return result;
}

std::shared_ptr<ImageBlocks> convertYcbcrToBlocks(std::shared_ptr<ImageYcbcr> input, int block_size) {
    std::shared_ptr<ImageBlocks> result(new ImageBlocks());
    std::vector<std::vector<std::shared_ptr<PixelYcbcr>>> blocks;
    result->width = input->width;
    result->height = input->height;
    int blocks_width = (input->width + block_size - 1) / block_size;
    int blocks_height = (input->height + block_size - 1) / block_size;
    // rows of blocks
    for (int i = 0; i < blocks_height; i++) {
        int start_index = i * block_size * block_size * blocks_width;
        // blocks in next row of image
        std::vector<std::vector<std::shared_ptr<PixelYcbcr>>> block_row(blocks_width);
        // rows of pixels in the row of blocks
        for (int j = 0; j < block_size; j++) {
            int row_index = start_index + j * block_size * blocks_width;
            // pixels in the row of pixels
            for (int k = 0; k < block_size * blocks_width; k++) {
                int pixel_index = row_index + k;
                std::shared_ptr<PixelYcbcr> pixel(new PixelYcbcr());
                // TODO check pixel in bounds
                pixel->y = input->pixels[pixel_index]->y;
                pixel->cb = input->pixels[pixel_index]->cb;
                pixel->cr = input->pixels[pixel_index]->cr;
                block_row[k / block_size].push_back(pixel);
            }
        }
        blocks.insert(blocks.end(), block_row.begin(), block_row.end());
    }
    result->blocks = blocks;
    downsampleCbcr(result, block_size);
    return result;
}

std::shared_ptr<ImageYcbcr> convertBlocksToYcbcr(std::shared_ptr<ImageBlocks> input, int block_size) {
    upsampleCbcr(input, block_size);
    std::shared_ptr<ImageYcbcr> result(new ImageYcbcr());
    std::vector<std::shared_ptr<PixelYcbcr>> pixels;
    result->width = input->width;
    result->height = input->height;
    int blocks_width = (input->width + block_size - 1) / block_size;
    int blocks_height = (input->height + block_size - 1) / block_size;
    for (int i = 0; i < blocks_height; i++) {
        // pixels in every row
        std::vector<std::vector<std::shared_ptr<PixelYcbcr>>> pixel_rows(block_size);
        for (int j = 0; j < blocks_width; j++) {
            auto block = input->blocks[sub2ind(blocks_width, j, i)];
            // TODO deal with pixel bounding
            for (unsigned int k = 0; k < block.size(); k++) {
                std::shared_ptr<PixelYcbcr> pixel(new PixelYcbcr());
                Coord block_coord = ind2sub(block_size, k);
                int row = block_coord.row;
                pixel->y = block[k]->y;
                pixel->cb = block[k]->cb;
                pixel->cr = block[k]->cr;
                pixel_rows[row].push_back(pixel);
            }
        }
        // add all pixel_rows to image
        for (int j = 0; j < block_size; j++) {
            pixels.insert(pixels.end(), pixel_rows[j].begin(), pixel_rows[j].end());
        }
    }
    result->pixels = pixels;
    return result;
}

void downsampleCbcr(std::shared_ptr<ImageBlocks> image, int block_size) {
    for (auto block : image->blocks) {
        for (unsigned int i = 0; i < block.size(); i++) {
            Coord coord = ind2sub(block_size, i);
            if ((coord.row < block_size / 2) && (coord.col < block_size / 2)) {
                Coord sample_coord;
                sample_coord.col = coord.col * 2;
                sample_coord.row = coord.row * 2;
                int sample_index = sub2ind(block_size, sample_coord);
                block[i]->cb = block[sample_index]->cb;
                block[i]->cr = block[sample_index]->cr;
            } else {
                block[i]->cb = 0;
                block[i]->cr = 0;
            }
        }
    }
}

void upsampleCbcr(std::shared_ptr<ImageBlocks> image, int block_size) {
    for (auto block : image->blocks) {
        // restore cb/cr to original locations
        for (unsigned int i = block.size() - 1; i > 0; i--) {
            Coord coord = ind2sub(block_size, i);
            Coord sample_coord;
            sample_coord.row = coord.row / 2;
            sample_coord.col = coord.col / 2;
            int sample_index = sub2ind(block_size, sample_coord);
            block[i]->cb = block[sample_index]->cb;
            block[i]->cr = block[sample_index]->cr;
        }
        // interpolate lost cb/cr by averaging 2 nearby pixels
        for (int i = 1; i < block_size - 1; i += 2) {
            for (int j = 1; j < block_size - 1; j += 2) {
                int lower_index = sub2ind(block_size, j-1, i-1);
                int index = sub2ind(block_size, j, i);
                int upper_index = sub2ind(block_size, j+1, i+1);
                double cb = (block[lower_index]->cb + block[upper_index]->cb) / 2;
                double cr = (block[lower_index]->cr + block[upper_index]->cr) / 2;
                block[index]->cb = cb;
                block[index]->cr = cr;
            }
        }
    }
}

// image utils

// check if a pixel is in bounds given the block corner and pixel coord in block
bool pixel_in_bounds(Coord block_corner, Coord coord_in_block, int width, int height) {
    Coord absolute;
    absolute.row = block_corner.row + coord_in_block.row;
    absolute.col = block_corner.col + coord_in_block.col;
    return (absolute.row < height && absolute.col < width);
}

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
