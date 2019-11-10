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

struct image_rgb {
    std::vector<pixel_rgba> pixels;
    int width;
    int height;
};

typedef struct image_ycbcr {
    std::vector<pixel_ycbcr> pixels;
    int width;
    int height;
};

image_ycbcr convert_rgb_ycbcr(image_rgb input);
image_rgb convert_ycbcr_rgb(image_ycbcr input);
