/*
 * PSP-Sharp-Vita: read-only PSPemu compatibility framebuffer probe.
 * Diagnostic source only: no pixel memory access or display mutation.
 * Target: Vita firmware 3.60; user plugin loaded from *ALL.
 */
#include <vitasdk.h>
#include <taihen.h>
#include <psp2/display.h>
#include <psp2/appmgr.h>
#include "../common/psp_scope.h"
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
  unsigned int source_size, capture_size;
  unsigned int has_source, base, pitch, pixelformat, width, height, flags;
} ProbeRecord;

static tai_hook_ref_t g_hook_ref;
static SceUID g_hook_id = -1;
static ProbeRecord g_records[MAX_RECORDS];
static unsigned int g_record_count;
static unsigned int g_null_source_calls;
static char g_app_category[8];
static int g_category_read_result = -1;
static PSPSharpContentScope g_content_candidate = PSP_SHARP_SCOPE_UNKNOWN;

static int compat_set_framebuf_hook(
    int head, int index,
    const void *arg3, int sync,
    const void *source_param, const void *capture_param) {
  (void)arg3;

  /*
   * Firmware's six-argument SceDisplayUser wrapper passes source_param and
   * capture_param separately. Do not interpret arg3 as an options structure.
   */
  if (source_param == NULL) {
    g_null_source_calls++;
  } else if (g_record_count < MAX_RECORDS) {
    ProbeRecord *r = &g_records[g_record_count];
    memset(r, 0, sizeof(*r));
    r->head = (unsigned int)head;
    r->index = (unsigned int)index;
    r->sync = (unsigned int)sync;
    if (source_param != NULL && ((uintptr_t)source_param & 3u) == 0u) {
      const ProbeFrameBuf *fb = (const ProbeFrameBuf *)source_param;
      const unsigned int source_size = *(const unsigned int *)source_param;
      r->source_size = source_size;
      if (source_size >= sizeof(ProbeFrameBuf) &&
          source_size <= sizeof(ProbeFrameBufCompat) &&
          fb->size == source_size) {
        r->has_source = 1;
        r->base = (unsigned int)(uintptr_t)fb->base;
        r->pitch = fb->pitch;
        r->pixelformat = fb->pixelformat;
        r->width = fb->width;
        r->height = fb->height;
        if (source_size >= sizeof(ProbeFrameBufCompat)) {
          const ProbeFrameBufCompat *compat_fb =
              (const ProbeFrameBufCompat *)source_param;
          r->flags = compat_fb->flags;
        }
      }
    }
    if (capture_param != NULL && ((uintptr_t)capture_param & 3u) == 0u)
      r->capture_size = *(const unsigned int *)capture_param;
    g_record_count++;
  }

  return TAI_CONTINUE(int, g_hook_ref,
                      head, index, arg3, sync, source_param, capture_param);
}

static void write_report(void) {
  char report[2048];
  int used = 0;
  SceUID fd;
  unsigned int i;

  used += snprintf(report + used, sizeof(report) - used,
                   "PSP-Sharp-Vita read-only probe\n"
                   "Non-NULL source records captured: %u\n"
                   "Calls with NULL source: %u\n"
                   "No pixel memory was read or modified.\n\n",
                   g_record_count, g_null_source_calls);

  used += snprintf(report + used, sizeof(report) - used,
                   "AppMgr CATEGORY status=0x%08X bytes=%02X,%02X scope=%u\\n"
                   "WARNING: process category is NOT verified PSP game metadata.\\n\\n",
                   (unsigned int)g_category_read_result,
                   (unsigned char)g_app_category[0],
                   (unsigned char)g_app_category[1],
                   (unsigned int)g_content_candidate);

  for (i = 0; i < g_record_count && used < (int)sizeof(report); i++) {
    const ProbeRecord *r = &g_records[i];
    used += snprintf(report + used, sizeof(report) - used,
                     "Call %u\n head=%u index=%u sync=%u\n"
                     "source_size=0x%X capture_size=0x%X\n"
                     "has_source=%u base=0x%08X\n"
                     "pitch=%u format=0x%08X width=%u height=%u flags=0x%08X\n\n",
                     i, r->head, r->index, r->sync,
                     r->source_size, r->capture_size,
                     r->has_source, r->base,
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

  /* This first probe is deliberately restricted to 3.60 retail PSPemu. */
  if (info.module_nid != 0x2714F07D)
    return SCE_KERNEL_START_NO_RESIDENT;

  /*
   * Observe the current process SFO category without reading a Title ID.
   * It may be NPXS10028 metadata, not the actual PSP/PS1 EBOOT.PBP;
   * classify only as a candidate. Never enable a filter from this value.
   */
  memset(g_app_category, 0, sizeof(g_app_category));
  g_category_read_result = sceAppMgrAppParamGetString(
      0, 8, /* AppMgr CATEGORY parameter on firmware 3.60 */
      g_app_category, sizeof(g_app_category));
  if (g_category_read_result == 0)
    g_content_candidate = psp_sharp_classify_category(
        g_app_category, sizeof(g_app_category));

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
