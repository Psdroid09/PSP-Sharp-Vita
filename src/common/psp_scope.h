#ifndef PSP_SHARP_SCOPE_H
#define PSP_SHARP_SCOPE_H

/*
 * Classification of documented PSP PARAM.SFO CATEGORY strings.
 * This is read-only metadata analysis, NOT proof that the value came from
 * the launched PSP game instead of the Vita PSPemu host application.
 * Unknown values must never authorize a graphics modification.
 */
#include <stddef.h>

typedef enum PSPSharpContentScope {
  PSP_SHARP_SCOPE_UNKNOWN = 0,
  PSP_SHARP_SCOPE_PSP_CANDIDATE = 1,
  PSP_SHARP_SCOPE_PS1_EXCLUDED = 2
} PSPSharpContentScope;

static inline PSPSharpContentScope psp_sharp_classify_category(
    const char *category, size_t capacity) {
  if (!category || capacity < 3 || category[2] != '\0')
    return PSP_SHARP_SCOPE_UNKNOWN;

  if (category[0] == 'M' && category[1] == 'E')
    return PSP_SHARP_SCOPE_PS1_EXCLUDED;

  if ((category[0] == 'E' && category[1] == 'G') ||
      (category[0] == 'M' && category[1] == 'G') ||
      (category[0] == 'U' && category[1] == 'G'))
    return PSP_SHARP_SCOPE_PSP_CANDIDATE;

  return PSP_SHARP_SCOPE_UNKNOWN;
}

/*
 * A plausible category is insufficient: callers must independently prove
 * the metadata belongs to the launched game. Unverified => filter disabled.
 */
static inline int psp_sharp_filter_permitted(PSPSharpContentScope scope,
                                             int game_media_verified) {
  return scope == PSP_SHARP_SCOPE_PSP_CANDIDATE &&
         game_media_verified == 1;
}

#endif
