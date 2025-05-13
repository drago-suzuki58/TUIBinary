#include <curses.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>


int main() {
    initscr();

    FILE *fp = fopen("./.dev/data.txt", "rb");
    if (fp == NULL) {
        printw("File could not be opened\n");
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

    int bytes_read = 0;
    size_t n = fread(arr, 1, filesize, fp);
    for (size_t i = 0; i < n; i++) {
        if (bytes_read % 16 == 0) {
            printw("%08X: ", (unsigned int)i);
        }

        printw("%02X ", (unsigned char)arr[i]);

        bytes_read++;

        if (bytes_read % 16 == 0) {
            printw("     ");
            for (int j = 0; j < 16; j++) {
                unsigned char c = arr[bytes_read - 16 + j];
                printw("%c", isprint(c) ? c : '.');
            }
            printw("\n");
        }
    }

    size_t remaining = n % 16;
    if (remaining != 0) {
        for (int i = 0; i < 16 - remaining; i++) {
            printw("   ");
        }

        printw("     ");

        for (int i = 0; i < remaining; i++) {
            unsigned char c = arr[bytes_read - remaining + i];
            printw("%c", isprint(c) ? c : '.');
        }
    }

    free(arr);
    fclose(fp);
    refresh();
    getch();
    endwin();
    return 0;
}
