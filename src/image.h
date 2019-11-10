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

ImageYcbcr convert_rgb_ycbcr(ImageRgb& input);
ImageRgb convert_ycbcr_rgb(ImageYcbcr& input);
