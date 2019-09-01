#!/usr/bin/python3

import os
import sys
import subprocess
import re
import logging
import string
import re
import time

cmd_gituser = 'git config user.name'
cmd_gitemail = 'git config user.email'
field_separator= '<|>'
entry_separator= '<||>'

str_out_sep = ' | '

log_entries = 25

# https://upload.wikimedia.org/wikipedia/commons/1/15/Xterm_256color_chart.svg

WHITE = '\033[37m'
CYAN = '\033[36m'
#BLUE = '\033[1;36m'
BLUE = '\033[{};2;{};{};{}m'.format(38, 0x87, 0xff, 0xaf)
GREEN = '\033[92m'
MAG = '\033[35m'
RED = '\033[{};2;{};{};{}m'.format(38, 0x87, 0x00, 0x00)
UNDERLINE = '\033[4m'
STRIKETHROUGH = '\033[9m'

END = '\033[0m'

BG_RED = '\033[{};2;{};{};{}m'.format(48, 0x44, 0x44, 0x44)
last_major = (BLUE, 0)

lst_fmt = ['\033[37m'
           '\033[36m',
           '\033[1;36m',
           '\033[93m',
           '\033[35m',
           '\033[91m',
           '\033[4m',
           '\033[41m',
           '\033[0m'
]

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

def get_formatted_entry(str_filter, columns, user_name, user_email, entry):

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
        dct_fields['4-author'] = RED + dct_fields['4-author'] + WHITE

    if str_filter:
        str_search = ''
        for field, strval in dct_fields.items():

            for sub_str in str_filter.lower().split():
                if re.search(sub_str, strval, re.IGNORECASE):
                    x = re.compile(re.escape(sub_str), re.IGNORECASE)
                    strval = x.sub(CYAN + sub_str.upper() + WHITE, strval)

            dct_fields[field] = strval

    global last_major
    (clr, last) = last_major
    major = dct_fields['2-major']
    if major != last:
        if clr == BLUE:
            clr = GREEN
        else:
            clr = BLUE
        last_major = (clr, major)
    dct_fields['2-major'] = clr + major + WHITE

    strout = ''
    for field, strval in sorted(dct_fields.items()):
        strout += strval
        strout += str_out_sep
    else:
        strout = strout[:-len(str_out_sep)]
    return strout.strip()

def gitlog(args, entries):

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

        if str_an == 'Gateway' and 'Version update ' in str_subject:
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

    display.add_underline(DisplayPos.SELECT)
    display.add_underline(DisplayPos.SELECT)

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
            lnsp[1] = lnsp[1].replace('+', '{}+{}'.format(GREEN, WHITE))
            lnsp[1] = lnsp[1].replace('-', '{}-{}'.format(RED, WHITE))
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


from enum import Enum
class DisplayPos(Enum):
    INFO = 1
    LOG = 2
    SELECT = 3

class Display:
    def __init__(self):
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
        if displaypos == DisplayPos.INFO:
            self.screenbuffer_info.append(UNDERLINE)
        elif displaypos == DisplayPos.LOG:
            self.screenbuffer_log.append(UNDERLINE)
        elif displaypos == DisplayPos.SELECT:
            self.screenbuffer_select.append(UNDERLINE)

    def max_log_lines(self):
        return self.rows - len(self.screenbuffer_info) - len(self.screenbuffer_select)

    def print(self):
        strbuf = ''
        for ln in self.screenbuffer_info:
            strbuf += pad(ln, self.columns, '<') + END
        for ln in self.screenbuffer_log:
            strbuf += pad(ln, self.columns, '<') + END

        pad_lines = self.rows - len(self.screenbuffer_info) - len(self.screenbuffer_log) - len(self.screenbuffer_select)
        if pad_lines > 0:
            for _ in range(pad_lines):
                strbuf += '\n'

        for ln in self.screenbuffer_select:
            strbuf += pad(ln, self.columns, '<') + END

        sys.stdout.writelines(strbuf)
        sys.stdout.flush()
        self.screenbuffer_info = []
        self.screenbuffer_log = []
        self.screenbuffer_select = []

#import pandas as pd
if __name__ == '__main__':

    logging.basicConfig(stream=sys.stderr, level=logging.DEBUG)

    display = Display()
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
    lst_matchesx = []
    rows = rows - 2
    ich = 0


    total_log_entries = None
    while (True):
        last_major = (BLUE, 0)
        updated = False
        if str_filter != last_str_filter:
            lst_matchesx = []
            should_print = rows

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

        if updated or xfilter != last_xfilter or xselect != last_xselect or xselect_pos != last_xselect_pos or xselect_entry != last_xselect_entry or xselect_diff:
            display.clear()

            strsearch = '{}search{}'.format(GREEN, WHITE) if xfilter else '{}search{}'.format(RED, WHITE)
            strselect = '{}select{}'.format(GREEN, WHITE) if xselect else '{}select{}'.format(RED, WHITE)
            str_cmd_filter = '[{}] [{}] log entries: {}'.format(strselect, strsearch, total_log_entries, xselect)

            infobar_l = 'jLOG {}'.format(str_cmd_filter)
            infobar_r = '{} ({})'.format(user_name, user_email)
            infobar = pad(infobar_l, columns - len(infobar_r), '<') + infobar_r
            display.add_line(DisplayPos.INFO, infobar)

            if str_filter or xfilter:
                searchbar = 'Hits: {:5} | Search string: {}{}{}'.format(len(lst_matchesx), CYAN, str_filter, WHITE)
                display.add_line(DisplayPos.INFO, searchbar)

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

            for indx, matchx in enumerate(lst_matchesx[current:current + max_log_lines]):
                strselect = '{}'.format(BG_RED) if xselect_pos == indx else ''

                formatted_entry = get_formatted_entry(str_filter, columns - 6, user_name, user_email, matchx)
                logentry = '{}{:4}: {}'.format(strselect, current+indx, formatted_entry)
                display.add_line(DisplayPos.LOG, logentry)

            display.print()
            last_xfilter = xfilter
            last_xselect = xselect
            last_xselect_pos = xselect_pos
            last_xselect_entry = xselect_entry

        ch = getChar()
        ich += ord(ch[0])
        #print('input: {} > {}'.format(ch, ich))

        if ch == '/' and not xfilter:
            current = 0
            xfilter = True
            continue

        if ich == 127 and xfilter:
            str_filter = str_filter[:-1]
            ich = 0
            continue

        if ch == '\n':
            xfilter = False
            print('<< filter quit >>  '.format(str_filter), end='\r')
            continue

        if xfilter:
            ich = 0
            str_filter += ch
            continue

        if xselect and ch == 'd':
            xselect_diff = True
            continue

        if ch == ' ':
            xselect = not xselect
            continue

        if xselect and ch == 'i':
            xselect_entry = not xselect_entry
            continue

        if ch == 'q' and not xfilter:
            display.clear()
            break

        # p or arrow
        if (ch == 'p' or ich == 183) and not xfilter:
            if (xselect):
                xselect_pos -= 1
            else:
                current = max(current - 10, 0)
                ich = 0
            continue

        # n or arrow
        if (ch == 'n' or ich == 184) and not xfilter:
            if xselect:
                xselect_pos += 1
            else:
                current = min(current + 10, len(lst_matchesx) - rows)
                ich = 0
            continue

        # page up
        if (ich == 297) and not xfilter:
            current = max(current - 50, 0)
            ich = 0
            continue

        # page down
        if (ich == 298) and not xfilter:
            current = min(current + 50, len(lst_matchesx) - rows)
            ich = 0
            continue
