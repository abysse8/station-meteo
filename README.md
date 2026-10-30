# Weather Station — MVP

A simulated weather station written in C with GTK 3 and Cairo.
It generates one day of temperature and humidity readings (one every 10 simulated minutes, 144 in total), shows the latest values, plots both over 24 hours and exports the full day to a text file.

## Project structure

```text
station-meteo/
├── main.c        starts GTK and opens the window
├── meteo.c/.h    simulation of the readings + TXT export (no GUI code)
├── interface.c/.h  window, labels, Cairo graphs, timer, export button
├── Makefile      make / make run / make clean
└── README.md
```

## Install the dependencies (macOS, Homebrew)

```sh
xcode-select --install          # Clang and make, if not already installed
brew install gtk+3 pkg-config   # GTK 3 (Cairo comes with it) and pkg-config
```

## Build and run (VS Code terminal)

```sh
make        # compile -> ./station-meteo
make run    # compile if needed, then launch
make clean  # delete build files
```

## How it works

**Simulated time vs real time.** A GTK timer fires every 0.2 real seconds. Each tick adds one reading representing 10 simulated minutes, so a full simulated day takes about 29 real seconds. The line under the title shows both clocks: the simulated time of the latest reading and the real time of the computer.

**Generating the measurements** (`meteo.c`). At start-up the program picks random parameters for the day: an average temperature, a daily amplitude and an average humidity. Each reading is:

- temperature = average + amplitude × daily cycle + drift
- humidity = average humidity − 2.5 × amplitude × daily cycle + drift

The daily cycle is a cosine equal to +1 at 15:00 (warmest) and −1 at 03:00 (coldest). Humidity follows the opposite cycle, because air holds relatively less moisture when it is warm. The drift is a random walk: it changes by only a small random amount between two readings, so the curves stay smooth. Values are clamped to −5…40 °C and 0…100 %.

**Drawing the graphs with Cairo** (`interface.c`, `on_draw_graph`). Each graph is a `GtkDrawingArea`. GTK calls its `draw` callback with a Cairo context `cr`, and the function then:

1. paints the background,
2. draws horizontal grid lines with value labels (the vertical axis, with its unit in the title),
3. draws ticks and time labels every 6 hours (the horizontal axis),
4. converts each reading to pixel coordinates and joins them with `cairo_move_to` / `cairo_line_to`, then `cairo_stroke`.

When a new reading arrives, `gtk_widget_queue_draw` asks GTK to redraw both graphs. The humidity axis is fixed at 0–100 %. The temperature axis adapts to the data in 5 °C steps.

**TXT export** (`weather_day_export_txt` in `meteo.c`). The button becomes active once all 144 readings exist. Clicking it writes `weather_data.txt` in the folder the program was launched from: a header with today's real date, then one line per reading. The function checks `fopen`, `ferror` and `fclose`. A dialog shows either the full path of the file or the system error message (for example "Permission denied").

## Limitations

- The data is simulated. There is no real sensor and no network connection.
- The export file name is fixed and the file is overwritten on each export.
- If the program is launched from Finder rather than the terminal, the export goes to the process's current folder, which may not be the project folder.
- The program simulates one day and then stops generating readings. Restart it to get a new day.
