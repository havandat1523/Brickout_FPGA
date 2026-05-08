#include "ssd1306.h"
#include "xiic.h"
#include "xgpio.h"
#include "xparameters.h"
#include "sleep.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ====== CẤU HÌNH PHẦN CỨNG ======
#define GPIO_DEVICE_ID  XPAR_GPIO_0_DEVICE_ID
#define IIC_DEVICE_ID   XPAR_IIC_0_DEVICE_ID
#define LED_CHANNEL     1
#define BTN_CHANNEL     2
#define BTN_LEFT_MASK   0x01
#define BTN_RIGHT_MASK  0x02
#define LED_WIN_MASK    0x0F

// ====== CẤU HÌNH GAME ======
#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   64
#define MAX_COLS        21  // 128px / 6px per char = 21 chars

#define PAD_Y           58
#define PAD_H           3
#define BALL_R          2

// Level & Gạch
#define ROWS            4
#define COLS            8
#define BRICK_W         14
#define BRICK_H         10
#define BRICK_GAP       1
#define BRICK_OFFSET_Y  12

typedef enum { MENU, PLAYING, LEVEL_TRANSITION, GAME_OVER, VICTORY } GameState;
GameState currentState = MENU;

#define MAX_BALLS 3
typedef struct {
    float x, y;
    float dx, dy;
    bool active;
} Ball;

typedef struct {
    float x;
    int w;
    float speed;
} Paddle;

XIic Iic;
XGpio Gpio;
Ball balls[MAX_BALLS];
Paddle paddle;
int bricks[ROWS][COLS];
int currentLevel = 1;
int score = 0;
int lives = 3;
int totalBricks = 0;

// ====== SYSTEM INIT ======
int Init_System() {
    if (SSD1306_Init(&Iic, IIC_DEVICE_ID) != XST_SUCCESS) return XST_FAILURE;
    if (XGpio_Initialize(&Gpio, GPIO_DEVICE_ID) != XST_SUCCESS) return XST_FAILURE;
    XGpio_SetDataDirection(&Gpio, LED_CHANNEL, 0x00000000);
    XGpio_SetDataDirection(&Gpio, BTN_CHANNEL, 0xFFFFFFFF);
    return XST_SUCCESS;
}

u32 Read_Buttons() {
    return XGpio_DiscreteRead(&Gpio, BTN_CHANNEL);
}

void Set_LEDs(u32 value) {
    XGpio_DiscreteWrite(&Gpio, LED_CHANNEL, value);
}

int Random(int min, int max) {
    return min + rand() % (max - min + 1);
}

// ====== GAME LOGIC ======
void Spawn_MultiBall(float x, float y) {
    int activeCount = 0;
    for(int i=0; i<MAX_BALLS; i++) if(balls[i].active) activeCount++;
    if (activeCount >= MAX_BALLS) return;

    for(int i=0; i<MAX_BALLS; i++) {
        if (!balls[i].active) {
            balls[i].active = true;
            balls[i].x = x;
            balls[i].y = y;
            balls[i].dy = -2.5;
            balls[i].dx = (Random(0, 1) == 0 ? -1.5 : 1.5);
            break;
        }
    }
}

void Expand_Paddle() {
    paddle.w = 48;
    if (paddle.x + paddle.w > SCREEN_WIDTH) paddle.x = SCREEN_WIDTH - paddle.w;
}

void Load_Level(int level) {
    currentLevel = level;
    totalBricks = 0;
    paddle.w = 24;

    for (int r = 0; r < ROWS; r++) for (int c = 0; c < COLS; c++) bricks[r][c] = 0;

    if (level == 1) {
        for (int r = 0; r < 3; r++) {
            for (int c = 1; c < 7; c++) { bricks[r][c] = 1; totalBricks++; }
        }
    }
    else if (level == 2) {
        bricks[1][3] = 9; totalBricks++;
        bricks[1][4] = 9; totalBricks++;
        for(int c=0; c<8; c++) { bricks[0][c] = 1; totalBricks++; }
        bricks[2][0] = 2; bricks[2][7] = 2; totalBricks+=2;
        bricks[2][3] = 2; bricks[2][4] = 2; totalBricks+=2;
    }
    else if (level == 3) {
        bricks[0][3] = 8; totalBricks++;
        bricks[0][4] = 8; totalBricks++;
        for(int c=0; c<8; c++) {
            if (c!=3 && c!=4) bricks[2][c] = -1;
        }
        bricks[3][2] = 1; bricks[3][5] = 1; totalBricks+=2;
    }
}

void Reset_Ball() {
    for(int i=0; i<MAX_BALLS; i++) balls[i].active = false;
    balls[0].active = true;
    balls[0].x = paddle.x + paddle.w / 2;
    balls[0].y = PAD_Y - BALL_R - 2;
    balls[0].dx = (Random(0, 1) == 0 ? -2.0 : 2.0);
    balls[0].dy = -3.0;
}

void Reset_Game() {
    score = 0;
    lives = 3;
    paddle.x = (SCREEN_WIDTH - 24) / 2;
    paddle.speed = 5.0;
    paddle.w = 24;
    Load_Level(1);
    Reset_Ball();
}

void Update_Game() {
    u32 btn = Read_Buttons();
    u32 ledState = 0;

    if (btn & BTN_LEFT_MASK) {
        paddle.x -= paddle.speed;
        ledState |= BTN_LEFT_MASK;
    }
    if (btn & BTN_RIGHT_MASK) {
        paddle.x += paddle.speed;
        ledState |= BTN_RIGHT_MASK;
    }

    if (paddle.x < 0) paddle.x = 0;
    if (paddle.x + paddle.w > SCREEN_WIDTH) paddle.x = SCREEN_WIDTH - paddle.w;

    Set_LEDs(ledState);

    int activeBalls = 0;
    for(int i=0; i<MAX_BALLS; i++) {
        if (!balls[i].active) continue;
        activeBalls++;

        balls[i].x += balls[i].dx;
        balls[i].y += balls[i].dy;

        if (balls[i].x <= 0) { balls[i].x = 0; balls[i].dx = -balls[i].dx; }
        if (balls[i].x >= SCREEN_WIDTH) { balls[i].x = SCREEN_WIDTH; balls[i].dx = -balls[i].dx; }
        if (balls[i].y <= BRICK_OFFSET_Y) { balls[i].y = BRICK_OFFSET_Y; balls[i].dy = -balls[i].dy; }

        if (balls[i].dy > 0 &&
            balls[i].y + BALL_R >= PAD_Y &&
            balls[i].y - BALL_R <= PAD_Y + PAD_H &&
            balls[i].x + BALL_R >= paddle.x &&
            balls[i].x - BALL_R <= paddle.x + paddle.w) {

            balls[i].dy = -balls[i].dy;
            float hitPoint = balls[i].x - (paddle.x + paddle.w/2);
            balls[i].dx = hitPoint * 0.25;
        }

        int bCol = (int)(balls[i].x / (BRICK_W + BRICK_GAP));
        int bRow = (int)((balls[i].y - BRICK_OFFSET_Y) / (BRICK_H + BRICK_GAP));

        if (bRow >= 0 && bRow < ROWS && bCol >= 0 && bCol < COLS) {
            if (bricks[bRow][bCol] != 0) {
                balls[i].dy = -balls[i].dy;
                int type = bricks[bRow][bCol];
                if (type == -1) {
                    // Immortal brick
                } else {
                    if (type == 8) { Spawn_MultiBall(balls[i].x, balls[i].y); score += 20; }
                    else if (type == 9) { Expand_Paddle(); score += 15; }
                    else { score += 10; }

                    if (type == 8 || type == 9) bricks[bRow][bCol] = 0;
                    else bricks[bRow][bCol]--;

                    if (bricks[bRow][bCol] == 0) totalBricks--;
                }
            }
        }

        if (balls[i].y > SCREEN_HEIGHT) balls[i].active = false;
    }

    if (activeBalls == 0) {
        lives--;
        if (lives > 0) {
            Reset_Ball();
            paddle.w = 24;
            usleep(500000);
        } else {
            currentState = GAME_OVER;
        }
    }

    if (totalBricks <= 0) {
        if (currentLevel < 3) currentState = LEVEL_TRANSITION;
        else currentState = VICTORY;
    }
}

// ====== FIXED DRAWING FUNCTIONS ======

void Draw_Game() {
    SSD1306_ClearDisplay();

    // --- FIX 1: HIỂN THỊ UI ---
    // Sử dụng cột (0-20) thay vì pixel

    char buffer[20];

    // Level ở cột 0 (Trái)
    sprintf(buffer, "Lv:%d", currentLevel);
    SSD1306_DrawStringXY(0, 0, buffer);

    // Score ở cột 7 (Giữa màn hình ~pixel 42)
    sprintf(buffer, "Scr:%d", score);
    SSD1306_DrawStringXY(7, 0, buffer);

    // Lives ở cột 16 (Phải màn hình ~pixel 96)
    // Dùng chữ "HP" thay vì "<3" để tránh lỗi font
    sprintf(buffer, "HP:%d", lives);
    SSD1306_DrawStringXY(16, 0, buffer);

    // Đường kẻ ngang phân cách
    SSD1306_DrawLine(0, 10, SCREEN_WIDTH, 10, 1);

    // Vẽ Gạch
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            int b = bricks[r][c];
            if (b != 0) {
                int x = c * (BRICK_W + BRICK_GAP);
                int y = r * (BRICK_H + BRICK_GAP) + BRICK_OFFSET_Y;
                SSD1306_FillRect(x, y, BRICK_W, BRICK_H, 1);

                // Ký hiệu đặc biệt
                if (b == 8) { // Multi-ball
                     SSD1306_DrawPixel(x+6, y+4, 0); SSD1306_DrawPixel(x+8, y+4, 0);
                }
                else if (b == 9) { // Wide
                     SSD1306_DrawLine(x+3, y+5, x+BRICK_W-3, y+5, 0);
                }
                else if (b == -1) { // Immortal
                     SSD1306_DrawRect(x+2, y+2, BRICK_W-4, BRICK_H-4, 0);
                }
            }
        }
    }

    // Vẽ Thanh đỡ & Bóng
    SSD1306_FillRect((int)paddle.x, PAD_Y, paddle.w, PAD_H, 1);
    for(int i=0; i<MAX_BALLS; i++) {
        if(balls[i].active) SSD1306_FillCircle((int)balls[i].x, (int)balls[i].y, BALL_R, 1);
    }

    SSD1306_Refresh();
}

// --- FIX 2: CĂN GIỮA CHỮ ---
void Draw_Center_Text(char* line1, char* line2) {
    SSD1306_ClearDisplay();

    // Tính toán dựa trên CỘT (Max 21 cột)
    int len1 = strlen(line1);
    int col1 = (MAX_COLS - len1) / 2;
    if (col1 < 0) col1 = 0;

    // Dòng 1 ở hàng 3 (y=3)
    SSD1306_DrawStringXY(col1, 3, line1);

    if (line2) {
        int len2 = strlen(line2);
        int col2 = (MAX_COLS - len2) / 2;
        if (col2 < 0) col2 = 0;

        // Dòng 2 ở hàng 5 (y=5)
        SSD1306_DrawStringXY(col2, 5, line2);
    }

    SSD1306_Refresh();
}

// ====== MAIN ======
int main() {
    if (Init_System() != XST_SUCCESS) return XST_FAILURE;

    Reset_Game();
    currentState = MENU;
    srand(1234);

    while (1) {
        switch (currentState) {
            case MENU:
                Draw_Center_Text("BLOCKS", "Press Button");
                Set_LEDs(0x00);
                if (Read_Buttons()) {
                    Reset_Game();
                    currentState = PLAYING;
                    usleep(300000);
                }
                break;

            case PLAYING:
                Update_Game();
                Draw_Game();
                break;

            case LEVEL_TRANSITION:
                Draw_Center_Text("NEXT LEVEL", "Get Ready!");
                // Chờ và kiểm tra LED nhấp nháy cho đẹp
                for(int i=0; i<4; i++) {
                    Set_LEDs(0x0F); usleep(250000);
                    Set_LEDs(0x00); usleep(250000);
                }
                Load_Level(currentLevel + 1);
                Reset_Ball();
                currentState = PLAYING;
                break;

            case GAME_OVER:
                {
                    char scoreStr[20];
                    sprintf(scoreStr, "Score: %d", score);
                    Draw_Center_Text("GAME OVER", scoreStr);
                    Set_LEDs(0x00);
                    if (Read_Buttons()) {
                        Reset_Game();
                        currentState = PLAYING;
                        usleep(500000);
                    }
                }
                break;

            case VICTORY:
                Draw_Center_Text("YOU WIN!", "Legend!");
                Set_LEDs(LED_WIN_MASK); usleep(100000);
                Set_LEDs(0x00); usleep(100000);

                if (Read_Buttons()) {
                    Reset_Game();
                    currentState = MENU;
                    usleep(500000);
                }
                break;
        }

        usleep(25000); // ~40 FPS
    }
    return 0;
}
