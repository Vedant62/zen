// includes
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <string.h>

// defines
#define CTRL_KEY(k) ((k) & 0x1f)
#define ZEN_VERSION "0.0.1";
// data
struct editorConfig
{
    int screenrows;
    int screencols;
    struct termios orig_termios;
};
struct editorConfig E;
// terminal
void die(const char *s)
{
    write(STDOUT_FILENO, "\x1b[2J", 4); // clears screen
    write(STDOUT_FILENO, "\x1b[H", 3);
    perror(s);
    exit(1);
}

void disableRawMode()
{
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
    {
        die("tcsetattr");
    }
}
void enableRawMode()
{
    if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1)
        die("tcgetattr");
    atexit(disableRawMode);
    struct termios raw = E.orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON); // for cntrl s and q and m (10->13)
    raw.c_oflag &= ~(OPOST);                                  // disables output processing
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN); // disables echoing, canonical mode, cntrl c and z, cntrl v
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
        die("tcsetattr");
}

char editorReadKey()
{
    int nread;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) // runs until a char is read
    {
        if (nread == -1 && errno != EAGAIN)
            die("read");
    }
    return c;
}
int getWindowSize(int *rows, int *cols)
{
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0)
    {
        return -1;
    }
    else
    {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}
// appendBuffer
struct abuf
{
    char *b;
    int len;
};
#define ABUF_INIT {NULL, 0};
void abAppend(struct abuf *ab, const char *s, int len)
{
    char *new = realloc(ab->b, ab->len + len); // syntax = realloc(ptr, newSize)
    if (new == NULL)
    {
        return;
    }
    memcpy(&new[ab->len], s, len); // syntax = memcpy(to, from, numBytes)
    ab->b = new;
    ab->len += len;
}
void abFree(struct abuf *ab)
{
    free(ab->b);
}
// input
void editorProcessKeypress()
{
    char c = editorReadKey();
    switch (c)
    {
    case CTRL_KEY('x'):
        write(STDOUT_FILENO, "\x1b[2J", 4); // clears screen
        write(STDOUT_FILENO, "\x1b[H", 3);
        exit(0);
        break;
    }
}
// output
void editorDrawRows(struct abuf *ab)
{
    int y;
    for (y = 0; y < E.screenrows; y++)
    {
        if (y == E.screenrows / 3)
        {
            char welcome[80];
            int welcomelen = snprintf(welcome, sizeof(welcome), "ZEN editor -- verison 0.0.1");
            if (welcomelen > E.screencols)
                welcomelen = E.screencols;
            int padding = (E.screencols - welcomelen) / 2;
            if (padding)
            {
                abAppend(ab, "~", 1);
                padding--;
            }
            while (padding--)
            {
                abAppend(ab, " ", 1);
            }
            abAppend(ab, welcome, welcomelen);
        }
        else
        {
            abAppend(ab, "~", 1);
        }

        abAppend(ab, "\x1b[K", 3);

        if (y < E.screenrows - 1)
        {
            abAppend(ab, "\r\n", 2);
        }
    }
}
void editorRefreshScreen()
{
    struct abuf ab = ABUF_INIT;
    abAppend(&ab, "\x1b[?25l", 6);
    // abAppend(&ab, "\x1b[2J", 4); // clears screen
    abAppend(&ab, "\x1b[H", 3); // cursor back to starting
    editorDrawRows(&ab);
    abAppend(&ab, "\x1b[H", 3);
    abAppend(&ab, "\x1b[?25h", 6);
    write(STDOUT_FILENO, ab.b, ab.len);
    abFree(&ab);
}

// init
void initEditor()
{
    if (getWindowSize(&E.screenrows, &E.screencols) == -1)
        die("getWindowSize");
}
int main()
{
    enableRawMode();
    initEditor();

    while (1)
    {
        editorRefreshScreen();
        editorProcessKeypress();
    }
    return 0;
}
