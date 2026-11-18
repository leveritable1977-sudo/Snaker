#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h>
#include <sys/select.h>

void hideCursor() {
    printf("\e[?25l");
}

int msleep(long msec) {
    struct timespec ts;
    int res;
    if (msec < 0) return -1;
    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;
    do {
        res = nanosleep(&ts, &ts);
    } while (res && errno == EINTR);
    return res;
}

// Terminalinställningar för WSL
struct termios orig_termios;

void disable_raw_mode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enable_raw_mode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disable_raw_mode);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON); 
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

// Enkel och helt säker inläsning för WASD
int get_input_char() {
    struct timeval tv = { 0L, 0L };
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    
    if (select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0) {
        char c;
        if (read(STDIN_FILENO, &c, 1) > 0) {
            return c;
        }
    }
    return 0;
}

#define ROWS 10 
#define COLS 30 
#define WALL '#'

typedef struct {
    int X;
    int Y;
} Point;

#define clrscr() printf("\e[1;1H\e[2J")

void gotoxy(int x, int y) {
    printf("%c[%d;%df", 0x1B, y, x);
}

void drawBoundaries() {
    for (int row = 0; row < ROWS + 2; row++) {
        for (int col = 0; col < COLS + 2; col++) {
            if (row == 0 || row == ROWS + 1 || col == 0 || col == COLS + 1) {
                printf("%c", WALL);         
            } else {
                printf(" ");
            }
        }
        printf("\n");
    }
}

typedef enum {
    KeyboardDir_Left,    
    KeyboardDir_Right,
    KeyboardDir_Up,
    KeyboardDir_Down,
} KeyboardDir;

void spawnFood(Point *food, const Point *snake, int snake_len) {
    int valid = 0;
    while (!valid) {
        food->X = (rand() % COLS) + 1;
        food->Y = (rand() % ROWS) + 1;
        
        valid = 1;
        for (int i = 0; i < snake_len; i++) {
            if (snake[i].X == food->X && snake[i].Y == food->Y) {
                valid = 0;
                break;
            }
        }
    }
    gotoxy(food->X, food->Y);
    printf("*");
    fflush(stdout);
}

int main() {
    srand(time(NULL));
    enable_raw_mode(); 

    int snake_len = 3;
    Point *snake = malloc(snake_len * sizeof(Point));
    if (snake == NULL) return 1;

    snake[0].X = 15; snake[0].Y = 5; // Huvud ('O')
    snake[1].X = 14; snake[1].Y = 5; // Kropp ('o')
    snake[2].X = 13; snake[2].Y = 5; // Kropp ('o')

    Point food;
    int score = 0;
    KeyboardDir current_dir = KeyboardDir_Right;

    clrscr();
    hideCursor();
    drawBoundaries();
    fflush(stdout);

    spawnFood(&food, snake, snake_len);

    while (1) {
        int ch = get_input_char();
        if (ch == 'q' || ch == 'Q') break;
        
        if ((ch == 'w' || ch == 'W') && current_dir != KeyboardDir_Down) current_dir = KeyboardDir_Up;
        else if ((ch == 's' || ch == 'S') && current_dir != KeyboardDir_Up) current_dir = KeyboardDir_Down;
        else if ((ch == 'a' || ch == 'A') && current_dir != KeyboardDir_Right) current_dir = KeyboardDir_Left;
        else if ((ch == 'd' || ch == 'D') && current_dir != KeyboardDir_Left) current_dir = KeyboardDir_Right;

        Point tail = snake[snake_len - 1];

        for (int i = snake_len - 1; i > 0; i--) {
            snake[i] = snake[i - 1];
        }

        if (current_dir == KeyboardDir_Up) snake[0].Y--;
        else if (current_dir == KeyboardDir_Down) snake[0].Y++;
        else if (current_dir == KeyboardDir_Left) snake[0].X--;
        else if (current_dir == KeyboardDir_Right) snake[0].X++;

        // Väggkollision
        if (snake[0].X == 0 || snake[0].X == COLS + 1 || snake[0].Y == 0 || snake[0].Y == ROWS + 1) {
            break; 
        }

        // Självkollision
        int self_collision = 0;
        for (int i = 1; i < snake_len; i++) {
            if (snake[0].X == snake[i].X && snake[0].Y == snake[i].Y) {
                self_collision = 1;
                break;
            }
        }
        if (self_collision) break;

        // Äta mat
        if (snake[0].X == food.X && snake[0].Y == food.Y) {
            score += 10;
            snake_len++;
            Point *temp = realloc(snake, snake_len * sizeof(Point));
            if (temp == NULL) break;
            snake = temp;
            snake[snake_len - 1] = tail;
            spawnFood(&food, snake, snake_len);
        } else {
            gotoxy(tail.X, tail.Y);
            printf(" ");
        }

        // Rita orm
        for (int i = 0; i < snake_len; i++) {
            gotoxy(snake[i].X, snake[i].Y);
            if (i == 0) printf("O");
            else printf("o");
        }
        fflush(stdout); // Tvingar utskrift direkt på skärmen

        gotoxy(1, ROWS + 2);
        printf("POANG: %-5d (Q för att avsluta)", score);
        fflush(stdout); // Tvingar utskrift av poäng

        msleep(1000); 
    }

    free(snake);
    gotoxy(1, ROWS + 3);
    printf("\n=== GAME OVER! Slutpoang: %d ===\n", score);
    fflush(stdout);

    return 0;
}