


#define _GNU_SOURCE

#include "giti_config.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "giti_log.h"
#include "giti_string.h"

#define STR_TIMEOUT "timeout"
#define STR_FRIENDS "friends"
#define STR_UP      "up"
#define STR_DOWN    "down"


#define DEFAULT_TIMEOUT 500
#define DEFAULT_FRIENDS "torvalds@linux-foundation.org\n  olof@lixom.net"
#define DEFAULT_UP      'k'
#define DEFAULT_DOWN    'j'


giti_config_t config;

enum {
    SET,
    APPEND,
};

#define CONFIG \
  X(STR_TIMEOUT, SET,    DEFAULT_TIMEOUT, config.timeout)         \
  X(STR_UP,      SET,    DEFAULT_UP,      config.navigation.up)   \
  X(STR_DOWN,    SET,    DEFAULT_DOWN,    config.navigation.down) \
  X(STR_FRIENDS, APPEND, "",              config.friends)   \

#define FMT(T) _Generic( (T), \
    char:  "%c",              \
    int:   "%d",              \
    long:  "%ld",             \
    char*: "%s"               \
)


long format_long(const char* str) { return atol(str); }
char format_char(const char* str) { return *str; }


#define format(T, str) _Generic( (T), \
    char: format_char(str),           \
    long: format_long(str)            \
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
                        \n\
%s:                     \n\
  %s",

STR_TIMEOUT,
DEFAULT_TIMEOUT,

STR_UP,
DEFAULT_UP,
STR_DOWN,
DEFAULT_DOWN,

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

#define X(str, op, def, ptr)                     \
    if (strncmp(str, line, strlen(str)) == 0) {  \
      if (op == SET) {                           \
          ptr = format(ptr, value);              \
      }                                          \
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
giti_config_print(const giti_config_t* config)
{
    log("--== GITi Config ==--");
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
}
