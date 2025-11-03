# Weather-Client-Server

A small C project that contains two parts:

- Weather CLI app: a console program that fetches current weather from Open‑Meteo for a chosen city. It maintains a local cache for both city metadata and recent weather responses.
- TCP client/server demo: a minimal chat-style example showing blocking sockets and a request/response loop between a client and a server.

Both parts live in the same codebase. The compiled binary from the provided Makefile runs the Weather CLI. The TCP server and client functions are included as library-style code and can be invoked from a small runner if you want to try them.

---

## Contents

- What the Weather app does
- How the Weather app works (flow and modules)
- What happens on the Server side (TCP demo)
- What happens on the Client side (TCP demo)
- Project layout
- Build and run
  - macOS (Makefile provided)
  - Linux
  - Windows (WSL or MSYS2/vcpkg)
- Data formats and caching
- Error handling and limits
- Troubleshooting

---

## What the Weather app does

From a list of cities (plus any you add via API search), you select a city and the app prints a compact current weather report, then offers to show a detailed report. It uses:

- Open‑Meteo Geocoding API to look up latitude/longitude when a city is missing.
- Open‑Meteo Forecast API to fetch current weather.
- Local caching on disk to avoid repeated API calls:
  - Cities you add are cached under `cities/`.
  - Weather responses are cached for 15 minutes under `cache/`.

The app handles Swedish characters in case-insensitive matching (å/ä/ö → a, ö → o) so you can type names comfortably.

---

## How the Weather app works

This section follows the execution path and the main modules.

### 1) Entry point: `src/main.c`

- Prints a welcome text and a city list.
- Runs an input loop: initialize cities, handle user choice, then ask if you want to fetch weather for another city.

### 2) Cities (list and selection): `includes/cities.h`, `src/cities.c`, `includes/city.h`, `src/city.c`

- A doubly linked list (`Cities`) stores `City` nodes with `name`, `latitude`, `longitude`.
- Initialization does three things:
  1) Ensure folders exist: `cache/` for weather data, `cities/` for city metadata.
  2) Read any existing city JSON files in `cities/` to prepopulate the list.
  3) Add the built‑in list of Swedish cities (see `cities_list`).
- The list is printed, then the user types a city name.
- Case-insensitive matching is done with Scandinavian character normalization (see `utils_replace_swedish_chars`).
- If the city isn’t found, the app offers to search Open‑Meteo Geocoding API and add it. New cities are saved as JSON under `cities/` so they persist.

### 3) Fetch current weather: `includes/meteo.h`, `src/meteo.c`

- Build a URL for Open‑Meteo Forecast API: `https://api.open-meteo.com/v1/forecast?latitude=…&longitude=…&current_weather=true`.
- Call the NetworkHandler to retrieve JSON (from cache or via HTTP).
- Parse JSON using cJSON into a `MeteoWeatherData` struct:
  - City name, coordinates, timezone, elevation
  - Current weather fields: time, interval, temperature, windspeed, winddirection, is_day, weathercode
- Print a compact summary. Ask if you want a detailed report with all fields.

### 4) Network and HTTP: `includes/networkhandler.h`, `src/networkhandler.c`, `includes/http.h`, `src/http.c`

- `networkhandler_get_data(url, &m, flag)` is the I/O gateway.
  - It MD5‑hashes the URL to derive a cache filename (see `utils_hash_url`).
  - If a cache file exists and is younger than 900 seconds, it’s read from disk.
  - Otherwise, it performs an HTTP request using libcurl and writes the cache file (when `flag == FLAG_WRITE`).
- `http_api_request` sets up curl, streams bytes into a `NetworkHandler`, and returns them to the caller.

### 5) JSON parsing helpers: `includes/parsedata.h`, `src/parsedata.c`

- Thin wrappers around cJSON to safely get `double`, `int`, `string` fields with sensible fallbacks.

### 6) Caching: `includes/cache.h`, `src/cache.c`

- Weather responses: directory `cache/`, filename `<md5(url)>.json`.
- City metadata: directory `cities/`, filename `<md5(name+lat+lon)>.json`.
- Helper functions read/write files and check existence.

### 7) Utilities: `includes/utils.h`, `src/utils.c`

- String duplication, MD5 hashing (OpenSSL EVP), path creation, cache age comparison, user input handling, case-insensitive compare, and Swedish character normalization.

---

## What happens on the Server side (TCP demo)

Files: `includes/server.h`, `src/server.c`

- Creates a TCP socket on port 8080, binds to all interfaces, and starts listening.
- Accepts one client and enters a loop:
  - Reads a message from the client.
  - Prints it on the server console and prompts the server operator to type a response.
  - Sends that response back to the client.
  - If the response starts with `exit`, the server prints “Server Exit…” and breaks the loop.
- Blocking I/O, minimal error handling; designed as a teaching/demo snippet.

Note: This server is not invoked by `main`. To run it, either create a small runner that calls `start_server()` or temporarily replace the weather program’s `main()` with a call to `start_server()`.

---

## What happens on the Client side (TCP demo)

Files: `includes/client.h`, `src/client.c`

- Creates a TCP socket and connects to `127.0.0.1:8080`.
- Enters a loop:
  - Reads a line from stdin, sends it to the server.
  - Waits for the server reply and prints it.
  - If the reply starts with `exit`, the client prints “Client Exit…” and quits.

Note: Like the server, this is a demo not wired to `main`. Create a tiny runner to call `start_client()` when you want to test the chat.

---

## Project layout

```
Weather/
  main                 # Executable output (after build, named "Weather")
  Makefile             # macOS-focused Makefile that builds the Weather CLI
  includes/            # Public headers for modules
  src/                 # C sources
    libs/cJSON/        # Embedded cJSON library
  cache/               # Weather response cache (runtime, created by app)
  cities/              # City metadata cache (runtime, created by app)
  build/               # Object files and dep files (generated by Makefile)
```

Key headers and sources:

- `main.c` – program entry point (Weather CLI)
- `cities.c`, `city.c` – city list and add/find
- `meteo.c` – API calls and printing weather
- `networkhandler.c`, `http.c` – caching + HTTP via curl
- `cache.c`, `parsedata.c`, `utils.c` – helpers and primitives
- `server.c`, `client.c` – TCP chat demo code

---

## Build and run

You need a C17 compiler, libcurl, and OpenSSL (for MD5). cJSON is vendored in `src/libs/cJSON`.

### macOS (Makefile provided)

- Requires Homebrew with `openssl@3` and `curl` available (macOS ships curl). The Makefile finds OpenSSL include/lib via `brew --prefix openssl@3`.
- Build:

```bash
make
```

- Run the Weather CLI:

```bash
make run
```

- Clean build files:

```bash
make clean
```

### Linux

- Install dependencies (example for Debian/Ubuntu): `build-essential`, `libssl-dev`, `libcurl4-openssl-dev`.
- Edit the Makefile if necessary to point to your OpenSSL include/lib paths (commented variables show where).
- Then run `make` as above.

### Windows

This codebase uses POSIX headers in the TCP demo and OpenSSL/cURL in the Weather app. Easiest paths on Windows:

1) WSL (Windows Subsystem for Linux)
   - Install Ubuntu from the Store.
   - Inside WSL, install `build-essential libssl-dev libcurl4-openssl-dev`.
   - Run `make` from the project folder. Then `./Weather`.

2) MSYS2 (native Windows toolchain)
   - Install MSYS2 and open a Mingw64 shell (not MSYS shell).
   - Install deps:
     - `pacman -S --needed mingw-w64-x86_64-toolchain mingw-w64-x86_64-curl mingw-w64-x86_64-openssl`
   - Adjust the Makefile include/lib flags to use `-I/mingw64/include` and `-L/mingw64/lib` if needed.
   - Build with `make` (Mingw64 make).

3) vcpkg (Visual Studio / CMake)
   - If you prefer MSVC + CMake, you can create a CMakeLists.txt and bring `curl` and `openssl` via vcpkg. This repo doesn’t include CMake yet.

PowerShell note: If you run from PowerShell on Windows without WSL/MSYS2, you’ll need a Windows-compatible toolchain and libraries available to the compiler/linker. Using WSL is typically the quickest route.

---

## Running the program

- From the terminal, execute the built binary `Weather`.
- You will see a city list. Type a city name (case-insensitive, å/ä → a, ö → o normalization).
- If the city is unknown, you can opt into fetching it from Open‑Meteo geocoding. If found, it’s added and cached.
- The program fetches current weather, prints a compact summary, and asks whether to show the full report.
- After finishing, it asks whether you want to fetch weather for another city.

Cache and data folders (`cache/`, `cities/`) are created automatically next to the executable when running from the repository root.

---

## Data formats and caching

- City files (`cities/*.json`):
  ```json
  { "name": "Gothenburg", "latitude": 57.7089, "longitude": 11.9746 }
  ```
  Filenames are MD5 of `"<name><lat><lon>"` (no separator). Example: `49d4039ea5d38688ea4c1517e62e4ddd.json`.

- Weather files (`cache/*.json`): Raw API responses from Open‑Meteo Forecast endpoint. Filename is MD5 of the request URL.
- Weather cache TTL: 900 seconds (15 minutes). If the file is older, a fresh API request is made and the file is overwritten.

---

## Error handling and limits

- Network failures are reported and abort the current action.
- JSON parsing uses defaults when a field is missing (`Unknown`, `0`, `0.0`).
- The TCP demo is blocking and single-connection; it’s for demonstration only.
- The app uses Open‑Meteo’s free endpoints; be considerate about rate limits. The cache is designed to limit needless requests.

---

## Troubleshooting

- Linker errors for OpenSSL or cURL:
  - Ensure include and lib paths are correct for your platform; adjust the Makefile variables.
- `cache/` or `cities/` not created:
  - The app calls `utils_create_folder`, which handles both Windows and POSIX. Ensure the binary runs from the repo root so relative paths resolve.
- Strange characters in city names:
  - Input normalization replaces å/ä/ö with ASCII equivalents for matching. Stored names preserve originals.

---

## Extending

- Add hourly/daily forecasts: extend the request URL and parse additional sections.
- Add a TUI/GUI: keep the modules; swap only the presentation layer.
- Persist settings (units, language) in a config file.

---

## Credits

- Weather and geocoding by Open‑Meteo (free and open APIs).
- cJSON for JSON parsing.
- libcurl for HTTP.
- OpenSSL EVP for MD5 hashing of cache keys.

---

## Quick start on WSL (Ubuntu) — build and run

Follow these minimal, copy‑paste steps to compile and run everything in Windows Subsystem for Linux.

1) Install toolchain and libraries (inside WSL):

```bash
sudo apt update
sudo apt install -y build-essential libssl-dev libcurl4-openssl-dev
```

2) Go to the project folder (Windows path is under /mnt/c):

```bash
cd /mnt/c/Academy/Main_project/Weather-Client-Server
```

3) Build from the repo root (a top‑level Makefile delegates to the Weather subproject):

```bash
make clean
make
```

4) Run the Weather CLI app:

```bash
make run
```

You’ll see the city list and an input prompt. Type a city name (e.g., "Stockholm"). If the city doesn’t exist locally, use the search option to add it; results are cached in `cities/` and weather in `cache/`.

5) Optional — run the TCP demo:

- Server (terminal A):
  ```bash
  make -C Weather/server
  make -C Weather/server run
  ```

- Client (terminal B):
  ```bash
  make -C Weather/client
  make -C Weather/client run
  ```

### Running from Windows PowerShell via WSL

If you prefer to trigger builds from PowerShell, call into WSL to use the Linux toolchain:

```powershell
wsl bash -lc "cd /mnt/c/Academy/Main_project/Weather-Client-Server && make clean && make"
wsl bash -lc "cd /mnt/c/Academy/Main_project/Weather-Client-Server && make run"
```

### Troubleshooting quick checks

- No makefile found: ensure you’re in the repo root or `Weather/` directory.
  ```bash
  pwd; ls -1 Makefile
  ```
- Missing headers (OpenSSL/cURL): install the dev packages shown above. On Ubuntu, cURL headers live under `/usr/include/x86_64-linux-gnu/curl/` and are already accounted for in the Makefiles.
