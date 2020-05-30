#ifndef GITI_CONFIG_H_
#define GITI_CONFIG_H_

#include <stdint.h>

#include "dlist.h"

typedef struct giti_config {
  struct {
    char* user_name;
    char* user_email;
    long  timeout;
  } general;
  struct {
    uint32_t up;
    uint32_t down;
    uint32_t back;
    uint32_t help;
    struct {
      uint32_t filter;
      uint32_t highlight;
      uint32_t my;
      uint32_t friends;
    } log;
    struct {
      uint32_t info;
      uint32_t files;
      uint32_t show;
    } commit;
  } keybinding;
  dlist_t* friends;
} giti_config_t;


giti_config_t*
giti_config_create(char* str_config, const char* user_name, const char* user_email);

char*
giti_config_to_string(const giti_config_t* config);

#endif
