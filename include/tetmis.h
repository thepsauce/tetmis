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

/* Parsed program arguments */
struct program_args {
	int fps;
	int level;
	int x, y;
	int scalex, scaley;
	int gridw, gridh;
	int colors[10][2];
};

int parse_args(int argc, const char **argv);

extern struct program_args program_args;

extern time_t time_stamp;
extern time_t elapsed_time;

void chktime(bool reset);

struct transform {
	int xPos, yPos;
	int xScale, yScale;

};

extern struct transform global_transform;

enum {
	GS_STARTUP,
	GS_PLAYING,
	GS_ENTERNAME,
	GS_PIECEFELL,
	GS_PAUSED,
	GS_GAMEOVER,
};

void game_setstate(int state);
void game_drawframe(int x, int y, int w, int h, bool fill);
void game_erase(int x, int y, int w, int h);

struct playing_data {
	int startLevel;
	int level;
	int points;
	int lines;
};

int level_getpoints(int level, int lines);
int level_getframespercell(int level);

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
};

struct piece {
	int x, y;
	char mat[4 * 4];
	int sqn;
};

extern struct playing_data main_data;
extern struct grid main_grid;

void playing_reset(void);

#endif
