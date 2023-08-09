#include "tetmis.h"

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

int game_state;

int gs_startup(int c);
int gs_playing(int c);
int gs_entername(int c);
int gs_piecefell(int c);
int gs_paused(int c);
int gs_gameover(int c);

int (*game_states[])(int c) = {
	[GS_STARTUP] = gs_startup,
	[GS_PLAYING] = gs_playing,
	[GS_ENTERNAME] = gs_entername,
	[GS_PIECEFELL] = gs_piecefell,
	[GS_PAUSED] = gs_paused,
	[GS_GAMEOVER] = gs_gameover,
};

void
game_setstate(int state)
{
	chktime(true);
	game_state = state;
}

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
game_drawframe(int x, int y, int w, int h, bool fill)
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
		for(int i = 0; i < (int) ARRLEN(text); i++)
			for(int j = 0; j < n; j++) {
				if(text[i][j] == ' ')
					continue;
				if(elapsed_time > 2000) {
					attr_set(0, j / 6, NULL);
				}
				mvaddstr((LINES - ARRLEN(text)) / 2 + i,
					(COLS - strlen(text[0])) / 2 + j,
					"\u2588");
			}
		return 0;
	}

	if(c != '\n') {
		attr_set(0, 0, NULL);
		mvaddstr((LINES - ARRLEN(text)) / 2 + ARRLEN(text) + 1,
			(COLS - 20) / 2, "Press enter to play!");
		return 0;
	}
	clear();
	game_setstate(GS_PLAYING);
	return 0;
}

int
main(int argc, char **argv)
{
	int argi;

	setlocale(LC_ALL, ""); /* to properly display unicode */
	srand(time(NULL));

	if((argi = parse_args(argc - 1, (const char**) argv + 1)) != 0) {
		argi++;
		fprintf(stderr, "failed parsing argument no. %d (%s)\n",
			argi, argv[argi]);
		return -1;
	}

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
	for(int i = 1; i < 9; i++)
		init_pair(i, program_args.colors[i][0], program_args.colors[i][1]);

	main_data.startLevel = program_args.level;
	main_data.level = program_args.level;
	main_grid.transform = (struct transform) {
		.xPos = (COLS - GRID_WIDTH - 10) / 2,
		.yPos = (LINES - GRID_HEIGHT - GRID_PADTOP) / 2,
		.xScale = program_args.scalex,
		.yScale = program_args.scaley,
	};
	main_grid.w = program_args.gridw;
	main_grid.h = program_args.gridh + GRID_PADTOP;

	main_grid.mat = malloc(main_grid.w * main_grid.h);
	memset(main_grid.mat, 0, main_grid.w * main_grid.h);
	playing_reset();

	game_setstate(GS_STARTUP);
	while(1) {
		const int c = getch();
		chktime(false);
		(*game_states[game_state])(c);
	}

	endwin();
	return 0;
}

