#include "dct.h"
#include "image.cpp"

// Forward DCT operation for NxN block
// If <all> set, then YCbCr each have DCT performed on them.
// Else, only Y has DCT performed on it.
std::vector<PixelYcbcr> DCT(std::vector<PixelYcbcr> pixels, N, bool all) {

    std::vector<PixelYcbcr> F = new std::vector<PixelYcbcr>();

    // Output: F(u,v)
    for (int u = 0; u < N; u++) {
        for (int v = 0; v < N; v++) {
            int vectorized_idx_uv = sub2ind(N, u, v);
            double cu = (u == 0) ? 1 : (1/sqrt(2));
            double cv = (v == 0) ? 1 : (1/sqrt(2));

            double tmp_y = 0;
            if (all) {
                double tmp_cb = 0;
                double tmp_cr = 0;
            }

            // Input: f(x,y)
            for (int x = 0; x < N; x++) {
                for (int y = 0; y < N; y++) {
                    int vectorized_idx_xy = sub2ind(N, x, y);
                    PixelYcbcr f_xy = pixels[vectorized_idx_xy];

                    double xprod = cos((2*x + 1)*u*M_PI/16);
                    double yprod = cos((2*y + 1)*v*M_PI/16);

                    tmp_y += ((f_xy.y) * xprod * yprod);
                    if (all) {
                        tmp_cr += ((f_xy.cr) * xprod * yprod);
                        tmp_cb += ((f_xy.cb) * xprod * yprod);
                    }
                }
            }

            F[vectorized_idx_uv].y = (1/4) * cu * cv * tmp_y;
            if (all) {
                F[vectorized_idx_uv].cr = (1/4) * cu * cv * tmp_cr;
                F[vectorized_idx_uv].cb = (1/4) * cu * cv * tmp_cb;
            } else {
                F[vectorized_idx_uv].cr = 0;
                F[vectorized_idx_uv].cb = 0;
            }
        }
    }
}

// Inverse DCT operation for NxN block
// If <all> set, then YCbCr each have IDCT performed on them.
// Else, only Y has IDCT performed on it.
std::vector<pixel_ycbcr> IDCT(std::vector<pixel_ycbcr> pixels, N, bool all) {

    std::vector<PixelYcbcr> F = new std::vector<PixelYcbcr>();

    // Output: f(x,y)
    for (int x = 0; x < N; x++) {
        for (int y = 0; y < N; y++) {
            int vectorized_idx_xy = sub2ind(N, x, y);

            double tmp_y = 0;
            if (all) {
                double tmp_cb = 0;
                double tmp_cr = 0;
            }

            // Input: F(u,v)
            for (int u = 0; u < N; u++) {
                for (int v = 0; v < N; v++) {
                    double cu = (u == 0) ? 1 : (1/sqrt(2));
                    double cv = (v == 0) ? 1 : (1/sqrt(2));
                    int vectorized_idx_uv = sub2ind(N, u, v);
                    PixelYcbcr f_uv = pixels[vectorized_idx_uv];

                    double xprod = cos((2*x + 1)*u*M_PI/16);
                    double yprod = cos((2*y + 1)*v*M_PI/16);

                    tmp_y += (cu * cv * (f_uv.y) * xprod * yprod);
                    if (all) {
                        tmp_cr += (cu * cv * (f_uv.cr) * xprod * yprod);
                        tmp_cb += (cu * cv * (f_uv.cb) * xprod * yprod);
                    }
                }
            }

            f[vectorized_idx_xy].y = (1/4) * tmp_y;
            if (all) {
                f[vectorized_idx_xy].cr = (1/4) * tmp_cr;
                f[vectorized_idx_xy].cb = (1/4) * tmp_cb;
            }
        }
    }
}

struct int2 {
    int x;
    int y;
}
