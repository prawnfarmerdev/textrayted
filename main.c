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
  int pos;   // absolute position in buffer
  int line;  // current line number
  int col;   // current column on that line
} Cursor;

int GetLineStart(const char* buffer, int lineNum)
{
  int line = 0;
  int i = 0;
  while (buffer[i] != '\0')
  {
    if (line == lineNum) return i;
    if (buffer[i] == '\n') line++;
    i++;
  }
  return i;
}

void UpdateCursor(Cursor* cursor, const char* buffer)
{
  int line = 0;
  int col = 0;
  for (int i = 0; i < cursor->pos; i++)
  {
    if (buffer[i] == '\n')
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

bool IsAnyKeyPressed()
{
  if (GetKeyPressed() > 0) return true;
  for (int key = 32; key <= 348; key++)
  {
    if (IsKeyDown(key)) return true;
  }
  return false;
}

void HandleCharInput(char* name, Cursor* cursor)
{
  int key = GetCharPressed();
  while (key > 0)
  {
    if ((key >= 32) && (key <= 125) && (cursor->pos < MAX_INPUT_CHARS))
    {
      name[cursor->pos] = (char)key;
      name[cursor->pos + 1] = '\0';
      (cursor->pos)++;
    }
    key = GetCharPressed();
  }
  if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER))
  {
    name[cursor->pos] = '\n';
    name[cursor->pos + 1] = '\0';
    (cursor->pos)++;
  }
  if (IsKeyPressed(KEY_TAB))
  {
    if (cursor->pos + TABWIDTH < MAX_INPUT_CHARS)
    {
      for (int i = 0; i < 4; i++)
      {
        name[cursor->pos] = ' ';
        (cursor->pos)++;
      }
      name[cursor->pos] = '\0';
    }
  }
  UpdateCursor(cursor, name);
}

void HandleBackspace(char* name, Cursor* cursor, float* backspaceHoldTimer,
                     float* lastDeleteTime, float holdDelay, float holdRepeat)
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
    else if (*backspaceHoldTimer > holdDelay &&
             (*backspaceHoldTimer - *lastDeleteTime) >= holdRepeat)
    {
      shouldDelete = true;
      *lastDeleteTime = *backspaceHoldTimer;
    }
    if (shouldDelete && cursor->pos > 0)
    {
      (cursor->pos)--;
      name[cursor->pos] = '\0';
    }
  }
  else
  {
    *backspaceHoldTimer = 0.0f;
    *lastDeleteTime = 0.0f;
  }
  UpdateCursor(cursor, name);
}

void DrawEditor(Rectangle textBox, char* name, Cursor cursor, int framesCounter)
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
    int len = (int)strlen(name);
    char lineBuf[MAX_INPUT_CHARS + 1];

    for (int i = 0; i <= len; i++)
    {
      if (name[i] == '\n' || name[i] == '\0')
      {
        int lineLen = i - lineStart;
        memcpy(lineBuf, name + lineStart, lineLen);
        lineBuf[lineLen] = '\0';
        DrawText(lineBuf, (int)textBox.x + 5,
                 (int)textBox.y + 8 + lineNum * LINE_HEIGHT, FONT_SIZE,
                 LIGHTGRAY);
        lineNum++;
        lineStart = i + 1;
      }
    }
  }

  /* status bar */
  DrawText(TextFormat("Ln %d  Col %d   chars: %d/%d", cursor.line + 1,
                      cursor.col + 1, cursor.pos, MAX_INPUT_CHARS),
           15, INITSCREENHEIGHT - 24, 20, DARKGRAY);

  // blinking cursor
  if (((framesCounter / 20) % 2) == 0)
  {
    int lineStart = GetLineStart(name, cursor.line);
    char lineText[MAX_INPUT_CHARS + 1];
    memcpy(lineText, name + lineStart, cursor.col);
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
  InitWindow(screenWidth, screenHeight, "Texrayted");

  char name[MAX_INPUT_CHARS + 1] = "\0";
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
    HandleCharInput(name, &cursor);
    HandleBackspace(name, &cursor, &backspaceHoldTimer, &lastDeleteTime,
                    holdDelay, holdRepeat);
    framesCounter++;
    DrawEditor(textBox, name, cursor, framesCounter);
  }

  CloseWindow();
  return 0;
}
