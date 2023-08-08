#include "tetmis.h"

struct transform global_transform;

/* For timed processes (e.g. falling piece, animations) */

time_t time_stamp;
time_t elapsed_time;

void
chktime(bool reset)
{
	struct timespec ts;
	time_t millis;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	millis = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
	if(reset) {
		time_stamp = millis;
		elapsed_time = 0;
	} else {
		elapsed_time = millis - time_stamp;
	}
}

/* Game state */

int game_state = GS_MENU;

int gs_startup(int c);
int gs_menu(int c);
int gs_playing(int c);
int gs_piecefell(int c);
int gs_paused(int c);
int gs_gameover(int c);

int (*game_states[])(int c) = {
	[GS_STARTUP] = gs_startup,
	[GS_MENU] = gs_menu,
	[GS_PLAYING] = gs_playing,
	[GS_PIECEFELL] = gs_piecefell,
	[GS_PAUSED] = gs_paused,
	[GS_GAMEOVER] = gs_gameover,
};

struct game_data {
	int startLevel;
	int level;
	int points;
	int lines;

};
struct game_data main_data;
int down_points;
char main_grid_mat[GRID_WIDTH * (GRID_HEIGHT + GRID_PADTOP)];
struct grid main_grid;
struct piece pieces[] = {
	/* O */
	{ 0, 0, { 	1, 1,
			1, 1 }, 2 },
	/* I */
	{ 0, 0, { 	0, 0, 0, 0,
			2, 2, 2, 2,
			0, 0, 0, 0,
			0, 0, 0, 0 }, 4 },
	/* S */
	{ 0, 0, { 	0, 3, 3,
			3, 3, 0,
			0, 0, 0 }, 3 },
	/* Z */
	{ 0, 0, { 	4, 4, 0,
			0, 4, 4,
			0, 0, 0 }, 3 },
	/* J */
	{ 0, 0, { 	5, 0, 0,
			5, 5, 5,
			0, 0, 0 }, 3 },
	/* L */
	{ 0, 0, { 	0, 0, 6,
			6, 6, 6,
			0, 0, 0 }, 3 },
	/* T */
	{ 0, 0, { 	0, 7, 0,
			7, 7, 7,
			0, 0, 0 }, 3 },
}, cur_piece, next_piece;

void
game_erase(int x, int y, int w, int h)
{
	const int sw = w;
	while(h--) {
		w = sw;
		while(w--)
			mvaddch(y + h, x + w, ' ');
	}
}

void
game_nextpiece(void)
{
	cur_piece = next_piece;
	cur_piece.x = (main_grid.w - cur_piece.sqn) / 2;
	cur_piece.y = GRID_PADTOP;
	next_piece = pieces[rand() % ARRLEN(pieces)];
}

void
game_reset(void)
{
	main_data.points = 0;
	main_data.lines = 0;
	down_points = 0;
	main_data.level = main_data.startLevel;
	game_nextpiece();
	game_nextpiece();
}

int
level_getframespercell(int level)
{
	if(level <= 8)
		return 48 - level * 5;
	if(level == 9)
		return 6;
	if(level < 13)
		return 5;
	if(level < 16)
		return 4;
	if(level < 19)
		return 3;
	if(level < 29)
		return 2;
	return 1;
}

int
level_getpoints(int level, int nLines)
{
	static const int multp[] = {
		0, 40, 100, 300, 1200
	};
	if(nLines > 4)
		return -1;
	return (level + 1) * multp[nLines];
}

bool
grid_collides(struct grid *grid, struct piece *piece)
{
	const int x = piece->x;
	const int y = piece->y;
	const char *const mat = piece->mat;
	const int sqn = piece->sqn;

	for(int i = 0; i < sqn; i++)
		for(int j = 0; j < sqn; j++) {
			if(mat[i + j * sqn] == 0)
				continue;
			if(x + i < 0 ||
			y + j < 0 ||
			x + i >= grid->w ||
			y + j >= grid->h)
				return true;
			if(grid->mat[x + i + (y + j) * grid->w] != 0)
				return true;
		}
	return false;
}

static inline void
grid_collapse(struct grid *grid, int row)
{
	/* shift down all rows above the given row */
	for(; row > 0; row--)
		/* use memcpy for non overlapping array regions */
		memcpy(grid->mat + row * grid->w,
			grid->mat + (row - 1) * grid->w,
			grid->w);
	/* clear first row */
	memset(grid->mat, 0, grid->w);
}

int
grid_lockpiece(struct grid *grid, struct piece *piece)
{
	const int x = piece->x;
	const int y = piece->y;
	const char *const mat = piece->mat;
	const int sqn = piece->sqn;

	/* place the piece into the grid */
	for(int i = 0; i < sqn; i++)
		for(int j = 0; j < sqn; j++) {
			const char ch = mat[i + j * sqn];
			if(ch != 0)
				grid->mat[x + i + (y + j) * grid->w] = ch;
		}
	/* check for line clears */
	grid->nLastLines = 0;
	for(int j = 0; j < sqn; j++) {
		int i;

		for(i = 0; i < grid->w; i++)
			if(grid->mat[i + (y + j) * grid->w] == 0)
				break;
		if(i == grid->w) {
			grid_collapse(grid, y + j);
			grid->lastLines[grid->nLastLines++] = y + j;
		}
	}
	return grid->nLastLines;
}

void
grid_display(struct grid *grid)
{
	const int px = grid->transform.xPos;
	const int py = grid->transform.yPos;
	const int sx = grid->transform.xScale;
	const int sy = grid->transform.yScale;
	const char *const mat = grid->mat;
	const int w = grid->w;
	const int h = grid->h;

	for(int i = 0; i < w; i++) {
		if(px + i < 0 || px + i >= COLS)
			continue;
		for(int j = GRID_PADTOP; j < h; j++) {
			const char ch = mat[i + j * w];
			if(py + j < 0 || py + j >= LINES)
				continue;
			if(ch != 0) {
				attr_set(A_NORMAL, ch, NULL);
				mvaddstr(j * sy + py, i * sx + px,
					"\u2588\u2588");
			} else {
				attr_set(A_DIM, 8, NULL);
				mvaddstr(j * sy + py, i * sx + px,
					"\u2588\u2588");
			}
		}
	}
}

int
piece_fall(struct grid *grid, struct piece *piece)
{
	const int x = piece->x;
	const int y = piece->y + 1;
	const char *const mat = piece->mat;
	const int sqn = piece->sqn;

	/* trace along the bottom of the piece (micro optimization) */
	for(int i = 0; i < sqn; i++)
		for(int j = sqn - 1; j >= 0; j--)
			if(mat[i + j * sqn] != 0) {
				if(y + j >= grid->h ||
				grid->mat[x + i + (y + j) * grid->w] != 0)
					return grid_lockpiece(grid, piece);
				break;
			}
	piece->y++;
	return -1;
}

void
piece_rot(struct piece *piece, bool ccw /* counter clock wise */)
{
	char rot[4 * 4];
	char *const mat = piece->mat;
	const int sqn = piece->sqn;

	for(int i = 0; i < sqn; i++)
		for(int j = 0; j < sqn; j++)
			if(ccw)
				rot[j + (sqn - 1 - i) * sqn] =
					mat[i + j * sqn];
			else
				rot[sqn - 1 - j + i * sqn] =
					mat[i + j * sqn];
	memcpy(mat, rot, sizeof(rot));
}

void
piece_display(const struct piece *piece, const struct transform *transform)
{
	const int x = piece->x;
	const int y = piece->y;
	const int px = transform->xPos;
	const int py = transform->yPos;
	const int sx = transform->xScale;
	const int sy = transform->yScale;
	const char *const mat = piece->mat;
	const int sqn = piece->sqn;

	for(int i = 0; i < sqn; i++)
		for(int j = 0; j < sqn; j++) {
			const char ch = mat[i + j * sqn];
			if(ch != 0) {
				attr_set(A_NORMAL, ch, NULL);
				mvaddstr((y + j) * sy + py,
					(x + i) * sx + px, "\u2588\u2588");
			}
		}
}

int
gs_piecefell(int c)
{
	if(c != -1)
		return 0;
	/* play a 800 milliseconds long flashing animation
	** for 4 line clears */
	if(elapsed_time < 800 && main_grid.nLastLines == 4) {
		color_set(7 - elapsed_time / 180, NULL);
		for(int i = 0; i < main_grid.nLastLines; i++) {
			move(main_grid.transform.yPos +
					main_grid.lastLines[i] *
				main_grid.transform.yScale,
				main_grid.transform.xPos);
			for(int j = 0; j < main_grid.w *
					main_grid.transform.xScale; j++)
				addstr("\u2588");
		}
		return 0;
	/* play a 500 milliseconds long animation for other line clears */
	} else if(elapsed_time < 500 && main_grid.nLastLines > 0 &&
			main_grid.nLastLines != 4) {
		const int pos = elapsed_time / 100;
		attr_set(A_DIM, 8, NULL);
		for(int i = 0; i < main_grid.nLastLines; i++) {
			move(main_grid.transform.yPos +
					main_grid.lastLines[i] *
					main_grid.transform.yScale,
				main_grid.transform.xPos + pos *
					main_grid.transform.xScale);
			addstr("\u2588\u2588");
			move(main_grid.transform.yPos +
					main_grid.lastLines[i] *
					main_grid.transform.yScale,
				main_grid.transform.xPos +
					(main_grid.w - pos - 1) *
					main_grid.transform.xScale);
			addstr("\u2588\u2588");
		}
		return 0;
	}

	if(main_grid.nLastLines > 0) {
		int level;

		main_data.lines += main_grid.nLastLines;
		level = main_data.lines / 10;
		if(level > main_data.level)
			main_data.level = level;
		main_data.points += level_getpoints(main_data.level, main_grid.nLastLines);
	}
	game_nextpiece();
	/* if the next piece collides with the
	** grid after spawning, it's game over */
	if(grid_collides(&main_grid, &cur_piece))
		game_state = GS_GAMEOVER;
	else
		game_state = GS_PLAYING;
	chktime(true);
	return 0;
}

void
draw_frame(int x, int y, int w, int h, bool fill)
{
	/* draw borders */
	mvhline(y, x, ACS_HLINE, w);
	mvhline(y + h, x, ACS_HLINE, w);
	mvvline(y, x, ACS_VLINE, h);
	mvvline(y, x + w, ACS_VLINE, h);
	/* draw corners */
	mvaddch(y, x, ACS_ULCORNER);
	mvaddch(y, x + w, ACS_URCORNER);
	mvaddch(y + h, x, ACS_LLCORNER);
	mvaddch(y + h, x + w, ACS_LRCORNER);
	if(fill) {
		attr_set(A_DIM, 8, NULL);
		for(y++, h--; h; y++, h--)
			for(int tx = x + 1; tx < x + w; tx++)
				mvaddstr(y, tx, "\u2588");
	}
}

int
gs_playing(int c)
{
	struct piece p;
	time_t ticks;

	p = cur_piece;
	switch(tolower(c)) {
	case -1: {
		int x, y, w, h;
		struct transform t;

		ticks = level_getframespercell(main_data.level) * 1000 / FPS;
		if(elapsed_time >= ticks) {
		down:
			if(piece_fall(&main_grid, &cur_piece) >= 0) {
				chktime(true);
				game_state = GS_PIECEFELL;
				return 0;
			}
			elapsed_time -= ticks;
			time_stamp += ticks;
		}
		grid_display(&main_grid);
		piece_display(&cur_piece, &main_grid.transform);
		/* erase the next piece area */
		t = (struct transform) {
			main_grid.transform.xPos +
				(main_grid.w + 1) *
				main_grid.transform.xScale + 2,
			main_grid.transform.yPos +
				(GRID_PADTOP + 1) *
				main_grid.transform.yScale,
			main_grid.transform.xScale,
			main_grid.transform.yScale,
		};
		attr_set(A_NORMAL, 7, NULL);
		draw_frame(t.xPos - t.xScale,
			t.yPos - t.yScale,
			6 * t.xScale - 1,
			4 * t.yScale, true);
		attr_set(A_NORMAL, 7, NULL);
		mvaddstr(t.yPos - t.yScale,
			t.xPos + (4 * t.xScale - 4) / t.xScale,
			"Next");
		piece_display(&next_piece, &t);
		attr_set(A_NORMAL, 7, NULL);
		/* draw points */
		t.yPos += 6 * t.yScale;
		draw_frame(t.xPos - t.xScale,
			t.yPos - t.yScale,
			6 * t.xScale - 1,
			2 * t.yScale, false);
		attr_set(A_NORMAL, 7, NULL);
		mvaddstr(t.yPos - t.yScale,
			t.xPos + (4 * t.xScale - 6) / t.xScale,
			"Points");
		mvprintw(t.yPos - t.yScale + 1,
			t.xPos - t.xScale + 1,
			"%d", main_data.points);
		/* draw level */
		t.yPos += 5 * t.yScale;
		draw_frame(t.xPos - t.xScale,
			t.yPos - t.yScale,
			6 * t.xScale - 1,
			2 * t.yScale, false);
		attr_set(A_NORMAL, 7, NULL);
		mvaddstr(t.yPos - t.yScale,
			t.xPos + (4 * t.xScale - 5) / t.xScale,
			"Level");
		mvprintw(t.yPos - t.yScale + 1,
			t.xPos - t.xScale + 1,
			"%d", main_data.level);
		/* draw frame around grid */
		x = main_grid.transform.xPos - 1;
		y = main_grid.transform.yPos - 1 + GRID_PADTOP;
		w = main_grid.w * main_grid.transform.xScale + 1;
		h = (main_grid.h + 1 - GRID_PADTOP) *
			main_grid.transform.yScale;
		draw_frame(x, y, w, h, false);
		/* draw title */
		mvprintw(y, x + (w - 7) / 2, "Lines %d", main_data.lines);
		return 0;
	}
	case '\n':
	case 0x1b:
		game_state = GS_PAUSED;
		return 0;
	case 'd':
		piece_rot(&p, true);
		break;
	case 'f':
		piece_rot(&p, false);
		break;
	case 'j':
		ticks = elapsed_time;
		goto down;
	case 'h':
		p.x--;
		break;
	case 'l':
		p.x++;
		break;

	}
	if(!grid_collides(&main_grid, &p))
		cur_piece = p;
	return 0;
}

int
gs_paused(int c)
{
	switch(c) {
	case -1:
		/* erase grid and next piece preview */
		game_erase(main_grid.transform.xPos,
			main_grid.transform.yPos + GRID_PADTOP,
			main_grid.w * main_grid.transform.xScale,
			(main_grid.h - GRID_PADTOP) * main_grid.transform.yScale);
		game_erase(main_grid.transform.xPos +
				(main_grid.w + 1) *
				main_grid.transform.xScale + 1,
			main_grid.transform.yPos +
				(GRID_PADTOP + 1) *
				main_grid.transform.yScale,
			main_grid.transform.xScale * 5,
			main_grid.transform.yScale * 3);
		/* draw paused text */
		mvaddstr(main_grid.transform.yPos + main_grid.h *
				main_grid.transform.yScale / 2,
			main_grid.transform.xPos + (main_grid.w *
				main_grid.transform.xScale - 7) / 2,
				"Paused!");
		break;
	case '\n':
	case 0x1b:
		chktime(true);
		game_state = GS_PLAYING;
		break;
	}
	return 0;
}

int
gs_gameover(int c)
{
	/* 2 second long game over animation */
	if(elapsed_time < 2000) {
		int amount;

		amount = elapsed_time / (2000 / (main_grid.h - 1 - GRID_PADTOP));
		move(main_grid.transform.yPos +
				(main_grid.h - amount) *
				main_grid.transform.yScale - 1,
			main_grid.transform.xPos);
		for(int x = 0; x < main_grid.w *
				main_grid.transform.xScale; x++)
			addch(' ');
		return 0;
	}
	switch(c) {
	case -1:
		mvaddstr(main_grid.transform.yPos + main_grid.h *
				main_grid.transform.yScale / 2,
			main_grid.transform.xPos + (main_grid.w *
				main_grid.transform.xScale - 10) / 2,
				"Game Over!");
		break;
	case '\n':
		memset(main_grid.mat, 0, main_grid.w * main_grid.h);
		chktime(true);
		game_state = GS_PLAYING;
		break;
	case 0x1b:
		game_state = GS_MENU;
		break;
	}
	return 0;
}

int
gs_startup(int c)
{
	/* show a little startup animation and
	** then ask the user to press enter */
	static const char *text[] = {
		" ##### ##### #####  # #   ###   #### ",
		"   #   #       #   #####   #   #     ",
		"   #   ###     #   # # #   #    ###  ",
		"   #   #       #   # # #   #       # ",
		"   #   #####   #   # # #  ###  ####  "
	};
	if(elapsed_time < 4000) {
		int nLetters, n;
	       
		nLetters = elapsed_time / 200;
		if(nLetters > 6)
			nLetters = 6;
		n = nLetters * 6;
		for(int i = 0; i < ARRLEN(text); i++)
			for(int j = 0; j < n; j++) {
				if(text[i][j] == ' ')
					continue;
				if(elapsed_time > 2000) {
					attr_set(0, j / 6, NULL);
				}
				mvaddstr((LINES - ARRLEN(text)) / 2 + i,
					(COLS - strlen(text[0])) / 2 + j, "\u2588");
			}
		return 0;
	}

	/* data initialization */
	main_grid.transform = (struct transform) {
		.xPos = 10,
		.yPos = 10 - GRID_PADTOP,
		.xScale = 2,
		.yScale = 1,
	};
	main_grid.w = GRID_WIDTH;
	main_grid.h = GRID_HEIGHT + GRID_PADTOP;
	main_grid.mat = main_grid_mat;
	game_reset();

	if(c != '\n') {
		attr_set(0, 0, NULL);
		mvaddstr((LINES - ARRLEN(text)) / 2 + ARRLEN(text) + 1,
			(COLS - 20) / 2, "Press enter to play!");
		return 0;
	}
	clear();
	game_state = GS_MENU;
	return 0;
}

int
gs_menu(int c)
{
	game_state = GS_PLAYING;
	return 0;
}

int
main(int argc, char **argv)
{
	setlocale(LC_ALL, ""); /* to properly display unicode */
	srand(time(NULL));

	initscr(); /* init ncurses */
	noecho(); /* don't display typed characters */
	cbreak(); /* disable line buffering */
	curs_set(0); /* hide cursor */
	timeout(0);	/* let getch() fall through and
			** return -1 when there is no character to read */
	keypad(stdscr, true);	/* if this is true,
				** keys such as the arrow keys return a
				** single value for getch() instead of
				** a special sequence */

	start_color();
	init_pair(1, COLOR_BLUE, COLOR_BLACK); /* O */
	init_pair(2, COLOR_RED, COLOR_BLACK); /* I */
	init_pair(3, COLOR_GREEN, COLOR_BLACK); /* Z */
	init_pair(4, COLOR_YELLOW, COLOR_BLACK); /* S */
	init_pair(5, COLOR_MAGENTA, COLOR_BLACK); /* J */
	init_pair(6, COLOR_WHITE, COLOR_BLACK); /* T */
	init_pair(7, COLOR_CYAN, COLOR_BLACK); /* L */
	init_pair(8, COLOR_BLUE, COLOR_BLUE); /* empty grid cell */

	chktime(true);
	game_state = GS_STARTUP;
	while(1) {
		const int c = getch();
		chktime(false);
		(*game_states[game_state])(c);
	}

	endwin();
	return 0;
}

