#ifndef GITI_CONFIG_H_
#define GITI_CONFIG_H_

#include <stdint.h>

#include "dlist.h"
#include "giti_color.h"

typedef char giti_config_key_strbuf_t[6];

typedef struct giti_config {
  struct {
    char*             user_name;
    char*             user_email;
    uint32_t          timeout;
  } general;
  struct {
    uint32_t          up;
    uint32_t          down;
    uint32_t          up_page;
    uint32_t          down_page;
    uint32_t          back;
    uint32_t          help;
    uint32_t          logs;
    uint32_t          branches;
    struct {
      uint32_t        filter;
      uint32_t        highlight;
      uint32_t        my;
      uint32_t        friends;
    } log;
    struct {
      uint32_t        info;
      uint32_t        files;
      uint32_t        show;
    } commit;
  } keybinding;
  struct {
    uint32_t          max_width_name;
  } format;
  dlist_t*            friends;
  giti_color_scheme_t color;
} giti_config_t;


giti_config_t*
giti_config_create(char* str_config, const char* user_name, const char* user_email);

char*
giti_config_to_string(const giti_config_t* config);

const char*
giti_config_key_to_string(uint32_t key, giti_config_key_strbuf_t strbuf);

#endif
