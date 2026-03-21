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
#define INIT_GAP_SIZE 64
#define MAX_CHARS 4096
#define GAP_GROW_SIZE 32

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

int gb_length(GapBuffer* gb)
{
        return gb->size - gb_get_gap_size(gb);
}

char gb_get_char_at(GapBuffer* gb, int pos)
{
        // if pos is before the gap read directly
        // if pos is after the gap offset by gap size to skip over it
        if (pos < gb->gap_start)
                return gb->buffer[pos];
        return gb->buffer[gb->gap_end + (pos - gb->gap_start)];
}

void gb_get_text(GapBuffer* gb, char* out)
{
        // copy everything before the gap then everything after into a flat string
        int before = gb->gap_start;
        int after = gb->size - gb->gap_end;
        memcpy(out, gb->buffer, before);
        memcpy(out + before, gb->buffer + gb->gap_end, after);
        out[before + after] = '\0';
}

void gb_grow(GapBuffer* gb) { /* TODO: allocate bigger buf, copy before and after gap, update gap_end and size */ }

void gb_move_gap(GapBuffer* gb)
{
        int pos = gb->cursor.pos;
        if (pos == gb->gap_start) return;

        if (pos < gb->gap_start)
        {
                int distance_to_gap = gb->gap_start - pos;
                gb->gap_end -= distance_to_gap;
                gb->gap_start -= distance_to_gap;
                memcpy(gb->buffer + gb->gap_end, gb->buffer + gb->gap_start, distance_to_gap);
        }
        else
        {
                int distance_to_gap =  pos - gb->gap_start;
                memcpy( gb->buffer + gb->gap_start, gb->buffer + gb->gap_end , distance_to_gap);
                gb->gap_end += distance_to_gap;
                gb->gap_start += distance_to_gap;
        }
}
void gb_update_cursor_linecol(GapBuffer* gb) { /* TODO: walk from 0 to cursor.pos counting newlines for line, chars for col */ }
void gb_insert_char(GapBuffer* gb, char c)
{
  /* TODO: grow if needed, move gap, write char, advance gap_start and cursor */
  if(gb_get_gap_size(gb)==0)
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
    else gb->cursor.col++;

}
void gb_insert_string(GapBuffer* gb, const char* s, int n) { /* TODO: call gb_insert_char for each character */ }
void gb_delete_before(GapBuffer* gb) { /* TODO: move gap, retreat gap_start, decrement cursor.pos, update linecol */ }
void gb_delete_after(GapBuffer* gb) { /* TODO: move gap, advance gap_end, update linecol */ }
void gb_delete_range(GapBuffer* gb, int from, int to) { /* TODO: cursor to from, move gap, advance gap_end by range size */ }

// ============================================================
// CURSOR MOVEMENT STUBS
// ============================================================

void gb_cursor_left(GapBuffer* gb) { /* TODO: decrement cursor.pos, update linecol */ }
void gb_cursor_right(GapBuffer* gb) { /* TODO: increment cursor.pos, update linecol */ }
void gb_cursor_up(GapBuffer* gb) { /* TODO: walk back to prev line, land on same col */ }
void gb_cursor_down(GapBuffer* gb) { /* TODO: walk forward to next line, land on same col */ }
void gb_cursor_line_start(GapBuffer* gb) { /* TODO: walk back to start of current line */ }
void gb_cursor_line_end(GapBuffer* gb) { /* TODO: walk forward to end of current line */ }
void gb_cursor_to(GapBuffer* gb, int pos) { /* TODO: clamp pos, set cursor.pos, update linecol */ }

// ============================================================
// INPUT HANDLERS STUBS
// ============================================================

void handle_char_input(GapBuffer* gb)
{
        // TODO: GetCharPressed loop, insert printable chars
        // TODO: KEY_ENTER inserts '\n'
        // TODO: KEY_TAB inserts TAB_WIDTH spaces
}

void handle_arrow_keys(GapBuffer* gb)
{
        // TODO: KEY_LEFT  -> gb_cursor_left
        // TODO: KEY_RIGHT -> gb_cursor_right
        // TODO: KEY_UP    -> gb_cursor_up
        // TODO: KEY_DOWN  -> gb_cursor_down
        // TODO: KEY_HOME  -> gb_cursor_line_start
        // TODO: KEY_END   -> gb_cursor_line_end
}

void handle_backspace(GapBuffer* gb, float* hold_timer, float* last_delete,
                      float hold_delay, float hold_repeat)
{
        // TODO: IsKeyPressed -> delete once, reset timers
        // TODO: IsKeyDown    -> accumulate hold_timer, repeat delete at hold_repeat interval after hold_delay
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

        // TODO: walk text, split on newlines, DrawText each line with LINE_HEIGHT spacing

        // TODO: cursor blink using (frame / 20) % 2
        //       measure text up to cursor.col on current line to get x position
        //       cursor.line * LINE_HEIGHT to get y position
        //       DrawRectangle 2px wide FONT_SIZE tall in GOLD

        // status bar
        DrawText(
                TextFormat("Ln %d  Col %d  chars: %d",
                           gb->cursor.line + 1,
                           gb->cursor.col + 1,
                           gb_length(gb)),
                10, SCREEN_H - 24, 20, DARKGRAY);

        // debug bar
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
        float hold_timer = 0.0f;
        float last_delete = 0.0f;
        float hold_delay = 0.4f;
        float hold_repeat = 0.05f;

        while (!WindowShouldClose())
        {
                handle_char_input(gb);
                handle_arrow_keys(gb);
                handle_backspace(gb, &hold_timer, &last_delete, hold_delay, hold_repeat);
                draw_editor(gb, box, frame);
                frame++;
        }

        gb_free(gb);
        CloseWindow();
        return 0;
}
