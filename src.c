#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


#define PIXEL_BUF_CTRL_BASE 0xFF203020  
#define PS2_BASE 0xFF200100           
#define AUDIO_BASE 0xFF203040  
#define SAMPLE_RATE 8000

//screen resolution
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

//colors
#define COLOR_BLACK 0x0000
#define COLOR_WHITE 0xFFFF
#define COLOR_BLUE 0x4D9B
#define COLOR_DARK_BLUE 0x1C58
#define COLOR_CURSOR 0x2818 //cursor color
#define COLOR_TEXT 0xFFFF
#define COLOR_HIGHLIGHT 0xFBE0 

//keyboard PS/2 scancodes
#define PS2_KEY_W 0x1D
#define PS2_KEY_A 0x1C
#define PS2_KEY_S 0x1B
#define PS2_KEY_D 0x23
#define PS2_KEY_ENTER 0x5A
#define PS2_BREAK 0xF0

static short int Buffer1[SCREEN_HEIGHT][512];
static short int Buffer2[SCREEN_HEIGHT][512];
volatile int pixel_buffer_start;


//GAME STATE
static bool gameOver = false;  //set true when lives <= 0
static bool flashPattern[49] = {false};
static bool selectedCells[49] = {false};
static bool enterPressed = false;
static int correctSelections = 0;
static int currentFlashCount = 0;
static char message[32] = "";
static int messageTimer = 0;

static int cursor_row = 0;
static int cursor_col = 0;
static int level = 1;
static int lives = 3;
static int chances = 3;


static bool inDifficultyMenu = true; // true until user picks easy/med/hard
static int difficultyIndex = 0; // 0 = easy, 1 = medium, 2 = hard
static int baseGrid = 3; // will be set to 3,4,5 once chosen

//grid constants
static const int CELL_SIZE = 20;
static const int GAP = 3;

static char read_PS2_data_byte(void);
static void poll_keyboard(void);
static void wait_for_vsync(void);
static void plot_pixel(int x, int y, short color);
static void clear_screen(short color);
static void draw_char(int x, int y, char c, short color);
static void draw_string(int x, int y, const char *text, short color);
static int get_grid_size(int lvl);
static int get_effective_grid_size(int lvl);
static void draw_grid(int n, short color);
static void draw_cursor(int row, int col, int n);
static int get_flash_count(int lvl);
static void flash_random_squares(int gridSize, int flashCount);
static void beep(void);
static void ding(void);
static void draw_scaled_char(int x, int y, char c, short color, int scale);
static void draw_big_string(int x, int y, const char *text, short color,
                            int scale);
static void draw_game_over_screen(void);
static void draw_difficulty_menu(int selected);

//FONT MAP FOR ASCII
static const unsigned char font5x7[59][7] = {
    //placeholders for ASCII 32..47
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x04, 0x04, 0x04, 0x04, 0x00, 0x00, 0x04},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},

    // '0'..'9'
    {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E},
    {0x04, 0x0C, 0x14, 0x04, 0x04, 0x04, 0x1F},
    {0x1E, 0x01, 0x01, 0x0E, 0x10, 0x10, 0x1F},
    {0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E},
    {0x11, 0x11, 0x11, 0x1F, 0x01, 0x01, 0x01},
    {0x1F, 0x10, 0x10, 0x1E, 0x01, 0x01, 0x1E},
    {0x0E, 0x10, 0x10, 0x1E, 0x11, 0x11, 0x0E},
    {0x1F, 0x01, 0x02, 0x04, 0x04, 0x04, 0x04},
    {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E},
    {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x01, 0x0E},

    //':'
    {0x00, 0x04, 0x00, 0x00, 0x00, 0x04, 0x00},

    //placeholders for ASCII 59..64
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},

    //'A'..'Z'
    {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11},
    {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E},
    {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E},
    {0x1C, 0x12, 0x11, 0x11, 0x11, 0x12, 0x1C},
    {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F},
    {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10},
    {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F},
    {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11},
    {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E},
    {0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0C},
    {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11},
    {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F},
    {0x11, 0x1B, 0x15, 0x11, 0x11, 0x11, 0x11},
    {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11},
    {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E},
    {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10},
    {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D},
    {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11},
    {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E},
    {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04},
    {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E},
    {0x11, 0x11, 0x11, 0x11, 0x0A, 0x0A, 0x04},
    {0x11, 0x11, 0x11, 0x15, 0x15, 0x1B, 0x11},
    {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11},
    {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04},
    {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F} 
};


static int get_flash_count(int lvl) { //tells how much squares to flash based on the lvl
  if (lvl < 3)
    return 4;
  else if (lvl < 6)
    return 5;
  else if (lvl < 10)
    return 9;
  else if (lvl < 15)
    return 13;
  else
    return 19;
}

//  get_grid_size: original logic -- get the grid dimensions based on the lvl
static int get_grid_size(int lvl) {
  if (lvl < 3) return 3;
  if (lvl < 6) return 4;
  if (lvl < 10) return 5;
  if (lvl < 15) return 6;
  return 7;
}

//+1 to the normal grid size for difficulty 
static int get_effective_grid_size(int lvl) {
  int normal = get_grid_size(lvl);
  int offset = (baseGrid - 3);
  int result = normal + offset;
  return result;
}

//upscaled for big writing like game over, easy, medium, hard difficulty menus
static void draw_scaled_char(int x, int y, char c, short color, int scale) {
  if (c < 32 || c > 90) return;
  int index = c - 32;

  for (int row = 0; row < 7; row++) {
    unsigned char row_bits = font5x7[index][row];
    for (int col = 0; col < 5; col++) {
      if (row_bits & (1 << (4 - col))) {
        // “Pixel” is on; draw a scale×scale block
        for (int dy = 0; dy < scale; dy++) {
          for (int dx = 0; dx < scale; dx++) {
            plot_pixel(x + col * scale + dx, y + row * scale + dy, color);
          }
        }
      }
    }
  }
}

static void draw_big_string(int x, int y, const char *text, short color,
                            int scale) {
  while (*text) {
    draw_scaled_char(x, y, *text, color, scale);
    // move x to the right by (5 * scale + 1) to space chars
    x += (5 * scale + 1);
    text++;
  }
}

//“GAME OVER” in large letters at screen center
static void draw_game_over_screen(void) {
  clear_screen(COLOR_BLACK);

  const char *msg = "GAME OVER";
  int scale = 4;
  int text_width = 9 * (5 * scale + 1); //approx 9 char width
  int text_height = 7 * scale;

  int x_start = (SCREEN_WIDTH - text_width) / 2;
  int y_start = (SCREEN_HEIGHT - text_height) / 2;

  draw_big_string(x_start, y_start, msg, COLOR_WHITE, scale);
}


static void flash_random_squares(int gridSize, int flashCount) {
  int total = gridSize * gridSize;
  int indices[total];
  for (int i = 0; i < total; i++) {
    indices[i] = i;
  }
  //shuffle the indices for the squares that we plan on showing
  for (int i = total - 1; i > 0; i--) {
    int j = rand() % (i + 1);
    int temp = indices[i];
    indices[i] = indices[j];
    indices[j] = temp;
  }

  int total_size = gridSize * CELL_SIZE + (gridSize - 1) * GAP;
  int start_x = (SCREEN_WIDTH - total_size) / 2;
  int start_y = (SCREEN_HEIGHT - total_size) / 2;

  clear_screen(COLOR_BLUE);
  draw_grid(gridSize, COLOR_DARK_BLUE);
  {
    char levelStr[16]; //this is basically drawing "Lives: 3" etc,. on the top right of the screen
    sprintf(levelStr, "LEVEL %d", level);
    draw_string(25, 5, levelStr, COLOR_TEXT);

    char livesStr[16];
    sprintf(livesStr, "LIVES: %d", lives);
    draw_string(230, 5, livesStr, COLOR_TEXT);

    char chanceStr[16];
    sprintf(chanceStr, "CHANCES: %d", chances);
    draw_string(230, 15, chanceStr, COLOR_TEXT);
  }

  //reset flash pattern
  for (int i = 0; i < total; i++) {
    flashPattern[i] = false;
    selectedCells[i] = false;
  }
  correctSelections = 0;
  currentFlashCount = flashCount;

  //flash squares
  for (int k = 0; k < flashCount; k++) {
    int idx = indices[k];
    flashPattern[idx] = true;
    int row = idx / gridSize;
    int col = idx % gridSize;
    int cell_x = start_x + col * (CELL_SIZE + GAP);
    int cell_y = start_y + row * (CELL_SIZE + GAP);
    for (int dy = 0; dy < CELL_SIZE; dy++) {
      for (int dx = 0; dx < CELL_SIZE; dx++) {
        plot_pixel(cell_x + dx, cell_y + dy, COLOR_WHITE);
      }
    }
  }

  volatile int *pixel_ctrl_ptr = (int *)PIXEL_BUF_CTRL_BASE;
  wait_for_vsync();
  pixel_buffer_start = *(pixel_ctrl_ptr + 1);

  //dead time loop, just wait 5s seconds
  for (volatile int i = 0; i < 10000000; i++);

  clear_screen(COLOR_BLUE);
  draw_grid(gridSize, COLOR_DARK_BLUE);
  {
    char levelStr[16];
    sprintf(levelStr, "LEVEL %d", level);
    draw_string(25, 5, levelStr, COLOR_TEXT);

    char livesStr[16];
    sprintf(livesStr, "LIVES: %d", lives);
    draw_string(230, 5, livesStr, COLOR_TEXT);

    char chanceStr[16];
    sprintf(chanceStr, "CHANCES: %d", chances);
    draw_string(230, 15, chanceStr, COLOR_TEXT);
  }
  wait_for_vsync();
  pixel_buffer_start = *(pixel_ctrl_ptr + 1);
}

//PS/2 KEYBOARD
static char read_PS2_data_byte(void) {
  volatile int *PS2_ptr = (int *)PS2_BASE;
  int ps2_data = *PS2_ptr; // read the 32-bit PS/2 data register
  int RVALID = ps2_data & 0x8000;  //bit 15 => RVALID

  if (!RVALID) return 0;  // no new data

  //the low 8 bits of ps2_data is the scancode
  return (char)(ps2_data & 0xFF);
}

static void beep(void) { //beep for any wasd
  struct audio_t {
    volatile unsigned int control;
    volatile unsigned char rarc;
    volatile unsigned char ralc;
    volatile unsigned char wsrc;
    volatile unsigned char wslc;
    volatile unsigned int ldata;
    volatile unsigned int rdata;
  };
  struct audio_t *audiop = (struct audio_t *)AUDIO_BASE;
  int amplitude = 0x0FFFFFFF;  // max 24-bit
  int frequency = 1000;        // beep freq (Hz)
  int period_samples = SAMPLE_RATE / (2 * frequency);
  int total_samples = SAMPLE_RATE / 10;  // ~0.1 sec beep
  int sample_count = 0;
  int wave_state = 1;
  int inner_sample = 0;

  while (sample_count < total_samples) {
    if (audiop->wsrc > 0 && audiop->wslc > 0) {
      int sample_value = wave_state ? amplitude : -amplitude;
      audiop->ldata = sample_value;
      audiop->rdata = sample_value;
      inner_sample++;
      if (inner_sample >= period_samples) {
        inner_sample = 0;
        wave_state = !wave_state;
      }
      sample_count++;
    }
  }
}

static void ding(void) { //ding for <enter>
  struct audio_t {
    volatile unsigned int control;
    volatile unsigned char rarc;
    volatile unsigned char ralc;
    volatile unsigned char wsrc;
    volatile unsigned char wslc;
    volatile unsigned int ldata;
    volatile unsigned int rdata;
  };
  struct audio_t *audiop = (struct audio_t *)AUDIO_BASE;
  int amplitude = 0x0FFFFFFF;  // max 24-bit
  int frequency = 1500;        // ding freq (Hz)
  int period_samples = SAMPLE_RATE / (2 * frequency);
  int total_samples = SAMPLE_RATE / 20;  // ~0.05 sec
  int sample_count = 0;
  int wave_state = 1;
  int inner_sample = 0;

  while (sample_count < total_samples) {
    if (audiop->wsrc > 0 && audiop->wslc > 0) {
      int sample_value = wave_state ? amplitude : -amplitude;
      audiop->ldata = sample_value;
      audiop->rdata = sample_value;
      inner_sample++;
      if (inner_sample >= period_samples) {
        inner_sample = 0;
        wave_state = !wave_state;
      }
      sample_count++;
    }
  }
}


static void poll_keyboard(void) { //basic poll loop for the keyboard
  static bool break_seen = false;
  char byte = read_PS2_data_byte();
  if (byte == 0) return;  // no new data
  if (byte == PS2_BREAK) {
    break_seen = true;
    return;
  }
  if (break_seen) {
    break_seen = false;
    return;
  }

  if (inDifficultyMenu) {
    //only listen for A, D, and Enter (no need for WS)
    switch (byte) {
      case PS2_KEY_A:
        // move selection left if possible
        if (difficultyIndex > 0) {
          difficultyIndex--;
          beep();
        }
        break;
      case PS2_KEY_D:
        // move selection right if possible
        if (difficultyIndex < 2) {
          difficultyIndex++;
          beep();
        }
        break;
      case PS2_KEY_ENTER:
        enterPressed = true;
        ding();
        break;
      default:
        // ignore other keys in the menu
        break;
    }
  } else {
    //normal game input
    switch (byte) {
      case PS2_KEY_W:
        cursor_row--;
        beep();
        break;
      case PS2_KEY_S:
        cursor_row++;
        beep();
        break;
      case PS2_KEY_A:
        cursor_col--;
        beep();
        break;
      case PS2_KEY_D:
        cursor_col++;
        beep();
        break;
      case PS2_KEY_ENTER:
        enterPressed = true;
        ding();
        break;
      default:
        break;
    }
  }
}

//basic VGA functions
static void wait_for_vsync(void) {
  volatile int *pixel_ctrl_ptr = (int *)PIXEL_BUF_CTRL_BASE;
  *pixel_ctrl_ptr = 1;  // start sync
  while ((*(pixel_ctrl_ptr + 3) & 0x01)) {
    // wait for status bit
  }
}

static void plot_pixel(int x, int y, short color) {
  if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return;
  volatile short *pixel_addr =
      (volatile short *)(pixel_buffer_start + (y << 10) + (x << 1));
  *pixel_addr = color;
}

static void clear_screen(short color) {
  for (int row = 0; row < SCREEN_HEIGHT; row++) {
    for (int col = 0; col < SCREEN_WIDTH; col++) {
      plot_pixel(col, row, color);
    }
  }
}

//draw the text using the ascii font maps
static void draw_char(int x, int y, char c, short color) {
  if (c < 32 || c > 90) return;
  int index = c - 32;

  for (int row = 0; row < 7; row++) {
    unsigned char row_bits = font5x7[index][row];
    for (int col = 0; col < 5; col++) {
      if (row_bits & (1 << (4 - col))) {
        plot_pixel(x + col, y + row, color);
      }
    }
  }
}

static void draw_string(int x, int y, const char *text, short color) {
  while (*text) {
    draw_char(x, y, *text, color);
    x += 6;  // char is 5 wide + 1 space
    text++;
  }
}


static void draw_grid(int n, short color) {
  int total_size = n * CELL_SIZE + (n - 1) * GAP;
  int start_x = (SCREEN_WIDTH - total_size) / 2;
  int start_y = (SCREEN_HEIGHT - total_size) / 2;

  for (int row = 0; row < n; row++) { //basically draw a square for n amount of times, where n is from the level we are on right now
    for (int col = 0; col < n; col++) {
      int x = start_x + col * (CELL_SIZE + GAP); //gap is the dead space between the squares
      int y = start_y + row * (CELL_SIZE + GAP);
      for (int dy = 0; dy < CELL_SIZE; dy++) {
        for (int dx = 0; dx < CELL_SIZE; dx++) {
          plot_pixel(x + dx, y + dy, color);
        }
      }
    }
  }
}

static void draw_cursor(int row, int col, int n) {
  // clamp row, col
  if (row < 0) row = 0;
  if (col < 0) col = 0;
  if (row >= n) row = n - 1;
  if (col >= n) col = n - 1;

  cursor_row = row;
  cursor_col = col;

  int total_size = n * CELL_SIZE + (n - 1) * GAP;
  int start_x = (SCREEN_WIDTH - total_size) / 2;
  int start_y = (SCREEN_HEIGHT - total_size) / 2;

  int cell_x = start_x + col * (CELL_SIZE + GAP);
  int cell_y = start_y + row * (CELL_SIZE + GAP);

  // enter of the cell
  int center = CELL_SIZE / 2;
  int halfLength = 4;
  int thickness = 3;

  //horizontal line
  for (int dx = center - halfLength; dx <= center + halfLength; dx++) {
    for (int t = -(thickness / 2); t <= (thickness / 2); t++) {
      int yPos = center + t;
      if (dx >= 0 && dx < CELL_SIZE && yPos >= 0 && yPos < CELL_SIZE) {
        plot_pixel(cell_x + dx, cell_y + yPos, COLOR_CURSOR);
      }
    }
  }
  //vertical line
  for (int dy = center - halfLength; dy <= center + halfLength; dy++) {
    for (int t = -(thickness / 2); t <= (thickness / 2); t++) {
      int xPos = center + t;
      if (dy >= 0 && dy < CELL_SIZE && xPos >= 0 && xPos < CELL_SIZE) {
        plot_pixel(cell_x + xPos, cell_y + dy, COLOR_CURSOR);
      }
    }
  }
}


// draw_difficulty_menu: draws three squares for "Easy", "Medium", "Hard"
static void draw_difficulty_menu(int selected) {
  clear_screen(COLOR_BLACK);

  //square dimension
  int sq_w = 60, sq_h = 50;
  // x positions for the 3 squares
  int left_x[3] = {50, 130, 210}; 
  int top_y = 80;
  const char *labels[3] = {"EASY", "MEDIUM", "HARD"};

  for (int i = 0; i < 3; i++) {
    //if i == selected, highlight
    short fillColor = (i == selected) ? COLOR_HIGHLIGHT : COLOR_BLUE;
    //draw the filled rectangle
    for (int y = 0; y < sq_h; y++) {
      for (int x = 0; x < sq_w; x++) {
        plot_pixel(left_x[i] + x, top_y + y, fillColor);
      }
    }

    int labelLen = strlen(labels[i]); //draw the label in the center of the squares
    int labelPxWidth = labelLen * 6;
    int labelX = left_x[i] + (sq_w - labelPxWidth) / 2;
    int labelY = top_y + (sq_h - 7) / 2;
    draw_string(labelX, labelY, labels[i], COLOR_TEXT);
  }
}

//MAIN LOOP
int main(void) {
  srand(time(NULL));  //seed random

  volatile int *pixel_ctrl_ptr = (int *)PIXEL_BUF_CTRL_BASE;

  //first back buffer -> Buffer1
  *(pixel_ctrl_ptr + 1) = (int)Buffer1;
  wait_for_vsync();
  pixel_buffer_start = *pixel_ctrl_ptr;
  clear_screen(COLOR_BLACK);

  //second back buffer -> Buffer2
  *(pixel_ctrl_ptr + 1) = (int)Buffer2;
  pixel_buffer_start = *(pixel_ctrl_ptr + 1);
  clear_screen(COLOR_BLACK);

  int lastFlashedLevel = -1;

  while (1) {
    //always poll keyboard first
    poll_keyboard();

    if (inDifficultyMenu) {
      //draw the difficulty selection screen
      draw_difficulty_menu(difficultyIndex);
      wait_for_vsync();
      pixel_buffer_start = *(pixel_ctrl_ptr + 1);

      if (enterPressed) {
        enterPressed = false;
        //set baseGrid based on difficultyIndex
        if (difficultyIndex == 0) {
          baseGrid = 3; //easy
        } else if (difficultyIndex == 1) {
          baseGrid = 4; //medium
        } else {
          baseGrid = 5; //hard
        }
        inDifficultyMenu = false; //start the game
        clear_screen(COLOR_BLACK);
      }
      //skip the rest of the game logic if still in menu
      if (inDifficultyMenu) {
        continue;
      }
    }

    //show "GAME OVER" and allow pressing Enter to reset
    if (gameOver) {
	  draw_game_over_screen();
	  wait_for_vsync();
	  pixel_buffer_start = *(pixel_ctrl_ptr + 1);

	  if (enterPressed) {
		enterPressed = false;
		gameOver = false;
		inDifficultyMenu = true;  //re-enable difficulty selection
		level = 1;
		lives = 3;
		chances = 3;
		cursor_row = 0;
		cursor_col = 0;
		message[0] = '\0';
		messageTimer = 0;
		lastFlashedLevel = -1;
		clear_screen(COLOR_BLACK);
		continue;  //immediately skip the rest of the game loop
	  }
	  continue;
	}
    int gridSize = get_effective_grid_size(level);

    //flash the squares if needed for this new level
    if (lastFlashedLevel != level) {
      int flashCount = get_flash_count(level);
      flash_random_squares(gridSize, flashCount);
      lastFlashedLevel = level;
    }

    if (enterPressed) {
      enterPressed = false;
      int index = cursor_row * gridSize + cursor_col; //if enter pressed we can see if its right or wrong

      if (flashPattern[index]) {
        //correct
        if (!selectedCells[index]) {
          selectedCells[index] = true;
          correctSelections++;
          strcpy(message, "CORRECT!");
        } else {
          strcpy(message, "ALREADY SELECTED");
        }
      } else {
        //incorrect
        chances--;
        strcpy(message, "INCORRECT! YOU LOST A CHANCE");
      }

      messageTimer = 15;

      //check if user found all squares
      if (correctSelections == currentFlashCount) {
        level++;
        chances = 3;
        lastFlashedLevel = -1;
        //clear leftover message
        message[0] = '\0';
        messageTimer = 0;
      }
      //or if user out of chances, lose a life
      else if (chances <= 0) {
        lives--;
        chances = 3;
        //reset selections for current level
        for (int i = 0; i < gridSize * gridSize; i++) {
          selectedCells[i] = false;
        }
        lastFlashedLevel = -1;
        //clear leftover message
        message[0] = '\0';
        messageTimer = 0;

        //if no lives left => game over
        if (lives <= 0) {
          gameOver = true;
          continue;  //next loop iteration draws "GAME OVER"
        }
      }
    }

    //draw everything
    clear_screen(COLOR_BLUE);
    draw_grid(gridSize, COLOR_DARK_BLUE);

    //draw game text
    {
      char levelStr[16];
      sprintf(levelStr, "LEVEL %d", level);
      draw_string(25, 5, levelStr, COLOR_TEXT);

      char livesStr[16];
      sprintf(livesStr, "LIVES: %d", lives);
      draw_string(230, 5, livesStr, COLOR_TEXT);

      char chanceStr[16];
      sprintf(chanceStr, "CHANCES: %d", chances);
      draw_string(230, 15, chanceStr, COLOR_TEXT);
    }

    {
      int total_size = gridSize * CELL_SIZE + (gridSize - 1) * GAP;
      int start_x = (SCREEN_WIDTH - total_size) / 2;
      int start_y = (SCREEN_HEIGHT - total_size) / 2;

      for (int i = 0; i < gridSize * gridSize; i++) { //redraw the grids
        if (selectedCells[i]) {
          int row = i / gridSize;
          int col = i % gridSize;
          int cell_x = start_x + col * (CELL_SIZE + GAP);
          int cell_y = start_y + row * (CELL_SIZE + GAP);
          for (int dy = 0; dy < CELL_SIZE; dy++) {
            for (int dx = 0; dx < CELL_SIZE; dx++) {
              plot_pixel(cell_x + dx, cell_y + dy, COLOR_WHITE);
            }
          }
        }
      }
    }

    draw_cursor(cursor_row, cursor_col, gridSize); //draw cursor
    if (messageTimer > 0) {
      draw_string(25, 25, message, COLOR_TEXT);
      messageTimer--;
    }
    wait_for_vsync();
    pixel_buffer_start = *(pixel_ctrl_ptr + 1);
  }
  return 0;
}
