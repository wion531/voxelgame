#include <windows.h>
#include <wt/wt.h>
#include "../constants.h"
#include "../guest.h"
#include <stdio.h>

typedef struct
{
  HMODULE module;
  guest_params_t params;
  FILETIME last_write_time;
  guest_proc_t proc;
} guest_t;

static struct
{
  guest_t guest;
} state;

static FILETIME file_last_write_time(const char *filename)
{
  // todo: should this be createfilew?
  HANDLE hdl = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE,
    NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  WT_ASSERT(hdl && "file not found");

  FILETIME last_write_time = { 0 };
  GetFileTime(hdl, NULL, NULL, &last_write_time);
  CloseHandle(hdl);
  return last_write_time;
}

static void stupid_pdb_hack(void)
{
  FILE *fp = fopen(GUEST_TEMP_NAME, "rb+");
  if (fp)
  {
    fseek(fp, 0x3c, SEEK_SET);
    u32 signature_offset = 0;
    fread(&signature_offset, 4, 1, fp);
    fseek(fp, signature_offset, SEEK_SET);

    u32 magic = 0;
    fread(&magic, 4, 1, fp);
    WT_ASSERT(magic == 0x00004550);

    IMAGE_FILE_HEADER header;
    fread(&header, sizeof(header), 1, fp);
    WT_ASSERT(header.SizeOfOptionalHeader > 0);

    IMAGE_OPTIONAL_HEADER64 opt_header;
    fread(&opt_header, sizeof(opt_header), 1, fp);
    WT_ASSERT(opt_header.NumberOfRvaAndSizes > 0);

    IMAGE_DATA_DIRECTORY data_dir =
      opt_header.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];

    IMAGE_SECTION_HEADER section_header;
    for (int i = 0; i < header.NumberOfSections; ++i)
    {
      fread(&section_header, sizeof(section_header), 1, fp);
      if (section_header.VirtualAddress > data_dir.VirtualAddress)
      {
        fseek(fp, -(i64)sizeof(IMAGE_SECTION_HEADER) * 2, SEEK_CUR);
        fread(&section_header, sizeof(section_header), 1, fp);
        break;
      }
    }
    i64 actual_offset = (i64)data_dir.VirtualAddress -
                        (i64)section_header.VirtualAddress +
                        (i64)section_header.PointerToRawData;
    fseek(fp, (long)actual_offset, SEEK_SET);

    IMAGE_DEBUG_DIRECTORY debug_dir;
    fread(&debug_dir, sizeof(debug_dir), 1, fp);

    typedef struct
    {
      u32 cv_signature;
      GUID guid;
      u32 age;
    } codeview_info_t;

    codeview_info_t cv_info;
    fseek(fp, debug_dir.PointerToRawData, SEEK_SET);
    fread(&cv_info, sizeof(cv_info), 1, fp);

    char pdb_filename[1024] = {0};
    long old_pos = ftell(fp);
    fgets(pdb_filename, 1024, fp);
    fseek(fp, old_pos, SEEK_SET);

    char *pdb_ext = strstr(pdb_filename, ".pdb");
    if (pdb_ext)
    {
      ptrdiff_t offset = (ptrdiff_t)pdb_ext - (ptrdiff_t)pdb_filename;
      fseek(fp, (long)offset, SEEK_CUR);
      fwrite("-temp.pdb", 1, 10, fp);
    }

    fclose(fp);
  }
}

static bool guest_proc_stub(guest_params_t params, guest_cmd_t cmd)
{
  // todo: maybe some kind of special error screen?
  WT_UNUSED(params);
  WT_UNUSED(cmd);
  return false;
}

static void guest_reload(void)
{
  guest_t *g = &state.guest;
  if (g->proc)
  {
    g->proc(g->params, GUEST_CMD_UNLOAD);
  }
  
  if (g->module)
    FreeLibrary(g->module);

  // copy guest.dll to guest-temp.dll
  for (;;)
  {
    bool success = CopyFileW(TEXT(GUEST_NAME), TEXT(GUEST_TEMP_NAME), false);
    if (!success)
    {
      DWORD error = GetLastError();
      if (error != 0x5)
      {
        break;
      }
    }
    else
    {
      break;
    }
  }
  CopyFileW(L"build/guest.pdb", L"build/guest-temp.pdb", false);
  stupid_pdb_hack();

  g->module = LoadLibraryW(TEXT(GUEST_TEMP_NAME));
  if (g->module)
  {
    g->proc = (guest_proc_t)GetProcAddress(g->module, "guest");
    if (g->proc)
      g->proc(g->params, GUEST_CMD_RELOAD);  
  }
  if (!g->proc)
    g->proc = guest_proc_stub;
}

static void guest_watch(void)
{
  FILETIME lwt = file_last_write_time(GUEST_NAME);
  if (CompareFileTime(&lwt, &state.guest.last_write_time) != 0)
  {
    guest_reload();
    state.guest.last_write_time = lwt;
  }
}

static int eske_main(int argc, char *argv[])
{
  guest_t *g = &state.guest;
  g->params.argc = argc;
  g->params.argv = argv;

  g->params.hunk = VirtualAlloc(NULL, HUNK_SIZE, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
  WT_ASSERT(g->params.hunk && "failed to allocate game memory");
  
  guest_reload();

  g->proc(g->params, GUEST_CMD_INIT);
  while (g->proc(g->params, GUEST_CMD_TICK))
  {
    guest_watch();
  }

  return 0;
}

// todo: make this configurable
#if 1
int main(int argc, char *argv[])
{
  return eske_main(argc, argv);
}
#else
int CALLBACK WinMain(HINSTANCE inst, HINSTANCE prev_inst, LPSTR cmd_line, INT show_code)
{
  WT_UNUSED(inst);
  WT_UNUSED(prev_inst);
  WT_UNUSED(cmd_line);
  WT_UNUSED(show_code);

  // todo: parse command line arguments
  return eske_main(0, NULL);
}
#endif
