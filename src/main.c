#include <curses.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void write_header(WINDOW *header, int cols, long filesize) {
    wclear(header);
    wattron(header, COLOR_PAIR(1));
    const char *left = " TUI Binary Viewer";
    char size_str[64];
    snprintf(size_str, sizeof(size_str), " Filesize: %ld (0x%lX) Bytes ", filesize, filesize);

    int left_len = (int)strlen(left);
    int right_len = (int)strlen(size_str);
    int spaces = cols - left_len - right_len;
    if (spaces < 1) spaces = 1;

    wprintw(header, "%s%*s%s", left, spaces, "", size_str);
    wattroff(header, COLOR_PAIR(1));
    wrefresh(header);
}

void write_footer(WINDOW *footer, int cols) {
    wclear(footer);
    wattron(footer, COLOR_PAIR(1));
    const char *left = " [Q] Quit [T] Jump to address (hex) [UP/DOWN] Scroll [LEFT/RIGHT] Jump Page";

    int left_len = (int)strlen(left);
    int spaces = cols - left_len;
    if (spaces < 0) spaces = 0;

    wprintw(footer, "%s%*s", left, spaces, "");
    wattroff(footer, COLOR_PAIR(1));
    wrefresh(footer);
}

void write_custom_footer(WINDOW *footer, const char *message) {
    wclear(footer);
    wprintw(footer, "%s", message);
    wrefresh(footer);
}


int main(int argc, char *argv[]) {
    initscr();

    if (argc < 2) {
        printw("Usage: %s <filename>\n", argv[0]);
        refresh();
        getch();
        endwin();
        return 1;
    }

    const char *filename = argv[1];
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        printw("File could not be opened: %s\n", filename);
        refresh();
        getch();
        endwin();
        return 1;
    }

    fseek(fp, 0, SEEK_END);
    long filesize = ftell(fp);
    rewind(fp);

    if (filesize == 0) {
        printw("File is empty\n");
        fclose(fp);
        refresh();
        getch();
        endwin();
        return 1;
    }

    char *arr = (char *)malloc(filesize);
    if (!arr) {
        printw("Memory allocation failed\n");
        fclose(fp);
        refresh();
        getch();
        endwin();
        return 1;
    }
    size_t n = fread(arr, 1, filesize, fp);

    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    int pad_lines = (n + 15) / 16 + 2;
    WINDOW *pad = newpad(pad_lines, cols);

    start_color();
    init_pair(1, COLOR_BLACK, COLOR_WHITE); // Header/Footer
    init_pair(2, COLOR_WHITE, COLOR_BLACK); // Common
    init_pair(3, COLOR_CYAN, COLOR_BLACK);  // Control characters

    int bytes_read = 0;
    for (size_t i = 0; i < n; i++) {
        if (bytes_read % 16 == 0) {
            wprintw(pad, "%08X: ", (unsigned int)i);
        }

        wprintw(pad, "%02X ", (unsigned char)arr[i]);

        bytes_read++;

        if (bytes_read % 16 == 0) {
            wprintw(pad, "     ");
            for (int j = 0; j < 16; j++) {
                unsigned char c = arr[bytes_read - 16 + j];
                if (isprint(c)) {
                    wattron(pad, COLOR_PAIR(2));
                    wprintw(pad, "%c", c);
                    wattroff(pad, COLOR_PAIR(2));
                } else {
                    wattron(pad, COLOR_PAIR(3));
                    wprintw(pad, ".");
                    wattroff(pad, COLOR_PAIR(3));
                }
            }
            wprintw(pad, "\n");
        }
    }

    size_t remaining = n % 16;
    if (remaining != 0) {
        for (int i = 0; i < 16 - remaining; i++) {
            wprintw(pad, "   ");
        }

        wprintw(pad, "     ");

        for (int i = 0; i < remaining; i++) {
            unsigned char c = arr[bytes_read - remaining + i];
            if (isprint(c)) {
                wattron(pad, COLOR_PAIR(2));
                wprintw(pad, "%c", c);
                wattroff(pad, COLOR_PAIR(2));
            } else {
                wattron(pad, COLOR_PAIR(3));
                wprintw(pad, ".");
                wattroff(pad, COLOR_PAIR(3));
            }
        }
    }

    free(arr);
    fclose(fp);
    refresh();

    WINDOW *header = newwin(1, cols, 0, 0);
    WINDOW *footer = newwin(1, cols, rows - 1, 0);

    int pad_view_rows = rows - 2;

    write_header(header, cols, filesize);
    write_footer(footer, cols);

    int pad_pos = 0;
    keypad(stdscr, TRUE);
    while (1) {
        prefresh(pad, pad_pos, 0, 1, 0, pad_view_rows, cols - 1);
        int ch = getch();
        if (ch == KEY_DOWN && pad_pos < pad_lines - rows) {
            pad_pos++;
        } else if (ch == KEY_UP && pad_pos > 0) {
            pad_pos--;
        } else if (ch == KEY_RIGHT && pad_pos < pad_lines - rows) {
            pad_pos += rows - 2;
            if (pad_pos > pad_lines - pad_view_rows) pad_pos = pad_lines - pad_view_rows - 2;
        } else if (ch == KEY_LEFT && pad_pos > 0) {
            pad_pos -= rows - 2;
            if (pad_pos < 0) pad_pos = 0;
        } else if (ch == 't' || ch == 'T') {
            write_header(header, cols, filesize);

            char buf[16] = {0};
            int idx = 0;
            int input_ch;

            char msg[64];
            snprintf(msg, sizeof(msg), "Jump to address (hex): %s", buf);
            write_custom_footer(footer, msg);

            while ((input_ch = wgetch(footer)) != 10 && input_ch != 13) {
                if ((
                    (input_ch >= '0' && input_ch <= '9') ||
                    (input_ch >= 'a' && input_ch <= 'f') ||
                    (input_ch >= 'A' && input_ch <= 'F')) && idx < 15) {
                buf[idx++] = (char)input_ch;
                    char msg[64];
                    snprintf(msg, sizeof(msg), "Jump to address (hex): %s", buf);
                    write_custom_footer(footer, msg);
                } else if ((input_ch == KEY_BACKSPACE || input_ch == 127) && idx > 0) {
                    idx--;
                    buf[idx] = '\0';
                    char msg[64];
                    snprintf(msg, sizeof(msg), "Jump to address (hex): %s", buf);
                    write_custom_footer(footer, msg);
                }
            }
            int address = (int)strtol(buf, NULL, 16);
            pad_pos = address / 16;
            if (pad_pos < 0) pad_pos = 0;
            if (pad_pos > pad_lines - pad_view_rows) pad_pos = pad_lines - pad_view_rows - 2;

            write_footer(footer, cols);
        } else if (ch == 'q' || ch == 'Q' || ch == 27) {
            break;
        } else {
            write_header(header, cols, filesize);
            write_footer(footer, cols);
        }
    }

    delwin(header);
    delwin(footer);
    delwin(pad);
    endwin();
    return 0;
}
