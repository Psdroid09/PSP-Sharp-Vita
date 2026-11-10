#ifndef PSP_SHARP_SCALE_H
#define PSP_SHARP_SCALE_H

/*
 * Experimental CPU reference only. Caller provides distinct, owned,
 * RGBA8888 buffers. NEVER pass a PSPemu/display framebuffer directly.
 * strength=0 is bilinear, 256 is crisp nearest-neighbour.
 * No global state, allocation, file access, GPU access, or VitaSDK.
 */
#include <stddef.h>
#include <stdint.h>

static inline int psp_sharp_rgba_scale(
    const uint8_t *src, unsigned sw, unsigned sh, size_t spitch,
    uint8_t *dst, unsigned dw, unsigned dh, size_t dpitch,
    unsigned strength) {
  unsigned x, y, c;
  if (!src || !dst || src == dst ||
      sw == 0 || sh == 0 || dw == 0 || dh == 0 ||
      sw > 960 || dw > 960 || sh > 544 || dh > 544 ||
      spitch < (size_t)sw * 4 || dpitch < (size_t)dw * 4 ||
      spitch > 8192 || dpitch > 8192 || strength > 256)
    return -1;

  for (y = 0; y < dh; y++) {
    int64_t fy = ((int64_t)(2u * y + 1u) * sh * 256) /
                 (2u * dh) - 128;
    unsigned y0, y1, wy, yn;
    uint8_t *out = dst + (size_t)y * dpitch;
    if (fy < 0)
      fy = 0;
    if (fy > (int64_t)(sh - 1) * 256)
      fy = (int64_t)(sh - 1) * 256;
    y0 = (unsigned)(fy >> 8);
    y1 = y0 + (y0 + 1 < sh);
    wy = (unsigned)(fy & 255);
    yn = (unsigned)(((uint64_t)(2u * y + 1u) * sh) / (2u * dh));

    for (x = 0; x < dw; x++) {
      int64_t fx = ((int64_t)(2u * x + 1u) * sw * 256) /
                   (2u * dw) - 128;
      unsigned x0, x1, wx, xn;
      const uint8_t *p00, *p01, *p10, *p11, *near;

      if (fx < 0)
        fx = 0;
      if (fx > (int64_t)(sw - 1) * 256)
        fx = (int64_t)(sw - 1) * 256;
      x0 = (unsigned)(fx >> 8);
      x1 = x0 + (x0 + 1 < sw);
      wx = (unsigned)(fx & 255);
      xn = (unsigned)(((uint64_t)(2u * x + 1u) * sw) / (2u * dw));
      p00 = src + (size_t)y0 * spitch + 4u * x0;
      p01 = src + (size_t)y0 * spitch + 4u * x1;
      p10 = src + (size_t)y1 * spitch + 4u * x0;
      p11 = src + (size_t)y1 * spitch + 4u * x1;
      near = src + (size_t)yn * spitch + 4u * xn;

      for (c = 0; c < 4; c++) {
        unsigned upper = (p00[c] * (256u - wx) +
                          p01[c] * wx + 128u) >> 8;
        unsigned lower = (p10[c] * (256u - wx) +
                          p11[c] * wx + 128u) >> 8;
        unsigned bilinear = (upper * (256u - wy) +
                             lower * wy + 128u) >> 8;
        out[x * 4u + c] = (uint8_t)(
            (bilinear * (256u - strength) +
             near[c] * strength + 128u) >> 8);
      }
    }
  }
  return 0;
}

#endif /* PSP_SHARP_SCALE_H */
