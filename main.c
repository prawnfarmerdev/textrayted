#include <stdlib.h>
#include <string.h>

#include "raylib.h"

// ============================================================
// CONSTANTS
// ============================================================

#define SCREEN_W 1600
#define SCREEN_H 900
#define FONT_SIZE 40
#define LINE_HEIGHT 48
#define TAB_WIDTH 4
#define INIT_GAP_SIZE 128
#define MAX_CHARS 4096
#define GAP_GROW_SIZE 64

// ============================================================
// TYPES
// ============================================================

typedef struct
{
        int pos;
        int line;
        int col;
} Cursor;

typedef struct
{
        char* buffer;
        int gap_start;
        int gap_end;
        int size;
        Cursor cursor;
} GapBuffer;

// ============================================================
// GAP BUFFER
// ============================================================

GapBuffer* gb_create(void)
{
        GapBuffer* gb = (GapBuffer*)calloc(1, sizeof(GapBuffer));
        if (!gb) return NULL;

        gb->buffer = (char*)calloc(INIT_GAP_SIZE, sizeof(char));
        if (!gb->buffer)
        {
                free(gb);
                return NULL;
        }

        gb->gap_start = 0;
        gb->gap_end = INIT_GAP_SIZE;
        gb->size = INIT_GAP_SIZE;
        gb->cursor.pos = 0;
        gb->cursor.line = 0;
        gb->cursor.col = 0;
        return gb;
}

void gb_free(GapBuffer* gb)
{
        if (!gb) return;
        free(gb->buffer);
        free(gb);
}

int gb_get_gap_size(GapBuffer* gb)
{
        return gb->gap_end - gb->gap_start;
}

int gb_get_length(GapBuffer* gb)
{
        return gb->size - gb_get_gap_size(gb);
}

char gb_get_char_at(GapBuffer* gb, int pos)
{
        if (pos < gb->gap_start)
                return gb->buffer[pos];
        return gb->buffer[gb->gap_end + (pos - gb->gap_start)];
}

void gb_get_text(GapBuffer* gb, char* out)
{
        int before = gb->gap_start;
        int after = gb->size - gb->gap_end;
        memcpy(out, gb->buffer, before);
        memcpy(out + before, gb->buffer + gb->gap_end, after);
        out[before + after] = '\0';
}

void gb_grow(GapBuffer* gb)
{
        int new_size = gb->size + GAP_GROW_SIZE;
        char* new_buffer = (char*)realloc(gb->buffer, new_size);
        if (!new_buffer) return;

        gb->buffer = new_buffer;

        // Shift the right side of the text to the end of the new, larger buffer
        int after_size = gb->size - gb->gap_end;
        int new_gap_end = new_size - after_size;

        memmove(gb->buffer + new_gap_end, gb->buffer + gb->gap_end, after_size);

        gb->gap_end = new_gap_end;
        gb->size = new_size;
}

void gb_move_gap(GapBuffer* gb)
{
        int pos = gb->cursor.pos;
        if (pos == gb->gap_start) return;

        if (pos < gb->gap_start)
        {
                int distance_to_gap = gb->gap_start - pos;
                gb->gap_end -= distance_to_gap;
                gb->gap_start -= distance_to_gap;
                // memcpy(gb->buffer + gb->gap_end, gb->buffer + gb->gap_start, distance_to_gap);
                memmove(gb->buffer + gb->gap_end, gb->buffer + gb->gap_start, distance_to_gap);
        }
        else
        {
                int distance_to_gap = pos - gb->gap_start;
                // memcpy(gb->buffer + gb->gap_start, gb->buffer + gb->gap_end, distance_to_gap);
                memmove(gb->buffer + gb->gap_start, gb->buffer + gb->gap_end, distance_to_gap);
                gb->gap_end += distance_to_gap;
                gb->gap_start += distance_to_gap;
        }
}

void gb_update_cursor_linecol(GapBuffer* gb)
{
        int line = 0;
        int col = 0;

        for (int i = 0; i < gb->cursor.pos; i++)
        {
                if (gb_get_char_at(gb, i) == '\n')
                {
                        line++;
                        col = 0;
                }
                else
                        col++;
        }

        gb->cursor.line = line;
        gb->cursor.col = col;
}

void gb_insert_char(GapBuffer* gb, char c)
{
        if (gb_get_gap_size(gb) == 0)
        {
                gb_grow(gb);
        }

        gb_move_gap(gb);
        gb->buffer[gb->gap_start] = c;
        gb->gap_start++;
        gb->cursor.pos++;

        if (c == '\n')
        {
                gb->cursor.line++;
                gb->cursor.col = 0;
        }
        else
                gb->cursor.col++;
}

void gb_insert_string(GapBuffer* gb, const char* s, int n) {}

void gb_delete_before(GapBuffer* gb)
{
        if (gb->cursor.pos == 0) return;
        gb_move_gap(gb);
        gb->gap_start--;
        gb->cursor.pos--;
        gb_update_cursor_linecol(gb);
}

void gb_delete_after(GapBuffer* gb) {}
void gb_delete_range(GapBuffer* gb, int from, int to) {}

// ============================================================
// CURSOR MOVEMENT
// ============================================================

void gb_cursor_left(GapBuffer* gb)
{
        if (gb->cursor.pos == 0) return;
        gb->cursor.pos--;
        gb_update_cursor_linecol(gb);
}

void gb_cursor_right(GapBuffer* gb)
{
        if (gb->cursor.pos == gb_get_length(gb)) return;
        gb->cursor.pos++;
        gb_update_cursor_linecol(gb);
}

void gb_cursor_up(GapBuffer* gb) {}
void gb_cursor_down(GapBuffer* gb) {}
void gb_cursor_line_start(GapBuffer* gb) {}
void gb_cursor_line_end(GapBuffer* gb) {}
void gb_cursor_to(GapBuffer* gb, int pos) {}

// ============================================================
// INPUT HANDLERS STUBS
// ============================================================

typedef struct
{
        float holdTimer;
        float repeatTimer;
} KeyRepeat;

bool KeyRepeatUpdate(KeyRepeat* kr, int key, float delay, float rate)
{
        if (IsKeyPressed(key))
        {
                kr->holdTimer = 0.0f;
                kr->repeatTimer = 0.0f;
                return true;
        }

        if (IsKeyDown(key))
        {
                kr->holdTimer += GetFrameTime();

                if (kr->holdTimer >= delay)
                {
                        kr->repeatTimer += GetFrameTime();

                        if (kr->repeatTimer >= rate)
                        {
                                kr->repeatTimer = 0.0f;
                                return true;
                        }
                }
        }
        else
        {
                kr->holdTimer = 0.0f;
                kr->repeatTimer = 0.0f;
        }

        return false;
}

void handle_key_repeat(KeyRepeat* kr, int key,
                       float delay, float rate,
                       void (*action)(GapBuffer*),
                       GapBuffer* gb)
{
        if (KeyRepeatUpdate(kr, key, delay, rate))
        {
                action(gb);
        }
}

void handle_char_input(GapBuffer* gb)
{
        int ch = GetCharPressed();
        while (ch > 0)
        {
                if (ch >= 32 && ch <= 125)
                        gb_insert_char(gb, (char)ch);
                ch = GetCharPressed();
        }

        if (IsKeyPressed(KEY_ENTER))
                gb_insert_char(gb, '\n');

        if (IsKeyPressed(KEY_TAB))
                gb_insert_string(gb, "    ", TAB_WIDTH);
}

void handle_arrow_keys(GapBuffer* gb,
                       KeyRepeat* left,
                       KeyRepeat* right,
                       KeyRepeat* up,
                       KeyRepeat* down,
                       float delay,
                       float rate)
{
        handle_key_repeat(left, KEY_LEFT, delay, rate, gb_cursor_left, gb);
        handle_key_repeat(right, KEY_RIGHT, delay, rate, gb_cursor_right, gb);
        handle_key_repeat(up, KEY_UP, delay, rate, gb_cursor_up, gb);
        handle_key_repeat(down, KEY_DOWN, delay, rate, gb_cursor_down, gb);
}

void handle_backspace(GapBuffer* gb,
                      KeyRepeat* kr,
                      float delay,
                      float rate)
{
        handle_key_repeat(kr, KEY_BACKSPACE, delay, rate, gb_delete_before, gb);
}

// ============================================================
// DRAW
// ============================================================

void draw_editor(GapBuffer* gb, Rectangle box, int frame)
{
        char text[MAX_CHARS + 1];
        gb_get_text(gb, text);

        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawRectangleRec(box, BLACK);
        DrawRectangleLinesEx(box, 1, RED);

        int x = (int)box.x + 8;
        int y = (int)box.y + 8;
        int start = 0;
        int length = gb_get_length(gb);

        for (int i = 0; i <= length; i++)
        {
                if (text[i] == '\n' || text[i] == '\0')
                {
                        char line[MAX_CHARS + 1];
                        int line_len = i - start;
                        memcpy(line, text + start, line_len);
                        line[line_len] = '\0';
                        DrawText(line, x, y, FONT_SIZE, RAYWHITE);
                        y += LINE_HEIGHT;
                        start = i + 1;
                }
        }

        if ((frame / 20) % 2 == 0)
        {
                char line_slice[MAX_CHARS + 1];
                int line_start = gb->cursor.pos - gb->cursor.col;
                int col = gb->cursor.col;

                for (int i = 0; i < col; i++)
                        line_slice[i] = gb_get_char_at(gb, line_start + i);
                line_slice[col] = '\0';

                int cursor_x = (int)box.x + 8 + MeasureText(line_slice, FONT_SIZE);
                int cursor_y = (int)box.y + 8 + gb->cursor.line * LINE_HEIGHT;
                DrawRectangle(cursor_x, cursor_y, 2, FONT_SIZE, GOLD);
        }

        DrawText(
                TextFormat("Ln %d  Col %d  chars: %d",
                           gb->cursor.line + 1,
                           gb->cursor.col + 1,
                           gb_get_length(gb)),
                10, SCREEN_H - 24, 20, DARKGRAY);

        DrawText(
                TextFormat("DEBUG: gap[%d, %d]  cursor.pos: %d",
                           gb->gap_start,
                           gb->gap_end,
                           gb->cursor.pos),
                10, SCREEN_H - 48, 20, DARKGRAY);

        EndDrawing();
}

// ============================================================
// MAIN
// ============================================================

int main(void)
{
        InitWindow(SCREEN_W, SCREEN_H, "Textrayted");
        SetTargetFPS(60);

        GapBuffer* gb = gb_create();
        if (!gb)
        {
                CloseWindow();
                return 1;
        }

        Rectangle box = {0, 0, SCREEN_W, SCREEN_H};

        int frame = 0;

        KeyRepeat left = {0}, right = {0}, up = {0}, down = {0};
        KeyRepeat backspace = {0};

        float delay = 0.2f;
        float rate = 0.03f;
        while (!WindowShouldClose())
        {
                handle_char_input(gb);
                handle_arrow_keys(gb, &left, &right, &up, &down, delay, rate);
                handle_backspace(gb, &backspace, delay, rate);
                draw_editor(gb, box, frame);
                frame++;
        }

        gb_free(gb);
        CloseWindow();
        return 0;
}
