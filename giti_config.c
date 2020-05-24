


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


#define DEFAULT_TIMEOUT 500 /* ms */
#define DEFAULT_FRIENDS "torvalds@linux-foundation.org\n  olof@lixom.net"
#define DEFAULT_UP      'k'
#define DEFAULT_DOWN    'j'


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

const char*
giti_config_get_value(const char* str) {

   char* str_value = strchr(str, ':');

   str_value += 1;

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
   return strtrimall(str_value);
}

giti_config_t*
giti_config_create(char* str_config)
{
    str_config = str_config ? str_config : giti_config_default_create();

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
