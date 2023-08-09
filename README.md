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

<kbd>h</kbd>   Move left
<kbd>j</kbd>   Move down
<kbd>l</kbd>   Move right

<kbd>d</kbd>   Rotate left
<kbd>f</kbd>   Rotate right

<kbd>Enter</kbd>  Pause

## Windows version

I made a version where I used WINAPI, it can be found here: https://github.com/thepsauce/Tetris-in-C
