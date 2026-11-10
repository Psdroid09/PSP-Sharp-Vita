#include <assert.h>
#include <stdio.h>
#include "psp_scope.h"

#define TEST(value, cap, expected) \
  assert(psp_sharp_classify_category((value), (cap)) == (expected))

int main(void) {
  TEST("EG", 3, PSP_SHARP_SCOPE_PSP_CANDIDATE);
  TEST("MG", 3, PSP_SHARP_SCOPE_PSP_CANDIDATE);
  TEST("UG", 3, PSP_SHARP_SCOPE_PSP_CANDIDATE);
  TEST("ME", 3, PSP_SHARP_SCOPE_PS1_EXCLUDED);
  TEST("MA", 3, PSP_SHARP_SCOPE_UNKNOWN);
  TEST("PG", 3, PSP_SHARP_SCOPE_UNKNOWN);
  TEST("MS", 3, PSP_SHARP_SCOPE_UNKNOWN);
  TEST("gd", 3, PSP_SHARP_SCOPE_UNKNOWN);
  TEST("", 1, PSP_SHARP_SCOPE_UNKNOWN);
  TEST("E", 2, PSP_SHARP_SCOPE_UNKNOWN);
  TEST("EGB", 4, PSP_SHARP_SCOPE_UNKNOWN);
  TEST("eg", 3, PSP_SHARP_SCOPE_UNKNOWN);
  TEST("MEX", 4, PSP_SHARP_SCOPE_UNKNOWN);
  TEST(NULL, 0, PSP_SHARP_SCOPE_UNKNOWN);
  TEST("EG", 0, PSP_SHARP_SCOPE_UNKNOWN);
  TEST("EG", 2, PSP_SHARP_SCOPE_UNKNOWN);

  assert(!psp_sharp_filter_permitted(PSP_SHARP_SCOPE_PSP_CANDIDATE, 0));
  assert(!psp_sharp_filter_permitted(PSP_SHARP_SCOPE_PSP_CANDIDATE, -1));
  assert(!psp_sharp_filter_permitted(PSP_SHARP_SCOPE_PS1_EXCLUDED, 1));
  assert(!psp_sharp_filter_permitted(PSP_SHARP_SCOPE_UNKNOWN, 1));
  assert(psp_sharp_filter_permitted(PSP_SHARP_SCOPE_PSP_CANDIDATE, 1));

  puts("PSP scope tests: all checks passed");
  return 0;
}
