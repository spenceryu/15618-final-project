struct pixel_rgba {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
}

struct pixel_ycbcr {
    double y;
    double cb;
    double cr;
}

typedef struct ImageRGB {
    std::vector<pixel_rgba> pixels;
    int width;
    int height;
} image_rgb;

typedef struct ImageYCbCr {
    std::vector<pixel_ycbcr> pixels;
    int width;
    int height;
} image_ycbcr;

image_ycbcr convert_rgb_ycbcr(image_rgb input);
image_rgb convert_ycbcr_rgb(image_ycbcr input);
