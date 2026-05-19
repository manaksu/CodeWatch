# CodeWatch

A Pebble Time Steel watchface that displays the current time as a Python function with VS Code syntax highlighting.

## Features
- Time displayed as Python code with VS Code colour scheme
- JetBrains Mono font for authentic code aesthetic
- Syntax highlighting: keywords, functions, variables, strings, numbers
- Vertical time display on right edge (>> HH:MM MON DD)
- Dark mode and Light mode
- Line numbers with gutter

## Theme
- **Purple** — `if` / `else` / `return` / `def`
- **Yellow** — function name
- **Blue** — variables
- **Orange** — strings `"AM"` / `"PM"`
- **Green** — numbers (hour, minute)

## CloudPebble Setup
Add the font **twice** in Resources:
| Name | File | Size |
|------|------|------|
| `FONT_JB_12` | `JetBrainsMono-Regular.ttf` | `12` |
| `FONT_JB_14` | `JetBrainsMono-Regular.ttf` | `14` |

## Settings
- **Dark** — VS Code dark theme (default)
- **Light** — VS Code light theme

## Description
> Time as code. The watch displays the current time as a Python function — syntax-highlighted in VS Code colours, with JetBrains Mono font. Every minute, the code updates. Read the return value to tell the time. Dark mode and light mode available. *For those who think in functions.*

## Credits
Built by manaksu. JetBrains Mono font by JetBrains (SIL Open Font License) Free.
