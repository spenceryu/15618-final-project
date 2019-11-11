struct PixelRgba {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
}

struct PixelYcbcr {
    double y;
    double cb;
    double cr;
}

struct ImageRgb {
    std::vector<PixelRgba> pixels;
    int width;
    int height;
};

struct ImageYcbcr {
    std::vector<PixelYcbcr> pixels;
    int width;
    int height;
};

struct ImageBlocks {
    std::vector<std::vector<PixelYcbcr>> blocks;
    int width;
    int height;
};

ImageYcbcr convert_rgb_ycbcr(ImageRgb& input);
ImageRgb convert_ycbcr_rgb(ImageYcbcr& input);

ImageBlocks convert_ycbcr_blocks(ImageYcbcr& input);

// image utils

struct Coord {
    int col;
    int row;
}

// Convert from (x,y) given size NxN array to vectorized idx
int sub2ind(int width, int col, int row);

// Convert from vectorized idx to (x,y) given size NxN array
Coord ind2sub(int width, int idx);
