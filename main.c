#include <string.h>

#include "raylib.h"
#define MAX_INPUT_CHARS 999999
#define INITSCREENWIDTH 1600
#define INITSCREENHEIGHT 900
#define TABWIDTH 4
#define FONT_SIZE 40
#define LINE_HEIGHT 48

typedef struct
{
  int pos;   // absolute position in content
  int line;  // current line number
  int col;   // current column on that line
} Cursor;

int GetLineStart(const char* content, int lineNum)
{
  int line = 0;
  int i = 0;
  while (content[i] != '\0')
  {
    if (line == lineNum) return i;
    if (content[i] == '\n') line++;
    i++;
  }
  return i;
}

void UpdateCursor(Cursor* cursor, const char* content)
{
  int line = 0;
  int col = 0;
  for (int i = 0; i < cursor->pos; i++)
  {
    if (content[i] == '\n')
    {
      line++;
      col = 0;
    }
    else
    {
      col++;
    }
  }
  cursor->line = line;
  cursor->col = col;
}

static void
InsertText(char* content, Cursor* cursor, const char* src, int len)
{
  int contentLen = (int)strlen(content);
  // Safety check for content overflow
  if (contentLen + len >= MAX_INPUT_CHARS)
    return;

  // Shift the tail right We use memmove because the source and destination overlap
  memmove(content + cursor->pos + len, content + cursor->pos, contentLen - cursor->pos + 1);

  // Insert new text We use memcpy here because 'src' is a different string entirely, so there is no risk of overlap.
  memcpy(content + cursor->pos, src, len);

  cursor->pos += len;
}

bool IsAnyKeyPressed()
{
  if (GetKeyPressed() > 0) return true;
  for (int key = 32; key <= 348; key++)
  {
    if (IsKeyDown(key)) return true;
  }
  return false;
}

void HandleCharInput(char* content, Cursor* cursor)
{
  int key = GetCharPressed();
  while (key > 0)
  {
    if ((key >= 32) && (key <= 125) && (cursor->pos < MAX_INPUT_CHARS))
    {
      char ch = (char)key;
      InsertText(content, cursor, &ch, 1);
    }
    key = GetCharPressed();
  }
  if ((IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressedRepeat(KEY_ENTER) || IsKeyPressedRepeat(KEY_KP_ENTER)) && (cursor->pos < MAX_INPUT_CHARS))
  {
    char ch = '\n';
    InsertText(content, cursor, &ch, 1);
  }
  if ((IsKeyPressed(KEY_TAB) || IsKeyPressedRepeat(KEY_TAB)) && (cursor->pos < MAX_INPUT_CHARS))
  {
    char spaces[TABWIDTH];
    memset(spaces, ' ', TABWIDTH);
    InsertText(content, cursor, spaces, TABWIDTH);
  }
  UpdateCursor(cursor, content);
}

void HandleArrowKeyInput(char* content, Cursor* cursor)
{
  if ((IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT)) && (cursor->pos < MAX_INPUT_CHARS))
  {
    if (cursor->pos > 0)
    {
      cursor->pos--;
      UpdateCursor(cursor, content);
    }
  }
  if ((IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT)) && (cursor->pos < MAX_INPUT_CHARS))
  {
    if (cursor->pos < strlen(content))
    {
      cursor->pos++;
      UpdateCursor(cursor, content);
    }
  }
  if (IsKeyPressed(KEY_UP) || IsKeyPressedRepeat(KEY_UP))
  {
    if (cursor->line > 0)
    {
      int prevLineStart = GetLineStart(content, cursor->line - 1);
      /* length of previous line */
      int nextLineStart = GetLineStart(content, cursor->line);
      int prevLineLen = nextLineStart - prevLineStart - 1; /* -1 for \n */
      int newCol = cursor->col < prevLineLen ? cursor->col : prevLineLen;
      cursor->pos = prevLineStart + newCol;
      UpdateCursor(cursor, content);
    }
  }
  if (IsKeyPressed(KEY_DOWN) || IsKeyPressedRepeat(KEY_DOWN))
  {
    int nextLineStart = GetLineStart(content, cursor->line + 1);
    if (content[nextLineStart] != '\0' || nextLineStart < (int)strlen(content))
    {
      /* find end of next line */
      int i = nextLineStart;
      while (content[i] != '\0' && content[i] != '\n')
        i++;
      int nextLineLen = i - nextLineStart;
      int newCol = cursor->col < nextLineLen ? cursor->col : nextLineLen;
      cursor->pos = nextLineStart + newCol;
      UpdateCursor(cursor, content);
    }
  }
}

void HandleBackspace(char* content, Cursor* cursor, float* backspaceHoldTimer, float* lastDeleteTime, float holdDelay, float holdRepeat)
{
  if (IsKeyDown(KEY_BACKSPACE))
  {
    *backspaceHoldTimer += GetFrameTime();
    bool shouldDelete = false;
    if (IsKeyPressed(KEY_BACKSPACE))
    {
      shouldDelete = true;
      *lastDeleteTime = *backspaceHoldTimer;
    }
    else if (*backspaceHoldTimer > holdDelay && (*backspaceHoldTimer - *lastDeleteTime) >= holdRepeat)
    {
      shouldDelete = true;
      *lastDeleteTime = *backspaceHoldTimer;
    }
    if (shouldDelete && cursor->pos > 0)
    {
      int contentLen = (int)strlen(content);
      /* shift everything from pos onwards one step left */
      memmove(content + cursor->pos - 1, content + cursor->pos, contentLen - cursor->pos + 1);
      cursor->pos--;
      UpdateCursor(cursor, content);
    }
  }
  else
  {
    *backspaceHoldTimer = 0.0f;
    *lastDeleteTime = 0.0f;
  }
  UpdateCursor(cursor, content);
}

void DrawEditor(Rectangle textBox, char* content, Cursor cursor,
                int framesCounter)
{
  BeginDrawing();

  ClearBackground(RAYWHITE);
  DrawRectangleRec(textBox, BLACK);
  DrawRectangleLines((int)textBox.x, (int)textBox.y, (int)textBox.width,
                     (int)textBox.height, RED);

  // draw text line by line (DrawText doesn't handle \n)
  {
    int lineNum = 0;
    int lineStart = 0;
    int len = (int)strlen(content);
    char lineBuf[MAX_INPUT_CHARS + 1];

    for (int i = 0; i <= len; i++)
    {
      if (content[i] == '\n' || content[i] == '\0')
      {
        int lineLen = i - lineStart;
        memcpy(lineBuf, content + lineStart, lineLen);
        lineBuf[lineLen] = '\0';
        DrawText(lineBuf, (int)textBox.x + 5, (int)textBox.y + 8 + lineNum * LINE_HEIGHT, FONT_SIZE, LIGHTGRAY);
        lineNum++;
        lineStart = i + 1;
      }
    }
  }

  /* status bar */
  DrawText(TextFormat("Ln %d  Col %d   chars: %d/%d", cursor.line + 1, cursor.col + 1, cursor.pos, MAX_INPUT_CHARS), 15, INITSCREENHEIGHT - 24, 20, DARKGRAY);

  // blinking cursor
  if (((framesCounter / 20) % 2) == 0)
  {
    int lineStart = GetLineStart(content, cursor.line);
    char lineText[MAX_INPUT_CHARS + 1];
    memcpy(lineText, content + lineStart, cursor.col);
    lineText[cursor.col] = '\0';

    int cursorX = (int)textBox.x + 5 + MeasureText(lineText, FONT_SIZE);
    int cursorY = (int)textBox.y + 8 + cursor.line * LINE_HEIGHT;

    DrawRectangle(cursorX, cursorY, 2, FONT_SIZE, GOLD);
  }

  EndDrawing();
}

int main(void)
{
  const int screenWidth = INITSCREENWIDTH;
  const int screenHeight = INITSCREENHEIGHT;
  InitWindow(screenWidth, screenHeight, "Textrayted");

  char content[MAX_INPUT_CHARS + 1] = "\0";
  Cursor cursor = {0, 0, 0};
  Rectangle textBox = {0, 0, screenWidth, screenHeight};
  int framesCounter = 0;
  float backspaceHoldTimer = 0.0f;
  float holdDelay = 0.4f;
  float holdRepeat = 0.05f;
  float lastDeleteTime = 0.0f;

  SetTargetFPS(60);

  while (!WindowShouldClose())
  {
    HandleCharInput(content, &cursor);
    HandleArrowKeyInput(content, &cursor);
    HandleBackspace(content, &cursor, &backspaceHoldTimer, &lastDeleteTime, holdDelay, holdRepeat);
    framesCounter++;
    DrawEditor(textBox, content, cursor, framesCounter);
  }

  CloseWindow();
  return 0;
}
