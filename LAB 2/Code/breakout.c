/***************************************************************************************************
 * DON'T REMOVE THE VARIABLES BELOW THIS COMMENT
 **************************************************************************************************/
// Had to redo variables as the Altera compiler stuff didn't like the way it was written :/
unsigned long long __attribute__((used, section(".data"))) VGAaddress = 0xc8000000; // Memory storing pixels
unsigned int __attribute__((used, section(".data"))) red = 0x0000F0F0;
unsigned int __attribute__((used, section(".data"))) green = 0x00000F0F;
unsigned int __attribute__((used, section(".data"))) blue = 0x000000FF;
unsigned int __attribute__((used, section(".data"))) white = 0x0000FFFF;
unsigned int __attribute__((used, section(".data"))) black = 0x0;

#define n_cols 15 // <- This variable might change depending on the size of the game. Supported value range: [1,18]

char *won = "You Won";       // DON'T TOUCH THIS - keep the string as is
char *lost = "You Lost";     // DON'T TOUCH THIS - keep the string as is
unsigned short height = 240; // DON'T TOUCH THIS - keep the value as is
unsigned short width = 320;  // DON'T TOUCH THIS - keep the value as is
char font8x8[128][8];        // DON'T TOUCH THIS - this is a forward declaration
/**************************************************************************************************/
/***
 * You might use and modify the struct/enum definitions below this comment
 */
typedef struct _block
{
    unsigned char destroyed;  // 1 if the block is destroyed
    unsigned char deleted;    // 1 if the block is deleted (or removed)
    unsigned int pos_x;       // X-coordinate of the block
    unsigned int pos_y;       // Y-coordinate of the block
    unsigned int color;       // Color of the block
} Block;

typedef enum _gameState
{
    Stopped = 0,
    Running = 1,
    Won = 2,
    Lost = 3,
    Exit = 4,
} GameState;



/***
 * TODO: Define your variables below this comment
 */

extern unsigned int red;
extern unsigned int green;
extern unsigned int blue;
extern unsigned int white;
extern unsigned int black;

unsigned int racket_width = 7;
unsigned int racket_height = 7 * 7;

unsigned int ball_width = 7;
unsigned int ball_height = 7;
#define BALL_SPEED 7 // (pixels per cycle)

#define NUM_BLOCK_ROWS 10  // Number of rows of blocks
unsigned int block_width = 15;
unsigned int block_height = 15;
Block blocks[NUM_BLOCK_ROWS][n_cols];  // Array to store all the blocks
unsigned int seed = 12345;  // Initial seed value

GameState currentState = Stopped;



/***
 * Here follow the C declarations for our assembly functions
 */

// Declaration of assembly functions
void ClearScreen(void);
void SetPixel(unsigned int x_coord, unsigned int y_coord, unsigned int color);
int IsUartDataReady();
int ReadUart();
void WriteUart(char c);

void DrawField(unsigned int x, unsigned int y, unsigned int width, unsigned int height, unsigned int color);
void DrawRacket(unsigned int x, unsigned int y, unsigned int width, unsigned int height, unsigned int color);
void DrawBall(unsigned int x, unsigned int y, unsigned int width, unsigned int height, unsigned int color);
unsigned int rand();
unsigned int generate_random_color();
void DrawBlock(unsigned int x, unsigned int y, unsigned int width, unsigned int height, unsigned int color_border, unsigned int color);

void delay_ms(unsigned int ms);
void clear_uart_buffer();
void update_game_state(unsigned int ball_x, unsigned int field_start_x);

/***
 * Now follow the assembly implementations
 */

asm(".align 4\n"
    "ClearScreen: \n\t"
    "    PUSH {LR} \n\t"              // Save the link register
    "    LDR R3, =VGAaddress \n\t"    // Load the base VGA address
    "    LDR R3, [R3] \n\t"           // Get the memory location of the VGA framebuffer
    "    MOV R1, #0x0000 \n\t"        // Load black color (16-bit color code for black)
    "    MOV R2, #0 \n\t"             // Y coordinate (starting at row 0)
    "clear_row_loop: \n\t"
    "    MOV R0, #0 \n\t"             // X coordinate (starting at column 0)
    "clear_column_loop: \n\t"
    "    STRH R1, [R3], #2 \n\t"      // Write black to the screen (16-bit per pixel)
    "    ADD R0, R0, #1 \n\t"         // Increment X coordinate
    "    CMP R0, #504 \n\t"           // Compare with the width of the screen (320 pixels)
    "    BNE clear_column_loop \n\t"  // If not done with the row, repeat for the next column
    "    ADD R2, R2, #1 \n\t"         // Increment Y coordinate (move to the next row)
    "    CMP R2, #250 \n\t"           // Compare with the height of the screen (240 rows)
    "    BNE clear_row_loop \n\t"     // If not done with all rows, repeat for the next row
    "    POP {LR} \n\t"               // Restore the link register
    "    BX LR");                     // Return from function

asm(".align 4\n"
    "SetPixel: \n\t"
    "    LDR R3, =VGAaddress \n\t"
    "    LDR R3, [R3] \n\t"
    "    LSL R1, R1, #10 \n\t"  // Shift Y coordinate for row (320 pixels wide)
    "    LSL R0, R0, #1 \n\t"   // Shift X coordinate for pixel
    "    ADD R1, R1, R0 \n\t"   // Calculate offset
    "    STRH R2, [R3,R1] \n\t" // Store pixel color at calculated offset
    "    BX LR");

asm(".align 4\n"
    "IsUartDataReady: \n\t"
    "    LDR R1, =0xFF201000 \n\t"   // UART data register address
    "    LDR R2, [R1] \n\t"          // Load the value from the UART data register
    "    TST R2, #0x8000 \n\t"       // Test bit 15 (RVALID) to check if data is available
    "    BEQ no_data_ready \n\t"     // If RVALID is 0, branch to no_data_ready
    "    MOV R0, #1 \n\t"            // Set return value to 1 (data ready)
    "    BX LR \n\t"                 // Return
    "no_data_ready: \n\t"
    "    MOV R0, #0 \n\t"            // Set return value to 0 (no data)
    "    BX LR");                    // Return

asm(".align 4\n"
    "ReadUart: \n\t"
    "    LDR R1, =0xFF201000 \n\t"   // UART data register address
    "    LDR R2, [R1] \n\t"          // Load the value from the UART data register
    "    AND R0, R2, #0xFF \n\t"     // Mask bits [7:0] (received byte)
    "    BX LR");                    // Return the byte in R0

asm(".align 4\n"
    "WriteUart: \n\t"
    "    LDR R1, =0xFF201000 \n\t"   // UART data register address
    "    STRB R0, [R1] \n\t"         // Store the byte from R0 into the UART data register
    "    BX LR");                    // Return

/***
 * Now follow the C implementations
 */
void DrawField(unsigned int x, unsigned int y, unsigned int width, unsigned int height, unsigned int color)
{
    for (unsigned int i = 0; i < width; i++) {
        for (unsigned int j = 0; j < height; j++) {
            SetPixel(x + i, y + j, color);  // Call SetPixel to draw individual pixels
        }
    }
}

void DrawRacket(unsigned int x, unsigned int y, unsigned int width, unsigned int height, unsigned int color) {
    for (unsigned int i = 0; i < width; i++) {
        for (unsigned int j = 0; j < height; j++) {
            SetPixel(x + i, y + j, color);  // Call SetPixel to draw individual pixels
        }
    }
}

void DrawBall(unsigned int x, unsigned int y, unsigned int width, unsigned int height, unsigned int color) {
    for (unsigned int i = 0; i < width; i++) {
        for (unsigned int j = 0; j < height; j++) {
            SetPixel(x + i, y + j, color);  // Call SetPixel to draw individual pixels
        }
    }
}

void DrawBlock(unsigned int x, unsigned int y, unsigned int width, unsigned int height, unsigned int color_border, unsigned int color) {
    // Draw the top and bottom border
    for (unsigned int i = 0; i < width; i++) {
        SetPixel(x + i, y, color_border);                  // Top border
        SetPixel(x + i, y + height - 1, color_border);     // Bottom border
    }

    // Draw the left and right border
    for (unsigned int j = 0; j < height; j++) {
        SetPixel(x, y + j, color_border);                  // Left border
        SetPixel(x + width - 1, y + j, color_border);      // Right border
    }

    // Fill the inside
    for (unsigned int i = 1; i < width - 1; i++) {
        for (unsigned int j = 1; j < height - 1; j++) {
            SetPixel(x + i, y + j, color);                 // Inside fill
        }
    }
}

unsigned int rand()
{
    seed = (seed * 1103515245 + 12345) & 0x7FFFFFFF; // Simple LCG formula
    return (seed >> 16) & 0xFFFF; // Return a 16-bit value as the random number
}

unsigned int generate_random_color()
{
    unsigned int red = (rand() & 0x1F) << 11;   // 5-bit red component (shifted to correct RGB565 position)
    unsigned int green = (rand() & 0x3F) << 5;  // 6-bit green component
    unsigned int blue = rand() & 0x1F;          // 5-bit blue component

    return red | green | blue;  // Combine the red, green, and blue components into a 16-bit color value
}

void delay_ms(unsigned int ms)
{
    // Adjust the multiplier based on the clock speed of your system
    volatile unsigned int count;
    for (count = 0; count < ms * 1000; count++) {
        // Busy-wait loop to simulate delay
    }
}

void clear_uart_buffer()
{
    while (IsUartDataReady()) {
        ReadUart();  // Discard any remaining characters in the UART buffer
    }
}

void update_game_state(unsigned int ball_x, unsigned int field_start_x)
{
    // 1. Check if all blocks are destroyed (win condition)
    unsigned int blocks_remaining = 0;
    for (unsigned int row = 0; row < NUM_BLOCK_ROWS; row++) {
        for (unsigned int col = 0; col < n_cols; col++) {
            if (!blocks[row][col].destroyed) {
                blocks_remaining++;
            }
        }
    }

    // If no blocks remain, the player has won
    if (blocks_remaining == 0) {
        currentState = Won;
        return;
    }

    // 2. Check if the ball is out of bounds (lose condition)
    if (ball_x < (field_start_x + ball_width)) {
        currentState = Lost;
        return;
    }
}

void write(char *str)
{
    while (*str)
    {
        WriteUart(*str++); // Send each character via UART
    }
}

void play()
{
    ClearScreen();

    // Draw Playing field
    unsigned int field_start_x = 10;      // X-coordinate for the bar's position
    unsigned int field_start_y = 10;     // Y-coordinate for the bar's position
    unsigned int field_width = block_width * (n_cols + 5);   // Width of the bar
    unsigned int field_height = block_height * NUM_BLOCK_ROWS; // Height of the bar
    unsigned int field_color = white; // Color of the bar

    DrawField(field_start_x, field_start_y, field_width, field_height, field_color); // Draw the playing field

    // Draw the racket in the middle of the field height
    unsigned int racket_x = field_start_x + racket_width; // Offset by racket width
    unsigned int racket_y = field_start_y + (field_height / 2) - (racket_height / 2); // Center racket vertically within the field
    DrawRacket(racket_x, racket_y, racket_width, racket_height, blue);

    // Draw the ball in the middle next to racket
    unsigned int ball_x = racket_x + ball_width;  // Center the ball horizontally within the field
    unsigned int ball_y = field_start_y + (field_height / 2) - (ball_height / 2);  // Center the ball vertically within the field
    DrawBall(ball_x, ball_y, ball_width, ball_height, red);  // Draw the ball with red color

    // Draw all the blocks
    unsigned int block_start_x = field_start_x + (ball_width * 9);
    unsigned int block_start_y = field_start_y;

    for (unsigned int row = 0; row < NUM_BLOCK_ROWS; row++) {
        for (unsigned int col = 0; col < n_cols; col++) {
            // Calculate block position based on grid
            unsigned int block_x = block_start_x + col * block_width;
            unsigned int block_y = block_start_y + row * block_height;

            // Initialize block attributes
            blocks[row][col].pos_x = block_x;
            blocks[row][col].pos_y = block_y;
            blocks[row][col].destroyed = 0;  // Not destroyed initially
            blocks[row][col].deleted = 0;    // Not deleted initially

            // Assign a more balanced random color to the block
            blocks[row][col].color = generate_random_color();  // Generate a random RGB565 color

            // Draw the block with a border
            DrawBlock(block_x, block_y, block_width, block_height, white, blocks[row][col].color);
        }
    }

    // Wait for user input, w, s or enter
    int input = 0;
    write("Press 'w' or 's' to move racket and to start the game.\n");

    while (1) {
        // Check if UART data is ready
        if (IsUartDataReady()) {
            input = ReadUart();
            if (input == 'w' || input == 's') {
                clear_uart_buffer();  // Clear any remaining UART data
                break;  // Exit loop to start the game
            }
        }
    }

    // Reset game state and uart
    currentState = Running;
    input = 0;

    // Game loop to handle UART input and move the racket and collisions
    int previous_ball_x = ball_x;
    int previous_ball_y = ball_y;
    unsigned int ball_vx = BALL_SPEED; // Start with the ball going into the opposite direction of the racket
    unsigned int ball_vy = 0;

    while (1) {
        // Check if UART data is ready
        if (IsUartDataReady()) {
            // Read user input from UART
            input = ReadUart();

            // Move the racket up ('w') or down ('s') by 15 pixels
            if (input == 'w' && racket_y > field_start_y) {
                racket_y -= 15; // Move up by 15 pixels
            }
            else if (input == 's' && racket_y + racket_height < field_start_y + field_height) {
                racket_y += 15; // Move down by 15 pixels
            }
            else if (input == '\n') {  // Enter key pressed, exit the game
                currentState = Exit;
                break;
            }

            // Redraw the racket at the updated position
            DrawRacket((field_start_x + racket_width), field_start_y, racket_width, field_height, white);
            DrawRacket(racket_x, racket_y, racket_width, racket_height, blue);
            DrawRacket((field_start_x + racket_width), 0, racket_width, field_start_y, black);
            DrawRacket((field_start_x + racket_width), (field_start_y + field_height), racket_width, field_start_y, black);

            // Clear any remaining message
            clear_uart_buffer();
        }

        // Ball logic (START) --------------------------------------------------
        // Update ball position
        ball_x += ball_vx;
        ball_y += ball_vy;

        // Check for collisions with walls (top, bottom, left, right)
        if (ball_y <= (field_start_y + ball_height)) {
            ball_vy = BALL_SPEED;
        }
        if (ball_y >= (field_start_y + field_height - 2 * ball_height)) {
            ball_vy = -BALL_SPEED;
        }
        if (ball_x <= field_start_x) {
            ball_vx = BALL_SPEED;
        }
        if (ball_x >= (field_start_x + field_width - 2 * ball_width)) {
            ball_vx = -BALL_SPEED;
        }

        // Check for collision with racket
        if (ball_x <= (racket_x + racket_width) && ball_y >= racket_y && ball_y <= (racket_y + racket_height)) {
            ball_vx = -ball_vx;  // Invert horizontal velocity upon racket collision

            // Calculate which part of the racket the ball hits (upper, middle, or lower)
            unsigned int racket_middle = (racket_y + (racket_height / 2));

            // Offset with heights to have wiggle room for middle
            if (ball_y < (racket_middle - ball_height)) {
                // Upper part of the racket: ball should bounce upward
                ball_vy = -BALL_SPEED;  // Ensure the vertical velocity is negative (upward)
            } 
            else if (ball_y > (racket_middle + ball_height)) {
                // Lower part of the racket: ball should bounce downward
                ball_vy = BALL_SPEED;  // Ensure the vertical velocity is positive (downward)
            } 
            else {
                // Middle part of the racket: ball goes straight (no change in vertical velocity)
                ball_vy = 0;  // Ball moves straight horizontally after hitting the middle
            }
        }

        // Block Collision
        for (unsigned int row = 0; row < NUM_BLOCK_ROWS; row++) {
            for (unsigned int col = 0; col < n_cols; col++) {
                Block *block = &blocks[row][col];

                if (block->destroyed) {
                    continue; // Skip destroyed blocks
                }

                unsigned int block_left = block->pos_x;
                unsigned int block_right = block->pos_x + block_width;
                unsigned int block_top = block->pos_y;
                unsigned int block_bottom = block->pos_y + block_height;

                if (ball_x + ball_width >= block_left && ball_x <= block_right &&
                    ball_y + ball_height >= block_top && ball_y <= block_bottom) {

                    // Block collision detected: mark as destroyed and redraw as white
                    block->destroyed = 1;
                    block->color = white;
                    DrawBlock(block->pos_x, block->pos_y, block_width, block_height, white, block->color); // Erase the block

                    // Bounce ball based on side hit
                    if (ball_x + ball_width >= block_left && ball_x <= block_left + 2) {
                        ball_vx = -BALL_SPEED; // Left side hit
                    } else if (ball_x <= block_right && ball_x + ball_width >= block_right - 2) {
                        ball_vx = BALL_SPEED; // Right side hit
                    }

                    if (ball_y + ball_height >= block_top && ball_y <= block_top + 2) {
                        ball_vy = -BALL_SPEED; // Top side hit
                    } else if (ball_y <= block_bottom && ball_y + ball_height >= block_bottom - 2) {
                        ball_vy = BALL_SPEED; // Bottom side hit
                    }

                    break; // Exit block loop after collision
                }
            }
        }

        // Clear the previous ball position by drawing over it with the field color
        DrawBall(previous_ball_x, previous_ball_y, ball_width, ball_height, white);

        // Redraw the ball at the updated position
        DrawBall(ball_x, ball_y, ball_width, ball_height, red);

        // Store the current ball position as the previous position for the next loop
        previous_ball_x = ball_x;
        previous_ball_y = ball_y;
        // Ball logic (STOP) --------------------------------------------------

        // Game logic
        // Call update_game_state to check for win or loss conditions
        update_game_state(ball_x, field_start_x);

        if (currentState == Won) {
            // If the player won, send a win message and break the loop
            write(won);  // Output "You Won" to the UART
            write("\n :DDD \n\n\n");
            break;
        }
        else if (currentState == Lost) {
            // If the player lost, send a lose message and break the loop
            write(lost);  // Output "You Lost" to the UART
            write("\n");
            write(": (");
            write("\n\n\n");
            break;
        }

        // Pause
        delay_ms(200);
    }
}

int main(int argc, char *argv[])
{
    ClearScreen();

    while (1)
    {
        play();
        if (currentState == Exit)
        {
            break;
        }
    }
    return 0;
}
