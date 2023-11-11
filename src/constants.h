#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <wt/wt.h>

#if WT_SYS_WIN32
#  define GUEST_NAME "build/guest.dll"
#  define GUEST_TEMP_NAME "build/guest-temp.dll"
#elif WT_SYS_MACOS
#  define GUEST_NAME "build/guest.dylib"
#  define GUEST_TEMP_NAME "build/guest-temp.dylib"
#elif WT_SYS_LINUX
#  define GUEST_NAME "build/guest.so"
#  define GUEST_TEMP_NAME "build/guest-temp.so"
#endif

// todo: configure this as needed
#if WT_DEBUG
#  define HUNK_SIZE WT_GIGABYTES(4)
#else
#  define HUNK_SIZE WT_GIGABYTES(4)
#endif

#define CHUNK_SIZE_X 16
#define CHUNK_SIZE_Y 256
#define CHUNK_SIZE_Z 16

#define CHUNK_NUM_BLOCKS (CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z)

#define WINDOW_NAME "voxel game"

#endif
