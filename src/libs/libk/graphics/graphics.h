#pragma once
#include <stdint.h>

void gfx_draw_pixel(int x, int y, uint32_t color);
void gfx_draw_rect(int x, int y, int w, int h, uint32_t color);
void gfx_fill_rect(int x, int y, int w, int h, uint32_t color);
void gfx_draw_line(int x0, int y0, int x1, int y1, uint32_t color);
void gfx_clear(uint32_t color);
void gfx_draw_rect_rounded_bottom(int x, int y, int w, int h, int radius, uint32_t color);
