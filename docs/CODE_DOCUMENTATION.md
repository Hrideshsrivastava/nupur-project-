# Code Documentation: Metro Route Navigation System

The whole program is in one file, **`metro.cpp`**. This document explains how it works and describes every function in the order it appears in the file.

---

## 1. Overview

```
main()
 ├─ buildNetwork()          creates stations, lines and connections (the graph)
 ├─ trieAddStation()        puts every station name into the Trie
 └─ menu loop
     ├─ 1 planJourney()      runs all 6 searches, compares them, recommends one
     ├─ 2 routeByCriterion() runs one search
     ├─ 3 searchStations()   Trie search
     ├─ 4 stationDetails()   lines, neighbours and crowd per hour
     ├─ 5 showLines()        list of lines and their stations
     ├─ 6 crowdedStations()  top 10 crowded stations at an hour (sorting)
     └─ 7 showHistory()      last 10 journeys (queue)
```

How a route is found:

1. `readStation()` turns what the user typed into a station id. It tries the **hash map** first and falls back to the **Trie**.
2. `findRoute()` runs **Dijkstra** (with a **priority queue**) for most criteria, or **BFS** for "Fewest Stops".
3. `computeStats()` works out the time, distance, line changes, fare and crowd for the route.
4. `printDirections()` prints the step-by-step directions.

The code only uses features from C++11 and the standard library, so it compiles with MinGW on Windows as well as on Linux and macOS. All output is plain ASCII text.

---

## 2. Constants

| Name | Value | Meaning |
|---|---|---|
| `INF` | `1e18` | "Infinity" for Dijkstra distances. |
| `CHANGE_MINUTES` | `5.0` | Minutes added every time you change lines. |
| `STOP_MINUTES` | `0.5` | Halt time of the train at each station. |
| `HISTORY_SIZE` | `10` | How many journeys the history queue keeps. |
| `HOURLY_DEMAND[24]` | `0.01` – `1.00` | Share of peak crowd for each hour of the day. The peaks are at 09:00 and 18:00. |

---

## 3. Data types

| Type | Fields | Meaning |
|---|---|---|
| `Edge` | `to`, `line`, `km`, `minutes` | One connection in the adjacency list: from a station to `to` on line `line`. |
| `Line` | `name`, `speed`, `premium`, `stations` | A metro line. `stations` holds the station ids in running order. `premium` is true for the Airport Express. |
| `Station` | `name`, `lines`, `popularity` | A station, the lines that stop there, and how busy it is (1–10). |
| `Stop` | `name`, `km` | Used only when building the network: a station name and its distance from the previous station. |
| `Route` | `label`, `path`, `hopLine`, `km`, `minutes`, `changes`, `fare`, `crowd`, `score` | A found route. `path` is the list of station ids. `hopLine[i]` is the line used between `path[i]` and `path[i+1]`. |
| `Criterion` | `FASTEST`, `SHORTEST`, `LEAST_CHANGES`, `CHEAPEST`, `LEAST_CROWDED`, `FEWEST_STOPS` | What a search optimises. `CRITERION_NAMES[]` holds the matching text. |
| `TrieNode` | `next`, `stationIds` | One node of the Trie. `next` maps a letter to a child node. `stationIds` lists the stations whose name ends at this node. |
| `QueueItem` | `cost`, `station`, `line` | One entry in Dijkstra's priority queue. `operator>` lets `greater<QueueItem>` turn the priority queue into a min-heap. |

## 4. Global data

| Name | Type | Meaning |
|---|---|---|
| `stations` | `vector<Station>` | All stations. A station's id is its index in this vector. |
| `lines` | `vector<Line>` | All lines. A line's id is its index. |
| `adj` | `vector<vector<Edge>>` | **Adjacency list**: `adj[s]` holds the edges leaving station `s`. |
| `stationIndex` | `unordered_map<string,int>` | **Hash map** from a simplified name (see `makeKey`) to a station id. |
| `history` | `queue<string>` | **Queue** of the last 10 journeys. |
| `trie` | `vector<TrieNode>` | The **Trie**. `trie[0]` is the root, and children are stored as indexes into this vector. |
| `inputEnded` | `bool` | Set when the input ends (Ctrl+Z / Ctrl+D, or the end of a piped file), so that the program exits cleanly. |
| `sortHour` | `int` | The hour used by `byCrowd` while sorting. |

---

## 5. Functions

### Small helpers

| Function | What it does |
|---|---|
| `string toStr(double value, int decimals)` | Converts a number to text with a fixed number of decimals, e.g. `toStr(3.14159, 1)` gives `"3.1"`. It uses `ostringstream`, which works on every compiler, including old MinGW where `to_string` is missing. |
| `string toStr(int value)` | Converts an integer to text. |
| `string trim(const string& s)` | Removes spaces, tabs and newlines from both ends. |
| `string makeKey(const string& s)` | Keeps only letters and digits, in lowercase: `"Dilli Haat - INA"` gives `"dillihaatina"`. Used as the hash map key, so `"rajiv   CHOWK"` still finds Rajiv Chowk. |
| `string makeWords(const string& s)` | Lowercase words separated by single spaces: `"Dilli Haat - INA"` gives `"dilli haat ina"`. Used for the Trie. |
| `string formatMinutes(double minutes)` | `42` gives `"42 min"`, and `75` gives `"1 h 15 min"`. |
| `string formatHour(int hour)` | `9` gives `"09:00"`. |

### Building the graph

| Function | What it does |
|---|---|
| `int addStation(const string& name)` | Looks the name up in the hash map. If the station already exists, it returns that id. This is how one station is shared by several lines and becomes an interchange. Otherwise it adds a new station, an empty adjacency list and a hash map entry, and returns the new id. |
| `void addLine(name, speed, premium, stops)` | Creates a line, then goes through its stops in order. For each stop it gets the station id with `addStation`, records that the line stops there, and connects it to the previous stop. Travel time is `km / speed × 60 + STOP_MINUTES`. Every connection is stored in **both directions**, because the graph is undirected. |
| `void setPopularity(const string& name, int value)` | Sets how busy a station is (1–10). |
| `void buildNetwork()` | Builds the whole network: 11 `addLine` calls (Yellow, Blue, Blue Branch, Red, Green, Green Branch, Violet, Magenta, Pink, Grey, Airport Express). Then it sets the popularity of the busiest stations. Every other station gets `3 + 2 × (number of lines − 1)`, so interchanges count as busier. |

### Trie

| Function | What it does |
|---|---|
| `void trieInsert(const string& text, int stationId)` | Walks down the Trie one letter at a time, creating missing nodes, and stores `stationId` at the last node. |
| `void trieAddStation(int stationId)` | Inserts the station's full name, plus every part that starts at a new word. For "Noida Sector 62" it inserts `noida sector 62`, `sector 62` and `62`, so a search can match any word of a name. |
| `void trieCollect(int node, vector<int>& result)` | Depth-first search that collects every station id stored at or below `node`. |
| `bool byName(int a, int b)` | Compares two stations by name; used by `sort`. |
| `vector<int> trieSearch(const string& prefix)` | Follows the typed letters from the root. If the path breaks off, there is no match. Otherwise it collects all ids below that node, removes duplicates (`sort` + `unique`) and sorts them by name. |

### Crowd and fare

| Function | What it does |
|---|---|
| `int crowdLevel(int station, int hour)` | Predicted crowd from 0 to 100: `(0.25 + 0.75 × popularity / 10) × HOURLY_DEMAND[hour] × 100`. |
| `string crowdLabel(int level)` | `"Low"` (below 25), `"Moderate"` (below 50), `"High"` (below 75), otherwise `"Very High"`. |
| `int slabFare(double km)` | Distance-based fare: up to 2 km Rs.11, up to 5 km Rs.21, up to 12 km Rs.32, up to 21 km Rs.43, up to 32 km Rs.54, otherwise Rs.64. |
| `int premiumFare(double km)` | Airport Express fare: `20 + 2 × km` (km rounded up), at most Rs.60. |
| `void computeStats(Route& r, int hour)` | For each hop of the route, it finds the edge on the line in `hopLine` and adds its distance and time. Each time the line differs from the previous hop, it adds a change and `CHANGE_MINUTES`. The fare is `slabFare` on the normal-line km plus `premiumFare` on the Airport Express km. The crowd is the average `crowdLevel` of all stations on the route. |

### Dijkstra's algorithm

| Function | What it does |
|---|---|
| `double edgeCost(const Edge& e, bool lineChange, Criterion criterion, int hour)` | The weight of an edge, depending on what we optimise. The travel minutes include the change time when `lineChange` is true. <br>• **Fastest**: minutes <br>• **Shortest**: km <br>• **Least Changes**: 1000 per change + minutes, so changes always matter most <br>• **Cheapest**: km, × 3 on the Airport Express <br>• **Least Crowded**: minutes × (1 + crowd at the next station / 50) |
| `Route dijkstra(int source, int target, Criterion criterion, int hour)` | **Dijkstra on (station, line) pairs.** Plain Dijkstra on stations can't tell whether you reached Rajiv Chowk on the Yellow or the Blue line, so it can't charge for changing lines. Here every state is a station *plus* the line you arrived on, which gives `dist[station][line + 1]`; slot 0 means "not on a train yet". <br>1. Push the source with cost 0 into a **min-heap** (`priority_queue` with `greater`). <br>2. Pop the cheapest state, skipping it if a cheaper cost was already found. Stop when the target is popped. <br>3. For every edge, work out whether it is a line change, add `edgeCost`, and if the new cost is lower, update `dist` and `parent` and push the new state. <br>4. Follow the `parent` links back from the target to rebuild `path` and `hopLine`, reverse them, and call `computeStats`. <br>Returns a route with an empty `path` if there is no route. |

### Breadth-first search

| Function | What it does |
|---|---|
| `Route bfs(int source, int target, int hour)` | **BFS** with a `queue`. It visits stations level by level, so the first time the target is reached, the path has the fewest stops. For each station it remembers the previous station and the line of the edge used to reach it, then rebuilds the route from those. |
| `Route findRoute(int source, int target, Criterion criterion, int hour)` | Calls `bfs` for "Fewest Stops" and `dijkstra` for everything else. |

### Printing

| Function | What it does |
|---|---|
| `string towards(int line, int a, int b)` | The direction of travel: compares the positions of `a` and `b` on the line and returns the last or the first station of the line. |
| `void printSummary(const Route& r)` | One line with time, distance, stops, changes, fare and crowd. |
| `void printDirections(const Route& r)` | Step-by-step directions. It prints "Board *line* at *station* (towards …)" at the start and at every line change, then each station passed, then the summary. |
| `void printLines(int station)` | Prints the names of the lines that stop at a station, separated by commas. |

### Input

| Function | What it does |
|---|---|
| `bool readLine(const string& prompt, string& text)` | Shows the prompt and reads one line with `getline`. Returns false (and sets `inputEnded`) when there is no more input. |
| `int readNumber(const string& prompt, int low, int high)` | Keeps asking until the user types a whole number between `low` and `high`. It uses `strtol`, so text like "abc" is rejected. Returns −1 at the end of input. |
| `int readStation(const string& prompt)` | Asks for a station. **1.** An exact name is found through the hash map. **2.** Otherwise the Trie is searched: one match is selected automatically, and several matches are listed (up to 15) for the user to pick by number. **3.** No match gives a message and asks again. An empty line returns −1, meaning cancel. |
| `int readHour()` | Asks for the departure hour (0–23). |
| `bool readJourney(int& source, int& target, int& hour)` | Asks for the source, destination and hour. Returns false if the user cancels or picks the same station twice. |
| `void addToHistory(const Route& r, int hour)` | Adds a one-line summary of the journey to the history queue. If the queue holds more than `HISTORY_SIZE`, the oldest entry is removed with `pop()`, so the queue is first in, first out. |

### Menu options

| Function | What it does |
|---|---|
| `bool sameRoute(const Route& a, const Route& b)` | True if two routes use the same stations and the same lines. |
| `bool byScore(const Route& a, const Route& b)` | Compares routes by score; used by `sort`. |
| `void planJourney()` | **Menu 1.** Runs all six searches. If two searches find the same route, it keeps one copy and joins their labels (for example "Fastest, Shortest"). Each route gets a score in "equivalent minutes": `minutes + 5 × changes + 0.2 × fare + 0.1 × crowd`. In other words, a line change counts as 5 extra minutes, a rupee as 0.2 minutes and each percent of crowd as 0.1 minutes. The routes are **sorted** by score and shown in a table, and route #1 is printed as the recommendation and added to the history. The user can then view the directions of any other route. |
| `void routeByCriterion()` | **Menu 2.** The user picks one criterion; the program finds that route, prints its directions and adds it to the history. |
| `void searchStations()` | **Menu 3.** Trie search: prints every matching station with its lines. |
| `void stationDetails()` | **Menu 4.** Shows a station's lines, whether it is an interchange, its neighbouring stations, and the expected crowd for every hour from 06:00 to 23:00 with a `#` bar. |
| `void showLines()` | **Menu 5.** Lists every line with its number of stations, length and end stations. The length only counts edges of that line, because two lines can share a stretch of track. The user can then pick a line to see all its stations, with the other lines available at each interchange. |
| `bool byCrowd(int a, int b)` | Compares two stations by crowd at `sortHour` (highest first); used by `sort`. |
| `void crowdedStations()` | **Menu 6.** Asks for an hour, **sorts** all stations by crowd and prints the top 10. |
| `void showHistory()` | **Menu 7.** Prints the history queue from oldest to newest. It works on a copy, because reading a `queue` means popping from it. |

### main

| Function | What it does |
|---|---|
| `int main()` | Builds the network and the Trie, prints how many lines and stations were loaded, then repeats the menu until the user enters 0 or the input ends. |

---

## 6. Time complexity

V = number of stations (224), E = number of connections, L = number of lines (11), k = length of the typed text.

| Operation | Time |
|---|---|
| Station lookup (hash map) | O(k) on average |
| Trie search | O(k + number of matches × log) |
| Dijkstra on (station, line) states | O((V·L + E·L) log(V·L)) |
| BFS | O(V + E) |
| Sorting stations by crowd | O(V log V) |
| Adding to the history queue | O(1) |

---

## 7. How to change things

- **Add a line or station:** add an `addLine(...)` call in `buildNetwork()`. A station that appears on two lines automatically becomes an interchange.
- **Change how routes are recommended:** edit the score formula in `planJourney()`.
- **Change fares or crowd levels:** edit `slabFare`, `premiumFare`, `HOURLY_DEMAND` or the `setPopularity` calls.
