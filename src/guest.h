#ifndef GUEST_H
#define GUEST_H

#include <wt/wt.h>

#if WT_SYS_WIN32
#  define ESKE_EXPORT __declspec(dllexport)
#else
#  define ESKE_EXPORT
#endif

typedef enum
{
  GUEST_CMD_INIT,
  GUEST_CMD_TICK,
  GUEST_CMD_UNLOAD,
  GUEST_CMD_RELOAD,
} guest_cmd_t;

typedef struct
{
  int argc;
  char **argv;

  void *hunk;
} guest_params_t;

typedef bool (*guest_proc_t)(guest_params_t, guest_cmd_t);

ESKE_EXPORT bool guest(guest_params_t params, guest_cmd_t cmd);

#endif
