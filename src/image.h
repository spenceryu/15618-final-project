struct pixel_rgba {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
}

struct pixel_ycbcr {
    unsigned char y;
    unsigned char cb;
    unsigned char cr;
}

typedef struct ImageRGB {
    std::vector<pixel_rgba> image;
    int width;
    int height;
} image_rgb;

typedef struct ImageYCbCr {
    std::vector<pixel_ycbcr> image;
    int width;
    int height;
} image_ycbcr;


