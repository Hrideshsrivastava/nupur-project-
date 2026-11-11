# Metro Route Navigation System

A simple C++ console program that finds routes on the Delhi NCR Metro.

**Jaypee Institute of Information Technology, Noida**
B.Tech Semester V. Data Structures and Algorithms Design Lab (25B17CI578). Project Based Learning.
Submitted to: Dr. Arpita Jadhav Bhatt

| Name | Enrolment No. |
|---|---|
| Suchismita Chaterjee | 2401020063 |
| Abhimanyu Sharma | 2401020068 |
| Nupur Bubna | 2401020073 |
| Aarushi Singh | 2401020075 |

---

## What it does

The metro network (11 lines, 224 stations) is stored as a graph. You enter a source station, a destination station and a departure hour, and the program finds routes by different criteria:

- **Fastest**: least travel time (Dijkstra)
- **Shortest**: least distance (Dijkstra)
- **Least Changes**: fewest line changes (Dijkstra)
- **Cheapest**: lowest fare (Dijkstra)
- **Least Crowded**: avoids crowded stations at that hour (Dijkstra)
- **Fewest Stops**: fewest stations on the way (BFS)

It compares all these routes, recommends the best one, and prints step-by-step directions.

It can also search stations by part of their name (Trie), show station details with the expected crowd for each hour of the day, list the metro lines, show the most crowded stations at a given hour, and keep a history of your last 10 journeys.

## DSA concepts used

| Requirement | DSA concept |
|---|---|
| Metro network | Graph |
| Network representation | Adjacency list |
| Fastest / shortest / cheapest / least changes / least crowded route | Dijkstra's algorithm |
| Route selection in Dijkstra | Priority queue (min-heap) |
| Fewest stops route | BFS |
| Station lookup by name | Hash map (`unordered_map`) |
| Station search / autocomplete | Trie |
| Journey history | Queue |
| Ranking routes and crowded stations | Sorting |

---

## How to run it

The whole program is one file, `metro.cpp`. It uses only the standard C++ library and works with any C++11 (or newer) compiler.

### Windows (MinGW)

1. Install MinGW if you don't have it. Any of these works:
   - **MSYS2** (https://www.msys2.org). In the *MSYS2 UCRT64* window, run `pacman -S mingw-w64-ucrt-x86_64-gcc`.
   - **Code::Blocks**: download the version that comes "with MinGW".
   - **Dev-C++**: includes MinGW.
2. Open **Command Prompt** or **PowerShell** in the project folder.
3. Compile and run:

```
g++ -std=c++11 -O2 -o metro.exe metro.cpp
metro.exe
```

> If `g++` is "not recognized", add MinGW's `bin` folder (for example `C:\msys64\ucrt64\bin`) to your PATH, or run the command from the MSYS2 UCRT64 window.

**Using an IDE instead:**

- **Code::Blocks**: *File, New, Empty file*. Paste or open `metro.cpp`, then press **F9** (Build and run).
- **Dev-C++**: *File, Open*, choose `metro.cpp`, then press **F11** (Compile and run).
  Older Dev-C++ (5.11) uses C++98 by default and shows errors. To fix this, go to *Tools, Compiler Options*, tick *"Add the following commands when calling the compiler"*, type `-std=c++11` and press OK. You only need to do this once.

### Linux

```
sudo apt install g++        # Ubuntu/Debian, if g++ is not installed
g++ -std=c++11 -O2 -o metro metro.cpp
./metro
```

### macOS

```
xcode-select --install      # installs the compiler, if needed
g++ -std=c++11 -O2 -o metro metro.cpp
./metro
```

---

## Example session

```
Metro Route Navigation System - Delhi NCR
11 lines, 224 stations loaded.

----------------- MENU -----------------
1. Plan a journey (compare all routes)
2. Find a route by one criterion
3. Search stations
4. Station details and crowd
5. Show metro lines
6. Most crowded stations at an hour
7. Journey history
0. Exit
Enter choice: 1

From station: dwarka sector 21
  Selected: Dwarka Sector 21
To station: botanical
  Selected: Botanical Garden
Departure hour (0-23, e.g. 9 for 9 AM): 9

Found 2 different route(s):
  #  Time        Km     Stops  Changes  Fare    Crowd  Found by
  1  1 h 14 min  38.7   19     2        Rs.103  60%    Fastest, Shortest, Least Crowded, Fewest Stops
  2  1 h 42 min  43.5   41     0        Rs.64   55%    Least Changes, Cheapest

Recommended route (#1, best balance of time, changes, fare and crowd):
  Dwarka Sector 21 -> Botanical Garden
  Board Airport Express at Dwarka Sector 21 (towards New Delhi)
      IGI Airport T3
      ...
  Get off at New Delhi and change line.
  Board Yellow Line at New Delhi (towards Millennium City Centre Gurugram)
      Rajiv Chowk
  ...
  Time: 1 h 14 min | Distance: 38.7 km | Stops: 19 | Changes: 2 | Fare: Rs. 103 | Crowd: 60% (High)
```

**Tips**

- You don't need to type full station names. Type part of a name, like `rajiv` or `sector 62`. If several stations match, the program lists them and you pick a number.
- Press **Enter** on an empty station prompt to cancel and go back to the menu.
- Everything stays on screen, so you can scroll up to see earlier results.

---

## Files

| File | Contents |
|---|---|
| `metro.cpp` | The complete program |
| `docs/CODE_DOCUMENTATION.md` | Explanation of the code, function by function |
| `README.md` | This file |

## Notes on the data

The station order and the interchange stations follow the real Delhi Metro. The distances between stations, train speeds, fares and crowd levels are approximations for this project, not official DMRC data.

The Aqua Line is not included, because it only connects to the rest of the network through a walkway between two differently named stations.
