#include "common.h"

#define NAME(key) \
  [_KEY_##key] = #key,

static const char *keyname[256] __attribute__((used)) = {
  [_KEY_NONE] = "NONE",
  _KEYS(NAME)
};

void switch_game();

size_t events_read(void *buf, size_t len) {
  int key = _read_key();
  bool down = false;
  if (key == _KEY_NONE)
    sprintf((char *)buf, "t %u\n", _uptime());
  else {
    if (key & 0x8000) {
      key ^= 0x8000;
      down = true;
    }
    // press KEY_F12 to switch game
    if (down && key == _KEY_F12)
      switch_game();
    sprintf((char *)buf, "k%c %s\n", (down ? 'd' : 'u'), keyname[key]);
  } 
  return strlen((char *)buf);
}

static char dispinfo[128] __attribute__((used));

void dispinfo_read(void *buf, off_t offset, size_t len) {
  memcpy(buf, dispinfo + offset, len);
}

//extern uint32_t* const fb;
void fb_write(const void *buf, off_t offset, size_t len) {
  int x, y;
  assert(offset % 4 == 0 && len % 4 == 0);
  offset /= sizeof(uint32_t);
  len /= sizeof(uint32_t);
  y = offset / _screen.width;
  x = offset % _screen.width;  
  _draw_rect((const uint32_t *)buf, x, y, len, 1);
  //memcpy((char *)fb + offset, buf, len);
}

void init_device() {
  _ioe_init();
  sprintf(dispinfo, "WIDTH:%d\nHEIGHT:%d\n", _screen.width, _screen.height);
}
