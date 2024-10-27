#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#include <linux/input.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <poll.h>



#include <fcntl.h> // fcntl.h provides definitions for open flags like O_RDWR, which we need to open the framebuffer in read/write mode
#include <stdint.h> // Provides fixed-width integer types like uint16_t
#include <sys/mman.h> // Provides mmap function and memory protection constants like PROT_READ and PROT_WRITE
#include <errno.h> // For errno and error codes like EAGAIN
#include <unistd.h> // For read(), close(), and other system calls



// Global Variables
#define FRAMEBUFFER_PATH "/dev/fb0"
#define MATRIX_SIZE 8
#define JOYSTICK_DEV_PATH "/dev/input/event0"
#define JOYSTICK_DEBOUNCE_TIME_US 800000 // in microseconds (0.8 seconds)

// Converts 8-bit RGB values to 16-bit RGB565 format used by the Sense Hat.
// RGB565 format uses:
// - 5 bits for Red
// - 6 bits for Green
// - 5 bits for Blue
// Each component is masked and shifted to fit within a 16-bit integer.
#define RGB565(r, g, b) \
    (((r & 0x1F) << 11) |  /* Red:   Keep only the lowest 5 bits and shift to the leftmost 5 bits of RGB565 */ \
     ((g & 0x3F) << 5)  |  /* Green: Keep the lowest 6 bits and shift to the middle 6 bits of RGB565 */ \
     (b & 0x1F))            /* Blue:  Keep only the lowest 5 bits, positioned in the rightmost 5 bits */

// Define a larger, varied color palette for Tetris tiles
static const uint16_t colors[] = {
    RGB565(31, 0, 0),    // Red
    RGB565(0, 63, 0),    // Green
    RGB565(0, 0, 31),    // Blue
    RGB565(31, 31, 0),   // Yellow
    RGB565(0, 31, 31),   // Cyan
    RGB565(31, 0, 31),   // Magenta
    RGB565(15, 31, 15),  // Lime
    RGB565(31, 15, 0),   // Orange
    RGB565(31, 10, 10),  // Salmon
    RGB565(10, 10, 31),  // Indigo
    RGB565(20, 31, 10),  // Spring Green
    RGB565(10, 31, 31),  // Light Blue
    RGB565(31, 20, 20),  // Pink
    RGB565(25, 25, 5),   // Olive
    RGB565(10, 20, 30),  // Slate Blue
    RGB565(20, 10, 30),  // Purple
    RGB565(15, 15, 31),  // Periwinkle
    RGB565(30, 25, 5),   // Goldenrod
    RGB565(5, 25, 5),    // Dark Green
    RGB565(15, 10, 5),   // Brown
    RGB565(5, 15, 10),   // Teal
    RGB565(20, 5, 10),   // Dark Red
    RGB565(15, 20, 31),  // Sky Blue
    RGB565(31, 15, 20),  // Rose
    RGB565(20, 31, 5),   // Lime Yellow
    RGB565(10, 5, 25),   // Deep Purple
    RGB565(31, 25, 15),  // Peach
    RGB565(25, 31, 10),  // Pale Green
    RGB565(15, 5, 31),   // Violet
    RGB565(10, 30, 25),  // Seafoam
    RGB565(25, 10, 5),   // Burnt Orange
    RGB565(5, 31, 10),   // Light Green
    RGB565(30, 15, 15),  // Coral
    RGB565(5, 5, 31),    // Electric Blue
    RGB565(31, 5, 5),    // Bright Red
    RGB565(10, 10, 5),   // Khaki
    RGB565(5, 31, 31),   // Aqua
    RGB565(31, 25, 5),   // Mustard
    RGB565(31, 15, 10),  // Rust
    RGB565(20, 5, 31),   // Plum
    RGB565(31, 31, 31)   // White
};

// Calculates the number of colors available in the 'colors' array.
// - 'sizeof(colors)' gives the total byte size of the colors array.
// - 'sizeof(colors[0])' gives the byte size of a single color element in the array.
// Dividing these gives the total number of elements (colors) in the array.
// This allows us to use COLOR_COUNT as a constant for array bounds or to safely select random colors.
#define COLOR_COUNT (sizeof(colors)/sizeof(colors[0])) 



// The game state can be used to detect what happens on the playfield
#define GAMEOVER 0
#define ACTIVE (1 << 0)
#define ROW_CLEAR (1 << 1)
#define TILE_ADDED (1 << 2)



// If you extend this structure, either avoid pointers or adjust
// the game logic allocate/deallocate and reset the memory
typedef struct
{
    bool occupied;
    uint16_t color; // Store the RGB565 color of the tile
} tile;

typedef struct
{
    unsigned int x;
    unsigned int y;
} coord;

typedef struct
{
    coord const grid;                     // playfield bounds
    unsigned long const uSecTickTime;     // tick rate
    unsigned long const rowsPerLevel;     // speed up after clearing rows
    unsigned long const initNextGameTick; // initial value of nextGameTick

    unsigned int tiles; // number of tiles played
    unsigned int rows;  // number of rows cleared
    unsigned int score; // game score
    unsigned int level; // game level

    tile *rawPlayfield; // pointer to raw memory of the playfield
    tile **playfield;   // This is the play field array
    unsigned int state;
    coord activeTile; // current tile

    unsigned long tick;         // incremeted at tickrate, wraps at nextGameTick
                                // when reached 0, next game state calculated
    unsigned long nextGameTick; // sets when tick is wrapping back to zero
                                // lowers with increasing level, never reaches 0
} gameConfig;

gameConfig game = {
    .grid = {8, 8},
    .uSecTickTime = 10000,
    .rowsPerLevel = 2,
    .initNextGameTick = 50,
};



// Custom Functions
uint16_t getRandomColor() {
    return colors[rand() % COLOR_COUNT]; // Selects a random color from the 'colors' array
}

int open_joystick() {
    // Opens the joystick device file specified by JOYSTICK_DEV_PATH for reading.
    // Parameters:
    // - O_RDONLY: Opens the file in read-only mode, as we only need to read joystick input events.
    // - O_NONBLOCK: Enables non-blocking mode, meaning `read()` calls on this file descriptor will not block 
    //   the program’s execution if there is no data to read at the moment. Instead, `read()` will return 
    //   immediately, allowing the program to continue processing other tasks and check again later.
    // If successful, `fd` will store the file descriptor for the joystick device; otherwise, an error occurs, 
    // and the program exits with a failure message.
    int fd = open(JOYSTICK_DEV_PATH, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("ERROR: Could not open joystick device");
        exit(EXIT_FAILURE);
    }
    return fd;
}

// This function should return the key that corresponds to the joystick press
// KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, with the respective direction
// and KEY_ENTER, when the the joystick is pressed
// !!! when nothing was pressed you MUST return 0 !!!
int read_joystick_event(int joystick_fd) {
    struct input_event event;

    // Reads joystick events from the device file descriptor `joystick_fd` into the `event` structure.
    // Parameters:
    // - joystick_fd: File descriptor for the joystick device, previously opened in non-blocking mode.
    // - &event: Pointer to the `event` structure where the read data will be stored.
    // - sizeof(event): Specifies the number of bytes to read, set to the size of the `event` structure.
    //
    // Returns:
    // - `bytes` will store the number of bytes actually read from the device.
    // - If `bytes` is less than `sizeof(event)`, it indicates an incomplete or failed read. This could 
    //   happen if no data is available (non-blocking mode) or an error occurred.
    // - If `errno` is set to `EAGAIN`, it means there’s no data currently available due to non-blocking mode.
    ssize_t bytes = read(joystick_fd, &event, sizeof(event));

    // Checks if the number of bytes read is less than the size of a full `event` structure, 
    // indicating an incomplete read or no data available.
    // - If `errno` is not `EAGAIN`, it means an error occurred during the read operation (other than 
    //   simply no data being available in non-blocking mode). In that case, it prints an error message.
    // - `EAGAIN` is a standard error code indicating "Resource temporarily unavailable," commonly used 
    //   with non-blocking reads to show no data is available at the moment.
    // Returns 0 to indicate no valid event was read, allowing the program to check again without blocking.
    if (bytes < (ssize_t) sizeof(event)) {
        if (errno != EAGAIN) perror("ERROR: Could not read joystick event");
        return 0; // No event or error
    }

    // Checks if the event type corresponds to a key event (button press or release).
    // - `event.type == EV_KEY` verifies that the event is a key-type event, meaning it represents a button action.
    //   If true, the code enters the switch statement to evaluate the specific button pressed.
    //
    // Inside the switch statement, the `event.code` is checked against joystick directional controls:
    // - `KEY_UP`, `KEY_DOWN`, `KEY_LEFT`, `KEY_RIGHT`, and `KEY_ENTER` represent directional buttons and
    //   the center (enter) button on the joystick.
    // - Each case corresponds to one of these button codes and returns the appropriate identifier,
    //   which the main game loop can use to control movement or actions within the game.
    // If none of these specific keys match, no action is taken.
    if (event.type == EV_KEY) {
        switch (event.code) {
            case KEY_UP:    return KEY_UP;
            case KEY_DOWN:  return KEY_DOWN;
            case KEY_LEFT:  return KEY_LEFT;
            case KEY_RIGHT: return KEY_RIGHT;
            case KEY_ENTER: return KEY_ENTER;
        }
    }

    return 0;
}

void handle_joystick_input(int key) {
    switch (key) {
        case KEY_LEFT: /* Move tile left */ break;
        case KEY_RIGHT: /* Move tile right */ break;
        case KEY_DOWN: /* Drop tile */ break;
        case KEY_ENTER: /* Exit game */ break;
        default: break;
    }
}



// This function is called on the start of your application
// Here you can initialize what ever you need for your task
// return false if something fails, else true
bool initializeSenseHat()
{
    return true;
}

// This function is called when the application exits
// Here you can free up everything that you might have opened/allocated
void freeSenseHat()
{
    // Open the framebuffer device file for the Sense Hat's LED matrix.
    // FRAMEBUFFER_PATH points to the framebuffer device (e.g., "/dev/fb0").
    // O_RDWR flag allows reading and writing to the framebuffer.
    // If successful, fb_fd stores a non-negative file descriptor for accessing the framebuffer;
    // otherwise, fb_fd will be -1, indicating an error (check with perror).
    int fb_fd = open(FRAMEBUFFER_PATH, O_RDWR);
    if (fb_fd < 0) {
        perror("ERROR: Could not open framebuffer device to clear LEDs");
        return;
    }

    // Map the framebuffer device to memory, allowing direct access to the LED matrix pixels.
    // Parameters:
    // - NULL: Lets the OS choose the starting address for the mapping.
    // - MATRIX_SIZE * MATRIX_SIZE * 2: Size of the mapping in bytes. Each pixel is 2 bytes (16-bit RGB565 format) in an 8x8 matrix.
    // - PROT_READ | PROT_WRITE: Sets the protection for the mapped memory to allow both reading and writing.
    // - MAP_SHARED: Allows changes to the mapped memory to be reflected in the underlying file (the framebuffer).
    // - fb_fd: File descriptor of the framebuffer device, obtained from open().
    // - 0: Offset from the start of the file (no offset here).
    //
    // The result is a pointer (fb) to the memory-mapped framebuffer, enabling direct pixel manipulation.
    uint16_t *fb = mmap(NULL, MATRIX_SIZE * MATRIX_SIZE * 2, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);
    if (fb == MAP_FAILED) {
        perror("ERROR: Could not map framebuffer to clear LEDs");
        close(fb_fd);
        return;
    }

    // Set each LED to black (0x0000 in RGB565) to clear the display
    for (int i = 0; i < MATRIX_SIZE * MATRIX_SIZE; i++) {
        fb[i] = 0x0000; // Black color in RGB565 format
    }

    // Unmap and release resources associated with the framebuffer:
    // - munmap(fb, MATRIX_SIZE * MATRIX_SIZE * 2): Unmaps the framebuffer from memory.
    //   Parameters:
    //   - fb: The pointer to the memory-mapped framebuffer (returned by mmap).
    //   - MATRIX_SIZE * MATRIX_SIZE * 2: The size of the mapped memory in bytes (8x8 matrix with 2 bytes per pixel).
    //   This step frees up the mapped memory space, ensuring no memory leaks.

    // - close(fb_fd): Closes the file descriptor for the framebuffer device (fb_fd).
    //   Closing fb_fd releases the underlying resource, finalizing access to the framebuffer.
    munmap(fb, MATRIX_SIZE * MATRIX_SIZE * 2);
    close(fb_fd);
}

// This function should render the gamefield on the LED matrix. It is called
// every game tick. The parameter playfieldChanged signals whether the game logic
// has changed the playfield
void renderSenseHatMatrix(bool const playfieldChanged)
{
    if (!playfieldChanged) return;

    // Open the framebuffer device file for the Sense Hat's LED matrix.
    // FRAMEBUFFER_PATH points to the framebuffer device (e.g., "/dev/fb0").
    // O_RDWR flag allows reading and writing to the framebuffer.
    // If successful, fb_fd stores a non-negative file descriptor for accessing the framebuffer;
    // otherwise, fb_fd will be -1, indicating an error (check with perror).
    int fb_fd = open(FRAMEBUFFER_PATH, O_RDWR);
    if (fb_fd < 0) {
        perror("ERROR: Could not open framebuffer device");
        return;
    }

    // Map the framebuffer device to memory, allowing direct access to the LED matrix pixels.
    // Parameters:
    // - NULL: Lets the OS choose the starting address for the mapping.
    // - MATRIX_SIZE * MATRIX_SIZE * 2: Size of the mapping in bytes. Each pixel is 2 bytes (16-bit RGB565 format) in an 8x8 matrix.
    // - PROT_READ | PROT_WRITE: Sets the protection for the mapped memory to allow both reading and writing.
    // - MAP_SHARED: Allows changes to the mapped memory to be reflected in the underlying file (the framebuffer).
    // - fb_fd: File descriptor of the framebuffer device, obtained from open().
    // - 0: Offset from the start of the file (no offset here).
    //
    // The result is a pointer (fb) to the memory-mapped framebuffer, enabling direct pixel manipulation.
    uint16_t *fb = mmap(NULL, MATRIX_SIZE * MATRIX_SIZE * 2, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);
    if (fb == MAP_FAILED) {
        perror("ERROR: Could not map framebuffer device to memory");
        close(fb_fd);
        return;
    }

    // Render the game playfield onto the LED matrix
    for (unsigned int y = 0; y < MATRIX_SIZE; y++) {          // Loop over each row in the 8x8 matrix
        for (unsigned int x = 0; x < MATRIX_SIZE; x++) {      // Loop over each column in the 8x8 matrix
            if (game.playfield[y][x].occupied) {              // Check if the current tile is occupied
                fb[y * MATRIX_SIZE + x] = game.playfield[y][x].color; // Set the LED color to the tile's assigned color
                // Explanation: 'fb' is a pointer to the framebuffer mapped to memory.
                // The expression 'y * MATRIX_SIZE + x' calculates the index in the framebuffer
                // for the pixel at coordinates (x, y) in the matrix.
                // game.playfield[y][x].color holds the RGB565 color assigned to the tile,
                // so occupied tiles retain their color while on the playfield.
            } else {
                fb[y * MATRIX_SIZE + x] = RGB565(0, 0, 0);    // Set to black (off) for empty spaces
                // Explanation: If the tile is not occupied, we set the color to RGB565(0, 0, 0),
                // which represents black (LED off) in the RGB565 color format, effectively
                // clearing any previous color from this pixel in the framebuffer.
            }
        }
    }

    // Unmap and release resources associated with the framebuffer:
    // - munmap(fb, MATRIX_SIZE * MATRIX_SIZE * 2): Unmaps the framebuffer from memory.
    //   Parameters:
    //   - fb: The pointer to the memory-mapped framebuffer (returned by mmap).
    //   - MATRIX_SIZE * MATRIX_SIZE * 2: The size of the mapped memory in bytes (8x8 matrix with 2 bytes per pixel).
    //   This step frees up the mapped memory space, ensuring no memory leaks.

    // - close(fb_fd): Closes the file descriptor for the framebuffer device (fb_fd).
    //   Closing fb_fd releases the underlying resource, finalizing access to the framebuffer.
    munmap(fb, MATRIX_SIZE * MATRIX_SIZE * 2);
    close(fb_fd);
}

// The game logic uses only the following functions to interact with the playfield.
// if you choose to change the playfield or the tile structure, you might need to
// adjust this game logic <> playfield interface

static inline void newTile(coord const target)
{
    game.playfield[target.y][target.x].occupied = true;
    game.playfield[target.y][target.x].color = getRandomColor(); // Assign random color from 'colors' array on spawn
}

static inline void copyTile(coord const to, coord const from)
{
    memcpy((void *)&game.playfield[to.y][to.x], (void *)&game.playfield[from.y][from.x], sizeof(tile));
}

static inline void copyRow(unsigned int const to, unsigned int const from)
{
    memcpy((void *)&game.playfield[to][0], (void *)&game.playfield[from][0], sizeof(tile) * game.grid.x);
}

static inline void resetTile(coord const target)
{
    memset((void *)&game.playfield[target.y][target.x], 0, sizeof(tile));
}

static inline void resetRow(unsigned int const target)
{
    memset((void *)&game.playfield[target][0], 0, sizeof(tile) * game.grid.x);
}

static inline bool tileOccupied(coord const target)
{
    return game.playfield[target.y][target.x].occupied;
}

static inline bool rowOccupied(unsigned int const target)
{
    for (unsigned int x = 0; x < game.grid.x; x++)
    {
        coord const checkTile = {x, target};
        if (!tileOccupied(checkTile))
        {
            return false;
        }
    }
    return true;
}

static inline void resetPlayfield()
{
    for (unsigned int y = 0; y < game.grid.y; y++)
    {
        resetRow(y);
    }
}

// Below here comes the game logic. Keep in mind: You are not allowed to change how the game works!
// that means no changes are necessary below this line! And if you choose to change something
// keep it compatible with what was provided to you!

bool addNewTile()
{
    game.activeTile.y = 0;
    game.activeTile.x = (game.grid.x - 1) / 2;
    if (tileOccupied(game.activeTile))
        return false;
    newTile(game.activeTile);
    return true;
}

bool moveRight()
{
    coord const newTile = {game.activeTile.x + 1, game.activeTile.y};
    if (game.activeTile.x < (game.grid.x - 1) && !tileOccupied(newTile))
    {
        copyTile(newTile, game.activeTile);
        resetTile(game.activeTile);
        game.activeTile = newTile;
        return true;
    }
    return false;
}

bool moveLeft()
{
    coord const newTile = {game.activeTile.x - 1, game.activeTile.y};
    if (game.activeTile.x > 0 && !tileOccupied(newTile))
    {
        copyTile(newTile, game.activeTile);
        resetTile(game.activeTile);
        game.activeTile = newTile;
        return true;
    }
    return false;
}

bool moveDown()
{
    coord const newTile = {game.activeTile.x, game.activeTile.y + 1};
    if (game.activeTile.y < (game.grid.y - 1) && !tileOccupied(newTile))
    {
        copyTile(newTile, game.activeTile);
        resetTile(game.activeTile);
        game.activeTile = newTile;
        return true;
    }
    return false;
}

bool clearRow()
{
    if (rowOccupied(game.grid.y - 1))
    {
        for (unsigned int y = game.grid.y - 1; y > 0; y--)
        {
            copyRow(y, y - 1);
        }
        resetRow(0);
        return true;
    }
    return false;
}

void advanceLevel()
{
    game.level++;
    switch (game.nextGameTick)
    {
    case 1:
        break;
    case 2 ... 10:
        game.nextGameTick--;
        break;
    case 11 ... 20:
        game.nextGameTick -= 2;
        break;
    default:
        game.nextGameTick -= 10;
    }
}

void newGame()
{
    game.state = ACTIVE;
    game.tiles = 0;
    game.rows = 0;
    game.score = 0;
    game.tick = 0;
    game.level = 0;
    resetPlayfield();
}

void gameOver()
{
    game.state = GAMEOVER;
    game.nextGameTick = game.initNextGameTick;
}

bool sTetris(int const key)
{
    bool playfieldChanged = false;

    if (game.state & ACTIVE)
    {
        // Move the current tile
        if (key)
        {
            playfieldChanged = true;
            switch (key)
            {
            case KEY_LEFT:
                moveLeft();
                break;
            case KEY_RIGHT:
                moveRight();
                break;
            case KEY_DOWN:
                while (moveDown())
                {
                };
                game.tick = 0;
                break;
            default:
                playfieldChanged = false;
            }
        }

        // If we have reached a tick to update the game
        if (game.tick == 0)
        {
            // We communicate the row clear and tile add over the game state
            // clear these bits if they were set before
            game.state &= ~(ROW_CLEAR | TILE_ADDED);

            playfieldChanged = true;
            // Clear row if possible
            if (clearRow())
            {
                game.state |= ROW_CLEAR;
                game.rows++;
                game.score += game.level + 1;
                if ((game.rows % game.rowsPerLevel) == 0)
                {
                    advanceLevel();
                }
            }

            // if there is no current tile or we cannot move it down,
            // add a new one. If not possible, game over.
            if (!tileOccupied(game.activeTile) || !moveDown())
            {
                if (addNewTile())
                {
                    game.state |= TILE_ADDED;
                    game.tiles++;
                }
                else
                {
                    gameOver();
                }
            }
        }
    }

    // Press any key to start a new game
    if ((game.state == GAMEOVER) && key)
    {
        playfieldChanged = true;
        newGame();
        addNewTile();
        game.state |= TILE_ADDED;
        game.tiles++;
    }

    return playfieldChanged;
}

int readKeyboard()
{
    struct pollfd pollStdin = {
        .fd = STDIN_FILENO,
        .events = POLLIN};
    int lkey = 0;

    if (poll(&pollStdin, 1, 0))
    {
        lkey = fgetc(stdin);
        if (lkey != 27)
            goto exit;
        lkey = fgetc(stdin);
        if (lkey != 91)
            goto exit;
        lkey = fgetc(stdin);
    }
exit:
    switch (lkey)
    {
    case 10:
        return KEY_ENTER;
    case 65:
        return KEY_UP;
    case 66:
        return KEY_DOWN;
    case 67:
        return KEY_RIGHT;
    case 68:
        return KEY_LEFT;
    }
    return 0;
}

void renderConsole(bool const playfieldChanged)
{
    if (!playfieldChanged)
        return;

    // Goto beginning of console
    fprintf(stdout, "\033[%d;%dH", 0, 0);
    for (unsigned int x = 0; x < game.grid.x + 2; x++)
    {
        fprintf(stdout, "-");
    }
    fprintf(stdout, "\n");
    for (unsigned int y = 0; y < game.grid.y; y++)
    {
        fprintf(stdout, "|");
        for (unsigned int x = 0; x < game.grid.x; x++)
        {
            coord const checkTile = {x, y};
            fprintf(stdout, "%c", (tileOccupied(checkTile)) ? '#' : ' ');
        }
        switch (y)
        {
        case 0:
            fprintf(stdout, "| Tiles: %10u\n", game.tiles);
            break;
        case 1:
            fprintf(stdout, "| Rows:  %10u\n", game.rows);
            break;
        case 2:
            fprintf(stdout, "| Score: %10u\n", game.score);
            break;
        case 4:
            fprintf(stdout, "| Level: %10u\n", game.level);
            break;
        case 7:
            fprintf(stdout, "| %17s\n", (game.state == GAMEOVER) ? "Game Over" : "");
            break;
        default:
            fprintf(stdout, "|\n");
        }
    }
    for (unsigned int x = 0; x < game.grid.x + 2; x++)
    {
        fprintf(stdout, "-");
    }
    fflush(stdout);
}

inline unsigned long uSecFromTimespec(struct timespec const ts)
{
    return ((ts.tv_sec * 1000000) + (ts.tv_nsec / 1000));
}



int main(int argc, char **argv)
{
    srand(time(NULL)); // Seed the random number generator for unique colors each game

    (void)argc;
    (void)argv;
    // This sets the stdin in a special state where each
    // keyboard press is directly flushed to the stdin and additionally
    // not outputted to the stdout
    {
        struct termios ttystate;
        tcgetattr(STDIN_FILENO, &ttystate);
        ttystate.c_lflag &= ~(ICANON | ECHO);
        ttystate.c_cc[VMIN] = 1;
        tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);
    }

    // Allocate the playing field structure
    game.rawPlayfield = (tile *)malloc(game.grid.x * game.grid.y * sizeof(tile));
    game.playfield = (tile **)malloc(game.grid.y * sizeof(tile *));
    if (!game.playfield || !game.rawPlayfield)
    {
        fprintf(stderr, "ERROR: could not allocate playfield\n");
        return 1;
    }
    for (unsigned int y = 0; y < game.grid.y; y++)
    {
        game.playfield[y] = &(game.rawPlayfield[y * game.grid.x]);
    }

    // Reset playfield to make it empty
    resetPlayfield();
    // Start with gameOver
    gameOver();

    if (!initializeSenseHat())
    {
        fprintf(stderr, "ERROR: could not initilize sense hat\n");
        return 1;
    };

    // Open joystick device
    int joystick_fd = open_joystick();

    // Initialize last joystick input time
    struct timeval lastJoystickTime = {0};

    // Clear console, render first time
    fprintf(stdout, "\033[H\033[J");
    renderConsole(true);
    renderSenseHatMatrix(true);

    while (true)
    {
        struct timeval sTv, eTv;
        gettimeofday(&sTv, NULL);

        // Read joystick input
        int key = read_joystick_event(joystick_fd);

        // Calculate the time difference from the last joystick event
        struct timeval currentTime;
        gettimeofday(&currentTime, NULL);

        unsigned long timeSinceLastJoystick = (currentTime.tv_sec - lastJoystickTime.tv_sec) * 1000000 + (currentTime.tv_usec - lastJoystickTime.tv_usec);

        if (key && timeSinceLastJoystick >= JOYSTICK_DEBOUNCE_TIME_US) {
            // Only process joystick input if enough time has passed since the last one
            lastJoystickTime = currentTime; // Update last joystick input time
        }
        else if (!key)
        {
            // Fall back to keyboard input if no joystick input was processed

            // NOTE: Uncomment the next line if you want to test your implementation with
            // reading the inputs from stdin. However, we expect you to read the inputs directly
            // from the input device and not from stdin (you should implement the readSenseHatJoystick method).
            key = readKeyboard();
        }
        if (key == KEY_ENTER)
            break;

        bool playfieldChanged = sTetris(key);
        renderConsole(playfieldChanged);
        renderSenseHatMatrix(playfieldChanged);

        // Wait for next tick
        gettimeofday(&eTv, NULL);
        unsigned long const uSecProcessTime = ((eTv.tv_sec * 1000000) + eTv.tv_usec) - ((sTv.tv_sec * 1000000 + sTv.tv_usec));
        if (uSecProcessTime < game.uSecTickTime)
        {
            usleep(game.uSecTickTime - uSecProcessTime);
        }
        game.tick = (game.tick + 1) % game.nextGameTick;
    }

    close(joystick_fd); // Close joystick device at the end
    freeSenseHat();
    free(game.playfield);
    free(game.rawPlayfield);

    return 0;
}
