#ifndef INCLUDED_TETMIS_H
#define INCLUDED_TETMIS_H

#include <ctype.h>
#include <locale.h>
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#define ARRLEN(a) (sizeof(a)/sizeof*(a))

#define FPS 30

struct transform {
	int xPos, yPos;
	int xScale, yScale;

};

enum {
	GS_STARTUP,
	GS_MENU,
	GS_PLAYING,
	GS_PIECEFELL,
	GS_PAUSED,
	GS_GAMEOVER,
};

struct game_data {
	int startLevel;
	int level;
	int points;
	int lines;
};

#define GRID_WIDTH 10
#define GRID_HEIGHT 30
#define GRID_PADTOP 3

struct grid {
	struct transform transform;
	int w, h;
	char *mat;
	/* up to four last cleared lines */
	int lastLines[4];
	int nLastLines;
} main_grid;

struct piece {
	int x, y;
	char mat[4 * 4];
	int sqn;
};

#endif
