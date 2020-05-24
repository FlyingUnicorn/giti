
#define _GNU_SOURCE

#include "giti_config.h"

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "giti_log.h"
#include "giti_string.h"

#define STR_TIMEOUT    "general.timeout"
#define STR_USER_NAME  "general.user_name"
#define STR_USER_EMAIL "general.user_email"
#define STR_FRIENDS    "friends"
#define STR_UP         "keybinding.up"
#define STR_DOWN       "keybinding.down"
#define STR_BACK       "keybinding.back"
#define STR_HELP       "keybinding.help"

#define DEFAULT_TIMEOUT 500
#define DEFAULT_FRIENDS ""
#define DEFAULT_UP      'k'
#define DEFAULT_DOWN    'j'
#define DEFAULT_BACK    'q'
#define DEFAULT_HELP    '?'

giti_config_t config;

typedef enum op {
    OP_INVALID,
    VALUE,
    LIST,
} op_t;

typedef enum group {
    GROUP_INVALID,
    GENERAL,
    KEYBINDING,
    FRIENDS,
} group_t;

#define CONFIG \
  X(GENERAL,    STR_TIMEOUT,    VALUE, DEFAULT_TIMEOUT, &config.general.timeout)    \
  X(GENERAL,    STR_USER_NAME,  VALUE, NULL,            &config.general.user_name)  \
  X(GENERAL,    STR_USER_EMAIL, VALUE, NULL,            &config.general.user_email) \
  X(KEYBINDING, STR_UP,         VALUE, DEFAULT_UP,      &config.keybinding.up)      \
  X(KEYBINDING, STR_DOWN,       VALUE, DEFAULT_DOWN,    &config.keybinding.down)    \
  X(KEYBINDING, STR_BACK,       VALUE, DEFAULT_BACK,    &config.keybinding.back)    \
  X(KEYBINDING, STR_HELP,       VALUE, DEFAULT_HELP,    &config.keybinding.help)    \
  X(FRIENDS,    STR_FRIENDS,    LIST,  DEFAULT_FRIENDS, config.friends)             \

size_t
snprintf_char(char* buf, size_t buf_sz, const char* header, op_t op, void* ptr)
{
  assert(op == VALUE);

  return snprintf(buf, buf_sz, "%s: %c", header, *(char*)ptr);
}

size_t
snprintf_long(char* buf, size_t buf_sz, const char* header, op_t op, void* ptr)
{
  assert(op == VALUE);

  return snprintf(buf, buf_sz, "%s: %lu", header, *(long*)ptr);
}

size_t
snprintf_string(char* buf, size_t buf_sz, const char* header, op_t op, void* ptr)
{
  assert(op == VALUE);

  return snprintf(buf, buf_sz, "%s: %s", header, *(char**)ptr);
}

size_t
snprintf_data(char* buf, size_t buf_sz, const char* header, op_t op, void* ptr)
{
  size_t pos = 0;
  dlist_iterator_t* it = dlist_iterator_create((dlist_t*)ptr);

  const char* str_friend = NULL;
  while ((str_friend = dlist_iterator_next(it))) {
    pos += snprintf(buf + pos, buf_sz - pos, "%s: %s\n", header, str_friend);
  }
  dlist_iterator_destroy(it);

  return pos;
}

#define snprintf_param(ptr, buf, buf_sz, header, op) _Generic( (ptr), \
    char*:   snprintf_char(buf, buf_sz, header, op, ptr),             \
    long*:   snprintf_long(buf, buf_sz, header, op, ptr),             \
    char**:  snprintf_string(buf, buf_sz, header, op, ptr),           \
    default: snprintf_data(buf, buf_sz, header, op, ptr)              \
)

void
format_long(op_t op, const char* str, void* ptr)
{
  assert(op == VALUE);

  long* p = ptr;
  *p = atol(str);
}

void
format_char(op_t op, const char* str, void* ptr)
{
  assert(op == VALUE);

  char* p = ptr;
  *p = *str;
}

void
format_string(op_t op, const char* str, void* ptr)
{
  assert(op == VALUE);

  // todp nullcheck

  ptr = strdup(str);
}

void
format_data(op_t op, const char* str, void* ptr)
{
  (void)str;
  switch (op) {
    case LIST:
      dlist_append((dlist_t*)ptr, strdup(str));
      break;
    default:
      log("failed: %s", str);
      assert(false);
  }
}

#define set_param(ptr, op, str) _Generic( (ptr), \
    char*:   format_char(op, str, ptr),          \
    long*:   format_long(op, str, ptr),          \
    char**:  format_string(op, str, ptr),        \
    default: format_data(op, str, ptr)           \
)

static char*
giti_config_default_create()
{
    char* str_config = NULL;
    int len = asprintf(&str_config,
"# --== GITi Config ==--\n\
%s: %u                  \n\
                        \n\
# Keybinding            \n\
%-6s: %c                \n\
%-6s: %c                \n\
%-6s: %c                \n\
%-6s: %c                \n\
                        \n\
%s: %s",

STR_TIMEOUT,
DEFAULT_TIMEOUT,

STR_UP,
DEFAULT_UP,
STR_DOWN,
DEFAULT_DOWN,
STR_BACK,
DEFAULT_BACK,
STR_HELP,
DEFAULT_HELP,

STR_FRIENDS,
DEFAULT_FRIENDS);

    return len > 0 ? str_config : NULL;
}

char*
giti_config_get_value(const char* str) {

  char* str_value = strchr(str, ':');
   str_value += 1;
   //log_debug("str: <%s> val: <%s>", str_value, strtrimall(str_value));

   return strtrimall(str_value);
}



void
giti_config_line(const char* line)
{
  char value[128] = { 0 };
  size_t value_pos = 0;
  for (const char* ch = strchr(line, ':'); *ch; ++ch) {
    if (isspace(*ch) || *ch == ':') {
      continue;
    }
    value[value_pos++] = *ch;
  }

  if (!value_pos) {
    return;
  }

#define X(group, str, op, def, ptr)              \
    if (strncmp(str, line, strlen(str)) == 0) {  \
      set_param(ptr, op, value);                 \
    }

    CONFIG
#undef X
}

giti_config_t*
giti_config_create(char* str_config, const char* user_name, const char* user_email)
{
   config.friends = dlist_create(); // todo fix this
   config.general.user_name  = strdup(user_name);
   config.general.user_email = strdup(user_email);

    str_config = str_config ? str_config : giti_config_default_create();
    char* curline = str_config;
    while(curline) {
        char* nextLine = strchr(curline, '\n');
        if (nextLine) {
            *nextLine = '\0';
            nextLine += 1;
        }

        if (*curline == '#' || isspace(*curline)) {
            goto next;
        }

        giti_config_line(curline);
next:
        curline = nextLine;
    }

    return &config;
}

void
giti_config_print(const giti_config_t* config_)
{
  (void)config_;

  char pos = 0;
  char buffer[512];

  group_t last_group = GROUP_INVALID;
  log("--== GITi Config ==--");
#define X(group, str, op, def, ptr)                                  \
  if (group != last_group) {                                         \
    switch(group) {                                                  \
    case GENERAL:                                                    \
      log("-= General =-");                                          \
      break;                                                         \
    case KEYBINDING:                                                 \
      log("\n-= Keybinding =-");                                     \
      break;                                                         \
    case FRIENDS:                                                    \
      log("\n-= Friends =-");                                        \
      break;                                                         \
    default:                                                         \
      assert(false);                                                 \
    }                                                                \
    last_group = group;                                              \
  }                                                                  \
                                                                     \
  snprintf_param(ptr, buffer + pos, sizeof(buffer) - pos, str, op);  \
  log("%s", buffer); \

  CONFIG
#undef X
}

char*
giti_config_to_string(const giti_config_t* config_)
{
  (void)config_;

  size_t buf_sz = 4096;
  size_t written = 0;
  char* str = calloc(1, buf_sz);

  group_t last_group = GROUP_INVALID;
  log("--== GITi Config ==--");
#define X(group, header, op, def, ptr)                                              \
  if (group != last_group) {                                                        \
    switch(group) {                                                                 \
    case GENERAL:                                                                   \
      written += snprintf(str + written, buf_sz - written, "-= General =-\n");      \
      break;                                                                        \
    case KEYBINDING:                                                                \
      written += snprintf(str + written, buf_sz - written, "\n-= Keybinding =-\n"); \
      break;                                                                        \
    case FRIENDS:                                                                   \
      written += snprintf(str + written, buf_sz - written, "\n-= Friends =-\n");    \
      break;                                                                        \
    default:                                                                        \
      assert(false);                                                                \
    }                                                                               \
    last_group = group;                                                             \
  }                                                                                 \
                                                                                    \
  written += snprintf_param(ptr, str + written, buf_sz - written, header, op);      \
  written += snprintf(str + written, buf_sz - written, "\n");                       \

  CONFIG
#undef X

 return str;
}
