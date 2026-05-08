#include "ssd1306.h"
#include "font.h"
#include "xiic.h"
#include "xil_printf.h"
#include "sleep.h"
#include "string.h"
#include <stdlib.h>


static XIic *IicInstance; // Con trỏ nội bộ
static u8 oled_buffer[128 * 64 / 8];
static int oled_current_x = 0;
static int oled_current_y = 0;

static int SSD1306_SendCommand(u8 cmd) {
    u8 SendBuffer[2];
    SendBuffer[0] = 0x00;
    SendBuffer[1] = cmd;
    // Fix: Dùng BaseAddress
    return XIic_Send(IicInstance->BaseAddress, SSD1306_IIC_ADDR, SendBuffer, 2, XIIC_STOP);
}

// Hàm này được công khai để Main gọi
void SSD1306_Refresh(void) {
    SSD1306_SendCommand(0x21); SSD1306_SendCommand(0); SSD1306_SendCommand(127);
    SSD1306_SendCommand(0x22); SSD1306_SendCommand(0); SSD1306_SendCommand(7);

    u8 SendBuffer[17];
    SendBuffer[0] = 0x40;

    for (int i = 0; i < 1024; i += 16) {
        memcpy(SendBuffer + 1, oled_buffer + i, 16);
        // Fix: Dùng BaseAddress
        XIic_Send(IicInstance->BaseAddress, SSD1306_IIC_ADDR, SendBuffer, 17, XIIC_STOP);
    }
}

void SSD1306_Fill(u8 color) {
    memset(oled_buffer, (color == 0) ? 0x00 : 0xFF, sizeof(oled_buffer));
    // Tối ưu: Không gọi Refresh ở đây
}

void SSD1306_SetPosition(int x, int y) {
    oled_current_x = x * 6;
    oled_current_y = y * 8;
}

static void SSD1306_DrawChar(char c) {
    if (oled_current_x > 122) {
        oled_current_x = 0;
        oled_current_y += 8;
        if (oled_current_y > 56) oled_current_y = 0;
    }
    for (int i = 0; i < 5; i++) {
        u8 line = font5x7[(c - 32) * 5 + i];
        for (int j = 0; j < 8; j++) {
            if (line & (1 << j)) {
                int x_pos = oled_current_x + i;
                int y_pos = oled_current_y + j;
                oled_buffer[x_pos + (y_pos / 8) * 128] |= (1 << (y_pos % 8));
            }
        }
    }
    oled_current_x += 6;
}

void SSD1306_DrawString(const char* str) {
    while (*str) {
        SSD1306_DrawChar(*str);
        str++;
    }
    // Tối ưu: Không gọi Refresh ở đây
}
void SSD1306_DrawPixel(int x, int y, u8 color) {
    if (x < 0 || x >= 128 || y < 0 || y >= 64) return;
    if (color)
        oled_buffer[x + (y / 8) * 128] |= (1 << (y % 8));
    else
        oled_buffer[x + (y / 8) * 128] &= ~(1 << (y % 8));
}
void SSD1306_DrawRect(int x, int y, int w, int h, u8 color) {
    for (int i = x; i < x + w; i++) {
        SSD1306_DrawPixel(i, y, color);
        SSD1306_DrawPixel(i, y + h - 1, color);
    }
    for (int j = y; j < y + h; j++) {
        SSD1306_DrawPixel(x, j, color);
        SSD1306_DrawPixel(x + w - 1, j, color);
    }
}
void SSD1306_DrawCircle(int x0, int y0, int r, u8 color) {
    int f = 1 - r;
    int ddF_x = 1;
    int ddF_y = -2 * r;
    int x = 0;
    int y = r;

    SSD1306_DrawPixel(x0, y0 + r, color);
    SSD1306_DrawPixel(x0, y0 - r, color);
    SSD1306_DrawPixel(x0 + r, y0, color);
    SSD1306_DrawPixel(x0 - r, y0, color);

    while(x < y) {
        if(f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        SSD1306_DrawPixel(x0 + x, y0 + y, color);
        SSD1306_DrawPixel(x0 - x, y0 + y, color);
        SSD1306_DrawPixel(x0 + x, y0 - y, color);
        SSD1306_DrawPixel(x0 - x, y0 - y, color);
        SSD1306_DrawPixel(x0 + y, y0 + x, color);
        SSD1306_DrawPixel(x0 - y, y0 + x, color);
        SSD1306_DrawPixel(x0 + y, y0 - x, color);
        SSD1306_DrawPixel(x0 - y, y0 - x, color);
    }
}
void SSD1306_DrawStringXY(int x, int y, const char* str) {
    SSD1306_SetPosition(x, y);
    SSD1306_DrawString(str);
}
void SSD1306_DrawLine(int x0, int y0, int x1, int y1, u8 color)
{
    int dx = abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    int e2;

    while (1) {
        SSD1306_DrawPixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}
void SSD1306_FillRect(int x, int y, int w, int h, u8 color)
{
    for (int i = x; i < x + w; i++) {
        for (int j = y; j < y + h; j++) {
            SSD1306_DrawPixel(i, j, color);
        }
    }
}
void SSD1306_FillCircle(int x0, int y0, int r, u8 color)
{
    for (int y = -r; y <= r; y++) {
        for (int x = -r; x <= r; x++) {
            if (x*x + y*y <= r*r) {
                SSD1306_DrawPixel(x0 + x, y0 + y, color);
            }
        }
    }
}
void SSD1306_ClearDisplay()
{
    SSD1306_Fill(0x00);
}
static int _textSize = 1;

void SSD1306_SetTextSize(int size)
{
    if (size < 1) size = 1;
    if (size > 3) size = 3;   // hạn chế tối đa cho an toàn
    _textSize = size;
}
static u8 _textColor = 1;

void SSD1306_SetTextColor(u8 color)
{
    _textColor = color ? 1 : 0;
}
void SSD1306_GetTextBounds(const char* str, int* w, int* h)
{
    int len = 0;
    while (*str++) len++;

    *w = len * 6;  // mỗi ký tự rộng 6px
    *h = 8;        // cao 8px
}


int SSD1306_Init(XIic *IicInst, u32 DeviceId) {
    IicInstance = IicInst; // Lưu con trỏ vào biến tĩnh
    int Status;

    Status = XIic_Initialize(IicInstance, DeviceId);
    if (Status != XST_SUCCESS) return XST_FAILURE;

    Status = XIic_SetAddress(IicInstance, XII_ADDR_TO_SEND_TYPE, SSD1306_IIC_ADDR);
    if (Status != XST_SUCCESS) return XST_FAILURE;

    SSD1306_SendCommand(0xAE); SSD1306_SendCommand(0xD5); SSD1306_SendCommand(0x80);
    SSD1306_SendCommand(0xA8); SSD1306_SendCommand(0x3F); SSD1306_SendCommand(0xD3);
    SSD1306_SendCommand(0x00); SSD1306_SendCommand(0x40); SSD1306_SendCommand(0x8D);
    SSD1306_SendCommand(0x14); SSD1306_SendCommand(0x20); SSD1306_SendCommand(0x00);
    SSD1306_SendCommand(0xA1); SSD1306_SendCommand(0xC8); SSD1306_SendCommand(0xDA);
    SSD1306_SendCommand(0x12); SSD1306_SendCommand(0x81); SSD1306_SendCommand(0xCF);
    SSD1306_SendCommand(0xD9); SSD1306_SendCommand(0xF1); SSD1306_SendCommand(0xDB);
    SSD1306_SendCommand(0x40); SSD1306_SendCommand(0xA4); SSD1306_SendCommand(0xA6);
    SSD1306_SendCommand(0xAF);

    usleep(100000);
    SSD1306_Fill(0x00);
    SSD1306_Refresh(); // Refresh lần đầu

    return XST_SUCCESS;
}
