#ifndef GITI_CONFIG_H_
#define GITI_CONFIG_H_

#include <stdbool.h>
#include <stdint.h>

#include "dlist.h"
#include "giti_color.h"

typedef char giti_config_key_strbuf_t[6];

typedef struct giti_keybinding {
  char     ch;
  bool     ctrl;
  bool     meta;
  uint32_t wch;
} giti_keybinding_t;

typedef struct giti_config {
  struct {
    char*               user_name;
    char*               user_email;
    uint32_t            timeout;
  } general;
  struct {
    giti_keybinding_t   up;
    giti_keybinding_t   down;
    giti_keybinding_t   up_page;
    giti_keybinding_t   down_page;
    giti_keybinding_t   back;
    giti_keybinding_t   help;
    giti_keybinding_t   logs;
    giti_keybinding_t   branches;
    struct {
      giti_keybinding_t filter;
      giti_keybinding_t highlight;
      giti_keybinding_t my;
      giti_keybinding_t friends;
    } log;
    struct {
      giti_keybinding_t info;
      giti_keybinding_t files;
      giti_keybinding_t show;
    } commit;
  } keybinding;
  struct {
    uint32_t            max_width_name;
  } format;
  dlist_t*              friends;
  giti_color_scheme_t   color;
} giti_config_t;


giti_config_t*
giti_config_create(char* str_config, const char* user_name, const char* user_email);

char*
giti_config_to_string(const giti_config_t* config);

const char*
giti_config_key_to_string(uint32_t key, giti_config_key_strbuf_t strbuf);

#endif
