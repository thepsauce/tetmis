# tetmis
Classical Tetris using NCurses

## Quick start

```
git clone https://github.com/thepsauce/tetmis
cd tetmis
gcc src/main.c src/args.c src/playing.c -Iinclude -lncursesw -o tetmis
./tetmis -help
```

## Controls

h   Move left
j   Move down
l   Move right

d   Rotate left
f   Rotate right

CR  Pause
