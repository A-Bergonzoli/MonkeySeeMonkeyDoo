# MonkeySeeMonkeyDoo
A terminal-based todo app written in C++ using the _Curses_ library.
This was a quick small over-the-weekend project because I was consuming too many post-it and I hate the fact that they never stick to anything. 

## Quick Start
Open an existing file:
```
./main.o <file_name>.todo
```
or create a new one:
```
./main.o
```

## Controls
| Key  | What |
| ------------- | ------------- |
| `q`  |  Save & Quit |
| `KEY_UP`  | Scroll Up  |
| `KEY_DOWN`  | Scroll Down  |
| `SHIFT` + `KEY_UP`  | Move Task Up  |
| `SHIFT` + `KEY_DOWN`  | Move Task Down  |
| `TAB`  | Change Focus (TODO / DONE)  |
| `ENTER`  | Move Item into TODO / DONE |
| `i`  | Insert a new task (`ENTER` when done) |


## References
- Original Idea: [Terminal To-Do App in Rust](https://www.youtube.com/watch?v=tR6p7ZC7RaU&pp=ugMICgJpdBABGAHKBQx0c29kaW5nIHRvZG8%3D)
    - **note**: this is neither a fork nor a re-write of rust in cpp, it's how I envisioned something based purely on my need and skills.
- Curses Library:
    - [DOCS](https://invisible-island.net/ncurses/man/)
    - [I actually prefer the Python's doc](https://docs.python.org/3/library/curses.html)