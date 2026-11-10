#ifndef PSP_SHARP_PBP_SCOPE_H
#define PSP_SHARP_PBP_SCOPE_H

/* Bounded read-only parser for a prefix of EBOOT.PBP; no VitaSDK dependency. */
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "psp_scope.h"

#define PSP_SHARP_PBP_PREFIX_MAX 16384u
#define PSP_SHARP_PBP_HEADER_SIZE 40u

static inline uint16_t psp_sharp_le16(const uint8_t *p) {
  return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}
static inline uint32_t psp_sharp_le32(const uint8_t *p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
         ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static inline int psp_sharp_range(size_t off, size_t size, size_t total) {
  return off <= total && size <= total - off;
}

static inline PSPSharpContentScope psp_sharp_classify_sfo(
    const uint8_t *sfo, size_t length) {
  uint32_t entries, key_start, data_start;
  size_t i;

  if (!sfo || length < 20 ||
      sfo[0] != 0 || sfo[1] != 'P' ||
      sfo[2] != 'S' || sfo[3] != 'F')
    return PSP_SHARP_SCOPE_UNKNOWN;

  key_start = psp_sharp_le32(sfo + 8);
  data_start = psp_sharp_le32(sfo + 12);
  entries = psp_sharp_le32(sfo + 16);
  if (entries > 128 ||
      !psp_sharp_range(20, (size_t)entries * 16, length) ||
      key_start < 20 + (size_t)entries * 16 ||
      key_start > data_start || data_start > length)
    return PSP_SHARP_SCOPE_UNKNOWN;

  for (i = 0; i < (size_t)entries; i++) {
    const uint8_t *entry = sfo + 20 + i * 16;
    uint16_t key_offset = psp_sharp_le16(entry);
    uint16_t format = psp_sharp_le16(entry + 2);
    uint32_t data_len = psp_sharp_le32(entry + 4);
    uint32_t data_capacity = psp_sharp_le32(entry + 8);
    uint32_t data_offset = psp_sharp_le32(entry + 12);
    size_t key = (size_t)key_start + (size_t)key_offset;
    size_t val = (size_t)data_start + (size_t)data_offset;

    if (key < key_start || key >= data_start ||
        !psp_sharp_range(key, 9, data_start))
      continue;
    if (memcmp(sfo + key, "CATEGORY", 8) != 0 || sfo[key + 8] != 0)
      continue;
    if ((format != 0x0204u && format != 0x0004u) ||
        data_len < 3 || data_len > 4 ||
        data_capacity < data_len || val < data_start ||
        !psp_sharp_range(val, (size_t)data_len, length) ||
        sfo[val + 2] != 0)
      return PSP_SHARP_SCOPE_UNKNOWN;

    {
      const char category[3] = {
          (char)sfo[val], (char)sfo[val + 1], 0};
      return psp_sharp_classify_category(category, sizeof(category));
    }
  }
  return PSP_SHARP_SCOPE_UNKNOWN;
}

static inline PSPSharpContentScope psp_sharp_classify_pbp_prefix(
    const uint8_t *pbp, size_t length) {
  uint32_t offsets[8];
  size_t i, start, end;

  if (!pbp || length < PSP_SHARP_PBP_HEADER_SIZE ||
      pbp[0] != 0 || pbp[1] != 'P' ||
      pbp[2] != 'B' || pbp[3] != 'P')
    return PSP_SHARP_SCOPE_UNKNOWN;

  for (i = 0; i < 8; i++) {
    offsets[i] = psp_sharp_le32(pbp + 8 + i * 4);
    if (offsets[i] < PSP_SHARP_PBP_HEADER_SIZE ||
        (i > 0 && offsets[i] < offsets[i - 1]))
      return PSP_SHARP_SCOPE_UNKNOWN;
  }
  start = offsets[0];
  end = offsets[1];
  if (end <= start || end > PSP_SHARP_PBP_PREFIX_MAX ||
      !psp_sharp_range(start, end - start, length))
    return PSP_SHARP_SCOPE_UNKNOWN;
  return psp_sharp_classify_sfo(pbp + start, end - start);
}

#endif
