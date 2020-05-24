


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

#define STR_TIMEOUT  "general.timeout"
#define STR_USERNAME "general.username"
#define STR_FRIENDS  "friends"
#define STR_UP       "keybinding.up"
#define STR_DOWN     "keybinding.down"
#define STR_BACK     "keybinding.back"

#define DEFAULT_TIMEOUT 500
#define DEFAULT_FRIENDS "torvalds@linux-foundation.org"
#define DEFAULT_UP      'k'
#define DEFAULT_DOWN    'j'
#define DEFAULT_BACK    'q'

giti_config_t config;

typedef enum op {
    OP_INVALID,
    VALUE,
    LIST,
} op_t;

typedef enum group {
    GROUP_INVALID,
    GENERAL,
    NAVIGATION,
    FRIENDS,
} group_t;

#define CONFIG \
  X(GENERAL,    STR_TIMEOUT,  VALUE, DEFAULT_TIMEOUT, &config.general.timeout)  \
  X(GENERAL,    STR_USERNAME, VALUE, "",              &config.general.username) \
  X(NAVIGATION, STR_UP,       VALUE, DEFAULT_UP,      &config.navigation.up)    \
  X(NAVIGATION, STR_DOWN,     VALUE, DEFAULT_DOWN,    &config.navigation.down)  \
  X(NAVIGATION, STR_BACK,     VALUE, DEFAULT_BACK,    &config.navigation.back)  \
  X(FRIENDS,    STR_FRIENDS,  LIST,  DEFAULT_FRIENDS, config.friends)           \

#define FMT(T) _Generic( (T), \
    char:  "%c",              \
    int:   "%d",              \
    long:  "%ld",             \
)

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

  return snprintf(buf, buf_sz, "%s: %s", header, (char*)ptr);
}

size_t
snprintf__(char* buf, size_t buf_sz, const char* header, op_t op, void* ptr)
{
  size_t pos = 0;
  dlist_iterator_t* it = dlist_iterator_create((dlist_t*)ptr);

  const char* str_friend = NULL;
  while ((str_friend = dlist_iterator_next(it))) {
    pos += snprintf(buf + pos, buf_sz - pos, "%s: %s\n", header, str_friend);
  }
  dlist_iterator_destroy(it);

  return 0;
}

#define xxx(T, buf, buf_sz, header, op, ptr) _Generic( (T), \
    char*: snprintf_char(buf, buf_sz, header, op, ptr),     \
    long*: snprintf_long(buf, buf_sz, header, op, ptr),     \
    char** : snprintf_string(buf, buf_sz, header, op, ptr), \
    default: snprintf__(buf, buf_sz, header, op, ptr)       \
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
format_(op_t op, const char* str, void* ptr)
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

#define format(T, op, str, ptr) _Generic( (T), \
    char*: format_char(op, str, ptr),           \
    long*: format_long(op, str, ptr),           \
    default: format_(op, str, ptr)             \
)

#if 0
static void
print()
{

  log("--------------------------");

  int x = 100;
  (void)x;

  const char* t =FMT(x);

  (void)t;

  log(t, x);

  char buffer[128];
#define X(str, def, ptr)                                   \
    ptr = def;                                             \
    log(str);                                              \
    snprintf(buffer, sizeof(buffer), "%s: %s", str, FMT(ptr)); \
    log(buffer, ptr);

    CONFIG
#undef X



#if 0
#define X(str, tpe, def)                        \
      tpe v = def; \
      char* s = "%s -> " + FMT(v)
      log(s, str, v);
  CONFIG
#undef X
#endif

    log("--------------------------");
}
#endif


static char*
giti_config_default_create()
{
    char* str_config = NULL;
    int len = asprintf(&str_config,
"# --== GITi Config ==--\n\
%s: %u                  \n\
                        \n\
# Navigation            \n\
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

STR_FRIENDS,
DEFAULT_FRIENDS);

    return len > 0 ? str_config : NULL;
}

char*
giti_config_get_value(const char* str) {

  char* str_value = strchr(str, ':');
   str_value += 1;

   log("[%s] str: <%s> val: <%s>", __func__, str_value, strtrimall(str_value));

#if 0
   while (!isprint(*str_value)) {
       if (*str_value == '\n') {
           str_value = NULL;
           goto exit;
       }
       else {
           str_value += 1;
       }
   }
 exit:
#endif

   return strtrimall(str_value);
}



void
giti_config_line(const char* line)
{
  //log("[%s] config line: %s", __func__, line);

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

#define X(group, str, op, def, ptr)                     \
    if (strncmp(str, line, strlen(str)) == 0) {  \
      format(ptr, op, value, ptr);              \
    }

    CONFIG
#undef X
}

giti_config_t*
giti_config_create(char* str_config)
{
   config.friends = dlist_create(); // todo fix this
  //log("%s\n", str_config);
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


#if 0
        if (strncmp(curline, STR_TIMEOUT, strlen(STR_TIMEOUT)) == 0) {
            const char* str_value = giti_config_get_value(curline);
            if (str_value) {
                config->timeout = atol(str_value);
            }
#endif
next:
        curline = nextLine;
    }

    log("?????????????????");
    giti_config_print(&config);
    log("?????????????????");


    return &config;
}

#if 0
giti_config_t*
giti_config_create(char* str_config)
{
    print();

    str_config = str_config ? str_config : giti_config_default_create();
    giti_config_create2(str_config);

    log("config:\n%s", str_config);

    giti_config_t* config = malloc(sizeof *config);
    *config = (giti_config_t) {
        .timeout = DEFAULT_TIMEOUT,
        .navigation.up = DEFAULT_UP,
        .navigation.down = DEFAULT_DOWN,
    };

    //log("%s\n", str_config);
    char* curline = str_config;
    bool friends = false;
    while(curline) {
        char* nextLine = strchr(curline, '\n');
        if (nextLine) {
            *nextLine = '\0';
            nextLine += 1;
        }

        if (curline[0] == '#') {
            goto next;
        }


        if (friends) {
            if (isspace(curline[0])) {
                char* str_friend = strtrimall(curline);
                //log("friend: %s\n%s", str_friend, nextLine);
                if (!config->friends) {
                    config->friends = dlist_create();
                }
                dlist_append(config->friends, strdup(str_friend));
            }
            else {
                friends = false;
            }
        }

        if (strncmp(curline, STR_TIMEOUT, strlen(STR_TIMEOUT)) == 0) {
            const char* str_value = giti_config_get_value(curline);
            if (str_value) {
                config->timeout = atol(str_value);
            }
        }
        else if (strncmp(curline, STR_FRIENDS, strlen(STR_FRIENDS)) == 0) {
            friends = true;
        }
        else if (strncmp(curline, STR_UP, strlen(STR_UP)) == 0) {
            const char* str_value = giti_config_get_value(curline);
            if (str_value && strlen(str_value) == 1) {
                config->navigation.up = *str_value;
                log("down: %s %c %c", str_value, *str_value, config->navigation.up);
            }
        }
        else if (strncmp(curline, STR_DOWN, strlen(STR_DOWN)) == 0) {
            const char* str_value = giti_config_get_value(curline);
            if (str_value && strlen(str_value) == 1) {
                config->navigation.down = *str_value;
                log("down: %s %c %c", str_value, *str_value, config->navigation.down);
            }
        }

next:
        curline = nextLine;
    }

    free(str_config);

    return config;
}
#endif

void
giti_config_print(const giti_config_t* config_)
{
  (void)config_;

  char pos = 0;
  char buffer[512];

  group_t last_group = GROUP_INVALID;
  log("--== GITi Config ==--");
#define X(group, str, op, def, ptr)                                        \
  if (group != last_group) { \
    switch(group) {           \
    case GENERAL: \
      log("-= General =-"); \
      break; \
    case NAVIGATION: \
      log("\n-= Navigation =-"); \
      break; \
    case FRIENDS: \
      log("\n-= Friends =-");\
      break; \
    default:\
      assert(false); \
    } \
    last_group = group; \
  } \
\
  xxx(ptr, buffer + pos, sizeof(buffer) - pos, str, op, ptr);  \
  log("%s", buffer); \

  CONFIG
#undef X



    return;
#if 0
    log("%s: %ld\n", STR_TIMEOUT, config->timeout);
    log("Navigation");
    log("  up  : %c", config->navigation.up);
    log("  down: %c", config->navigation.down);

    if (config->friends) {
        log("%s:", STR_FRIENDS);

        dlist_iterator_t* it = dlist_iterator_create(config->friends);

        const char* str_friend = NULL;
        while ((str_friend = dlist_iterator_next(it))) {
            log("  \"%s\"", str_friend);
        }
        dlist_iterator_destroy(it);
    }
    #endif
}
