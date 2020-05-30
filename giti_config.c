
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

giti_config_t config;

typedef enum op {
    OP_INVALID,
    VALUE,
    LIST,
    KEY,
} op_t;

typedef enum group {
    GROUP_INVALID,
    GENERAL,
    KEYBINDING,
    FRIENDS,
} group_t;

#define CONFIG \
  X(GENERAL,    "general.timeout",          VALUE, 500,  config.general.timeout)          \
  X(GENERAL,    "general.user_name",        VALUE, NULL, config.general.user_name)        \
  X(GENERAL,    "general.user_email",       VALUE, NULL, config.general.user_email)       \
  X(KEYBINDING, "keybinding.up",            KEY,   "k",  config.keybinding.up)            \
  X(KEYBINDING, "keybinding.down",          KEY,   "j",  config.keybinding.down)          \
  X(KEYBINDING, "keybinding.back",          KEY,   "q",  config.keybinding.back)          \
  X(KEYBINDING, "keybinding.help",          KEY,   "?",  config.keybinding.help)          \
  X(KEYBINDING, "keybinding.log.filter",    KEY,   "s",  config.keybinding.log.filter)    \
  X(KEYBINDING, "keybinding.log.highlight", KEY,   "S",  config.keybinding.log.highlight) \
  X(KEYBINDING, "keybinding.log.my",        KEY,   "m",  config.keybinding.log.my)        \
  X(KEYBINDING, "keybinding.log.friends",   KEY,   "M",  config.keybinding.log.friends)   \
  X(KEYBINDING, "keybinding.commit.info",   KEY,   "i",  config.keybinding.commit.info)   \
  X(KEYBINDING, "keybinding.commit.files",  KEY,   "f",  config.keybinding.commit.files)  \
  X(KEYBINDING, "keybinding.commit.show",   KEY,   "d",  config.keybinding.commit.show)   \
  X(FRIENDS,    "friends",                  LIST,  NULL, config.friends)                  \

size_t
snprintf_char(char* buf, size_t buf_sz, const char* header, op_t op, char val)
{
  assert(op == VALUE);

  return snprintf(buf, buf_sz, "%s: %c", header, val);
}

size_t
snprintf_long(char* buf, size_t buf_sz, const char* header, op_t op, long val)
{
  assert(op == VALUE);

  return snprintf(buf, buf_sz, "%s: %ld", header, val);
}

size_t
snprintf_uint(char* buf, size_t buf_sz, const char* header, op_t op, unsigned int val)
{
  assert(op == VALUE || op == KEY);

  return snprintf(buf, buf_sz, "%s: %u", header, val);
}

size_t
snprintf_string(char* buf, size_t buf_sz, const char* header, op_t op, char* val)
{
  assert(op == VALUE || op == KEY);

  return snprintf(buf, buf_sz, "%s: %s", header, val);
}

size_t
snprintf_data(char* buf, size_t buf_sz, const char* header, op_t op, void* data)
{
  if (!data) {
    return 0;
  }
  size_t            pos = 0;
  dlist_iterator_t* it  = dlist_iterator_create((dlist_t*)data);

  const char* str_friend = NULL;
  while ((str_friend = dlist_iterator_next(it))) {
    pos += snprintf(buf + pos, buf_sz - pos, "%s: %s\n", header, str_friend);
  }
  dlist_iterator_destroy(it);

  return pos;
}

#define snprintf_param(ptr, buf, buf_sz, header, op) _Generic( (ptr), \
    char:         snprintf_char,                                      \
    int:          snprintf_long,                                      \
    unsigned int: snprintf_uint,                                      \
    long:         snprintf_long,                                      \
    char*:        snprintf_string,                                    \
    default:      snprintf_data                                       \
) (buf, buf_sz, header, op, ptr)

void
format_uint(op_t op, const char* str, void* ptr)
{
  unsigned int* p = ptr;
  switch (op) {
    case VALUE:
      *p = strtoul(str, NULL, 10);
      break;
    case KEY:
      *p = str[0];
      log_debug("str: %s val: %u", str, *p);
      break;
    default:
      abort();
  }
}

void
format_long(op_t op, const char* str, void* ptr)
{
  assert(op == VALUE);

  long* p = ptr;
  *p = atol(str);
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
    unsigned int*: format_uint,                  \
    long*:         format_long,                  \
    char**:        format_string,                \
    default:       format_data                   \
) (op, str, ptr)

static char*
giti_config_default_create()
{
  size_t pos        = 0;
  size_t len        = 2048;
  char*  str_config = malloc(len);

  group_t last_group = GROUP_INVALID;
  pos += snprintf(str_config + pos, len - pos, "# --== GITi Config ==--\n");
#define X(group, str, op, def, ptr)                                           \
  if (group != last_group) {                                                  \
    switch(group) {                                                           \
    case GENERAL:                                                             \
      pos += snprintf(str_config + pos, len - pos, "# -= General =-\n");      \
      break;                                                                  \
    case KEYBINDING:                                                          \
      pos += snprintf(str_config + pos, len - pos, "\n# -= Keybinding =-\n"); \
      break;                                                                  \
    case FRIENDS:                                                             \
      pos += snprintf(str_config + pos, len - pos, "\n# -= Friends =-\n");    \
      break;                                                                  \
    default:                                                                  \
      assert(false);                                                          \
    }                                                                         \
    last_group = group;                                                       \
  }                                                                           \
                                                                              \
  pos += snprintf_param(def, str_config + pos, len - pos, str, op);           \
  pos += snprintf(str_config + pos, len - pos, "\n");                         \

  CONFIG
#undef X

  return str_config;
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
      set_param(&ptr, op, value);                \
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

          if (*curline == '#' || isspace(*curline) || strlen(curline) == 0) {
            goto next;
        }

        giti_config_line(curline);
next:
        curline = nextLine;
    }

    return &config;
}

char*
giti_config_to_string(const giti_config_t* config_)
{
  (void)config_;

  size_t buf_sz  = 4096;
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
