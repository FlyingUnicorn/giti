#!/usr/bin/python3

import os
import sys
import subprocess
import re
import logging
import string
import re
import time
from enum import Enum

log_entries = 25
field_separator = '<|>'

class PaletteColors(Enum):
    BACKGROUND          = 'background'
    TEXT                = 'text'
    TEXT_INPUT          = 'text_input'
    TEXT_ALT1           = 'text_alt1'
    TEXT_ALT2           = 'text_alt2'
    AUTHOR              = 'author'
    ACTIVE              = 'active'
    INACTIVE            = 'inactive'
    SELECT_BG           = 'select_bg'
    SELECT_FG           = 'select_fg'
    SELECT_INACTIVE_BG  = 'select_inactive_bg'
    SELECT_INACTIVE_FG  = 'select_inactive_fg'

class Palette:
    def __init__(self, theme=None):
        self.d = {}

        # https://upload.wikimedia.org/wikipedia/commons/1/15/Xterm_256color_chart.svg
        self.d[PaletteColors.BACKGROUND]          = self.color('bg', '080808')
        self.d[PaletteColors.TEXT]                = self.color('fg', 'c6c6c6')
        self.d[PaletteColors.TEXT_INPUT]          = self.color('fg', '5fafd7')
        self.d[PaletteColors.TEXT_ALT1]           = self.color('fg', 'af00af')
        self.d[PaletteColors.TEXT_ALT2]           = self.color('fg', 'ff0087')
        self.d[PaletteColors.AUTHOR]              = self.color('fg', '00d7ff')
        self.d[PaletteColors.ACTIVE]              = self.color('fg', '00af00')
        self.d[PaletteColors.INACTIVE]            = self.color('fg', 'd70000')
        self.d[PaletteColors.SELECT_BG]           = self.color('bg', 'ffffff')
        self.d[PaletteColors.SELECT_FG]           = self.color('fg', '080808')
        self.d[PaletteColors.SELECT_INACTIVE_BG]  = self.color('bg', '808080')
        self.d[PaletteColors.SELECT_INACTIVE_FG]  = self.color('fg', '080808')

        self.underline = '\033[4m'
        self.reset     = '\033[0m'

        if theme:
            dirname = os.path.dirname(__file__)
            filename = os.path.join(dirname, theme)
            for line in open(filename):
                ln = re.sub(r"\s+", '', line)
                ln = ln.split('#')[0]
                palette_color_str, color_code = ln.split('=')

                if len(color_code) != 6:
                    logging.error('Invalid format for line: "{}"'.format(line.strip()))
                    sys.exit()

                try:
                    int(color_code, 16)
                except ValueError:
                    logging.error('Invalid format for line: "{}"'.format(line.strip()))
                    sys.exit()

                try:
                    self.set_color(palette_color_str, color_code)
                except ValueError:
                    logging.error('Invalid format for line: \n >>> "{}"'.format(line.strip()))
                    sys.exit()


    def color(self, level, hex_clr):
        level_code = 38 if level == 'fg' else 48
        r = int(hex_clr[0:2], 16)
        g = int(hex_clr[2:4], 16)
        b = int(hex_clr[4:6], 16)
        return '\033[{};2;{};{};{}m'.format(level_code, r, g, b)

    def set_color(self, palette_color_str, color_code):
        level = self.d[PaletteColors(palette_color_str)].split('\033[')[-1].split(';')[0]
        level = 'fg' if level == '38' else 'bg'
        self.d[PaletteColors(palette_color_str)] = self.color(level, color_code)


def visible_characters(s):

    strip_ANSI_escape_sequences_sub = re.compile(r"""
        \x1b     # literal ESC
        \[       # literal [
        [;\d]*   # zero or more digits or semicolons
        [A-Za-z] # a letter
        """, re.VERBOSE).sub

    return len(strip_ANSI_escape_sequences_sub("", s))

lst_pos_separators = []
def set_separator(s):
    lst_pos_separators.append(visible_characters(s) + 1)
    return ' |'

def get_separators():
    #print(lst_pos_separators)
    str_sep = list(' ' * (lst_pos_separators[-1]+1))
    for sep in lst_pos_separators:
        str_sep[sep] = '|'
    #print(''.join(str_sep))
    return ''.join(str_sep)


def pad(s, size, pos):
    l = visible_characters(s)
    p = size - l
    if p < 0:
        #for c in lst_fmt:
        #    if c in s:
        #        print(s)
        #        print(s[:size-2] + '..')
        #        sys.exit()
        return s[:size-2] + '..'

    pad = ' ' * p

    if not pos:
        return s

    if pos == '>':
        return pad + s

    if pos == '<':
        return s + pad


def get_valid_entry(str_filter, major, entry):

    (h, date, time, cr, an, ae, s, b) = entry
    if str_filter:
        str_search = (h + field_separator + major + field_separator + date + field_separator + time + field_separator + an + field_separator + ae + field_separator + s).lower()

        for sub_str in str_filter.lower().split():
            if sub_str not in str_search:
                return False

    return (h, major, date, time, cr, an, ae, s, b)

def get_formatted_entry(display, str_filter, columns, user_name, user_email, entry, major_toggle):
    str_out_sep = ' | '

    (h, m, d, t, cr, an, ae, s, _) = entry
    dct_fields = {
        '1-hash' : (11, h, '<'),
        '2-major' : (10, m, '<'),
        '3-date-time' : (16, d + ' ' + t, '<'),
        '4-author' : (20, an, '>'),
        '5-subject' : (0, s, '<')}

    str_pos = 0
    for field in dct_fields:
        width, strval, pos = dct_fields[field]
        if width:
            strval = pad(strval, width, pos)
            dct_fields[field] = strval
            str_pos += width + len(str_out_sep)
        else:
            width = columns - str_pos
            strval = pad(strval, width, pos)
            dct_fields[field] = strval


    if user_name == an and user_email == ae:
        dct_fields['4-author'] = display.color_text(PaletteColors.AUTHOR, dct_fields['4-author'])

    if str_filter:
        str_search = ''
        for field, strval in dct_fields.items():

            for sub_str in str_filter.lower().split():
                if re.search(sub_str, strval, re.IGNORECASE):
                    x = re.compile(re.escape(sub_str), re.IGNORECASE)
                    strval = x.sub(display.color_text(PaletteColors.TEXT_INPUT, sub_str.upper()), strval)

            dct_fields[field] = strval

    text_alt_color = PaletteColors.TEXT_ALT1 if major_toggle else PaletteColors.TEXT_ALT2
    dct_fields['2-major'] = display.color_text(text_alt_color, dct_fields['2-major'])

    strout = ''
    for field, strval in sorted(dct_fields.items()):
        strout += strval
        strout += str_out_sep
    else:
        strout = strout[:-len(str_out_sep)]
    return strout.strip()

def gitlog(args, entries):
    entry_separator= '<||>'

    #result = subprocess.run('stty size', stdout=subprocess.PIPE, shell=True)
    #result = result.stdout.decode('utf-8').strip().split()
    #rows = int(result[0])
    #columns = int(result[1])
    #print('user name:  <{}>'.format(user_name))
    #print('user email: <{}>'.format(user_email))
    #print("cols: {} rows: {}".format(columns, rows))

    lst_fields = ['%h', '%ci', '%cr', '%an', '%ae', '%s', '%b']
    str_format = ''
    str_format += entry_separator
    for indx, f in enumerate(lst_fields):
        str_format += f
        if indx < len(lst_fields) - 1:
             str_format += field_separator

    cmd = 'git log -n {} --pretty=format:"{}" {}'.format(entries, str_format, ' '.join(args))
    logging.debug('command: {}'.format(cmd))

    result = subprocess.run(cmd, stdout=subprocess.PIPE, shell=True)

    res = result.stdout.decode("utf-8").split(entry_separator)

    lst_major = []
    lst_entries = []
    last_major = "   ---"
    for indx, ln in enumerate(res[1:]):
        #logging.debug('result[{}]: {}'.format(indx, ln))
        fields = ln.split(field_separator)

        str_hash = fields[0]
        str_datetime = fields[1]
        str_date = fields[1].split()[0]
        str_time = ':'.join(fields[1].split()[1].split(':')[:2])

        str_cr = fields[2]
        str_an = fields[3]
        str_ae = fields[4]

        str_subject = fields[5]

        #try:
        str_body = fields[6]
        #except IndexError:
        #    str_body = ''

        if 'Gateway' in str_an and 'Version update ' in str_subject:
            major = str_subject.split('Version update ')[-1]
            lst_major.append((last_major, lst_entries))
            last_major = major
            lst_entries = []
            continue

        lst_entries.append((str_hash, str_date, str_time, str_cr, str_an, str_ae, str_subject, str_body))

    lst_sorted = []
    for major, lst_entries in lst_major:
        for e in lst_entries:
            lst_sorted.append((major, e))

    for e in lst_entries:
        lst_sorted.append(('  ---  ', e))

    return lst_sorted


def getChar():
    # figure out which function to use once, and store it in _func
    if "_func" not in getChar.__dict__:
        try:
            # for Windows-based systems
            import msvcrt # If successful, we are on Windows
            getChar._func=msvcrt.getch

        except ImportError:
            # for POSIX-based systems (with termios & tty support)
            import tty, sys, termios # raises ImportError if unsupported

            def _ttyRead():
                fd = sys.stdin.fileno()
                oldSettings = termios.tcgetattr(fd)

                try:
                    tty.setcbreak(fd)
                    answer = sys.stdin.read(1)

                finally:
                    termios.tcsetattr(fd, termios.TCSADRAIN, oldSettings)
                return answer
            getChar._func=_ttyRead

    return getChar._func()

def format_select(display, entry):

    columns = display.columns
    (h, m, d, t, cr, an, ae, s, b) = entry

    display.add_select_delimiter()
    strselect = ''
    strselect_l = 'Author:  {} ({})\n'.format(an, ae)
    strselect_l += 'Date:    {} {} ({})\n'.format(d, t, cr)
    strselect_l += 'Subject: {}\n'.format(s)
    strselect_l += '\n{}'.format(b.strip())

    cmd = 'git diff-tree --no-commit-id --stat -r {}'.format(h)
    result = subprocess.run(cmd, stdout=subprocess.PIPE, shell=True)
    strselect_r = result.stdout.decode("utf-8").strip()

    lst_l_raw = strselect_l.splitlines()
    lst_r = strselect_r.splitlines()

    split_column = 80
    if columns > 145:
        split_column = columns - 85 # diff-tree max width
    else:
        split_column = columns # remove diff-tree

    lst_l = []
    for indx in range(len(lst_l_raw)):
        ln_l = lst_l_raw[indx]

        if len(ln_l) >= split_column:
            ln = ''
            lnsp = ln_l.split()
            for w in lnsp:
                if len(ln) + len(w) >= split_column:
                    lst_l.append(ln)
                    ln = ''
                ln += w + ' '
            lst_l.append(ln)
        else:
            lst_l.append(ln_l)

    for indx in range(len(lst_l)):
        lst_l[indx] = pad(lst_l[indx], split_column, '<')

    lst_l_len = len(lst_l)
    lst_r_len = len(lst_r)

    for indx in range(len(lst_r)):
        ln_r = lst_r[indx]
        lnsp = ln_r.split('|')
        if len(lnsp) > 1 and '>' not in lnsp[1]:
            lnsp[1] = lnsp[1].replace('+', '{}'.format(display.color_text(PaletteColors.ACTIVE, '+')))
            lnsp[1] = lnsp[1].replace('-', '{}'.format(display.color_text(PaletteColors.INACTIVE, '-')))
            lst_r[indx] = '|'.join(lnsp)

    try:
        lst_r.insert(0, lst_r.pop(lst_r_len - 1))
    except IndexError:
        pass

    max_rows = 20

    for n in range(max_rows + 1):
        strselect = ''
        printed_l = 0
        if n < lst_l_len:
            if n == max_rows:
                max_reached_msg = '   < max lines reached ... >'
                strselect += max_reached_msg
                printed_l = len(max_reached_msg)
            else:
                strselect += lst_l[n]
                printed_l = len(lst_l[n])
        strselect += ' ' * (split_column - printed_l) + ' | '

        if n < lst_r_len:
            if n == max_rows:
                max_reached_msg = '   < max changed files reached ... >'
                strselect += max_reached_msg
            else:
                strselect += lst_r[n].strip()
        display.add_line(DisplayPos.SELECT, strselect)


class DisplayPos(Enum):
    INFO   = 1
    LOG    = 2
    SELECT = 3

class Display:
    def __init__(self, palette):
        self.palette = palette

        result = subprocess.run('stty size', stdout=subprocess.PIPE, shell=True)
        result = result.stdout.decode('utf-8').strip().split()
        self.rows = int(result[0])
        self.columns = int(result[1])

        self.screenbuffer_info = []
        self.screenbuffer_log = []
        self.screenbuffer_select = []

    def clear(self):
        subprocess.call('clear', shell=True)

    def add_line(self, displaypos, line):
        if displaypos == DisplayPos.INFO:
            self.screenbuffer_info.append(line)
        elif displaypos == DisplayPos.LOG:
            self.screenbuffer_log.append(line)
        elif displaypos == DisplayPos.SELECT:
            self.screenbuffer_select.append(line)

    def add_underline(self, displaypos):
        underline = self.palette.underline
        if displaypos == DisplayPos.INFO:
            self.screenbuffer_info.append(underline)
        elif displaypos == DisplayPos.LOG:
            self.screenbuffer_log.append(underline)
        elif displaypos == DisplayPos.SELECT:
            self.screenbuffer_select.append(underline)

    def add_select_delimiter(self):
        underline = self.palette.underline
        self.screenbuffer_select.append(underline)
        self.screenbuffer_select.append(underline + '\U00002588' * self.columns)

    def color_text(self, palettecolor, text):
        return self.palette.d[palettecolor] + text + self.palette.d[PaletteColors.TEXT]

    def max_log_lines(self):
        return self.rows - len(self.screenbuffer_info) - len(self.screenbuffer_select)

    def draw(self, selected_indx, select_active):

        bg = self.palette.d[PaletteColors.BACKGROUND]
        fg = self.palette.d[PaletteColors.TEXT]
        select_bg = self.palette.d[PaletteColors.SELECT_BG] if select_active else self.palette.d[PaletteColors.SELECT_INACTIVE_BG]
        select_fg = self.palette.d[PaletteColors.SELECT_FG] if select_active else self.palette.d[PaletteColors.SELECT_INACTIVE_FG]
        reset = self.palette.reset

        strbuf = ''
        for ln in self.screenbuffer_info:
            strbuf += bg + fg + pad(ln, self.columns, '<') + reset

        for indx, ln in enumerate(self.screenbuffer_log):
            ln = bg + fg + pad(ln, self.columns, '<') + reset
            if indx == selected_indx:
                ln = ln.replace(fg, select_fg)
                ln = ln.replace(bg, select_bg)
            strbuf += ln

        pad_lines = self.rows - len(self.screenbuffer_info) - len(self.screenbuffer_log) - len(self.screenbuffer_select)
        if pad_lines > 0:
            for _ in range(pad_lines):
                strbuf += '\n' + bg + ' ' * self.columns + reset

        for ln in self.screenbuffer_select:
            strbuf += bg + fg + pad(ln, self.columns, '<') + reset

        sys.stdout.writelines(strbuf)
        sys.stdout.flush()
        self.screenbuffer_info = []
        self.screenbuffer_log = []
        self.screenbuffer_select = []



if __name__ == '__main__':

    logging.basicConfig(stream=sys.stderr, level=logging.INFO)

    theme = 'themes/dark.txt'
    palette = Palette(theme)
    display = Display(palette)
    #for n in range(1, 255):
    #    print('{}: \033[{}mHELLO WORLD!{}'.format(n, n, END))
    #sys.exit()
    result = subprocess.run('stty size', stdout=subprocess.PIPE, shell=True)
    result = result.stdout.decode('utf-8').strip().split()
    rows = int(result[0])
    columns = int(result[1])
    #print('user name:  <{}>'.format(user_name))
    #print('user email: <{}>'.format(user_email))
    #print("cols: {} rows: {}".format(columns, rows))
    #sys.exit()

    cmd_gituser = 'git config user.name'
    cmd_gitemail = 'git config user.email'
    result = subprocess.run(cmd_gituser, stdout=subprocess.PIPE, shell=True)
    user_name = result.stdout.decode('utf-8').strip()

    result = subprocess.run(cmd_gitemail, stdout=subprocess.PIPE, shell=True)
    user_email = result.stdout.decode('utf-8').strip()

    lst_sorted = gitlog(sys.argv[1:], 10000)
    current = 0
    last_current = - 2
    str_filter = ''
    last_str_filter = ''
    xfilter = False
    xselect = False
    xselect_diff = False
    xselect_pos = 0
    xselect_entry = False
    last_xfilter = False
    last_xselect = False
    last_xselect_entry = False
    xexec = False
    last_xexec = False
    str_exec = ''
    last_str_exec = ''
    lst_matchesx = []
    rows = rows - 2
    ich = 0
    exec_hash = ''
    meta = False
    str_debug = ''
    total_log_entries = None
    while (True):

        updated = False
        if str_filter != last_str_filter:
            lst_matchesx = []

            for major, e in lst_sorted:
                matchx = get_valid_entry(str_filter, major, e)

                if matchx:
                    lst_matchesx.append(matchx)

            updated = True
            last_str_filter = str_filter

        elif current != last_current:
            if not lst_matchesx:
                for major, e in lst_sorted:
                    matchx = get_valid_entry(str_filter, major, e)

                    if matchx:
                        lst_matchesx.append(matchx)

            last_current = current
            updated = True

        if total_log_entries == None: total_log_entries = len(lst_matchesx) # first time only

        if updated or xfilter != last_xfilter or xselect != last_xselect or xselect_pos != last_xselect_pos or xselect_entry != last_xselect_entry or xselect_diff or str_exec != last_str_exec or xexec != last_xexec:
            display.clear()
            
            strsearch = display.color_text(PaletteColors.ACTIVE, 'search') if xfilter else display.color_text(PaletteColors.INACTIVE, 'search')
            strselect = display.color_text(PaletteColors.ACTIVE, 'select') if xselect else display.color_text(PaletteColors.INACTIVE, 'select')
            strexecute = display.color_text(PaletteColors.ACTIVE, 'execute') if xexec else display.color_text(PaletteColors.INACTIVE, 'execute')
            str_cmd_filter = '[{}] [{}] [{}] log entries: {}'.format(strselect, strsearch, strexecute, total_log_entries, xselect)

            infobar_l = display.color_text(PaletteColors.TEXT_ALT1, 'GIT') + display.color_text(PaletteColors.TEXT_ALT2, 'i') + ' ' + str_cmd_filter
            infobar_r = '{} ({})'.format(user_name, user_email)
            infobar = pad(infobar_l, columns - len(infobar_r), '<') + infobar_r
            display.add_line(DisplayPos.INFO, infobar)

            if str_filter or xfilter:
                searchbar = 'Hits: {:5} | Search string: {}'.format(len(lst_matchesx), display.color_text(PaletteColors.TEXT_INPUT, str_filter))
                display.add_line(DisplayPos.INFO, searchbar)

            if xexec:
                execbar = 'Execute command on commit ({}): {}'.format(exec_hash, str_exec)
                display.add_line(DisplayPos.INFO, execbar)

            if str_debug:
                debugbar = 'DEBUG: {}'.format(str_debug)
                display.add_line(DisplayPos.INFO, debugbar)
                
            display.add_underline(DisplayPos.INFO)

            if xselect_pos > len(lst_matchesx):
                xselect_pos = 0
            elif xselect_pos + current >= len(lst_matchesx):
                xselect_pos -= 1

            if xselect_entry:
                format_select(display, lst_matchesx[current + xselect_pos])

            max_log_lines = display.max_log_lines()
            if xselect_pos >= max_log_lines:
                xselect_pos = min(xselect_pos, max_log_lines - 1)
                current += 1
            elif xselect_pos + current < current:
                xselect_pos = 0
                current = max(current - 1, 0)

            if xselect_diff:
                (h, _, _, _, _, _, _, _, _) = lst_matchesx[current + xselect_pos]
                subprocess.call('git show {}'.format(h), shell=True)
                xselect_diff = False

            major_toggle = False
            last_major = ''
            for indx, matchx in enumerate(lst_matchesx[current:current + max_log_lines]):
                (h, m, _, _, _, _, _, _, _) = lst_matchesx[indx]
                if xselect_pos == indx:
                    exec_hash = h

                if m != last_major:
                    major_toggle = not major_toggle
                    last_major = m
                formatted_entry = get_formatted_entry(display, str_filter, columns - 6, user_name, user_email, matchx, major_toggle)
                logentry = '{:4}: {}'.format(current+indx, formatted_entry)
                display.add_line(DisplayPos.LOG, logentry)

            display.draw(xselect_pos, xselect)
            last_xfilter = xfilter
            last_xselect = xselect
            last_xselect_pos = xselect_pos
            last_xselect_entry = xselect_entry
            last_str_exec = str_exec
            last_xexec = xexec


        meta = False
        ch = getChar()
        ich = ord(ch[0])

        #str_debug = 'input: "{}" > {}'.format(ch, ich)
        #print('input: "{}" > {} e={}'.format(ch, ich, ch.encode()))

        if ch == '/' and not xfilter:
            current = 0
            xfilter = True
            continue

        if ich == 27:
            # esc
            if xfilter:
                xfilter = False
                str_filter = ''

            if xexec:
                xexec = False
                str_exec = ''

            if xselect_entry:
                xselect_entry = False

            continue

        if ich == 127:
            # backspace
            if xfilter:
                str_filter = str_filter[:-1]
            if xexec:
                str_exec = str_exec[:-1]
            ich = 0
            continue

        if ch == '\n':
            if xfilter:
                xfilter = False
                print('<< filter quit >>  '.format(str_filter), end='\r')

            if xexec:
                xexec = False
                if str_exec:
                    display.clear()
                    cmd = '{} {}'.format(str_exec, exec_hash)
                    print('command executed: {}'.format(cmd))
                    subprocess.call(cmd, shell=True)
                    sys.exit()
            continue

        if xfilter:
            ich = 0
            str_filter += ch
            continue

        if xexec:
            ich = 0
            str_exec += ch
            continue

        if xselect and ch == 'd':
            xselect_diff = True
            continue

        if ch == ' ':
            xselect = not xselect
            continue

        if ch == 'x' and xselect:
            xexec = True

        if xselect and ch == 'i':
            xselect_entry = not xselect_entry
            continue

        if ch == 'q' and not xfilter:
            display.clear()
            break

        # p or arrow
        if (ich == 112 or ich == 183) and not xfilter:
            if (xselect):
                xselect_pos -= 1
            else:
                current = max(current - 10, 0)
                ich = 0
            continue

        # n or arrow
        if (ich == 110 or ich == 184) and not xfilter:
            if xselect:
                xselect_pos += 1
            else:
                current = min(current + 10, len(lst_matchesx) - rows)
                ich = 0
            continue
