#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "pbp_scope.h"

static void put16(uint8_t *p, unsigned n) {
  p[0] = (uint8_t)n;
  p[1] = (uint8_t)(n >> 8);
}
static void put32(uint8_t *p, uint32_t n) {
  p[0] = (uint8_t)n;
  p[1] = (uint8_t)(n >> 8);
  p[2] = (uint8_t)(n >> 16);
  p[3] = (uint8_t)(n >> 24);
}
static void fixture(uint8_t *pbp, const char *category) {
  size_t i;
  uint8_t *sfo = pbp + 40;
  memset(pbp, 0, 128);
  memcpy(pbp + 1, "PBP", 3);
  put32(pbp + 4, 0x00010000);
  put32(pbp + 8, 40);
  for (i = 1; i < 8; i++)
    put32(pbp + 8 + i * 4, 92);
  memcpy(sfo + 1, "PSF", 3);
  put32(sfo + 4, 0x00000101);
  put32(sfo + 8, 36);
  put32(sfo + 12, 48);
  put32(sfo + 16, 1);
  put16(sfo + 20, 0);
  put16(sfo + 22, 0x0204);
  put32(sfo + 24, 3);
  put32(sfo + 28, 4);
  put32(sfo + 32, 0);
  memcpy(sfo + 36, "CATEGORY", 9);
  memcpy(sfo + 48, category, 2);
}
int main(void) {
  uint8_t pbp[128];
  fixture(pbp, "EG");
  assert(psp_sharp_classify_pbp_prefix(pbp, 92) ==
         PSP_SHARP_SCOPE_PSP_CANDIDATE);
  fixture(pbp, "MG");
  assert(psp_sharp_classify_pbp_prefix(pbp, 92) ==
         PSP_SHARP_SCOPE_PSP_CANDIDATE);
  fixture(pbp, "UG");
  assert(psp_sharp_classify_pbp_prefix(pbp, 92) ==
         PSP_SHARP_SCOPE_PSP_CANDIDATE);
  fixture(pbp, "ME");
  assert(psp_sharp_classify_pbp_prefix(pbp, 92) ==
         PSP_SHARP_SCOPE_PS1_EXCLUDED);
  fixture(pbp, "XX");
  assert(psp_sharp_classify_pbp_prefix(pbp, 92) ==
         PSP_SHARP_SCOPE_UNKNOWN);

  fixture(pbp, "EG");
  assert(psp_sharp_classify_pbp_prefix(pbp, 91) ==
         PSP_SHARP_SCOPE_UNKNOWN);
  assert(psp_sharp_classify_pbp_prefix(NULL, 92) ==
         PSP_SHARP_SCOPE_UNKNOWN);
  pbp[1] = 'Z';
  assert(psp_sharp_classify_pbp_prefix(pbp, 92) ==
         PSP_SHARP_SCOPE_UNKNOWN);
  fixture(pbp, "EG");
  put32(pbp + 12, 0xffffffffu);
  assert(psp_sharp_classify_pbp_prefix(pbp, 92) ==
         PSP_SHARP_SCOPE_UNKNOWN);
  fixture(pbp, "EG");
  put32(pbp + 12, 39);
  assert(psp_sharp_classify_pbp_prefix(pbp, 92) ==
         PSP_SHARP_SCOPE_UNKNOWN);
  fixture(pbp, "EG");
  put32(pbp + 40 + 16, 0xffffffffu);
  assert(psp_sharp_classify_pbp_prefix(pbp, 92) ==
         PSP_SHARP_SCOPE_UNKNOWN);
  fixture(pbp, "EG");
  put32(pbp + 40 + 32, 0xffffffffu);
  assert(psp_sharp_classify_pbp_prefix(pbp, 92) ==
         PSP_SHARP_SCOPE_UNKNOWN);
  fixture(pbp, "ME");
  pbp[40 + 36] = 'X';
  assert(psp_sharp_classify_pbp_prefix(pbp, 92) ==
         PSP_SHARP_SCOPE_UNKNOWN);

  puts("PBP/SFO parser: tests passed");
  return 0;
}
