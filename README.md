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

<kbd>h</kbd>   Move left<br>
<kbd>j</kbd>   Move down<br>
<kbd>l</kbd>   Move right<br>

<kbd>d</kbd>   Rotate left<br>
<kbd>f</kbd>   Rotate right<br>

<kbd>Enter</kbd>  Pause

## Windows version

I made a version where I used WINAPI, it can be found here: https://github.com/thepsauce/Tetris-in-C
