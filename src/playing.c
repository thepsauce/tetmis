#include "tetmis.h"
#include <sys/stat.h> /* mkdir */

struct playing_data main_data;
int down_points;
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
	{ 0, 0, { 	0, 0, 0,
			7, 7, 7,
			0, 7, 0 }, 3 },
}, cur_piece, next_piece;

static void
playing_nextpiece(void)
{
	cur_piece = next_piece;
	cur_piece.x = (main_grid.w - cur_piece.sqn) / 2;
	cur_piece.y = GRID_PADTOP;
	next_piece = pieces[rand() % ARRLEN(pieces)];
}

void
playing_reset(void)
{
	main_data.points = 0;
	main_data.lines = 0;
	down_points = 0;
	main_data.level = main_data.startLevel;
	playing_nextpiece();
	playing_nextpiece();
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
			move(j * sy + py, i * sx + px);
			if(ch != 0) {
				attr_set(A_NORMAL, ch, NULL);
				for(int s = 0; s < main_grid.transform.xScale;
						s++)
					addstr("\u2588");
			} else {
				attr_set(A_DIM, 8, NULL);
				for(int s = 0; s < main_grid.transform.xScale;
						s++)
					addstr("\u2588");
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
				move((y + j) * sy + py, (x + i) * sx + px);
				for(int s = 0; s < main_grid.transform.xScale;
						s++)
					addstr("\u2588");
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
			for(int s = 0; s < main_grid.transform.xScale;
					s++)
				addstr("\u2588");
			move(main_grid.transform.yPos +
					main_grid.lastLines[i] *
					main_grid.transform.yScale,
				main_grid.transform.xPos +
					(main_grid.w - pos - 1) *
					main_grid.transform.xScale);
			for(int s = 0; s < main_grid.transform.xScale;
					s++)
				addstr("\u2588");
		}
		return 0;
	}

	if(main_grid.nLastLines > 0) {
		int level;

		main_data.lines += main_grid.nLastLines;
		level = main_data.lines / 10;
		if(level > main_data.level)
			main_data.level = level;
		main_data.points += level_getpoints(main_data.level,
				main_grid.nLastLines);
	}
	main_data.points += down_points;
	down_points = 0;
	playing_nextpiece();
	/* if the next piece collides with the
	** grid after spawning, it's game over */
	game_setstate(grid_collides(&main_grid, &cur_piece) ?
			GS_GAMEOVER : GS_PLAYING);
	return 0;
}

int
gs_playing(int c)
{
	struct piece p;
	time_t ticks;
	bool ispress = false;

	p = cur_piece;
	switch(tolower(c)) {
	case -1: {
		int x, y, w, h;
		struct transform t;

		ticks = level_getframespercell(main_data.level) *
			1000 / program_args.fps;
		if(elapsed_time >= ticks) {
		down:
			if(piece_fall(&main_grid, &cur_piece) >= 0) {
				game_setstate(GS_PIECEFELL);
				return 0;
			}
			if(ispress)
				down_points++;
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
		game_drawframe(t.xPos - t.xScale,
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
		game_drawframe(t.xPos - t.xScale,
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
		game_drawframe(t.xPos - t.xScale,
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
		game_drawframe(x, y, w, h, false);
		/* draw title */
		mvprintw(y, x + (w - 7) / 2, "Lines %d", main_data.lines);
		return 0;
	}
	case '\n':
	case 0x1b:
		game_setstate(GS_PAUSED);
		return 0;
	case 'd':
		piece_rot(&p, true);
		break;
	case 'f':
		piece_rot(&p, false);
		break;
	case 'j':
		ticks = elapsed_time;
		ispress = true;
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
		game_setstate(GS_PLAYING);
		break;
	}
	return 0;
}

int
gs_gameover(int c)
{
	(void) c;
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
	if(elapsed_time < 3000) {
		mvaddstr(main_grid.transform.yPos + main_grid.h *
				main_grid.transform.yScale / 2,
			main_grid.transform.xPos + (main_grid.w *
				main_grid.transform.xScale - 10) / 2,
				"Game Over!");
		return 0;
	}
	game_setstate(GS_ENTERNAME);
	return 0;
}

int
gs_entername(int c)
{
	static char namebuf[15];
	static int nnamebuf;
	switch(c) {
	case -1:
		mvaddstr(main_grid.transform.yPos + main_grid.h *
				main_grid.transform.yScale / 2,
			main_grid.transform.xPos + (main_grid.w *
				main_grid.transform.xScale - 10) / 2,
				"Enter name:");
		mvprintw(main_grid.transform.yPos + main_grid.h *
				main_grid.transform.yScale / 2 + 1,
			main_grid.transform.xPos,
				"%*s", main_grid.w *
					main_grid.transform.xScale, "");
		mvaddstr(main_grid.transform.yPos + main_grid.h *
				main_grid.transform.yScale / 2 + 1,
			main_grid.transform.xPos + (main_grid.w *
				main_grid.transform.xScale - nnamebuf) / 2,
				namebuf);
		return 0;
		break;
	case KEY_DC: case KEY_BACKSPACE: case 0x7f:
		if(nnamebuf)
			namebuf[--nnamebuf] = 0;
		break;
	case '\n': {
		char path[4096];
		const char *home;

		home = getenv("HOME");
		if(home) {
			size_t n;
			struct tm *tm;
			FILE *fp;

			strcpy(path, home);
			strcat(path, "/.tetmis/");
			mkdir(path, 0700);
			n = strlen(path);
			memcpy(path + n, namebuf, nnamebuf);
			n += nnamebuf;

			tm = gmtime(&(time_t) { time(NULL) });
			strftime(path + n, sizeof(path) - n,
				"_%Y-%m-%d_%H:%M:%S.dat", tm);

			fp = fopen(path, "w");
			if(fp) {
				fwrite(namebuf, 1, nnamebuf, fp);
				fputc(0, fp);
				fwrite(&main_data, sizeof(main_data), 1, fp);
				fclose(fp);
			}
		}
		memset(main_grid.mat, 0, main_grid.w * main_grid.h);
		game_setstate(GS_PLAYING);
		break;
	}
	default:
		if(nnamebuf + 1 == sizeof(namebuf))
			namebuf[sizeof(namebuf) - 1] = c;
		else
			namebuf[nnamebuf++] = c;
	}
	return 0;
}
