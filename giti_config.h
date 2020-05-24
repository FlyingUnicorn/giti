#ifndef GITI_CONFIG_H_
#define GITI_CONFIG_H_

#include "dlist.h"

typedef struct giti_config {
  struct {
    char* user_name;
    char* user_email;
    long  timeout;
  } general;
  struct {
    char up;
    char down;
    char back;
  } navigation;
  dlist_t* friends;
} giti_config_t;


giti_config_t*
giti_config_create(char* str_config, const char* user_name, const char* user_email);

void
giti_config_print(const giti_config_t* config);

#endif
