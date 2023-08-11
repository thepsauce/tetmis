#include "tetmis.h"

struct program_args program_args = {
	.fps = 30,
	.level = 0,
	.x = -1, .y = -1,
	.scalex = 2, .scaley = 1,
	.gridw = 10, .gridh = 30,
	.colors = {
		{ COLOR_BLACK, COLOR_BLACK },
		{ COLOR_BLUE, COLOR_BLACK },
		{ COLOR_RED, COLOR_BLACK },
		{ COLOR_GREEN, COLOR_BLACK },
		{ COLOR_YELLOW, COLOR_BLACK },
		{ COLOR_MAGENTA, COLOR_BLACK },
		{ COLOR_WHITE, COLOR_BLACK },
		{ COLOR_CYAN, COLOR_BLACK },
		{ COLOR_BLUE, COLOR_BLACK },
	},
};

void
version(void)
{
	endwin();
	printf("Tetmis version 1.0.0\n");
	exit(0);
}

void
usage(void)
{
	endwin();
	printf("Tetmis version 1.0.0, built 08/11/2023\n");
	printf("Play tetris within your terminal!\n\n");
	printf("--version\tShow the version\n");
	printf("--usage | --help\tShow this help\n");
	printf("The following program arguments can be used to change the experience:\n");
	printf("-level=number\tSet the initial level\n");
	printf("-color*=number,number\tSet the color pair\n");
	printf("-x=number\tSet the grid x-position\n");
	printf("-y=number\tSet the grid y-position\n");
	printf("-scalex=number\tSet the grid x-scale\n");
	printf("-scaley=number\tSet the grid y-scale\n");
	printf("-gridw=number\tSet the grid width\n");
	printf("-gridh=number\tSet the grid height\n");
	exit(0);
}

static const char *possible_args[] = {
	"&", (char*) version, "-version", "v",
	"&", (char*) usage, "-usage", "usage",
	"&", (char*) usage, "-help", "help", "h",
	"&i", (char*) &program_args.fps, "fps",
	"&i", (char*) &program_args.level, "level",
	"&ii", (char*) &program_args.colors[0][0],
		(char*) &program_args.colors[0][1], "color0",
	"&ii", (char*) &program_args.colors[1][0],
		(char*) &program_args.colors[1][1], "color1",
	"&ii", (char*) &program_args.colors[2][0],
		(char*) &program_args.colors[2][1], "color2",
	"&ii", (char*) &program_args.colors[3][0],
		(char*) &program_args.colors[3][1], "color3",
	"&ii", (char*) &program_args.colors[4][0],
		(char*) &program_args.colors[4][1], "color4",
	"&ii", (char*) &program_args.colors[5][0],
		(char*) &program_args.colors[5][1], "color5",
	"&ii", (char*) &program_args.colors[6][0],
		(char*) &program_args.colors[6][1], "color6",
	"&ii", (char*) &program_args.colors[7][0],
		(char*) &program_args.colors[7][1], "color7",
	"&ii", (char*) &program_args.colors[8][0],
		(char*) &program_args.colors[8][1], "color8",
	"&ii", (char*) &program_args.colors[9][0],
		(char*) &program_args.colors[9][1], "color9",
	"&i", (char*) &program_args.x, "x",
	"&i", (char*) &program_args.y, "y",
	"&i", (char*) &program_args.scalex, "scalex",
	"&i", (char*) &program_args.scalex, "scaley",
	"&i", (char*) &program_args.gridw, "gridw",
	"&i", (char*) &program_args.gridh, "gridh",
};

int
parse_args(int argc, const char **argv)
{
	const char **pinfo, *info, *arg, *pos;
	int p;
	size_t plen;

	for(int argi = 0; argi < argc; argi++) {
		arg = argv[argi];
		if(*arg != '-')
			return argi;
		while(*arg == '-')
			arg++;

		for(p = 0; p < (int) ARRLEN(possible_args); p++) {
			pos = possible_args[p];
			if(pos[0] == '&') {
				pinfo = possible_args + p;
				p += pos[1] == 0 ? 1 : strlen(pos) - 1;
				continue;
			}
			plen = strlen(pos);
			if(!strncmp(arg, pos, plen))
				break;
		}
		if(p == (int) ARRLEN(possible_args))
			return argi;
		arg += plen;
		info = *pinfo + 1;
		if((info[0] != 0) != (*arg == '='))
			return argi;
		pinfo++;
		if(info[0] == 0) {
			((void (*)(void)) *pinfo)();
			continue;
		}
		arg++;
		for(; *info; info++, pinfo++) switch(*info) {
		case 'i':
			*(int*) *pinfo = strtol(arg, (char**) &arg, 10);
			while(*arg == ',')
				arg++;
			break;
		}
	}
	return 0;
}

