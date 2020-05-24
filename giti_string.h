#ifndef GITI_STRING_H_
#define GITI_STRING_H_

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>


static inline char*
strtrim(char* str)
{
    size_t len = strlen(str);

    if (len == 0) {
        return str;
    }

    int pos = len - 1;
    for (; pos > 0 && isspace(str[pos]); --pos) { }
    str[pos + 1] = '\0';

    return str;
}

static inline char*
strtrimall(char* str)
{
    while(*str != '\0') {
        if (isspace(*str)) {
            ++str;
        }
        else {
            break;
        }
    }
    return strtrim(str);
}

static inline int
strlines(const char* str)
{
    int lines = 0;
    for (const char* ch = str; *ch; ++ch) {
        if (*ch == '\n') {
            ++lines;
        }
    }
    return lines;
}

static inline bool
strwcmp(const wchar_t* needle, const wchar_t* haystack)
{
    if (needle == NULL || haystack == NULL || wcslen(needle) > wcslen(haystack)) {
        return false;
    }

    for (; *haystack ; ++haystack) {
        bool match = true;
        for (const wchar_t* h = haystack, *n = needle; *n; ++n, ++h) {
            if (towlower(*n) != towlower(*h)) {
                match = false;
                break;
            }
        }
        if (match) {
            return true;
        }
    }
    return false;
}

static inline int
strwcasecmp(const wchar_t* s1, const wchar_t* s2, size_t n)
{
    if (!s1 || !s2) {
        return -1;
    }

    for (int i = 0; *s1 && i < n; ++s1, ++s2, ++n) {
        if (!*s2 | (towlower(*s1) != towlower(*s2))) {
            return -1;
        }
    }
    return 0;
}

#endif
