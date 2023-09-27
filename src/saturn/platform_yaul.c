#include <yaul.h>
#include <string.h>

//Hack for yaul missing declaration
extern int strcasecmp(const char *_l, const char *_r);

#include "system.h"
#include "mem.h"
#include "input.h"
#include "render_vdp2.h"

extern void mem_init(void);

static cdfs_filelist_t _filelist;

static void setup_fs(void) {
  cdfs_filelist_entry_t * const filelist_entries = cdfs_entries_alloc(-1);
  assert(filelist_entries != NULL);
  cdfs_filelist_default_init(&_filelist, filelist_entries, -1);
}

static void controller_update(void) {
  LOGD("Update Controller\n");
  smpc_peripheral_digital_t _digital;
  smpc_peripheral_process(); //A chaque process
  smpc_peripheral_digital_port(1, &_digital);

  if(_digital.held.button.start) LOGD("START\n");
  if(_digital.held.button.up) LOGD("UP\n");
  if(_digital.held.button.down) LOGD("DOWN\n");

  input_set_button_state(INPUT_GAMEPAD_DPAD_UP, _digital.held.button.up);
  input_set_button_state(INPUT_GAMEPAD_DPAD_DOWN, _digital.held.button.down);
  input_set_button_state(INPUT_GAMEPAD_DPAD_LEFT, _digital.held.button.left);
  input_set_button_state(INPUT_GAMEPAD_DPAD_RIGHT, _digital.held.button.right);
  input_set_button_state(INPUT_GAMEPAD_START, _digital.held.button.start);
  input_set_button_state(INPUT_GAMEPAD_A, _digital.held.button.b);
  input_set_button_state(INPUT_GAMEPAD_B, _digital.held.button.a);
  input_set_button_state(INPUT_GAMEPAD_X, _digital.held.button.y);
  input_set_button_state(INPUT_GAMEPAD_Y, _digital.held.button.x);
  input_set_button_state(INPUT_GAMEPAD_L_SHOULDER, _digital.held.button.l);
  input_set_button_state(INPUT_GAMEPAD_R_SHOULDER, _digital.held.button.r);
}

void main(void) {
  //Init Saturn
  mem_init();
  LOGD("Init filesystem access\n");
  setup_fs();
  //Init game engine
  LOGD("Init Game\n");
  system_init();

  while (true) {
    //Update controller
    controller_update();

    LOGD("Update Game\n");
    system_update();
    vdp2_video_sync();
  }

  system_cleanup();
}

static void _vblank_out_handler(void *work __unused)
{
  smpc_peripheral_intback_issue();
}

void user_init(void)
{
        smpc_peripheral_init();
        vdp_sync_vblank_out_set(_vblank_out_handler, NULL);
}

void platform_exit(void) {
}

vec2i_t platform_screen_size(void) {
  vec2i_t ret = {.x=0, .y=0};
  switch (SCREEN_MODE) {
    case VDP2_TVMD_HORZ_NORMAL_B:
      switch(SCREEN_SIZE) {
        case VDP2_TVMD_VERT_240:
        default:
        ret.x = 352;
        ret.y = 240;
      }
    case VDP2_TVMD_HORZ_NORMAL_A:
    default:
      switch(SCREEN_SIZE) {
        case VDP2_TVMD_VERT_240:
        default:
        ret.x = 320;
        ret.y = 240;
      }
  }
  return ret;
}

double platform_now(void) {
  const smpc_time_t * const time = smpc_rtc_time_get();
  smpc_time_dec_t time_dec;
  smpc_peripheral_process(); //A chaque process
  smpc_rtc_time_bcd_from(time, &time_dec);
  uint32_t current = ((uint32_t)time_dec.hours * 3600UL) + ((uint32_t)time_dec.minutes * 60UL) + (uint32_t)time_dec.seconds;
  /* Use SMPC RTC date as a 32-bit seed for the default PRNG */
  LOGD("Return time %d\n", current);
  return current;
}

bool platform_get_fullscreen(void){
  return true;
}

void platform_set_fullscreen(bool fullscreen __attribute__((unused))) {
//Always fullscreen on saturn
  LOGD("Set fullscreen\n");
};

void platform_set_audio_mix_cb(void (*cb)(float *buffer, uint32_t len)) {
//TODO
}

static void split_path_file(char** p, char** f, const char *pf) {
  char *next;
  next = strpbrk(pf + 1, "\\/");
  if (next != NULL) {
    *p = strndup(pf, next - pf);
    next++;
    *f = strdup(next);
    next++;
  } else {
    *f = (char *)pf;
    *p = NULL;
  }
}

static cdfs_filelist_entry_t *getFileEntry(const char *name, cdfs_filelist_t *fl) {
  for (uint32_t i = 0; i<fl->entries_count; i++) {
    if (strcasecmp (name, fl->entries[i].name) == 0) {
      return &fl->entries[i];
    }
  }
  return NULL;
}

static cdfs_filelist_entry_t* getEntry(const char *file) {
  char *path = NULL;
  char *name = NULL;
  cdfs_filelist_entry_t *file_entry;
  cdfs_filelist_root_read(&_filelist);
  split_path_file(&path, &name, file);
  while (path != NULL) {
    char *remaining = name;
    file_entry = getFileEntry(path, &_filelist);
    cdfs_filelist_read(&_filelist, *file_entry);
    free(path);
    assert (file_entry != NULL);
    assert (file_entry->type == CDFS_ENTRY_TYPE_DIRECTORY);
    split_path_file(&path, &name, remaining);
    if (name != remaining) free(remaining);
  }
  // Name is the searched entry
  file_entry = getFileEntry(name, &_filelist);
  assert (file_entry != NULL);
  assert (file_entry->type == CDFS_ENTRY_TYPE_FILE);
  free (name);
  return file_entry;
}

uint8_t *platform_load_asset(const char *name, uint32_t *bytes_read) {
  LOGD("Load asset %s\n", name);
  int ret __unused;
  cdfs_filelist_entry_t *file_entry = getEntry(name);
  uint8_t *bytes = (uint8_t *)mem_temp_alloc(file_entry->size);
  assert (bytes != NULL);
  ret = cd_block_sectors_read(file_entry->starting_fad, (void *)bytes, file_entry->size);
  assert(ret == 0);
  *bytes_read = file_entry->size;
  return bytes;
}

uint8_t *platform_load_userdata(const char *name, uint32_t *bytes_read) {
  //load to backup
  return NULL;
}

uint32_t platform_store_userdata(const char *name, void *bytes, int32_t len) {
  //save to backup
  return 0;
}