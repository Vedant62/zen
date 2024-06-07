// includes
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>

// defines
#define CTRL_KEY(k) ((k) & 0x1f)
// data
struct termios orig_termios;
// terminal
void die(const char *s)
{
    write(STDOUT_FILENO, "\x1b[2J", 4); // clears screen
    write(STDOUT_FILENO, "\1xb[H", 3);
    perror(s);
    exit(1);
}

void disableRawMode()
{
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1)
    {
        die("tcsetattr");
    }
}
void enableRawMode()
{
    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1)
        die("tcgetattr");
    atexit(disableRawMode);
    struct termios raw = orig_termios;
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
// input
void editorProcessKeypress()
{
    char c = editorReadKey();
    switch (c)
    {
    case CTRL_KEY('x'):
        write(STDOUT_FILENO, "\x1b[2J", 4); // clears screen
        write(STDOUT_FILENO, "\1xb[H", 3);
        exit(0);
        break;
    }
}
// output
void editorDrawRows()
{
    int y;
    for (y = 0; y < 24; y++)
    {
        write(STDOUT_FILENO, "~\r\n", 3);
    }
}
void editorRefreshScreen()
{
    write(STDOUT_FILENO, "\x1b[2J", 4); // clears screen
    write(STDOUT_FILENO, "\1xb[H", 3);  // cursor back to starting
    editorDrawRows();
    write(STDOUT_FILENO, "\1xb[H", 3);
}

// init
int main()
{
    enableRawMode();

    while (1)
    {
        editorRefreshScreen();
        editorProcessKeypress();
    }
    return 0;
}
