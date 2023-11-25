#ifndef SYSTEM_H
#define SYSTEM_H

#include <wt/wt.h>

void sys_init(void);

bool sys_should_close(void);
void sys_poll_events(void);

wt_vec2_t sys_window_get_size(void);

// === file i/o ===
typedef void *sys_file_t;

typedef enum
{
  SYS_FILE_READ  = (1 << 0),
  SYS_FILE_WRITE = (1 << 1),
} sys_file_access_t;

typedef struct
{
  byte_t *data;
  usize size;
} sys_file_contents_t;

sys_file_t          sys_file_open(const char *filename, sys_file_access_t access);
usize               sys_file_get_size(sys_file_t file);
bool                sys_file_read(sys_file_t file, void *buf, usize num_bytes);
sys_file_contents_t sys_file_read_to_scratch_buffer(const char *filename, bool null_terminator);
bool                sys_file_write(sys_file_t file, void *buf, usize num_bytes);
void                sys_file_close(sys_file_t file);

// === user input ===

// todo: needs more
typedef enum
{
  SYS_KEYCODE_A, SYS_KEYCODE_B, SYS_KEYCODE_C, SYS_KEYCODE_D, SYS_KEYCODE_E,
  SYS_KEYCODE_F, SYS_KEYCODE_G, SYS_KEYCODE_H, SYS_KEYCODE_I, SYS_KEYCODE_J,
  SYS_KEYCODE_K, SYS_KEYCODE_L, SYS_KEYCODE_M, SYS_KEYCODE_N, SYS_KEYCODE_O,
  SYS_KEYCODE_P, SYS_KEYCODE_Q, SYS_KEYCODE_R, SYS_KEYCODE_S, SYS_KEYCODE_T,
  SYS_KEYCODE_U, SYS_KEYCODE_V, SYS_KEYCODE_W, SYS_KEYCODE_X, SYS_KEYCODE_Y,
  SYS_KEYCODE_Z,

  SYS_KEYCODE_SPACE, SYS_KEYCODE_LSHIFT, SYS_KEYCODE_LCTRL,
  SYS_KEYCODE_ESCAPE,

  SYS_KEYCODE_UP, SYS_KEYCODE_DOWN, SYS_KEYCODE_LEFT, SYS_KEYCODE_RIGHT,

  SYS_KEYCODE_MAX,
} sys_keycode_t;

typedef enum
{
  SYS_MOUSE_LEFT,
  SYS_MOUSE_RIGHT,
  SYS_MOUSE_MIDDLE,

  SYS_MOUSE_MAX,
} sys_mouse_button_t;

bool sys_key_down(sys_keycode_t k);
bool sys_key_pressed(sys_keycode_t k);
bool sys_key_released(sys_keycode_t k);

void sys_mouse_capture(void);
void sys_mouse_uncapture(void);

bool sys_mouse_down(sys_mouse_button_t b);
bool sys_mouse_pressed(sys_mouse_button_t b);
bool sys_mouse_released(sys_mouse_button_t b);

wt_vec2_t sys_mouse_get_pos(void);
wt_vec2_t sys_mouse_get_delta(void);
i32       sys_mouse_get_wheel(void);

// === timing ===
u64 sys_get_performance_counter(void);
u64 sys_get_performance_frequency(void);

// === threads and mutexes and shit ===

typedef void *sys_thread_t;
typedef u32 (*sys_thread_func_t)(void *param);

// todo: maybe a cpu module for processor info?
u32          sys_cpu_get_num_cores(void);

sys_thread_t sys_thread_new(sys_thread_func_t fn, void *param);
u32          sys_thread_get_id(sys_thread_t thread);
u32          sys_thread_get_current_id(void);
void         sys_thread_sleep(u32 ms);
void         sys_thread_yield(void);
bool         sys_thread_active(sys_thread_t thd);
void         sys_thread_stop(sys_thread_t thd);

typedef void *sys_mutex_t;

sys_mutex_t  sys_mutex_new(void);
void         sys_mutex_lock(sys_mutex_t mtx);
bool         sys_mutex_try_lock(sys_mutex_t mtx);
void         sys_mutex_unlock(sys_mutex_t mtx);
void         sys_mutex_free(sys_mutex_t mtx);

#endif
