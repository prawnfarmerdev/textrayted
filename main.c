#include <stdlib.h>
#include <string.h>

#include "raylib.h"

// ============================================================
// CONSTANTS
// ============================================================

#define SCREEN_W    1600       // Window width in pixels
#define SCREEN_H    900        // Window height in pixels
#define FONT_SIZE   40         // Font size used for rendering text
#define LINE_HEIGHT 48         // Vertical spacing between lines (font size + padding)
#define TAB_WIDTH   8          // Number of spaces inserted per Tab keypress
#define INIT_GAP_SIZE 128      // Initial size of the gap buffer in bytes
#define MAX_CHARS   4096       // Max characters that can be rendered at once
#define GAP_GROW_SIZE 64       // How many bytes to add when the gap is exhausted

// ============================================================
// TYPES
// ============================================================

// Tracks the logical position of the text cursor, along with
// its computed line and column (used for display and movement).
typedef struct {
	int pos;   // Absolute character index in the logical text
	int line;  // Zero-based line number
	int col;   // Zero-based column number within the current line
} Cursor;

// A gap buffer is a classic data structure for text editing.
// The buffer array is split into two halves by a "gap" of unused bytes.
// Text before the gap lives at [0, gap_start), and text after lives at [gap_end, size).
// Inserting at the cursor is O(1) as long as the gap is already there;
// moving the gap requires a memmove proportional to the distance moved.
typedef struct {
	char *buffer;   // Raw backing array (size bytes total)
	int gap_start;  // First byte of the gap (exclusive end of left text)
	int gap_end;    // First byte after the gap (start of right text)
	int size;       // Total allocated bytes (text + gap)
	Cursor cursor;  // Current cursor state
} GapBuffer;

// ============================================================
// GAP BUFFER — Lifecycle
// ============================================================

// Allocates and initialises a new, empty gap buffer.
// The entire buffer starts as one big gap (nothing has been typed yet).
// Returns NULL on allocation failure.
GapBuffer *gb_create(void)
{
	GapBuffer *gb = (GapBuffer *)calloc(1, sizeof(GapBuffer));
	if (!gb)
		return NULL;

	gb->buffer = (char *)calloc(INIT_GAP_SIZE, sizeof(char));
	if (!gb->buffer) {
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

// Frees all memory associated with the gap buffer.
void gb_free(GapBuffer *gb)
{
	if (!gb)
		return;
	free(gb->buffer);
	free(gb);
}

// ============================================================
// GAP BUFFER — Metrics
// ============================================================

// Returns the current size of the gap (unused bytes between the two text halves).
int gb_get_gap_size(GapBuffer *gb)
{
	return gb->gap_end - gb->gap_start;
}

// Returns the total number of logical text characters (excluding the gap).
int gb_get_length(GapBuffer *gb)
{
	return gb->size - gb_get_gap_size(gb);
}

// ============================================================
// GAP BUFFER — Character Access
// ============================================================

// Returns the character at logical position `pos`, transparently
// jumping over the gap so callers don't need to think about it.
char gb_get_char_at(GapBuffer *gb, int pos)
{
	if (pos < gb->gap_start)
		return gb->buffer[pos];
	return gb->buffer[gb->gap_end + (pos - gb->gap_start)];
}

// Copies the full logical text (both halves joined) into `out`.
// `out` must be large enough to hold gb_get_length(gb) + 1 bytes.
void gb_get_text(GapBuffer *gb, char *out)
{
	int before = gb->gap_start;
	int after = gb->size - gb->gap_end;
	memcpy(out, gb->buffer, before);
	memcpy(out + before, gb->buffer + gb->gap_end, after);
	out[before + after] = '\0';
}

// ============================================================
// GAP BUFFER — Internal Operations
// ============================================================

// Expands the buffer by GAP_GROW_SIZE bytes when the gap is exhausted.
// The right-hand text is shifted to the end of the newly allocated memory,
// so the gap logically appears in the same place but is now larger.
void gb_grow(GapBuffer *gb)
{
	int new_size = gb->size + GAP_GROW_SIZE;
	char *new_buffer = (char *)realloc(gb->buffer, new_size);
	if (!new_buffer)
		return;

	gb->buffer = new_buffer;

	// Shift the right side of the text to the end of the new, larger buffer
	int after_size = gb->size - gb->gap_end;
	int new_gap_end = new_size - after_size;

	memmove(gb->buffer + new_gap_end, gb->buffer + gb->gap_end, after_size);

	gb->gap_end = new_gap_end;
	gb->size = new_size;
}

// Slides the gap so that gap_start aligns with the cursor position.
// Characters between the old and new gap positions are shifted across
// using memmove (safe for overlapping regions).
void gb_move_gap(GapBuffer *gb)
{
	int pos = gb->cursor.pos;
	if (pos == gb->gap_start)
		return;

	if (pos < gb->gap_start) {
		// Cursor is to the left: shift left-side text rightward into the gap
		int distance_to_gap = gb->gap_start - pos;
		gb->gap_end -= distance_to_gap;
		gb->gap_start -= distance_to_gap;
		memmove(gb->buffer + gb->gap_end, gb->buffer + gb->gap_start,
			distance_to_gap);
	} else {
		// Cursor is to the right: shift right-side text leftward into the gap
		int distance_to_gap = pos - gb->gap_start;
		memmove(gb->buffer + gb->gap_start, gb->buffer + gb->gap_end,
			distance_to_gap);
		gb->gap_end += distance_to_gap;
		gb->gap_start += distance_to_gap;
	}
}

// Recomputes cursor.line and cursor.col by scanning from position 0 to cursor.pos.
// Called after any operation that can change which line the cursor sits on
// (e.g. backspace, cursor up/down).
void gb_update_cursor_linecol(GapBuffer *gb)
{
	int line = 0;
	int col = 0;

	for (int i = 0; i < gb->cursor.pos; i++) {
		if (gb_get_char_at(gb, i) == '\n') {
			line++;
			col = 0;
		} else
			col++;
	}

	gb->cursor.line = line;
	gb->cursor.col = col;
}

// ============================================================
// GAP BUFFER — Insertion
// ============================================================

// Inserts a single character at the current cursor position, then
// advances the cursor.  Grows the buffer first if the gap is full.
void gb_insert_char(GapBuffer *gb, char c)
{
	if (gb_get_gap_size(gb) == 0) {
		gb_grow(gb);
	}

	gb_move_gap(gb);
	gb->buffer[gb->gap_start] = c;
	gb->gap_start++;
	gb->cursor.pos++;

	// Keep line/col in sync without a full rescan
	if (c == '\n') {
		gb->cursor.line++;
		gb->cursor.col = 0;
	} else
		gb->cursor.col++;
}

// Inserts `len` characters from `str` one at a time.
// Used for multi-character insertions such as Tab expansion.
void gb_insert_string(GapBuffer *gb, const char *str, int len)
{
        for (int i = 0; i < len; i++)
                gb_insert_char(gb, str[i]);

}

// ============================================================
// GAP BUFFER — Deletion
// ============================================================

// Deletes the character immediately before the cursor (backspace).
// Shrinks the gap leftward by one, which effectively erases the character.
void gb_delete_before(GapBuffer *gb)
{
	if (gb->cursor.pos == 0)
		return;
	gb_move_gap(gb);
	gb->gap_start--;
	gb->cursor.pos--;
	gb_update_cursor_linecol(gb);
}

// TODO: Delete the character immediately after the cursor (forward delete / Delete key).
void gb_delete_after(GapBuffer *gb)
{
}

// TODO: Delete all characters in the half-open range [from, to).
void gb_delete_range(GapBuffer *gb, int from, int to)
{
}

// ============================================================
// CURSOR MOVEMENT
// ============================================================

// Moves the cursor one character to the left (does nothing at start of buffer).
// Recalculates line/col after the move.
void gb_cursor_left(GapBuffer *gb)
{
	if (gb->cursor.pos == 0)
		return;
	gb->cursor.pos--;
	gb_update_cursor_linecol(gb);
}

// Moves the cursor one character to the right (does nothing at end of buffer).
// Recalculates line/col after the move.
void gb_cursor_right(GapBuffer *gb)
{
	if (gb->cursor.pos == gb_get_length(gb))
		return;
	gb->cursor.pos++;
	gb_update_cursor_linecol(gb);
}

// Moves the cursor up one line, trying to preserve the current column.
// Algorithm:
//   1. Walk backward to find the start of the current line.
//   2. Step back past the preceding newline to land on the target line.
//   3. Walk forward up to target_col (or the end of that line).
void gb_cursor_up(GapBuffer *gb)
{
        if (gb->cursor.line == 0) return;

        int target_line = gb->cursor.line - 1;
        int target_col = gb->cursor.col;

        // walk back to find start of current line
        int pos = gb->cursor.pos;
        while (pos > 0 && gb_get_char_at(gb, pos - 1) != '\n') pos--;
        // pos is now at start of current line

        // walk back one more to get past the newline
        if (pos > 0) pos--;

        // walk back to find start of target line
        while (pos > 0 && gb_get_char_at(gb, pos - 1) != '\n') pos--;
        // pos is now at start of target line

        // walk forward up to target_col or until newline
        int col = 0;
        while (col < target_col &&
               pos < gb_get_length(gb) &&
               gb_get_char_at(gb, pos) != '\n')
        {
                pos++;
                col++;
        }

        gb->cursor.pos = pos;
        gb_update_cursor_linecol(gb);
}

// Moves the cursor down one line, trying to preserve the current column.
// Algorithm:
//   1. Walk forward to find the next newline.
//   2. Step past it to land on the first character of the next line.
//   3. Walk forward up to target_col (or the end of that line).
void gb_cursor_down(GapBuffer *gb)
{
        int target_col = gb->cursor.col;

        // walk forward to find the next newline
        int pos = gb->cursor.pos;
        while (pos < gb_get_length(gb) && gb_get_char_at(gb, pos) != '\n') pos++;

        // if no newline found, already on last line
        if (pos >= gb_get_length(gb)) return;

        // step past the newline
        pos++;

        // walk forward up to target_col or until next newline
        int col = 0;
        while (col < target_col &&
               pos < gb_get_length(gb) &&
               gb_get_char_at(gb, pos) != '\n')
        {
                pos++;
                col++;
        }

        gb->cursor.pos = pos;
        gb_update_cursor_linecol(gb);

}

// TODO: Jump the cursor to the first character of the current line (Home key).
void gb_cursor_line_start(GapBuffer *gb)
{
}

// TODO: Jump the cursor past the last character of the current line (End key).
void gb_cursor_line_end(GapBuffer *gb)
{
}

// TODO: Move the cursor to an arbitrary absolute position.
void gb_cursor_to(GapBuffer *gb, int pos)
{
}

// ============================================================
// INPUT — Key Repeat
// ============================================================

// State for a single repeating key.  Hold the key briefly (delay) before it
// starts firing repeatedly at the given rate.  This mimics OS key-repeat behaviour.
typedef struct {
	float holdTimer;    // Seconds the key has been held so far
	float repeatTimer;  // Accumulator used to fire repeated actions at `rate`
} KeyRepeat;

// Returns true on the frame a key action should fire, either on initial press
// or while repeating (after `delay` seconds of hold, then once every `rate` seconds).
bool KeyRepeatUpdate(KeyRepeat *kr, int key, float delay, float rate)
{
	if (IsKeyPressed(key)) {
		kr->holdTimer = 0.0f;
		kr->repeatTimer = 0.0f;
		return true;
	}

	if (IsKeyDown(key)) {
		kr->holdTimer += GetFrameTime();

		if (kr->holdTimer >= delay) {
			kr->repeatTimer += GetFrameTime();

			if (kr->repeatTimer >= rate) {
				kr->repeatTimer = 0.0f;
				return true;
			}
		}
	} else {
		// Key released: reset both timers
		kr->holdTimer = 0.0f;
		kr->repeatTimer = 0.0f;
	}

	return false;
}

// Convenience wrapper: calls `action(gb)` whenever the key-repeat logic fires.
void handle_key_repeat(KeyRepeat *kr, int key, float delay, float rate,
		       void (*action)(GapBuffer *), GapBuffer *gb)
{
	if (KeyRepeatUpdate(kr, key, delay, rate)) {
		action(gb);
	}
}

// ============================================================
// INPUT — Handlers
// ============================================================

// Reads all printable characters queued this frame and inserts them.
// Also handles Enter (newline) and Tab (space expansion).
void handle_char_input(GapBuffer *gb)
{
	int ch = GetCharPressed();
	while (ch > 0) {
		if (ch >= 32 && ch <= 125)
			gb_insert_char(gb, (char)ch);
		ch = GetCharPressed();
	}

	if (IsKeyPressed(KEY_ENTER))
		gb_insert_char(gb, '\n');

	if (IsKeyPressed(KEY_TAB))
		gb_insert_string(gb, "        ", TAB_WIDTH);
}

// Polls all four arrow keys and fires the matching cursor-move function
// according to the shared key-repeat settings (delay, rate).
void handle_arrow_keys(GapBuffer *gb, KeyRepeat *left, KeyRepeat *right,
		       KeyRepeat *up, KeyRepeat *down, float delay, float rate)
{
	handle_key_repeat(left,  KEY_LEFT,  delay, rate, gb_cursor_left,  gb);
	handle_key_repeat(right, KEY_RIGHT, delay, rate, gb_cursor_right, gb);
	handle_key_repeat(up,    KEY_UP,    delay, rate, gb_cursor_up,    gb);
	handle_key_repeat(down,  KEY_DOWN,  delay, rate, gb_cursor_down,  gb);
}

// Polls the Backspace key and fires gb_delete_before with key-repeat support.
void handle_backspace(GapBuffer *gb, KeyRepeat *kr, float delay, float rate)
{
	handle_key_repeat(kr, KEY_BACKSPACE, delay, rate, gb_delete_before, gb);
}

// ============================================================
// DRAW
// ============================================================

// Renders the editor for one frame:
//   - Draws each line of text inside `box`.
//   - Draws a blinking cursor bar at the correct line/column.
//   - Draws a status bar at the bottom with line, column, and debug info.
// `frame` is used to drive the cursor blink (toggles every 20 frames).
void draw_editor(GapBuffer *gb, Rectangle box, int frame)
{
	char text[MAX_CHARS + 1];
	gb_get_text(gb, text);

	BeginDrawing();
	ClearBackground(RAYWHITE);

	// Background and border for the editing area
	DrawRectangleRec(box, BLACK);
	DrawRectangleLinesEx(box, 1, RED);

	// Render text line by line, splitting on newline characters
	int x = (int)box.x + 8;
	int y = (int)box.y + 8;
	int start = 0;
	int length = gb_get_length(gb);

	for (int i = 0; i <= length; i++) {
		if (text[i] == '\n' || text[i] == '\0') {
			char line[MAX_CHARS + 1];
			int line_len = i - start;
			memcpy(line, text + start, line_len);
			line[line_len] = '\0';
			DrawText(line, x, y, FONT_SIZE, RAYWHITE);
			y += LINE_HEIGHT;
			start = i + 1;
		}
	}

	// Draw cursor: a thin vertical bar, blinking every 20 frames.
	// We measure the pixel width of the text up to the cursor column so the
	// bar lands exactly at the insertion point even with a proportional font.
	if ((frame / 20) % 2 == 0) {
		char line_slice[MAX_CHARS + 1];
		int line_start = gb->cursor.pos - gb->cursor.col;
		int col = gb->cursor.col;

		for (int i = 0; i < col; i++) {
			line_slice[i] = gb_get_char_at(gb, line_start + i);
		}

		line_slice[col] = '\0';

		int cursor_x =
			(int)box.x + 8 + MeasureText(line_slice, FONT_SIZE);
		int cursor_y = (int)box.y + 8 + gb->cursor.line * LINE_HEIGHT;
		DrawRectangle(cursor_x, cursor_y, 2, FONT_SIZE, GOLD);
	}

	// Status bar: human-readable position and raw gap-buffer debug info
	DrawText(TextFormat("Ln %d  Col %d  chars: %d", gb->cursor.line + 1,
			    gb->cursor.col + 1, gb_get_length(gb)),
		 10, SCREEN_H - 24, 20, DARKGRAY);
	DrawText(TextFormat("DEBUG: gap[%d, %d]  cursor.pos: %d", gb->gap_start,
			    gb->gap_end, gb->cursor.pos),
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

	// Create the gap buffer that holds all editor state
	GapBuffer *gb = gb_create();
	if (!gb) {
		CloseWindow();
		return 1;
	}

	// The editor fills the entire window
	Rectangle box = { 0, 0, SCREEN_W, SCREEN_H };

	int frame = 0;

	// One KeyRepeat struct per repeating key
	KeyRepeat left = { 0 }, right = { 0 }, up = { 0 }, down = { 0 };
	KeyRepeat backspace = { 0 };

	// delay: seconds before repeat starts; rate: seconds between repeat fires
	float delay = 0.2f;
	float rate  = 0.03f;

	while (!WindowShouldClose()) {
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
