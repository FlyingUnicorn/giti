#ifndef GITI_COLOR_H_
#define GITI_COLOR_H_

#include <stdint.h>

typedef struct giti_color {
    union {
        struct {
            uint8_t g;
            uint8_t b;
            uint8_t r;
        };
        uint32_t c;
    };
} giti_color_t;


typedef struct giti_color_scheme {
    giti_color_t fg;
    giti_color_t bg;
    giti_color_t bg_selected;
    giti_color_t fg1;
    giti_color_t fg2;
    giti_color_t fg3;
    giti_color_t on;
    giti_color_t off;
    giti_color_t inactive;
} giti_color_scheme_t;

#endif
