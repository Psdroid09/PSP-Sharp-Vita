/*
 * PSP-Sharp-Vita: read-only PSPemu compatibility framebuffer probe.
 * Diagnostic only: no pixel memory access or display mutation.
 * Target: Vita firmware 3.60; user plugin loaded from *ALL.
 */
#include <vitasdk.h>
#include <taihen.h>
#include <psp2/display.h>
#include <psp2/io/fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define DISPLAY_USER_LIB_NID 0x4FAACD11
#define DISPLAY_COMPAT_SET_FB_NID 0x8C36B628
#define MAX_RECORDS 8
#define REPORT_PATH "ux0:data/PSPSharp_compat_probe.txt"

typedef struct ProbeFrameBuf {
  SceSize size;
  void *base;
  unsigned int pitch;
  unsigned int pixelformat;
  unsigned int width;
  unsigned int height;
} ProbeFrameBuf;

typedef struct ProbeFrameBufCompat {
  SceSize size;
  void *base;
  unsigned int pitch;
  unsigned int pixelformat;
  unsigned int width;
  unsigned int height;
  SceUInt32 flags;
} ProbeFrameBufCompat;

typedef char ProbeFrameBufSizeCheck[(sizeof(ProbeFrameBuf) == 0x18) ? 1 : -1];
typedef char ProbeFrameBufCompatSizeCheck[(sizeof(ProbeFrameBufCompat) == 0x1C) ? 1 : -1];

typedef struct ProbeRecord {
  unsigned int head, index, sync;
  unsigned int source_size, capture_present;
  unsigned int descriptor_valid, base, pitch, pixelformat, width, height, flags;
} ProbeRecord;

static tai_hook_ref_t g_hook_ref;
static SceUID g_hook_id = -1;
static ProbeRecord g_records[MAX_RECORDS];
static volatile unsigned int g_record_slots;
static volatile unsigned int g_record_count;
static volatile unsigned int g_nonnull_calls;
static volatile unsigned int g_null_source_calls;
static volatile unsigned int g_invalid_descriptors;

/*
 * The original firmware wrapper reads/copies the sized descriptors before
 * returning. Observe the source descriptor only AFTER a successful original
 * call, so this probe does not dereference an argument the wrapper rejected.
 * The caller-owned descriptor is still assumed to remain valid for the
 * duration of this hook invocation; no pixel pointer is ever dereferenced.
 */
static int compat_set_framebuf_hook(
    int head, int index,
    const void *arg3, int sync,
    const void *source_param, const void *capture_param) {
  int ret = TAI_CONTINUE(int, g_hook_ref,
                         head, index, arg3, sync, source_param, capture_param);

  if (source_param == NULL) {
    __sync_fetch_and_add(&g_null_source_calls, 1);
    return ret;
  }

  __sync_fetch_and_add(&g_nonnull_calls, 1);

  if (ret >= 0) {
    const ProbeFrameBuf *fb;
    unsigned int source_size;
    unsigned int slot;

    if (((uintptr_t)source_param & 3u) != 0u) {
      __sync_fetch_and_add(&g_invalid_descriptors, 1);
      return ret;
    }

    source_size = *(const unsigned int *)source_param;
    if (source_size != sizeof(ProbeFrameBuf) &&
        source_size != sizeof(ProbeFrameBufCompat)) {
      __sync_fetch_and_add(&g_invalid_descriptors, 1);
      return ret;
    }

    fb = (const ProbeFrameBuf *)source_param;
    if (fb->size != source_size) {
      __sync_fetch_and_add(&g_invalid_descriptors, 1);
      return ret;
    }

    slot = __sync_fetch_and_add(&g_record_count, 1);
    if (slot < MAX_RECORDS) {
      ProbeRecord *r = &g_records[slot];
      memset(r, 0, sizeof(*r));
      r->head = (unsigned int)head;
      r->index = (unsigned int)index;
      r->sync = (unsigned int)sync;
      r->source_size = source_size;
      r->capture_present = (capture_param != NULL);
      r->base = (unsigned int)(uintptr_t)fb->base;
      r->pitch = fb->pitch;
      r->pixelformat = fb->pixelformat;
      r->width = fb->width;
      r->height = fb->height;
      if (source_size == sizeof(ProbeFrameBufCompat)) {
        const ProbeFrameBufCompat *ext =
            (const ProbeFrameBufCompat *)source_param;
        r->flags = ext->flags;
      }
      r->descriptor_valid =
          (fb->base != NULL && fb->width > 0 && fb->height > 0 &&
           fb->pitch >= fb->width);
      __sync_synchronize();
      __sync_fetch_and_add(&g_record_count, 1);
    }
  }

  return ret;
}

static void write_report(void) {
  char report[2048];
  int used = 0;
  SceUID fd;
  unsigned int i;
  unsigned int records = g_record_count;

  if (records > MAX_RECORDS)
    records = MAX_RECORDS;

  used += snprintf(report + used, sizeof(report) - used,
                   "PSP-Sharp-Vita read-only probe\n"
                   "Successful non-NULL source calls: %u\n"
                   "Calls with NULL source: %u\n"
                   "Invalid/unrecognized descriptors: %u\n"
                   "Metadata records retained: %u\n"
                   "No pixel memory was read or modified.\n\n",
                   (unsigned int)g_nonnull_calls,
                   (unsigned int)g_null_source_calls,
                   (unsigned int)g_invalid_descriptors, records);

  for (i = 0; i < records && used < (int)sizeof(report); i++) {
    const ProbeRecord *r = &g_records[i];
    used += snprintf(report + used, sizeof(report) - used,
                     "Call %u\n head=%u index=%u sync=%u\n"
                     "source_size=0x%X capture_present=%u\n"
                     "descriptor_valid=%u base=0x%08X\n"
                     "pitch=%u format=0x%08X width=%u height=%u flags=0x%08X\n\n",
                     i, r->head, r->index, r->sync,
                     r->source_size, r->capture_present,
                     r->descriptor_valid, r->base,
                     r->pitch, r->pixelformat, r->width, r->height, r->flags);
  }

  if (used <= 0 || used >= (int)sizeof(report))
    return;

  fd = sceIoOpen(REPORT_PATH, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
  if (fd >= 0) {
    sceIoWrite(fd, report, (SceSize)used);
    sceIoClose(fd);
  }
}

void _start() __attribute__((weak, alias("module_start")));

int module_start(SceSize args, void *argp) {
  tai_module_info_t info;
  (void)args;
  (void)argp;
  memset(&info, 0, sizeof(info));
  info.size = sizeof(info);

  /* Leave every non-PSPemu process immediately. */
  if (taiGetModuleInfo("ScePspemu", &info) < 0)
    return SCE_KERNEL_START_NO_RESIDENT;

  /* Restrict this diagnostic build to the 3.60 retail PSPemu module. */
  if (info.module_nid != 0x2714F07D)
    return SCE_KERNEL_START_NO_RESIDENT;

  g_hook_id = taiHookFunctionImport(
      &g_hook_ref, "ScePspemu",
      DISPLAY_USER_LIB_NID, DISPLAY_COMPAT_SET_FB_NID,
      compat_set_framebuf_hook);

  if (g_hook_id < 0)
    return SCE_KERNEL_START_NO_RESIDENT;
  return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize args, void *argp) {
  (void)args;
  (void)argp;
  if (g_hook_id >= 0) {
    taiHookRelease(g_hook_id, g_hook_ref);
    g_hook_id = -1;
  }
  /* One report at unload; never write from the frame callback. */
  write_report();
  return SCE_KERNEL_STOP_SUCCESS;
}
