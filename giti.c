

#define _GNU_SOURCE

#include <assert.h>
#include <locale.h>
#include <ctype.h>
#include <ncurses.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <wchar.h>
#include <wctype.h>

#include "dlist.h"

#include "giti_color.h"
#include "giti_config.h"
#include "giti_log.h"
#include "giti_string.h"

#define MIN(v1, v2) (((v1) < (v2)) ? (v1) : (v2))
#define MAX(v1, v2) (((v1) > (v2)) ? (v1) : (v2))

#define H_SZ  12
#define CI_SZ 25
#define CR_SZ 20
#define AN_SZ 40
#define AE_SZ 40
#define S_SZ  256

#define array_size(a) (sizeof a) / (sizeof (a)[0])


giti_config_t* g_config;

typedef char giti_strbuf_t[256];
typedef char giti_strbuf_info_t[4096];

typedef char* (*giti_menu_item_str_cb_t)(giti_strbuf_t strbuf, void* arg);
typedef void (*giti_menu_item_action_cb_t)(void* arg);
typedef char* (*giti_window_title_cb_t)(void* ctx, giti_strbuf_t strbuf);
typedef dlist_t* (*giti_window_filter_cb_t)(void* ctx);
typedef void (*giti_window_destroy_cb_t)(void* ctx);

typedef enum s_item_type {
    S_ITEM_TYPE_INVALID,
    S_ITEM_TYPE_MENU,
    S_ITEM_TYPE_ACTION,
    S_ITEM_TYPE_TEXT,
} s_item_type_t;

typedef struct giti_window_opt {
    s_item_type_t            type;
    int                      xmax; /* -1 = calc width, 0 = max width, >0 width */
    union {
        const dlist_t*      menu;
        const char*         text;
    };
    void*                    cb_arg;
    void*                    cb_arg_destroy;
    giti_window_title_cb_t   cb_title;
    giti_window_filter_cb_t  cb_filter;
    giti_window_destroy_cb_t cb_destroy;
    bool (*cb_shortcut)(void* arg, uint32_t wch, uint32_t action_id, struct giti_window_opt* opt);
} giti_window_opt_t;

typedef struct giti_window {
    WINDOW*           w;
    int               pos;
    size_t            ymax;
    giti_strbuf_t     title;
    giti_window_opt_t opt;
} giti_window_t;

typedef struct giti_menu_item {
    s_item_type_t           type;
    void*                   cb;
    giti_strbuf_t           name;
    giti_menu_item_str_cb_t cb_str;
    void*                   cb_arg;
    uint32_t                action_id;
    bool (*cb_action)(void* arg, uint32_t action_id, struct giti_window_opt* opt);
} giti_menu_item_t;

/* HELPER FUNCTIONS GENERAL */
static size_t
strlen_formatted(const char* str)
{
    size_t len = 0;
    for(const char* ch = str; *ch; ++ch) {

        if (strncmp(ch, "<giti-clr-1>", strlen("<giti-clr-1>")) == 0) {
            ch += strlen("<giti-clr-1>") - 1;
            continue;
        }
        if (strncmp(ch, "<giti-clr-2>", strlen("<giti-clr-2>")) == 0) {
            ch += strlen("<giti-clr-2>") - 1;
            continue;
        }
        if (strncmp(ch, "<giti-clr-3>", strlen("<giti-clr-3>")) == 0) {
            ch += strlen("<giti-clr-3>") - 1;
            continue;
        }
        if (strncmp(ch, "<giti-clr-on>", strlen("<giti-clr-on>")) == 0) {
            ch += strlen("<giti-clr-on>") - 1;
            continue;
        }
        if (strncmp(ch, "<giti-clr-off>", strlen("<giti-clr-off>")) == 0) {
            ch += strlen("<giti-clr-off>") - 1;
            continue;
        }
        if (strncmp(ch, "<giti-clr-inactive>", strlen("<giti-clr-inactive>")) == 0) {
            ch += strlen("<giti-clr-inactive>") - 1;
            continue;
        }
        else if (strncmp(ch, "<giti-clr-end>", strlen("<giti-clr-end>")) == 0) {
            ch += strlen("<giti-clr-end>") - 1;
            continue;
        }

        ++len;
    }
    return len;
}

/* HELPER FUNCTIONS GITI */
static short
convclr(short c)
{
    return MIN(c * 4, 1000);
}

static void
giti_color_scheme_init(giti_color_scheme_t* cs)
{
    init_color(200, convclr(cs->fg.r),          convclr(cs->fg.b),          convclr(cs->fg.g));
    init_color(201, convclr(cs->bg.r),          convclr(cs->bg.b),          convclr(cs->bg.g));
    init_color(202, convclr(cs->bg_selected.r), convclr(cs->bg_selected.b), convclr(cs->bg_selected.g));
    init_color(203, convclr(cs->fg1.r),         convclr(cs->fg1.b),         convclr(cs->fg1.g));
    init_color(204, convclr(cs->fg2.r),         convclr(cs->fg2.b),         convclr(cs->fg2.g));
    init_color(205, convclr(cs->fg3.r),         convclr(cs->fg3.b),         convclr(cs->fg3.g));
    init_color(206, convclr(cs->on.r),          convclr(cs->on.b),          convclr(cs->on.g));
    init_color(207, convclr(cs->off.r),         convclr(cs->off.b),         convclr(cs->off.g));
    init_color(208, convclr(cs->inactive.r),    convclr(cs->inactive.b),    convclr(cs->inactive.g));

    init_pair(1, 200, 201);
    init_pair(2, 203, 201);
    init_pair(3, 204, 201);
    init_pair(4, 205, 201);
    init_pair(5, 206, 201);
    init_pair(6, 207, 201);
    init_pair(7, 208, 201);

    init_pair(11, 200, 202);
    init_pair(12, 203, 202);
    init_pair(13, 204, 202);
    init_pair(14, 205, 202);
    init_pair(15, 206, 202);
    init_pair(16, 207, 202);
    init_pair(17, 208, 202);
}

static int
giti_color_scheme(int clr, bool selected)
{
    return COLOR_PAIR((selected ? 10 : 0) + clr + 1);
}

typedef struct color_filter{
    wchar_t str[256];
    uint8_t code;
} color_filter_t;

static void
giti_print(WINDOW* w, dlist_t* color_filter, int y, int x, int xmax, const char* str, bool selected, bool top_window)
{
    int i = 0;
    int filter_clr = 0;
    int filter_x_len = 0;
    int filter_x_clr = 0;
    // https://www.unix.com/attachments/programming/6051d1418489361-ncurses-colors-selection_128png

    wchar_t wstr[256];
    mbstowcs(wstr, str, array_size(wstr));

    for (const wchar_t* ch = wstr; i < xmax; ++i) {

        int fg = 0;
        wchar_t print_ch = L'\0';
        if (*ch) {

            if (wcsncmp(ch, L"<giti-clr-1>", wcslen(L"<giti-clr-1>")) == 0) {
                filter_clr = 1;
                ch += wcslen(L"<giti-clr-1>");
                --i;
                continue;
            }
            if (wcsncmp(ch, L"<giti-clr-2>", wcslen(L"<giti-clr-2>")) == 0) {
                filter_clr = 2;
                ch += wcslen(L"<giti-clr-2>");
                --i;
                continue;
            }
            if (wcsncmp(ch, L"<giti-clr-3>", wcslen(L"<giti-clr-3>")) == 0) {
                filter_clr = 3;
                ch += wcslen(L"<giti-clr-3>");
                --i;
                continue;
            }
            if (wcsncmp(ch, L"<giti-clr-on>", wcslen(L"<giti-clr-on>")) == 0) {
                filter_clr = 4;
                ch += wcslen(L"<giti-clr-on>");
                --i;
                continue;
            }
            if (wcsncmp(ch, L"<giti-clr-off>", wcslen(L"<giti-clr-off>")) == 0) {
                filter_clr = 5;
                ch += wcslen(L"<giti-clr-off>");
                --i;
                continue;
            }

            else if (wcsncmp(ch, L"<giti-clr-end>", wcslen(L"<giti-clr-end>")) == 0) {
                filter_clr = 0;
                ch += wcslen(L"<giti-clr-end>");
                --i;
                continue;
            }

            if (color_filter && filter_x_len == 0 && top_window) {
                for (int i = 0; i < dlist_size(color_filter); ++i) {
                    color_filter_t* f = dlist_get(color_filter, i);

                    //log("needle: %ls haystack: %ls", f->str, ch);
                    if (strwcasecmp(f->str, ch, wcslen(f->str)) == 0) {
                        /* match */
                        filter_x_len = wcslen(f->str);
                        filter_x_clr = f->code;

                        break;
                    }
                }
            }

            if (!top_window) {
                fg = 6; /* inactive */
            }
            else if (filter_x_len) {
                fg = filter_x_clr;
                --filter_x_len;
            }
            else if (filter_clr) {
                fg = filter_clr;
            }
            print_ch = *ch;
        }
        else if (selected) {
            print_ch = ' ';
        }
        else {
            return;
        }

        wattron(w, giti_color_scheme(fg, selected));
        //mvwaddch(w, y, x + i, print_ch);
        mvwaddnwstr(w, y, x + i, &print_ch, 1);

        //wattroff(w, COLOR_PAIR(fg + bg));

        if (*ch) {
            ++ch;
        }
    }
}

static void
giti_exit(int code, const char* msg)
{
    bkgd(0);
    erase();
    refresh();
    endwin();
    printf("GITi exit:\n%s\n", msg);
    exit(code);
}

static void
window_title(WINDOW* w, int max, const char* title)
{
    if (title) {
        int x = (max - 4 - strlen_formatted(title)) / 2;

        giti_strbuf_t strbuf;
        mvwaddch(w, 0, x, ACS_RTEE);
        snprintf(strbuf, sizeof(giti_strbuf_t), " %s ", title);
        mvwaddch(w, 0, x + strlen_formatted(strbuf) + 1, ACS_LTEE);
        giti_print(w, NULL, 0, x + 1, max - 2, strbuf, false, true);
    }
}

static size_t
giti_window_len(const giti_window_t* s)
{
    size_t res = 0;

    switch (s->opt.type) {
    case S_ITEM_TYPE_MENU:
        res = dlist_size(s->opt.menu);
        break;
    case S_ITEM_TYPE_TEXT:
        res = strlines(s->opt.text) + 1;
        break;
    default:
        break;
    }

    return res;
}

static size_t
giti_window_ymax(const giti_window_t* s)
{
    size_t res = MIN(s->ymax, giti_window_len(s) + 2);
    return res;
}

static size_t
giti_window_xmax(const giti_window_t* s)
{
    size_t res = s->opt.xmax ? MIN(s->opt.xmax, COLS) : COLS;

    return res;
}

static void
giti_window_set_xmax(giti_window_t* s)
{
    int strlenmax = 0;
    if (s->opt.cb_title) {
        giti_strbuf_t strbuf = { 0 };
        strlenmax = strlen(s->opt.cb_title(s->opt.cb_arg, strbuf)) + 2;
    }

    switch (s->opt.type) {
    case S_ITEM_TYPE_MENU: {
        for (int i = 0; i < dlist_size(s->opt.menu); ++i) {

            giti_menu_item_t* item = dlist_get(s->opt.menu, i);

            if (item->cb_str) {
                giti_strbuf_t strbuf;
                item->cb_str(strbuf, item->cb_arg);
                strlenmax = MAX(strlen(strbuf), strlenmax);
            }
            else {
                strlenmax = MAX(strlen(item->name), strlenmax);
            }
        }
        break;
    }
    case S_ITEM_TYPE_TEXT: {
        int len = 0;
        for (const char* ch = s->opt.text; *ch; ++ch) {
            if (*ch == '\n') {
                strlenmax = MAX(len, strlenmax);
                len = 0;
            } else {
                ++len;
            }
        }
        break;
    }
    default:
        break;
    }
    s->opt.xmax = strlenmax + 4;
}

static size_t
giti_window_lnmax(const giti_window_t* s)
{
    return giti_window_xmax(s) - 4;
}

static size_t
giti_window_lnymax(const giti_window_t* s)
{
    return giti_window_ymax(s) - 2;
}

static size_t
giti_window_posmax(const giti_window_t* s)
{
    size_t res = 0;

    switch (s->opt.type) {
    case S_ITEM_TYPE_MENU:
        res = dlist_size(s->opt.menu) - 1;
        break;
    case S_ITEM_TYPE_TEXT:
        res = MAX(0, (int)giti_window_len(s) + 1 - (int)s->ymax);
        break;
    default:
        break;
    }

    return res;
}

/* GIT FUNCTIONS */
static char*
giti_user_name()
{
    FILE *fp;
    char path[4096];

    const char* cmd = "git config user.name";
    fp = popen(cmd, "r");
    if (fp == NULL) {
        printf("Failed to run command\n" );
        exit(1);
    }

    char* res = NULL;
    while (fgets(path, sizeof(path), fp) != NULL) {
        res = strdup(path);
    }
    pclose(fp);

    return strtrim(res);
}

static char*
giti_commit_body(const char* hash)
{
    FILE *fp;
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "git log --format=%%b -n 1 %s", hash);

    fp = popen(cmd, "r");
    if (fp == NULL) {
        printf("Failed to run command\n" );
        exit(1);
    }

    char* res = calloc(1, 4096);
    fread(res, sizeof(char), 4096, fp);
    pclose(fp);

    return res;
}

static char*
giti_user_email()
{
    FILE *fp;
    char path[4096];

    const char* cmd = "git config user.email";
    fp = popen(cmd, "r");
    if (fp == NULL) {
        printf("Failed to run command\n" );
        exit(1);
    }

    char* res = NULL;
    while (fgets(path, sizeof(path), fp) != NULL) {
        res = strdup(path);
    }
    pclose(fp);

    return strtrim(res);
}

typedef struct giti_file_status {
    char          staged_status;
    char          unstaged_status;
    giti_strbuf_t str;
    struct {
        uint32_t add;
        uint32_t del;
    } staged;
    struct {
        uint32_t add;
        uint32_t del;
    } unstaged;
} giti_file_status_t;

static void
giti_status_(dlist_t* dlist)
{
    FILE *fp;
    giti_strbuf_t strbuf;

    const char* cmd = "git status --porcelain=v1";
    fp = popen(cmd, "r");
    if (fp == NULL) {
        printf("Failed to run command\n" );
        exit(1);
    }

    while (fgets(strbuf, sizeof strbuf, fp) != NULL) {
        giti_file_status_t* gfs = malloc(sizeof *gfs);
        gfs->staged_status = strbuf[0];
        gfs->unstaged_status = strbuf[1];
        strncpy(gfs->str, strtrim(strbuf + 3), sizeof gfs->str);
        dlist_append(dlist, gfs);
    }
    pclose(fp);
}

static void
giti_status_changes(giti_file_status_t* gfs)
{
    FILE *fp;

    giti_strbuf_t cmd;
    snprintf(cmd, sizeof cmd, "git diff --numstat --cached -- %s", gfs->str);
    fp = popen(cmd, "r");
    if (fp == NULL) {
        printf("Failed to run command\n" );
        exit(1);
    }

    char* end;
    giti_strbuf_t strbuf;
    while (fgets(strbuf, sizeof strbuf, fp) != NULL) {
        gfs->staged.add = strtoul(strbuf, &end, 10);
        gfs->staged.del = strtoul(end, &end, 10);
    }
    pclose(fp);

    snprintf(cmd, sizeof cmd, "git diff --numstat -- %s", gfs->str);
    fp = popen(cmd, "r");
    if (fp == NULL) {
        printf("Failed to run command\n" );
        exit(1);
    }

    while (fgets(strbuf, sizeof strbuf, fp) != NULL) {
        gfs->unstaged.add = strtoul(strbuf, &end, 10);
        gfs->unstaged.del = strtoul(end, &end, 10);
    }
    pclose(fp);

}

static void
giti_status(dlist_t* dlist)
{
    giti_status_(dlist);

    dlist_iterator_t* it = dlist_iterator_create(dlist);

    giti_file_status_t* gfs;
    while ((gfs = dlist_iterator_next(it))) {
        giti_status_changes(gfs);

    }
    dlist_iterator_destroy(it);
}

static char*
giti_current_branch()
{
    FILE *fp;
    char path[4096];

    const char* cmd = "git rev-parse --abbrev-ref HEAD";
    fp = popen(cmd, "r");
    if (fp == NULL) {
        printf("Failed to run command\n" );
        exit(1);
    }

    char* res = NULL;
    while (fgets(path, sizeof(path), fp) != NULL) {
        res = strdup(path);
    }
    pclose(fp);

    return strtrim(res);
}

static void
giti_branch_checkout(const char* name)
{
    FILE *fp;
    char path[4096];

    char cmd[256];
    snprintf(cmd, sizeof(path), "git checkout %s", name);
    fp = popen(cmd, "r");
    if (fp == NULL) {
        giti_exit(1, "Failed to run command");
    }

    fread(path, sizeof(char), sizeof(path), fp);
    pclose(fp);

    giti_exit(0, path);
}

static void
giti_branch_rebase(const char* upstream)
{
    FILE *fp;
    char path[4096];

    char cmd[256];
    snprintf(cmd, sizeof(path), "git rebase %s", upstream);
    fp = popen(cmd, "r");
    if (fp == NULL) {
        giti_exit(1, "Failed to run command");
    }

    fread(path, sizeof(char), sizeof(path), fp);
    pclose(fp);

    giti_exit(0, path);
}

static char*
giti_commit_files_(const char* hash)
{
    FILE *fp;
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "git diff --stat %s^!", hash);

    fp = popen(cmd, "r");
    if (fp == NULL) {
        printf("Failed to run command\n" );
        exit(1);
    }

    char* res = calloc(1, 4096);
    fread(res, sizeof(char), 4096, fp);
    pclose(fp);

    return res;
}


/* GITi COMMIT */
typedef struct giti_commit {
    union {
        struct {
            char h[H_SZ + 1];
            union {
                char ci[CI_SZ + 1];
                struct {
                    char ci_date[11];
                    char ci_time[8];
                };
            };
            char cr[CR_SZ + 1];
            char an[AN_SZ + 1];
            char ae[AE_SZ + 1];
            char s[S_SZ + 1];
        };
        char raw[H_SZ + CI_SZ + CR_SZ + AN_SZ + AE_SZ + S_SZ + 7 + 50]; /* 50 for multi-char chars */
    };
    char* b;
    char* files;
    bool is_self;
    bool is_friend;
    giti_strbuf_info_t info;
} giti_commit_t;

static void
giti_commit_destroy(void* c_)
{
    giti_commit_t* c = c_;

    if (c->b) {
        free(c->b);
    }

    if (c->files) {
        free(c->files);
    }
    free(c);
}

static char*
giti_commit_row(giti_strbuf_t strbuf, void* e_)
{
    giti_commit_t* e = e_;
    size_t pos = 0;
    pos += snprintf(strbuf + pos, sizeof(giti_strbuf_t) - pos, "%s <giti-clr-2>%s<giti-clr-end> <giti-clr-3>%s<giti-clr-end>", e->h, e->ci_date, e->ci_time);

    char* str_name = malloc(g_config->format.max_width_name);
    snprintf(str_name, g_config->format.max_width_name, "%s", e->an);
    if (strlen(e->an) > g_config->format.max_width_name) {
        str_name[g_config->format.max_width_name - 3] = '.';
        str_name[g_config->format.max_width_name - 2] = '.';
        str_name[g_config->format.max_width_name - 1] = '\0';
    }

    if (e->is_self) {
        pos += snprintf(strbuf + pos, sizeof(giti_strbuf_t) - pos, " <giti-clr-1>%*s<giti-clr-end>", g_config->format.max_width_name, str_name);
    }
    else if (e->is_friend) {
        pos += snprintf(strbuf + pos, sizeof(giti_strbuf_t) - pos, " <giti-clr-on>%*s<giti-clr-end>", g_config->format.max_width_name, str_name);
    }
    else {
        pos += snprintf(strbuf + pos, sizeof(giti_strbuf_t) - pos, " %*s", g_config->format.max_width_name, str_name);
    }
    pos += snprintf(strbuf + pos, sizeof(giti_strbuf_t) - pos, " %s", e->s);
    return strbuf;
}

static void
giti_commit_diff(const giti_commit_t* c)
{
    char buf[100];
    snprintf(buf, sizeof(buf), "git diff --color=always %s^! | less -r", c->h); /* show changes in specific commit */
    log("[%s] cmd: %s", __func__, buf);
    system(buf);
}

static char*
giti_commit_info(giti_commit_t* c)
{
    if (!c->b) {
        c->b = giti_commit_body(c->h);
    }

    snprintf(c->info, sizeof(giti_strbuf_info_t), "Suject: %s\nAuthor: %s (%s)\nDate:   %s (%s)\n\n%s", c->s, c->an, c->ae, c->ci, c->cr, c->b);
    return c->info;
}

static char*
giti_commit_files(giti_commit_t* c)
{
    if (!c->files) {
        c->files = giti_commit_files_(c->h);
    }
    return c->files;
}

static char*
giti_commit_menu_title_str(void* c_, giti_strbuf_t strbuf)
{
    (void)c_;

    snprintf(strbuf, sizeof(giti_strbuf_t), "Commit options");
    return strbuf;
}

static char*
giti_commit_info_title_str(void* c_, giti_strbuf_t strbuf)
{
    (void)c_;

    snprintf(strbuf, sizeof(giti_strbuf_t), "Commit Info");
    return strbuf;
}

static char*
giti_commit_files_title_str(void* c_, giti_strbuf_t strbuf)
{
    (void)c_;

    snprintf(strbuf, sizeof(giti_strbuf_t), "Commit Files");
    return strbuf;
}

static dlist_t*
giti_commit_files_color_filter(void* c_)
{
    (void)c_;

    /* change to regexp */
    dlist_t* color_filter = dlist_create();

    color_filter_t* f = NULL;

    f = malloc(sizeof *f);
    wcsncpy(f->str, L"+", array_size(f->str));
    f->code = 4;
    dlist_append(color_filter, f);

    f = malloc(sizeof *f);
    wcsncpy(f->str, L"-", array_size(f->str));
    f->code = 5;
    dlist_append(color_filter, f);

    return color_filter;
}

typedef struct giti_window_menu {
    dlist_t* menu;
} giti_window_menu_t;

static giti_window_menu_t*
giti_window_menu_create()
{
    giti_window_menu_t* gwm = malloc(sizeof *gwm);
    gwm->menu = dlist_create();

    return gwm;
}

static void
giti_window_menu_destroy(void* gwm_)
{
    giti_window_menu_t* gwm = gwm_;
    dlist_destroy(gwm->menu, free);
    free(gwm);
}

static bool
giti_commit_shortcut(void* c_, uint32_t wch, uint32_t action_id, giti_window_opt_t* opt);

static bool
giti_commit_action(void* c_, uint32_t action_id, giti_window_opt_t* opt)
{
    giti_commit_t* c = c_;

    bool claimed = true;
    if (action_id == (uint32_t)g_config->keybinding.commit.info) {
        opt->text = giti_commit_info(c);
        opt->type = S_ITEM_TYPE_TEXT;
        opt->cb_title = giti_commit_info_title_str;
        opt->cb_arg = c;
        opt->xmax = -1;
    }
    else if (action_id == (uint32_t)g_config->keybinding.commit.show) {
        giti_commit_diff(c);
    }
    else if (action_id == (uint32_t)g_config->keybinding.commit.files) {
        opt->text = giti_commit_files(c);
        opt->type = S_ITEM_TYPE_TEXT;
        opt->cb_title = giti_commit_files_title_str;
        opt->cb_filter = giti_commit_files_color_filter;
        opt->cb_arg = c;
        opt->xmax = -1;
    }
    else if (action_id == ' ') {
        giti_config_key_strbuf_t strbuf = { 0 };

        giti_window_menu_t* gwm = giti_window_menu_create();
        opt->type = S_ITEM_TYPE_MENU;
        opt->menu = gwm->menu;
        opt->cb_title = giti_commit_menu_title_str;
        opt->cb_shortcut = giti_commit_shortcut;
        opt->cb_destroy = giti_window_menu_destroy;
        opt->cb_arg = c;
        opt->cb_arg_destroy = gwm;
        opt->xmax = -1;

        giti_menu_item_t* item = NULL;
        item = calloc(1, sizeof *item);
        item->type = S_ITEM_TYPE_TEXT;
        item->cb_arg = c;
        item->cb_action = giti_commit_action;
        item->action_id = g_config->keybinding.commit.info;
        snprintf(item->name, sizeof(item->name), "Show commit info    [%s]",
                 giti_config_key_to_string(item->action_id, strbuf));
        dlist_append(gwm->menu, item);

        item = calloc(1, sizeof *item);
        item->type = S_ITEM_TYPE_ACTION;
        item->cb_arg = c;
        item->cb_action = giti_commit_action;
        item->action_id = g_config->keybinding.commit.show;
        snprintf(item->name, sizeof(item->name), "Open commit diff    [%s]",
                 giti_config_key_to_string(item->action_id, strbuf));
        dlist_append(gwm->menu, item);

        item = calloc(1, sizeof *item);
        item->type = S_ITEM_TYPE_TEXT;
        item->cb_arg = c;
        item->cb_action = giti_commit_action;
        item->action_id = g_config->keybinding.commit.files;
        snprintf(item->name, sizeof(item->name), "Show commit files   [%s]",
                 giti_config_key_to_string(item->action_id, strbuf));
        dlist_append(gwm->menu, item);
    }
    else {
        claimed = false;
    }

    return claimed;
}

static bool
giti_commit_shortcut(void* c_, uint32_t wch, uint32_t action_id, giti_window_opt_t* opt)
{
    giti_commit_t* c = c_;

    bool claimed = false;
    if (wch == ' ') {
        claimed = giti_commit_action(c, action_id, opt);
    }

    return claimed;
}

/* GITi Log */
typedef struct giti_log {
    giti_strbuf_t branch;
    dlist_t*      entries;
    dlist_t*      filtered_entries;
    struct {
        bool      self;
        bool      friends;
        wchar_t   str[256];
        size_t    str_pos;
        bool      active;
        bool      show_all;
        size_t    found;
    } filter;
} giti_log_t;

static dlist_t*
giti_log_entries_(const char* precmd, const char* postcmd, uint32_t max_duration_ms)
{
    FILE *fp;
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "git log %s --pretty=format:\"%%<(%d,trunc)%%h %%<(%d,trunc)%%ci %%<(%d,trunc)%%cr %%<(%d,trunc)%%an %%<(%d,trunc)%%ae %%<(%d,trunc)%%s\" %s", precmd ? precmd : "", H_SZ, CI_SZ, CR_SZ, AN_SZ, AE_SZ, S_SZ, postcmd ? postcmd : "");

    fp = popen(cmd, "r");
    if (fp == NULL) {
        printf("Failed to run command\n" );
        exit(1);
    }

    long start_ms = 0;
    struct timeval timecheck;
    if (max_duration_ms) {
        gettimeofday(&timecheck, NULL);
        start_ms = (long)timecheck.tv_sec * 1000 + (long)timecheck.tv_usec / 1000;
    }

    dlist_t* list = dlist_create();
    giti_commit_t* e = calloc(1, sizeof *e);
    uint64_t cnt = 0;
    while (fgets(e->raw, sizeof(e->raw), fp) != NULL) {
        e->h[H_SZ]     = '\0';
        e->ci[19]      = '\0';
        e->ci_date[10] = '\0';
        e->cr[CR_SZ]   = '\0';
        e->an[AN_SZ]   = '\0';
        e->ae[AE_SZ]   = '\0';
        e->s[S_SZ]     = '\0';

        strtrim(e->h);
        strtrim(e->ci);
        strtrim(e->cr);
        strtrim(e->an);
        strtrim(e->ae);
        strtrim(e->s);

        //log("e->h:  %s", e->h);
        //log("e->ci: %s", e->ci);
        //log("e->ch: %s", e->cr);
        //log("e->an: %s", e->an);
        //log("e->ae: %s", e->ae);
        //log("e->s:  %s", e->s);

        if (strncmp(g_config->general.user_email, e->ae, strlen(e->ae)) == 0) {
            e->is_self = true;
        }
        else {
            const char* friend = NULL;
            dlist_iterator_t* it = dlist_iterator_create(g_config->friends);
            while ((friend = dlist_iterator_next(it))) {
                if (strncmp(friend, e->ae, strlen(friend)) == 0) {
                    e->is_friend = true;
                    break;
                }
            }
            dlist_iterator_destroy(it);
        }

        dlist_append(list, e);
        ++cnt;

        e = calloc(1, sizeof *e);

        if (cnt % 1000 == 0 && start_ms) {
            gettimeofday(&timecheck, NULL);
            long now_ms = (long)timecheck.tv_sec * 1000 + (long)timecheck.tv_usec / 1000;
            if (now_ms > start_ms + max_duration_ms) {
                log("[%s] fetched %lu entries took: %lu ms", __func__, cnt, now_ms - start_ms);
                break;
              }
        }

    }
    free(e);
    pclose(fp);

    return list;
}

static dlist_t*
giti_log_entries(const char* branch, size_t entries)
{
    char precmd[512];
    snprintf(precmd, sizeof(precmd), "-n %lu", entries);

    char postcmd[512];
    snprintf(postcmd, sizeof(postcmd), "%s --", branch);

    return giti_log_entries_(precmd, postcmd, g_config->general.timeout);
}

static void
giti_log_filter(giti_log_t* gl)
{
    if (gl->filtered_entries) {
        dlist_clear(gl->filtered_entries, free);
    }
    else {
        gl->filtered_entries = dlist_create();
    }
    gl->filter.found = 0;

    giti_commit_t* e = NULL;
    dlist_iterator_t* it = dlist_iterator_create(gl->entries);

    while ((e = dlist_iterator_next(it))) {

        wchar_t wstr[256] = { 0 };
        swprintf(wstr, array_size(wstr), L"%s %s %s %s %s", e->h, e->ci_date, e->an, e->ae, e->s);

        const wchar_t* str = gl->filter.str_pos ? gl->filter.str : NULL;

        bool match = !str && !gl->filter.self && !gl->filter.friends;
        match |= str && strwcmp(str, wstr);
        match |= gl->filter.self && e->is_self;
        match |= gl->filter.friends && e->is_friend;

        if (match) {
            ++gl->filter.found;
        }

        if (match || gl->filter.show_all) {
            giti_menu_item_t* item = calloc(1, sizeof *item);
            item->type = S_ITEM_TYPE_MENU;
            item->cb_str = giti_commit_row;
            item->cb_arg = e;
            item->cb_action = giti_commit_action;
            strncpy(item->name, e->s, sizeof(item->name));
            dlist_append(gl->filtered_entries, item);
        }
    }
    dlist_iterator_destroy(it);
}

static bool
giti_log_shortcut(void* gl_, uint32_t wch, uint32_t action_id, giti_window_opt_t* opt)
{
    (void)action_id;
    (void)opt;

    giti_log_t* gl = gl_;

    bool claimed = true;
    if (gl->filter.active) {
        if (wch == '\n') {
            gl->filter.active = false;
        }
        else if (wch == KEY_BACKSPACE || wch == 127 || wch == '\b') { /* backspace */
            if (gl->filter.str_pos) {
                gl->filter.str[--gl->filter.str_pos] = L'\0';
            }
        }
        else if (iswprint(wch) && gl->filter.str_pos < sizeof(gl->filter.str) - 1) {
            gl->filter.str[gl->filter.str_pos++] = wch;
        }
    }
    else {
        if (wch == g_config->keybinding.log.my) {
            gl->filter.self = !gl->filter.self;
        }
        else if (wch == g_config->keybinding.log.friends) {
            gl->filter.friends = !gl->filter.friends;
        }
        else if (wch == g_config->keybinding.log.filter) {
            gl->filter.active   = true;
            gl->filter.show_all = false;
            gl->filter.str_pos  = 0;
            memset(gl->filter.str, 0, sizeof(gl->filter.str));
        }
        else if (wch == g_config->keybinding.log.highlight) {
            gl->filter.active   = true;
            gl->filter.show_all = true;
            gl->filter.str_pos  = 0;
            memset(gl->filter.str, 0, sizeof(gl->filter.str));
        }
        else if (wch == g_config->keybinding.back) {
            if (gl->filter.str_pos) {
                gl->filter.str_pos  = 0;
                gl->filter.show_all = false;
            }
            else {
                claimed = false;
            }
        }
        else {
            claimed = false;
        }
    }

    if (claimed) {
        giti_log_filter(gl);
    }

    return claimed;
}

static dlist_t*
giti_log_color_filter(void* gl_)
{
    giti_log_t* gl = gl_;

    dlist_t* color_filter = NULL;

    if (gl->filter.str_pos) {
        color_filter = dlist_create();

        color_filter_t* f = malloc(sizeof *f);
        wcsncpy(f->str, gl->filter.str, array_size(f->str));
        f->code = 5;
        dlist_append(color_filter, f);
    }

    return color_filter;
}

static char*
giti_log_title_str(void* gl_, giti_strbuf_t strbuf)
{
    giti_log_t* gl = gl_;
    int pos = snprintf(strbuf, sizeof(giti_strbuf_t), "GITi log for Branch: <giti-clr-2>%s<giti-clr-end>", gl->branch);

    if (gl->entries && (gl->filter.active || gl->filter.str_pos)) {
        pos += snprintf(strbuf + pos, sizeof(giti_strbuf_t) - pos, " [entries: %ld/%ld]", gl->filter.found, dlist_size(gl->entries));
    }
    else {
        pos += snprintf(strbuf + pos, sizeof(giti_strbuf_t) - pos, " [entries: %ld]", dlist_size(gl->entries));
    }

    if (gl->filter.active) {
        pos += snprintf(strbuf + pos, sizeof(giti_strbuf_t) - pos, " %s[<giti-clr-on>ON<giti-clr-end>] %ls", gl->filter.show_all ? "highlight" : "filter", gl->filter.str);
    }
    else if (gl->filter.str_pos) {
        pos += snprintf(strbuf + pos, sizeof(giti_strbuf_t) - pos, " %s[<giti-clr-off>OFF<giti-clr-end>] %ls", gl->filter.show_all ? "highlight" : "filter", gl->filter.str);
    }

    return strbuf;
}

static void
giti_log_destroy(void* gl_)
{
    giti_log_t* gl = gl_;

    dlist_destroy(gl->entries, giti_commit_destroy);
    dlist_destroy(gl->filtered_entries, free);
    free(gl);
}

static giti_window_opt_t
giti_log_create(const char* branch, size_t entries)
{
    giti_log_t* gl = calloc(1, sizeof *gl);

    strncpy(gl->branch, branch, sizeof(gl->branch));

    gl->entries = giti_log_entries(branch,
    entries);

    giti_log_filter(gl);

    giti_window_opt_t opt = {
        .cb_shortcut    = giti_log_shortcut,
        .cb_filter      = giti_log_color_filter,
        .type           = S_ITEM_TYPE_MENU,
        .cb_title       = giti_log_title_str,
        .cb_destroy     = giti_log_destroy,
        .cb_arg         = gl,
        .cb_arg_destroy = gl,
        .menu           = gl->filtered_entries,
    };

    return opt;
}

/* GIIi Branch */
typedef struct giti_branch {
    giti_strbuf_t name;
    giti_strbuf_t upstream;
    dlist_t*      commits;
    bool          is_current_branch;
} giti_branch_t;

static void
giti_branch_destroy(void* gb_)
{
    giti_branch_t* gb = gb_;
    if (gb->commits) {
        dlist_destroy(gb->commits, giti_commit_destroy);
    }
    free(gb);
}

static char*
giti_branch_row(giti_strbuf_t strbuf, void* b_)
{
    giti_branch_t* b = b_;
    if (b->is_current_branch) {
        snprintf(strbuf, sizeof(giti_strbuf_t) - 1, "%s <giti-clr-3>[selected]<giti-clr-end>", b->name);
    }
    else {
        snprintf(strbuf, sizeof(giti_strbuf_t) - 1, "%s", b->name);
    }
    return strbuf;
}

static char*
giti_branch_commit_row(giti_strbuf_t strbuf, void* e_)
{
    giti_commit_t* e = e_;
    snprintf(strbuf, sizeof(giti_strbuf_t), "  %s <giti-clr-2>%s<giti-clr-end> <giti-clr-3>%s<giti-clr-end> <giti-clr-1>%-30s<giti-clr-end> %s", e->h, e->ci_date, e->ci_time, e->an, e->s);

    return strbuf;
}

static dlist_t*
giti_branch(const char* current_branch)
{
    FILE *fp;
    char res[4096];

    const char* cmd = "git for-each-ref --format='<giti-start>\n<giti-refname>%(refname:short)\n<giti-upstream>%(upstream:short)\n<giti-end>' --sort=committerdate refs/heads/";
    fp = popen(cmd, "r");
    if (fp == NULL) {
        printf("Failed to run command\n" );
        exit(1);
    }

    dlist_t* branches = dlist_create();

    giti_branch_t* b = NULL;
    while (fgets(res, sizeof(res), fp) != NULL) {
        strtrim(res);
        if (strncmp(res, "<giti-start>", strlen("<giti-start>")) == 0) {
            b = calloc(1, sizeof *b);
            b->name[0] = '\0';
            b->upstream[0] = '\0';
            b->commits = NULL;
        }
        else if (strncmp(res, "<giti-refname>", strlen("<giti-refname>")) == 0) {
            strncpy(b->name, res + strlen("<giti-refname>"), sizeof(b->name) - 1);
            b->is_current_branch = strcmp(current_branch, b->name) == 0;
        }
        else if (strncmp(res, "<giti-upstream>", strlen("<giti-upstream>")) == 0) {
            strncpy(b->upstream, res + strlen("<giti-upstream>"), sizeof(b->upstream) - 1);
        }
        else if (strncmp(res, "<giti-end>", strlen("<giti-end>")) == 0) {
            dlist_append(branches, b);
            b = NULL;
        }
    }
    pclose(fp);

    for (int i = 0; i < dlist_size(branches); ++i) {
        giti_branch_t* b = dlist_get(branches, i);
        //log("branch name: %s upstream: %s", b->name, b->upstream);

        if (strlen(b->upstream) != 0) {
            char precmd[1024];
            snprintf(precmd, sizeof(precmd), "--cherry %s..%s", b->upstream, b->name);
            b->commits = giti_log_entries_(precmd, NULL, g_config->general.timeout);
        }
    }

    return branches;
}

static char*
giti_branch_menu_title_str(void* e_, giti_strbuf_t strbuf)
{
    (void)e_;

    snprintf(strbuf, sizeof(giti_strbuf_t), "Branch Options");
    return strbuf;
}

static bool
giti_branch_shortcut(void* b_, uint32_t wch, uint32_t action_id, giti_window_opt_t* opt);

static bool
giti_branch_action(void* b_, uint32_t action_id, giti_window_opt_t* opt)
{
    giti_branch_t* b = b_;

    bool claimed = true;

    if (action_id == g_config->keybinding.logs) {
        //log("log %s", b->name);
        *opt = giti_log_create(b->name, 10000);
    }
    else if (action_id == ' ') {
        giti_config_key_strbuf_t strbuf = { 0 };

        giti_window_menu_t* gwm = giti_window_menu_create();
        opt->menu           = gwm->menu;
        opt->type           = S_ITEM_TYPE_MENU;
        opt->cb_shortcut    = giti_branch_shortcut;
        opt->xmax           = -1;
        opt->menu           = gwm->menu;
        opt->cb_title       = giti_branch_menu_title_str;
        opt->cb_destroy     = giti_window_menu_destroy;
        opt->cb_arg         = b;
        opt->cb_arg_destroy = gwm;

        giti_menu_item_t* item = NULL;
        item = calloc(1, sizeof *item);
        item->type = S_ITEM_TYPE_MENU;
        item->cb_arg = b;
        item->cb_action = giti_branch_action;
        item->action_id = g_config->keybinding.logs;
        snprintf(item->name, sizeof(item->name), "Show branch log    [%s]",
                 giti_config_key_to_string(item->action_id, strbuf));

        dlist_append(gwm->menu, item);

        if (!b->is_current_branch) {
            item = calloc(1, sizeof *item);
            item->type = S_ITEM_TYPE_MENU;
            item->cb_arg = b;
            item->cb_action = giti_branch_action;
            item->action_id = 12345;
            strncpy(item->name, "Checkout branch", sizeof(item->name));
            dlist_append(gwm->menu, item);
        }

        if (b->is_current_branch && strncmp(b->name, b->upstream, strlen(b->name)) != 0) {
            item = calloc(1, sizeof *item);
            snprintf(item->name, sizeof(item->name), "Rebase upstream branch: %s", b->upstream);
            item->type = S_ITEM_TYPE_MENU;
            item->cb_arg = b;
            item->cb_action = giti_branch_action;
            item->action_id = 12346;
            dlist_append(gwm->menu, item);
        }
    }
    else if (action_id == 12345) {
        giti_branch_checkout(b->name);
    }
    else if (action_id == 12346) {
        giti_branch_rebase(b->upstream);
    }
    else {
        claimed = false;
    }

    return claimed;
}

static bool
giti_branch_shortcut(void* b_, uint32_t wch, uint32_t action_id, giti_window_opt_t* opt)
{
    /* this could be a general function */
    giti_branch_t* b = b_;

    bool claimed = false;

    if (wch == ' ') {
        claimed = giti_branch_action(b, action_id, opt);
    }

    return claimed;
}

/* GITi Branches */
typedef struct giti_branches {
    dlist_t*      branches;
    dlist_t*      filtered_branches;
    giti_strbuf_t current_branch;
} giti_branches_t;

static bool
giti_branches_shortcut(void* gb_, uint32_t wch, uint32_t action_id, giti_window_opt_t* opt)
{
    (void)gb_;
    (void)wch;
    (void)action_id;
    (void)opt;

    return false;
}

static void
giti_branches_menu(void* data, void* arg)
{
    dlist_t* list = arg;
    giti_branch_t* b = data;

    giti_menu_item_t* item = calloc(1, sizeof *item);
    item->type = S_ITEM_TYPE_ACTION;
    item->cb_action = giti_branch_action;
    item->cb_str = giti_branch_row;
    item->cb_arg = b;
    strncpy(item->name, b->name, sizeof(item->name));
    dlist_append(list, item);

    for (int i = 0; i < dlist_size(b->commits); ++i) {
        giti_commit_t* c = dlist_get(b->commits, i);

        giti_menu_item_t* item = calloc(1, sizeof *item);
        item->type = S_ITEM_TYPE_MENU;
        item->cb_action = giti_commit_action;
        item->cb_str = giti_branch_commit_row;
        item->cb_arg = c;
        strncpy(item->name, c->s, sizeof(item->name));
        dlist_append(list, item);
    }
}

static char*
giti_branches_title_str(void* gb_, giti_strbuf_t strbuf)
{
    giti_branches_t* gb = gb_;
    snprintf(strbuf, sizeof(giti_strbuf_t), "Branches [found: %ld]", dlist_size(gb->branches));

    return strbuf;
}

static void
giti_branches_destroy(void* gbs_)
{
    giti_branches_t* gbs = gbs_;

    dlist_destroy(gbs->filtered_branches, free);
    free(gbs);
}

static giti_window_opt_t
giti_branches_create(const char* current_branch, dlist_t* branches)
{
    giti_branches_t* gb = calloc(1, sizeof *gb);
    gb->filtered_branches = dlist_create();
    gb->branches = branches;
    strncpy(gb->current_branch, current_branch, sizeof(gb->current_branch));

    giti_window_opt_t opt = {
        .type           = S_ITEM_TYPE_MENU,
        .menu           = gb->filtered_branches,
        .cb_shortcut    = giti_branches_shortcut,
        .cb_destroy     = giti_branches_destroy,
        .cb_title       = giti_branches_title_str,
        .cb_arg         = gb,
        .cb_arg_destroy = gb,
    };

    dlist_foreach(gb->branches, giti_branches_menu, gb->filtered_branches);

    return opt;
}


typedef struct giti_summary {
    giti_strbuf_t branch;
    dlist_t*      changes;
    dlist_t*      branches;
    char          text[8096];
    char*         help;
} giti_summary_t;

static char*
giti_summary_help(giti_summary_t* gs)
{
    if (!gs->help) {
        gs->help = giti_config_to_string(g_config);
    }
    return gs->help;
}

static char*
giti_summary_help_title_str(void* gs_, giti_strbuf_t strbuf)
{
    (void)gs_;
   
    snprintf(strbuf, sizeof(giti_strbuf_t), "Help");
    return strbuf;
}

static void
giti_summary_destroy(void* gs_)
{
    giti_summary_t* gs = gs_;

    dlist_destroy(gs->changes, free);
    dlist_destroy(gs->branches, giti_branch_destroy);
    free(gs);
}

static bool
giti_summary_shortcut(void* gs_, uint32_t wch, uint32_t action_id, giti_window_opt_t* opt);

static bool
giti_summary_action(void* gs_, uint32_t action_id, giti_window_opt_t* opt)
{
    giti_summary_t* gs = gs_;

    bool claimed = true;
    if (action_id == g_config->keybinding.logs) {
        *opt = giti_log_create(gs->branch, 50000);
    }
    else if (action_id == g_config->keybinding.branches) {
        *opt = giti_branches_create(gs->branch, gs->branches);
    }
    else if (action_id == ' ') {
        giti_config_key_strbuf_t strbuf = { 0 };

        giti_window_menu_t* gwm = giti_window_menu_create();
        opt->type               = S_ITEM_TYPE_MENU;
        opt->menu               = gwm->menu;
        opt->cb_shortcut        = giti_summary_shortcut;
        opt->cb_destroy         = giti_window_menu_destroy;
        opt->cb_arg             = gs;
        opt->cb_arg_destroy     = gwm;
        opt->xmax               = -1;

        giti_menu_item_t* item = NULL;
        item = calloc(1, sizeof *item);
        item->type      = S_ITEM_TYPE_MENU;
        item->cb_arg    = gs;
        item->cb_action = giti_summary_action;
        item->action_id = g_config->keybinding.logs;
        snprintf(item->name, sizeof(item->name), "Show log         [%s]",
                 giti_config_key_to_string(item->action_id, strbuf));
        dlist_append(gwm->menu, item);

        item = calloc(1, sizeof *item);
        item->type      = S_ITEM_TYPE_MENU;
        item->cb_arg    = gs;
        item->cb_action = giti_summary_action;
        item->action_id = g_config->keybinding.branches;
        snprintf(item->name, sizeof(item->name), "Show branches    [%s]",
                 giti_config_key_to_string(item->action_id, strbuf));
        dlist_append(gwm->menu, item);
    }
    else {
        claimed = false;
    }

    return claimed;
}

static bool
giti_summary_shortcut(void* gs_, uint32_t wch, uint32_t action_id, giti_window_opt_t* opt)
{
    giti_summary_t* gs = gs_;

    bool claimed = false;
    if (wch == g_config->keybinding.logs || wch == g_config->keybinding.branches || wch == ' ') {
        claimed = giti_summary_action(gs, action_id ? action_id : wch, opt);
    }
    else if (wch == g_config->keybinding.help) {
        opt->text     = giti_summary_help(gs);
        opt->type     = S_ITEM_TYPE_TEXT;
        opt->cb_title = giti_summary_help_title_str;
        opt->cb_arg   = gs;
        opt->xmax     = -1;
        claimed       = true;
    }

    return claimed;
}

static char*
giti_summary_title_str(void* gs_, giti_strbuf_t strbuf)
{
    (void)gs_;

    snprintf(strbuf, sizeof(giti_strbuf_t), "<giti-clr-1>GIT<giti-clr-end>i");
    return strbuf;
}

static giti_window_opt_t
giti_summary_create(const char* branch)
{
    giti_summary_t* gs = calloc(1, sizeof *gs);
    gs->changes = dlist_create();
    gs->branches = giti_branch(branch);

    strncpy(gs->branch, branch, sizeof(gs->branch));

    giti_status(gs->changes);

    giti_branch_t* gb = NULL;
    dlist_iterator_t* it = dlist_iterator_create(gs->branches);
    while ((gb = dlist_iterator_next(it))) {
        if (strncmp(gs->branch, gb->name, strlen(gs->branch)) == 0) {
            break;
        }
    }
    dlist_iterator_destroy(it);

    size_t pos = 0;
    pos += snprintf(gs->text + pos, sizeof(gs->text) - pos, "Hello %s (%s)\n", g_config->general.user_name, g_config->general.user_email);
    if (gb) {
        pos += snprintf(gs->text + pos, sizeof(gs->text) - pos, "On Branch <giti-clr-1>%s<giti-clr-end>", gs->branch);
        if (strlen(gb->upstream)) {
            pos += snprintf(gs->text + pos, sizeof(gs->text) - pos, " upstream: <giti-clr-2>%s<giti-clr-end>", gb->upstream);
        }
    }
    pos += snprintf(gs->text + pos, sizeof(gs->text) - pos, "\n\n");

    if (gb && dlist_size(gb->commits)) {
        pos += snprintf(gs->text + pos, sizeof(gs->text) - pos, "Commits relative to upstream\n");

        giti_commit_t* c = NULL;
        it = dlist_iterator_create(gb->commits);
        while ((c = dlist_iterator_next(it))) {
            pos += snprintf(gs->text + pos, sizeof(gs->text) - pos, "  %s %s %s %-20s %s\n", c->h, c->ci_date, c->ci_time, c->an, c->s);
        }
        dlist_iterator_destroy(it);

        pos += snprintf(gs->text + pos, sizeof(gs->text) - pos, "\n\n");
    }

    size_t pos_staged = 0;
    size_t pos_unstaged = 0;
    size_t pos_untracked = 0;

    char staged[1024];
    char unstaged[1024];
    char untracked[1024];

    giti_file_status_t* gfs = NULL;
    it = dlist_iterator_create(gs->changes);
    while ((gfs = dlist_iterator_next(it))) {
        if (gfs->staged_status == '?' && gfs->unstaged_status == '?') {
            pos_untracked += snprintf(untracked + pos_untracked, sizeof(untracked) - pos_untracked, "  %s\n", gfs->str);
            continue;
        }

        if (gfs->staged_status == 'M') {
            pos_staged += snprintf(staged + pos_staged, sizeof(staged) - pos_staged, "  Modified: %s (<giti-clr-on>%u<giti-clr-end> / <giti-clr-off>%u<giti-clr-end>)\n", gfs->str, gfs->staged.add, gfs->staged.del);
        }
        else if (gfs->staged_status == 'A') {
            pos_staged += snprintf(staged + pos_staged, sizeof(staged) - pos_staged, "  Added:    %s (<giti-clr-on>%u<giti-clr-end>)\n", gfs->str, gfs->staged.add);
        }
        else if (gfs->staged_status == 'D') {
            pos_staged += snprintf(staged + pos_staged, sizeof(staged) - pos_staged, "  Deleted:  %s (<giti-clr-off>%u<giti-clr-end>)\n", gfs->str, gfs->staged.del);
        }
        else if (gfs->staged_status != ' ') {
            pos_staged += snprintf(staged + pos_staged, sizeof(staged) - pos_staged, "  (UNKOWN) [s:%c|u:%c] %s\n", gfs->staged_status, gfs->unstaged_status, gfs->str);
        }

        if (gfs->unstaged_status == 'M') {
            pos_unstaged += snprintf(unstaged + pos_unstaged, sizeof(unstaged) - pos_unstaged, "  Modified: %s (<giti-clr-on>%u<giti-clr-end> / <giti-clr-off>%u<giti-clr-end>)\n", gfs->str, gfs->unstaged.add, gfs->unstaged.del);
        }
        else if (gfs->unstaged_status == 'D') {
            pos_unstaged += snprintf(unstaged + pos_unstaged, sizeof(unstaged) - pos_unstaged, "  Deleted:  %s (<giti-clr-off>%u<giti-clr-end>)\n", gfs->str, gfs->unstaged.del);
        }
        else if (gfs->unstaged_status != ' ') {
            pos_unstaged += snprintf(unstaged + pos_unstaged, sizeof(unstaged) - pos_unstaged, "  (UNKOWN) [s:%c|u:%c] %s\n", gfs->staged_status, gfs->unstaged_status, gfs->str);
        }
    }
    dlist_iterator_destroy(it);

    if (pos_staged) {
        pos += snprintf(gs->text + pos, sizeof(gs->text) - pos, "Staged files:\n%s\n", staged);
    }

    if (pos_unstaged) {
        pos += snprintf(gs->text + pos, sizeof(gs->text) - pos, "Unstaged files:\n%s\n", unstaged);
    }

    if (pos_untracked) {
        pos += snprintf(gs->text + pos, sizeof(gs->text) - pos, "Untracked files:\n%s\n", untracked);
    }

    giti_window_opt_t opt = {
        .cb_shortcut    = giti_summary_shortcut,
        //.cb_filter      = giti_log_color_filter,
        .type           = S_ITEM_TYPE_TEXT,
        .cb_title       = giti_summary_title_str,
        .cb_destroy     = giti_summary_destroy,
        .cb_arg         = gs,
        .cb_arg_destroy = gs,
        .text           = gs->text,
    };

    return opt;
}


static giti_window_t*
giti_window_create(giti_window_opt_t opt)
{
    giti_window_t* w = calloc(1, sizeof *w);
    w->opt = opt;

    if (w->opt.xmax == -1) {
        giti_window_set_xmax(w);
    }
    w->ymax = LINES;

    return w;
}

typedef struct giti_window_stack {
    size_t         stack_pos;
    giti_window_t* stack[25];
} giti_window_stack_t;

static void
giti_window_stack_push(giti_window_stack_t* gws, giti_window_opt_t opt)
{
    if (gws->stack_pos < 25 - 1) {
        giti_window_t* w = giti_window_create(opt);
        w->w = subwin(gws->stack[0]->w, giti_window_ymax(w), giti_window_xmax(w), 0, 0);
        gws->stack[++gws->stack_pos] = w;
    }
}

static bool
giti_window_stack_pop(giti_window_stack_t* gws)
{
    giti_window_t* gw = gws->stack[gws->stack_pos];
    werase(gw->w);
    delwin(gw->w);

    if (gw->opt.cb_destroy) {
        gw->opt.cb_destroy(gw->opt.cb_arg_destroy);
    }
    gws->stack[gws->stack_pos] = NULL;

    free(gw);

    return gws->stack_pos-- == 0;
}

static void
giti_window_stack_destroy(giti_window_stack_t* gws)
{
    free(gws);
}

typedef enum giti_window_stack_pos {
    GITI_WINDOW_STACK_INVALID,
    GITI_WINDOW_STACK_BOTTOM,
    GITI_WINDOW_STACK_TOP,
} giti_window_stack_pos_t;

static giti_window_t*
giti_window_stack_get(giti_window_stack_t* gws, giti_window_stack_pos_t pos)
{
    giti_window_t* w = NULL;
    if (pos == GITI_WINDOW_STACK_BOTTOM) {
        w = gws->stack[0];
    }
    else if (pos == GITI_WINDOW_STACK_TOP) {
        w = gws->stack[gws->stack_pos];
    }
    return w;
}

static void
giti_window_stack_display(giti_window_stack_t* gws)
{
    for (int i = 0; i <= gws->stack_pos; ++i) {
        giti_window_t* sw = gws->stack[i];
        wclear(sw->w);

        wbkgd(sw->w, giti_color_scheme(1, false));
        box(sw->w, 0, 0);

        if (sw->opt.cb_title) {
            window_title(sw->w, giti_window_xmax(sw), sw->opt.cb_title(sw->opt.cb_arg, sw->title));
        }

        dlist_t* color_filter = NULL;
        if (sw->opt.cb_filter) {
            color_filter = sw->opt.cb_filter(sw->opt.cb_arg);
        }

        if (sw->opt.type == S_ITEM_TYPE_MENU && sw->opt.menu) {
            int n = MAX(sw->pos - LINES + 3, 0);
            for (int p = 0; p < MIN((int)dlist_size(sw->opt.menu), LINES - 2); ++n, ++p) {
                giti_menu_item_t* item = dlist_get(sw->opt.menu, n);

                if (item->cb_str) {
                    giti_strbuf_t strbuf;
                    item->cb_str(strbuf, item->cb_arg);
                    giti_print(sw->w, color_filter, p + 1, 2, giti_window_lnmax(sw), strbuf, n == sw->pos, i == gws->stack_pos);
                }
                else {
                    giti_print(sw->w, color_filter, p + 1, 2, giti_window_lnmax(sw), item->name, n == sw->pos, i == gws->stack_pos);
                }
            }
        }
        else if (sw->opt.type == S_ITEM_TYPE_TEXT) {
            char *start, *end;
            start = end = (char*)sw->opt.text;
            for (int n = 0, p = 0; (end = strchr(start, '\n')) && n < giti_window_lnymax(sw); ++p){
                giti_strbuf_t strbuf;
                if (giti_window_posmax(sw) && sw->pos != giti_window_posmax(sw) && i == giti_window_lnymax(sw) - 1) {
                    snprintf(strbuf, sizeof(giti_strbuf_t), "%s", "<giti-clr-1>(more below)<giti-clr-end>");
                    giti_print(sw->w, color_filter, n + 1, 2, giti_window_lnmax(sw), strbuf, false, i == gws->stack_pos);
                }
                else if (giti_window_len(sw) <= LINES - 7 || (p >= sw->pos && giti_window_len(sw) > LINES - 7)) {
                    snprintf(strbuf, sizeof(giti_strbuf_t), "%.*s", (int)MIN(giti_window_lnmax(sw), end - start), start);
                    giti_print(sw->w, color_filter, n + 1, 2, giti_window_lnmax(sw), strbuf, false, i == gws->stack_pos);
                    ++n;
                }
                start = end + 1;
            }
        }

        if (color_filter) {
            dlist_destroy(color_filter, free);
            color_filter = NULL;
        }
    }
}

static giti_window_stack_t*
giti_window_stack_create(giti_window_opt_t opt)
{
    giti_window_stack_t* gws = calloc(1, sizeof *gws);

    giti_window_t* w = giti_window_create(opt);
    w->w = newwin(0, 0, 0, 0);
    keypad(w->w, TRUE);
    gws->stack[0] = w;

    giti_window_stack_display(gws);

    return gws;
}


/* TODO
 * version number
 * more below not working
 * scrolling
 * dot file
 * git rebase crash
 * git rebase option
 * git branch show remote
 * Help
 * arguments
 * start sceen
 *        uncommited changed
 *        log diff to upstream
 * config
 *  - clenaup memory
 *  - create config
 * add option to fetch more log entries
 */

/* MAIN */
int
main()
{
    setlocale(LC_ALL, "");
    log("-- START --");

    const char* path_config = getenv("GITI_CONFIG_FILE"); /* add this to help */
    if (!path_config) {
        path_config = "~/.gitirc";
    }
    char* str_config = NULL;
    FILE* f = fopen(path_config, "r");
    if (f) {
        fseek(f, 0L, SEEK_END);
        long len = ftell(f);
        fseek(f, 0L, SEEK_SET);
        str_config = calloc(1, len + 1);
        fread(str_config, 1, len, f);
        fclose(f);
    }

    char* user_name = giti_user_name();
    char* user_email = giti_user_email();
    g_config = giti_config_create(str_config, user_name, user_email);
    free(user_name);
    free(user_email);

    char* current_branch = giti_current_branch();

    initscr();
    start_color();
    noecho();
    curs_set(0);

    giti_window_opt_t opt = giti_summary_create(current_branch);
    giti_window_stack_t* gws = giti_window_stack_create(opt);
    //https://htmlcolorcodes.com/color-chart/
    giti_color_scheme_init(&g_config->color);

    uint32_t wch;
    while (true) {
        wget_wch(giti_window_stack_get(gws, GITI_WINDOW_STACK_BOTTOM)->w, &wch);
        //log("wch: %d", wch);

        giti_window_t* tw = giti_window_stack_get(gws, GITI_WINDOW_STACK_TOP);
        bool claimed = false;

        giti_menu_item_t* item = NULL;
        if (tw->opt.type == S_ITEM_TYPE_MENU) {
            item = dlist_get(tw->opt.menu, tw->pos);
        }
        if (tw->opt.cb_shortcut) {
            giti_window_opt_t opt = { 0 };
            claimed = tw->opt.cb_shortcut(tw->opt.cb_arg, wch, item ? item->action_id : 0, &opt);
            if (opt.type) {
                giti_window_stack_push(gws, opt);
            }
        }
        if (!claimed && tw->opt.type == S_ITEM_TYPE_MENU) {
            if (item && item->cb_action) {
                giti_window_opt_t opt = { 0 };
                claimed = item->cb_action(item->cb_arg, wch, &opt);
                if (opt.type) {
                    giti_window_stack_push(gws, opt);
                }
            }
        }

        if (!claimed) {
            if (wch == g_config->keybinding.back) {
                if (giti_window_stack_pop(gws)) {
                    goto exit;
                }
            }
            else if (wch == KEY_UP ||
                     wch == g_config->keybinding.up ||
                     wch == KEY_PPAGE ||
                     wch == g_config->keybinding.up_page) {

                int i = 1;
                i = wch == KEY_PPAGE ? 15 : i;
                i = wch == g_config->keybinding.up_page ? tw->ymax - 2 : i;
                tw->pos = MAX(0, tw->pos - i);
            }
            else if (wch == KEY_DOWN ||
                     wch == g_config->keybinding.down ||
                     wch == KEY_NPAGE ||
                     wch == g_config->keybinding.down_page) {

                int i = 1;
                i = wch == KEY_NPAGE ? 15 : i;
                i = wch == g_config->keybinding.down_page ? tw->ymax - 2 : i;
                tw->pos = MIN(giti_window_posmax(tw), tw->pos + i);
            }
        }

        giti_window_stack_display(gws);
    }

exit:
    giti_window_stack_destroy(gws);
    free(current_branch);

    log("-- END --");
    giti_exit(0, "OK");
}
