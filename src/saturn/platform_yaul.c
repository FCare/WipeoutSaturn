#include <yaul.h>
#include <string.h>

//Hack for yaul missing declaration
extern int strcasecmp(const char *_l, const char *_r);

#include "system.h"
#include "mem.h"
#include "input.h"
#include "render_vdp2.h"

#define CPU_FRT_INTERRUPT_PRIORITY_LEVEL 8

extern void mem_init(void);

static cdfs_filelist_t _filelist;

static int16_t nb_ovf = 0;
static fix16_t ovf_time_step;

static void
_frt_compare_output_handler(void)
{
        uint16_t frt_count = cpu_frt_count_get();

        int16_t count_diff __unused;
        count_diff = frt_count - (CPU_FRT_NTSC_320_128_COUNT_1MS*300);

        if (count_diff >= 0) {
                cpu_frt_count_set(count_diff);
        }
        nb_ovf++;
}

fix16_t platform_now(void) {
  //Need to use th FRT
  // 0.000004768 * 16 /
  // At div 128, FRT is 0.000004768×16 = 5 in FIX16 So lets use a div of 16 for counter
  uint16_t frt_count = cpu_frt_count_get();
  return fix16_int16_mul(ovf_time_step, nb_ovf) + (cpu_frt_count_get()>>4)*5;
}

static void timer_init(void)
{
  //Mode 0 26MHz  26.8465875 MHz (master clock x2, minus 3.579545/2) => 37 ns/Tick
  //Mode 1 28MHz 28.63636 MHz (master clock x2)

  //divider 128 => 4.768 µs/ticks
  //CPU_FRT_NTSC_320_128_COUNT_1MS = 0xD2 => 210 ticks = 0,99456ms
  nb_ovf = FIX16_ZERO;
  cpu_frt_init(CPU_FRT_CLOCK_DIV_128);
  cpu_frt_oca_set(CPU_FRT_NTSC_320_128_COUNT_1MS*300, _frt_compare_output_handler);
  //Will trigger every 0,99456ms * 300 = 0,298368s
  cpu_frt_count_set(0);
  cpu_frt_interrupt_priority_set(CPU_FRT_INTERRUPT_PRIORITY_LEVEL);
  ovf_time_step = FIX16(0.298368);
}

static void setup_fs(void) {
  cdfs_filelist_entry_t * const filelist_entries = cdfs_entries_alloc(-1);
  assert(filelist_entries != NULL);
  cdfs_filelist_default_init(&_filelist, filelist_entries, -1);
}

static void controller_update(void) {
  smpc_peripheral_digital_t ctrl_state;
  LOGD("Update Controller\n");
  smpc_peripheral_digital_port(1, &ctrl_state);
  smpc_peripheral_intback_issue();

  if(ctrl_state.pressed.button.start) LOGD("START\n");
  if(ctrl_state.pressed.button.up) LOGD("UP\n");
  if(ctrl_state.pressed.button.down) LOGD("DOWN\n");

  input_set_button_state(INPUT_GAMEPAD_DPAD_UP, ctrl_state.pressed.button.up);
  input_set_button_state(INPUT_GAMEPAD_DPAD_DOWN, ctrl_state.pressed.button.down);
  input_set_button_state(INPUT_GAMEPAD_DPAD_LEFT, ctrl_state.pressed.button.left);
  input_set_button_state(INPUT_GAMEPAD_DPAD_RIGHT, ctrl_state.pressed.button.right);
  input_set_button_state(INPUT_GAMEPAD_START, ctrl_state.pressed.button.start);
  input_set_button_state(INPUT_GAMEPAD_A, ctrl_state.pressed.button.b);
  input_set_button_state(INPUT_GAMEPAD_B, ctrl_state.pressed.button.a);
  input_set_button_state(INPUT_GAMEPAD_X, ctrl_state.pressed.button.y);
  input_set_button_state(INPUT_GAMEPAD_Y, ctrl_state.pressed.button.x);
  input_set_button_state(INPUT_GAMEPAD_L_SHOULDER, ctrl_state.pressed.button.l);
  input_set_button_state(INPUT_GAMEPAD_R_SHOULDER, ctrl_state.pressed.button.r);

}

void main(void) {
  //Init Saturn
  timer_init();
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

    vdp2_sync();
    vdp1_sync();
    vdp2_sync_wait();
    vdp1_sync_wait();
    vdp2_video_sync();
  }

  system_cleanup();
}

static void _vblank_out_handler(void *work __unused)
{
    smpc_peripheral_process(); //A chaque process
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
      break;
    case VDP2_TVMD_HORZ_NORMAL_A:
    default:
      switch(SCREEN_SIZE) {
        case VDP2_TVMD_VERT_240:
        default:
        ret.x = 320;
        ret.y = 240;
      }
      break;
  }
  return ret;
}

bool platform_get_fullscreen(void){
  return true;
}

void platform_set_fullscreen(bool fullscreen __attribute__((unused))) {
//Always fullscreen on saturn
  LOGD("Set fullscreen\n");
};

void platform_set_audio_mix_cb(void (*cb)(fix16_t *buffer, uint32_t len)) {
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