#include "../system.h"
#include "../game.h"
#include "../constants.h"
#include "../memory.h"
#include <windows.h>
#include <wt/wt.h>

#define MAX_MUTEXES 512

typedef struct
{
  bool keys[SYS_KEYCODE_MAX];
  bool mouse_buttons[SYS_MOUSE_MAX];
  wt_vec2_t mouse_pos;
  wt_vec2_t mouse_delta;
  i32 mouse_wheel;
  bool mouse_captured;
} w32_input_sys_state_t;

typedef struct
{
  HWND hwnd;
  bool should_quit;

  w32_input_sys_state_t input, prev_input;

  wt_pool_t thread_info_pool;
  wt_pool_t critical_section_pool;

  WNDPROC wnd_proc;
} sys_state_t;

static sys_state_t *get_state(void)
{
  return game_get_state()->modules.sys;
}

static void incarcerate_the_mouse(void)
{
  sys_state_t *s = get_state();
  RECT client_rect;
  GetClientRect(s->hwnd, &client_rect);

  POINT center = {
    client_rect.left + (client_rect.right - client_rect.left) / 2,
    client_rect.top + (client_rect.bottom - client_rect.top) / 2
  };
  RECT prison_cell = { center.x, center.y, center.x, center.y };
  ClipCursor(&prison_cell);
}

static void the_mouse_is_not_guilty(void)
{
  ReleaseCapture();
  ClipCursor(NULL);
}

static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  sys_state_t *s = get_state();

  switch (msg)
  {
  case WM_DESTROY:
    s->should_quit = true;
    break;
  case WM_INPUT:
    if (s->input.mouse_captured)
    {
      HRAWINPUT ri = (HRAWINPUT)lparam;
      byte_t data[512];
      UINT size = sizeof(data);
      GetRawInputData(ri, RID_INPUT, &data, &size, sizeof(RAWINPUTHEADER));

      const RAWINPUT *raw_data = (const RAWINPUT*)data;
      if (raw_data->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE)
      {
        LONG new_x = raw_data->data.mouse.lLastX;
        LONG new_y = raw_data->data.mouse.lLastY;

        s->input.mouse_delta.x = new_x - s->input.mouse_pos.x;
        s->input.mouse_delta.y = new_y - s->input.mouse_pos.y;

        s->input.mouse_pos.x = new_x;
        s->input.mouse_pos.y = new_y;
      }
      else
      {
        s->input.mouse_delta.x = raw_data->data.mouse.lLastX;
        s->input.mouse_delta.y = raw_data->data.mouse.lLastY;
      }
    }
    break;
  case WM_LBUTTONDOWN:
    s->input.mouse_buttons[SYS_MOUSE_LEFT] = true;
    if (s->input.mouse_captured)
    {
      incarcerate_the_mouse();
    }
    break;
  case WM_LBUTTONUP:   s->input.mouse_buttons[SYS_MOUSE_LEFT]   = false; break;
  case WM_RBUTTONDOWN: s->input.mouse_buttons[SYS_MOUSE_RIGHT]  = true;  break;
  case WM_RBUTTONUP:   s->input.mouse_buttons[SYS_MOUSE_RIGHT]  = false; break;
  case WM_MBUTTONDOWN: s->input.mouse_buttons[SYS_MOUSE_MIDDLE] = true;  break;
  case WM_MBUTTONUP:   s->input.mouse_buttons[SYS_MOUSE_MIDDLE] = false; break;
  case WM_MOUSEWHEEL:
    s->input.mouse_wheel = GET_WHEEL_DELTA_WPARAM(wparam) / WHEEL_DELTA;
    break;

  // this is all a bunch of stupid shit just to get something that should have been
  // a single API call to work
  // seriously, this is such a common use-case for the mouse API that they should
  // have made easy.
  case WM_NCLBUTTONUP:
    if (s->input.mouse_captured)
    {
      incarcerate_the_mouse();
    }
    break;
  case WM_SETFOCUS:
    if (s->input.mouse_captured && LOWORD(lparam) == HTCLIENT)
    {
      incarcerate_the_mouse();
    }
    break;
  case WM_SETCURSOR:
    if (s->input.mouse_captured && LOWORD(lparam) == HTCLIENT)
    {
      SetCursor(NULL);
    }
    else
    {
      SetCursor(LoadCursor(NULL, IDC_ARROW));
    }
    break;
  default:
    return DefWindowProcW(hwnd, msg, wparam, lparam);
  }

  return 0;
}

void sys_init(void)
{
  game_state_t *gs = game_get_state();
  sys_state_t *s = gs->modules.sys = mem_hunk_push(sizeof(sys_state_t));

  HINSTANCE inst = GetModuleHandle(0);
  WNDCLASSW wnd_class = {
    .hInstance = inst,
    .lpfnWndProc = wnd_proc,
    .lpszClassName = TEXT(WINDOW_NAME),
    .hCursor = LoadCursorW(NULL, IDC_ARROW),
  };
  s->wnd_proc = wnd_proc;
  RegisterClassW(&wnd_class);

  s->hwnd = CreateWindowExW(0, TEXT(WINDOW_NAME), TEXT(WINDOW_NAME),
    WS_OVERLAPPEDWINDOW|WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
    NULL, NULL, inst, NULL);

  usize num_bytes = sizeof(CRITICAL_SECTION) * MAX_MUTEXES;
  void *pool_buffer = mem_hunk_push(num_bytes);
  s->critical_section_pool = wt_pool_new(pool_buffer, MAX_MUTEXES, sizeof(CRITICAL_SECTION));

  num_bytes = 2 * sizeof(void*) * 128;
  pool_buffer = mem_hunk_push(num_bytes);
  s->thread_info_pool = wt_pool_new(pool_buffer, 128, sizeof(void*) * 2);
}

HWND sys_win32_get_hwnd(void)
{
  sys_state_t *s = get_state();
  return s->hwnd;
}

bool sys_should_close(void)
{
  sys_state_t *s = get_state();
  return s->should_quit;
}

int sys_keycode_to_vk(sys_keycode_t k)
{
  if (k >= SYS_KEYCODE_A && k <= SYS_KEYCODE_Z)
  {
    return 'A' + (k - SYS_KEYCODE_A);
  }
  else switch (k)
  {
  case SYS_KEYCODE_SPACE:  return VK_SPACE;
  case SYS_KEYCODE_LSHIFT: return VK_LSHIFT;
  case SYS_KEYCODE_LCTRL:  return VK_LCONTROL;
  case SYS_KEYCODE_ESCAPE: return VK_ESCAPE;
  case SYS_KEYCODE_UP:     return VK_UP;
  case SYS_KEYCODE_DOWN:   return VK_DOWN;
  case SYS_KEYCODE_LEFT:   return VK_LEFT;
  case SYS_KEYCODE_RIGHT:  return VK_RIGHT;
  default: return 0;
  }
}

void sys_poll_events(void)
{
  // the address of wnd_proc might get moved around between reloads, so we
  // reset the WndProc if the pointer changes
  sys_state_t *s = get_state();
  if (s->wnd_proc != wnd_proc)
  {
    s->wnd_proc = wnd_proc;
    SetWindowLongPtrW(s->hwnd, GWLP_WNDPROC, (LONG_PTR)wnd_proc);
  }

  s->prev_input = s->input;

  s->input.mouse_delta.x = 0;
  s->input.mouse_delta.y = 0;
  s->input.mouse_wheel = 0;
  if (GetFocus() == s->hwnd)
  {
    for (u32 i = 0; i < SYS_KEYCODE_MAX; ++i)
    {
      int vk = sys_keycode_to_vk(i);
      s->input.keys[i] = (GetKeyState(vk) & 0x8000) ? true : false;
    }

    if (!s->input.mouse_captured)
    {
      POINT mouse_pos;
      GetCursorPos(&mouse_pos);
      ScreenToClient(s->hwnd, &mouse_pos);
      s->input.mouse_pos.x = mouse_pos.x;
      s->input.mouse_pos.y = mouse_pos.x;
    }
  }
  else
  {
    memset(s->input.keys, 0, sizeof(s->input.keys));
    s->input.mouse_delta.x = 0;
    s->input.mouse_delta.y = 0;
  }

  MSG msg;
  while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
  {
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }
}

wt_vec2_t sys_window_get_size(void)
{
  sys_state_t *s = get_state();
  RECT client_rect = { 0 };
  GetClientRect(s->hwnd, &client_rect);
  return wt_vec2(client_rect.right - client_rect.left, client_rect.bottom - client_rect.top);
}

sys_file_t sys_file_open(const char *filename, sys_file_access_t access)
{
  DWORD desired_access = 0;
  bool read = access & SYS_FILE_READ;
  bool write = access & SYS_FILE_WRITE;
  if (read)  { desired_access |= GENERIC_READ; }
  if (write) { desired_access |= GENERIC_WRITE; }

  DWORD creation_disposition = 0;
  if (write) { creation_disposition = CREATE_ALWAYS; }
  if (read)  { creation_disposition = OPEN_EXISTING; }

  HANDLE hdl = CreateFileA(filename, desired_access, FILE_SHARE_READ, NULL,
    creation_disposition, FILE_ATTRIBUTE_NORMAL, NULL);
  if (hdl == INVALID_HANDLE_VALUE)
    return 0;
  else
    return (void*)hdl;
}

usize sys_file_get_size(sys_file_t file)
{
  LARGE_INTEGER li = { 0 };
  GetFileSizeEx(file, &li);
  return li.QuadPart;
}

bool sys_file_read(sys_file_t file, void *buf, usize num_bytes)
{
  DWORD useless = 0;
  return ReadFile(file, buf, num_bytes, &useless, NULL);
}

sys_file_contents_t sys_file_read_to_scratch_buffer(const char *filename, bool null_terminator)
{
  sys_file_contents_t res = { 0 };
  sys_file_t file = sys_file_open(filename, SYS_FILE_READ);
  if (file)
  {
    res.size = sys_file_get_size(file);
    res.data = mem_scratch_push(res.size + (null_terminator ? 1 : 0));
    sys_file_read(file, res.data, res.size);
    sys_file_close(file);
  }
  return res;
}

bool sys_file_write(sys_file_t file, void *buf, usize num_bytes)
{
  DWORD useless = 0;
  return WriteFile(file, buf, num_bytes, &useless, NULL);
}

void sys_file_close(sys_file_t file)
{
  CloseHandle(file);
}

bool sys_key_down(sys_keycode_t k)
{
  sys_state_t *s = get_state();
  return s->input.keys[k];
}

bool sys_key_pressed(sys_keycode_t k)
{
  sys_state_t *s = get_state();
  return s->input.keys[k] && !s->prev_input.keys[k];
}

bool sys_key_released(sys_keycode_t k)
{
  sys_state_t *s = get_state();
  return !s->input.keys[k] && s->prev_input.keys[k];
}

void sys_mouse_capture(void)
{
  sys_state_t *s = get_state();
  SetCapture(s->hwnd);
//  ShowCursor(false);
  SetCursor(NULL);

  incarcerate_the_mouse();

  const RAWINPUTDEVICE rid = {
    0x01,   // HID_USAGE_PAGE_GENERIC
    0x02,   // HID_USAGE_GENERIC_MOUSE
    0,      // dwFlags
    s->hwnd // hwndTarget
  };
  RegisterRawInputDevices(&rid, 1, sizeof(rid));

  s->input.mouse_captured = true;
}

void sys_mouse_uncapture(void)
{
  sys_state_t *s = get_state();
  the_mouse_is_not_guilty();

  const RAWINPUTDEVICE rid = { 0x01, 0x02, RIDEV_REMOVE, NULL };
  RegisterRawInputDevices(&rid, 1, sizeof(rid));

  s->input.mouse_captured = false;
}

bool sys_mouse_down(sys_mouse_button_t b)
{
  sys_state_t *s = get_state();
  return s->input.mouse_buttons[b];
}

bool sys_mouse_pressed(sys_mouse_button_t b)
{
  sys_state_t *s = get_state();
  return s->input.mouse_buttons[b] && !s->prev_input.mouse_buttons[b];
}

bool sys_mouse_released(sys_mouse_button_t b)
{
  sys_state_t *s = get_state();
  return !s->input.mouse_buttons[b] && s->prev_input.mouse_buttons[b];
}

wt_vec2_t sys_mouse_get_pos(void)
{
  sys_state_t *s = get_state();
  return s->input.mouse_pos;
}

wt_vec2_t sys_mouse_get_delta(void)
{
  sys_state_t *s = get_state();
  return s->input.mouse_delta;
}

i32 sys_mouse_get_wheel(void)
{
  sys_state_t *s = get_state();
  return s->input.mouse_wheel;
}

u64 sys_get_performance_counter(void)
{
  LARGE_INTEGER li;
  QueryPerformanceCounter(&li);
  return li.QuadPart;
}

u64 sys_get_performance_frequency(void)
{
  LARGE_INTEGER li;
  QueryPerformanceFrequency(&li);
  return li.QuadPart;
}

u32 sys_cpu_get_num_cores(void)
{
  SYSTEM_INFO si = { 0 };
  GetSystemInfo(&si);
  return si.dwNumberOfProcessors;
}

typedef struct
{
  sys_thread_func_t fn;
  void *param;
} thread_info_t;

static unsigned long stupid_thread_func(void *param)
{
  thread_info_t *ti = (thread_info_t*)param;
  return ti->fn(ti->param);
}

sys_thread_t sys_thread_new(sys_thread_func_t fn, void *param)
{
  // todo: this is dumb
  thread_info_t *ti = malloc(sizeof(thread_info_t));
  ti->fn = fn;
  ti->param = param;

  HANDLE hdl = CreateThread(NULL, 0, stupid_thread_func, ti, 0, NULL);
  return hdl;
}

u32 sys_thread_get_id(sys_thread_t thread)
{
  return GetThreadId(thread);
}

u32 sys_thread_get_current_id(void)
{
  return GetCurrentThreadId();
}

void sys_thread_sleep(u32 ms)
{
  Sleep(ms);
}

void sys_thread_yield(void)
{
  SwitchToThread();
}

bool sys_thread_active(sys_thread_t thd)
{
  if (thd)
    return WaitForSingleObject(thd, 0) != WAIT_OBJECT_0;
  else
    return false;
}

void sys_thread_stop(sys_thread_t thd)
{
  TerminateThread(thd, 0);
}

sys_mutex_t sys_mutex_new(void)
{
  sys_state_t *s = get_state();
  CRITICAL_SECTION *cs = wt_pool_alloc(&s->critical_section_pool);
  InitializeCriticalSection(cs);
  return cs;
}

void sys_mutex_lock(sys_mutex_t mtx)
{
  WT_ASSERT(mtx);
  EnterCriticalSection(mtx);
}

bool sys_mutex_try_lock(sys_mutex_t mtx)
{
  WT_ASSERT(mtx);
  return TryEnterCriticalSection(mtx);
}

void sys_mutex_unlock(sys_mutex_t mtx)
{
  WT_ASSERT(mtx);
  LeaveCriticalSection(mtx);
}

void sys_mutex_free(sys_mutex_t mtx)
{
  sys_state_t *s = get_state();
  DeleteCriticalSection(mtx);
  wt_pool_free(&s->critical_section_pool, mtx);
}
