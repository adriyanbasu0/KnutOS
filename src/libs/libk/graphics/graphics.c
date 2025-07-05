#include "graphics.h"
#include <devices/framebuffer/framebuffer.h>
#include <stdint.h>

extern struct GFX_Struct gfx;

void gfx_draw_pixel(int x, int y, uint32_t color) {
    framebuffer_draw_pixel(x, y, color);
}

void gfx_draw_rect(int x, int y, int w, int h, uint32_t color) {
    for (int i = 0; i < w; ++i) {
        gfx_draw_pixel(x + i, y, color);
        gfx_draw_pixel(x + i, y + h - 1, color);
    }
    for (int j = 0; j < h; ++j) {
        gfx_draw_pixel(x, y + j, color);
        gfx_draw_pixel(x + w - 1, y + j, color);
    }
}

void gfx_fill_rect(int x, int y, int w, int h, uint32_t color) {
    for (int j = 0; j < h; ++j)
        for (int i = 0; i < w; ++i)
            gfx_draw_pixel(x + i, y + j, color);
}

void gfx_draw_line(int x0, int y0, int x1, int y1, uint32_t color) {
    framebuffer_draw_line(x0, y0, x1, y1, color);
}

void gfx_clear(uint32_t color) {
    framebuffer_set_background_color(color);
}

// Draw a filled quarter-circle (for rounded corners)
static void gfx_fill_quarter_circle(int cx, int cy, int r, int quadrant, uint32_t color) {
    for (int y = 0; y <= r; ++y) {
        for (int x = 0; x <= r; ++x) {
            if (x*x + y*y <= r*r) {
                int px = cx, py = cy;
                if (quadrant == 0) { px += x; py += y; }         // bottom right
                else if (quadrant == 1) { px -= x; py += y; }    // bottom left
                else if (quadrant == 2) { px -= x; py -= y; }    // top left
                else if (quadrant == 3) { px += x; py -= y; }    // top right
                gfx_draw_pixel(px, py, color);
            }
        }
    }
}

void gfx_draw_rect_rounded_bottom(int x, int y, int w, int h, int radius, uint32_t color) {
    // Draw straight sides and top
    for (int i = 0; i < w; ++i) {
        gfx_draw_pixel(x + i, y, color); // top
    }
    for (int j = 1; j < h - radius; ++j) {
        gfx_draw_pixel(x, y + j, color); // left
        gfx_draw_pixel(x + w - 1, y + j, color); // right
    }
    // Draw bottom straight segment
    for (int i = radius; i < w - radius; ++i) {
        gfx_draw_pixel(x + i, y + h - 1, color);
    }
    // Draw filled quarter-circles for bottom corners
    gfx_fill_quarter_circle(x + radius, y + h - radius - 1, radius, 1, color); // bottom left
    gfx_fill_quarter_circle(x + w - radius - 1, y + h - radius - 1, radius, 0, color); // bottom right
}
