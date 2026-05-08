#ifndef SSD1306_H_
#define SSD1306_H_

#include "xiic.h"
#include "xstatus.h"

#define SSD1306_IIC_ADDR 0x3C

// ---------------- CÁC HÀM CƠ BẢN ----------------
int SSD1306_Init(XIic *IicInst, u32 DeviceId);
void SSD1306_Refresh(void);
void SSD1306_Fill(u8 color);
void SSD1306_SetPosition(int x, int y);
void SSD1306_DrawString(const char* str);

// ---------------- GRAPHICS CƠ BẢN ----------------
void SSD1306_DrawPixel(int x, int y, u8 color);
void SSD1306_DrawRect(int x, int y, int w, int h, u8 color);
void SSD1306_DrawCircle(int x, int y, int r, u8 color);
//void SSD1306_DrawStringXY(int x, int y, const char* str);
void SSD1306_ClearLine(int y);

// ---------------- GRAPHICS MỞ RỘNG ----------------
void SSD1306_DrawLine(int x0, int y0, int x1, int y1, u8 color);
void SSD1306_FillRect(int x, int y, int w, int h, u8 color);
void SSD1306_FillCircle(int x0, int y0, int r, u8 color);
void SSD1306_ClearDisplay(void);

// ---------------- TEXT CONTROL ----------------
void SSD1306_SetTextSize(int size);
void SSD1306_SetTextColor(u8 color);
void SSD1306_GetTextBounds(const char* str, int* w, int* h);

#endif
