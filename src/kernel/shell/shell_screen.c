/*
	This file is part of an x86_64 hobbyist operating system called KnutOS
	Everything is openly developed on GitHub: https://github.com/Tix3Dev/KnutOS/

	Copyright (C) 2021-2022  Yves Vollmeier <https://github.com/Tix3Dev>
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <stdint.h>

#include <boot/stivale2.h>
#include <devices/ps2/keyboard/keyboard.h>
#include <shell/shell_screen.h>
#include <libk/stdio/stdio.h>
#include <libk/ssfn.h>
#include <firmware/acpi/acpi.h>
#include <libk/string/string.h>
#include <libk/graphics/graphics.h>
#include <libk/stdlib/stdlib.h>
#include "../fs/fs.h"

void system_reboot(void);

static int shell_prompt_x_barrier;
static int shell_prompt_y_barrier;
static int taskbar_drawn = 0;
static int terminal_open = 0;

// create a "new" screen and print a basic shell prompt
// after that activate keyboard processing and configure it to call
// shell_print_char
void shell_screen_init(void)
{
    framebuffer_reset_screen();
    shell_prompt();

    activate_keyboard_processing(*shell_print_char);
}

// print basic shell prompt
void shell_prompt(void)
{
    printk(GFX_PURPLE, "\n┌─ CoreFusion\n");
    printk(GFX_PURPLE, "└→ ");

    // set x, y barrier to last know cursor position
    shell_prompt_x_barrier = ssfn_dst.x;
    shell_prompt_y_barrier = ssfn_dst.y;
}

static char shell_input_buffer[128];
static int shell_input_index = 0;

// Use strcmp from libk string
static void shell_execute_command(const char *cmd) {
    if (strcmp(cmd, "ls") == 0) {
        char names[FS_MAX_FILES][FS_MAX_FILENAME];
        int count = 0;
        fs_list(names, &count);
        for (int i = 0; i < count; ++i)
            printk(GFX_WHITE, "%s\n", names[i]);
    } else if (strncmp(cmd, "cat ", 4) == 0) {
        char buf[FS_MAX_FILESIZE+1] = {0};
        if (fs_read(cmd+4, buf, FS_MAX_FILESIZE) > 0)
            printk(GFX_WHITE, "%s\n", buf);
        else
            printk(GFX_RED, "File not found\n");
    } else if (strncmp(cmd, "rm ", 3) == 0) {
        if (fs_delete(cmd+3) == 0)
            printk(GFX_GREEN, "File deleted\n");
        else
            printk(GFX_RED, "File not found\n");
    } else if (strncmp(cmd, "touch ", 6) == 0) {
        if (fs_create(cmd+6) == 0)
            printk(GFX_GREEN, "File created\n");
        else
            printk(GFX_RED, "File exists or error\n");
    } else if (strncmp(cmd, "echo ", 5) == 0) {
        printk(GFX_WHITE, "%s\n", cmd+5);
    } else if (strncmp(cmd, "write ", 6) == 0) {
        const char *space = strchr(cmd+6, ' ');
        if (space) {
            char filename[FS_MAX_FILENAME];
            size_t len = space - (cmd+6);
            if (len >= FS_MAX_FILENAME) len = FS_MAX_FILENAME-1;
            strncpy(filename, cmd+6, len);
            filename[len] = 0;
            if (!fs_exists(filename)) fs_create(filename);
            if (fs_write(filename, space+1, strlen(space+1)) == 0)
                printk(GFX_GREEN, "Wrote to %s\n", filename);
            else
                printk(GFX_RED, "Write failed\n");
        } else {
            printk(GFX_RED, "Usage: write <file> <content>\n");
        }
    } else if (strncmp(cmd, "more ", 5) == 0) {
        char buf[FS_MAX_FILESIZE+1] = {0};
        if (fs_read(cmd+5, buf, FS_MAX_FILESIZE) > 0) {
            printk(GFX_WHITE, "%s\n", buf);
        } else {
            printk(GFX_RED, "File not found\n");
        }
    } else if (strcmp(cmd, "clear") == 0) {
        framebuffer_reset_screen();
        shell_prompt();
    } else if (strcmp(cmd, "help") == 0) {
        printk(GFX_CYAN, "Commands: ls, cat <file>, rm <file>, touch <file>, write <file> <text>, more <file>, echo <text>, clear, help, shutdown, reboot\n");
    } else if (strcmp(cmd, "shutdown") == 0) {
        printk(GFX_GREEN, "Shutting down via ACPI...\n");
        acpi_shutdown();
    } else if (strcmp(cmd, "reboot") == 0) {
        printk(GFX_GREEN, "Rebooting system...\n");
        system_reboot();
    } else if (strcmp(cmd, "taskbar") == 0) {
        int bar_height = 40;
        int radius = 16;
        int x = 0;
        int y = gfx.fb_height - bar_height;
        int w = gfx.fb_width;
        int h = bar_height;
        uint32_t color = 0xFF0078D7; // Windows blue
        gfx_draw_rect_rounded_bottom(x, y, w, h, radius, color);
        taskbar_drawn = 1;
        terminal_open = 0;
        printk(GFX_GREEN, "Windows-style taskbar drawn at bottom of screen.\n");
    } else if (strcmp(cmd, "close-term") == 0) {
        if (terminal_open) {
            int term_x = 100, term_y = gfx.fb_height - 300, term_w = gfx.fb_width - 200, term_h = 200;
            gfx_fill_rect(term_x, term_y, term_w, term_h, 0xFF222222);
            terminal_open = 0;
            printk(GFX_GREEN, "Terminal closed.\n");
        } else {
            printk(GFX_RED, "No terminal open.\n");
        }
    } else {
        printk(GFX_RED, "Unknown command: %s\n", cmd);
    }
}

// get informations about what was being pressed
// and just print if it is a valid ascii_character
void shell_print_char(KEY_INFO_t key_info)
{
    // if it's nothing to print, return
    if (key_info.ascii_character == '\0')
        return;
    // if return is pressed, make a newline and create a new prompt
    else if (key_info.ascii_character == KEY_RETURN)
    {
        shell_input_buffer[shell_input_index] = '\0';
        shell_execute_command(shell_input_buffer);
        shell_input_index = 0;

        // move barrier if screen scolls
        if (ssfn_dst.y + gfx.glyph_height == gfx.fb_height)
            shell_prompt_y_barrier += gfx.glyph_height;

        printk(GFX_BLUE, "\n");
        shell_prompt();
    }
    // if backspace is pressed check for barrier (prompt)
    else if (key_info.ascii_character == KEY_BACKSPACE)
    {
        if (ssfn_dst.x == shell_prompt_x_barrier && ssfn_dst.y == shell_prompt_y_barrier)
            return;
        if (shell_input_index > 0) shell_input_index--;
        printk(GFX_BLUE, "\b");
    }
    else
    {
        if (shell_input_index < (int)sizeof(shell_input_buffer) - 1) {
            shell_input_buffer[shell_input_index++] = key_info.ascii_character;
        }

        // move barrier if screen scrolls
        if (ssfn_dst.y + gfx.glyph_height == gfx.fb_height && ssfn_dst.x + gfx.glyph_width == gfx.fb_width)
            shell_prompt_y_barrier -= gfx.glyph_height;

        printk(GFX_BLUE, "%c", key_info.ascii_character);
    }

    // If taskbar is drawn and user types 't', open a terminal window
    if (taskbar_drawn && !terminal_open && key_info.ascii_character == 't') {
        int term_x = 100, term_y = gfx.fb_height - 300, term_w = gfx.fb_width - 200, term_h = 200;
        gfx_fill_rect(term_x, term_y, term_w, term_h, 0xFF222222); // dark background
        gfx_draw_rect_rounded_bottom(term_x, term_y, term_w, term_h, 16, 0xFFAAAAAA); // border
        // Print a title bar
        gfx_fill_rect(term_x, term_y, term_w, 24, 0xFF444444);
        // Print 'Terminal' label (using printk for now)
        int old_x = ssfn_dst.x, old_y = ssfn_dst.y;
        ssfn_dst.x = term_x + 10;
        ssfn_dst.y = term_y + 4;
        printk(GFX_WHITE, "Terminal");
        ssfn_dst.x = old_x;
        ssfn_dst.y = old_y;
        terminal_open = 1;
        return;
    }
}
