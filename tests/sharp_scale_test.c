#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "sharp_scale.h"

int main(void) {
  const uint8_t input[16] = {
      255, 0, 0, 255,   0, 0, 255, 255,
      0, 255, 0, 255,   255, 255, 255, 255
  };
  uint8_t output[4 * 4 * 4 + 2];
  unsigned x, y;

  memset(output, 0xAB, sizeof(output));
  assert(psp_sharp_rgba_scale(input, 2, 2, 8,
                              output + 1, 4, 4, 16, 256) == 0);
  for (y = 0; y < 4; y++) {
    for (x = 0; x < 4; x++) {
      const uint8_t *ref = input + ((y / 2) * 2 + (x / 2)) * 4;
      const uint8_t *pixel = output + 1 + (y * 4 + x) * 4;
      assert(memcmp(ref, pixel, 4) == 0);
    }
  }
  assert(output[0] == 0xAB && output[sizeof(output) - 1] == 0xAB);

  memset(output, 0xAB, sizeof(output));
  assert(psp_sharp_rgba_scale(input, 2, 2, 8,
                              output + 1, 4, 4, 16, 0) == 0);
  assert(output[1 + 4] < 255);  /* red blended into blue */
  assert(output[1 + 4 + 2] > 0);/* blue blended into red */
  assert(output[1 + 3] == 255);
  assert(output[0] == 0xAB && output[sizeof(output) - 1] == 0xAB);

  assert(psp_sharp_rgba_scale(NULL, 2, 2, 8,
                              output, 4, 4, 16, 256) == -1);
  assert(psp_sharp_rgba_scale(input, 2, 2, 7,
                              output, 4, 4, 16, 256) == -1);
  assert(psp_sharp_rgba_scale(input, 2, 2, 8,
                              output, 4, 4, 15, 256) == -1);
  assert(psp_sharp_rgba_scale(input, 2, 2, 8,
                              output, 4, 4, 16, 257) == -1);
  assert(psp_sharp_rgba_scale(input, 0, 2, 8,
                              output, 4, 4, 16, 0) == -1);
  assert(psp_sharp_rgba_scale(input, 2, 2, 8,
                              output, 961, 4, 3844, 0) == -1);
  assert(psp_sharp_rgba_scale(input, 2, 2, 8,
                              (uint8_t *)input, 2, 2, 8, 0) == -1);

  puts("RGBA8888 reference scaler: tests passed");
  return 0;
}
