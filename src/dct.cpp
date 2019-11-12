#include "dct.h"

// Forward DCT operation for NxN block
// If <all> set, then YCbCr each have DCT performed on them.
// Else, only Y has DCT performed on it.
std::vector<std::shared_ptr<PixelYcbcr>> DCT(std::vector<std::shared_ptr<PixelYcbcr>> pixels, int block_size, bool all) {

    std::vector<std::shared_ptr<PixelYcbcr>> F(block_size * block_size);

    // Output: F(u,v)
    for (int u = 0; u < block_size; u++) {
        for (int v = 0; v < block_size; v++) {
            int vectorized_idx_uv = sub2ind(block_size, u, v);
            double cu = (u == 0) ? 1 : (1/sqrt(2));
            double cv = (v == 0) ? 1 : (1/sqrt(2));

            double tmp_y, tmp_cb, tmp_cr;
            tmp_y = 0;
            if (all) {
                tmp_cb = 0;
                tmp_cr = 0;
            }

            // Input: f(x,y)
            for (int x = 0; x < block_size; x++) {
                for (int y = 0; y < block_size; y++) {
                    int vectorized_idx_xy = sub2ind(block_size, x, y);
                    std::shared_ptr<PixelYcbcr> f_xy = pixels[vectorized_idx_xy];

                    double xprod = cos((2*x + 1)*u*M_PI/16);
                    double yprod = cos((2*y + 1)*v*M_PI/16);

                    tmp_y += ((f_xy->y) * xprod * yprod);
                    if (all) {
                        tmp_cr += ((f_xy->cr) * xprod * yprod);
                        tmp_cb += ((f_xy->cb) * xprod * yprod);
                    }
                }
            }

            F[vectorized_idx_uv]->y = (1/4) * cu * cv * tmp_y;
            if (all) {
                F[vectorized_idx_uv]->cr = (1/4) * cu * cv * tmp_cr;
                F[vectorized_idx_uv]->cb = (1/4) * cu * cv * tmp_cb;
            } else {
                F[vectorized_idx_uv]->cr = 0;
                F[vectorized_idx_uv]->cb = 0;
            }
        }
    }
    return F;
}

// Inverse DCT operation for NxN block
// If <all> set, then YCbCr each have IDCT performed on them.
// Else, only Y has IDCT performed on it.
std::vector<std::shared_ptr<PixelYcbcr>> IDCT(std::vector<std::shared_ptr<PixelYcbcr>> pixels, int block_size, bool all) {

    std::vector<std::shared_ptr<PixelYcbcr>> f(block_size * block_size);

    // Output: f(x,y)
    for (int x = 0; x < block_size; x++) {
        for (int y = 0; y < block_size; y++) {
            int vectorized_idx_xy = sub2ind(block_size, x, y);

            double tmp_y, tmp_cb, tmp_cr;
            tmp_y = 0;
            if (all) {
                tmp_cb = 0;
                tmp_cr = 0;
            }

            // Input: F(u,v)
            for (int u = 0; u < block_size; u++) {
                for (int v = 0; v < block_size; v++) {
                    double cu = (u == 0) ? 1 : (1/sqrt(2));
                    double cv = (v == 0) ? 1 : (1/sqrt(2));
                    int vectorized_idx_uv = sub2ind(block_size, u, v);
                    std::shared_ptr<PixelYcbcr> f_uv = pixels[vectorized_idx_uv];

                    double xprod = cos((2*x + 1)*u*M_PI/16);
                    double yprod = cos((2*y + 1)*v*M_PI/16);

                    tmp_y += (cu * cv * (f_uv->y) * xprod * yprod);
                    if (all) {
                        tmp_cr += (cu * cv * (f_uv->cr) * xprod * yprod);
                        tmp_cb += (cu * cv * (f_uv->cb) * xprod * yprod);
                    }
                }
            }

            f[vectorized_idx_xy]->y = (1/4) * tmp_y;
            if (all) {
                f[vectorized_idx_xy]->cr = (1/4) * tmp_cr;
                f[vectorized_idx_xy]->cb = (1/4) * tmp_cb;
            }
        }
    }
    return f;
}
