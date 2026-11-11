// =============================================================================
//  Metro Route Navigation System (Delhi NCR Metro)
//  JIIT Noida - Data Structures and Algorithms Design Lab (25B17CI578) - PBL
//
//  Build:  g++ -std=c++11 -O2 -o metro metro.cpp      (Windows: -o metro.exe)
//  Run:    ./metro                                    (Windows: metro.exe)
//
//  DSA concepts used:
//    Graph + adjacency list ...... the metro network
//    Hash map (unordered_map) .... station name -> station id
//    Trie ........................ station search / autocomplete
//    Dijkstra + priority queue ... fastest, shortest, cheapest, least changes,
//                                  least crowded routes
//    BFS (queue) ................. route with the fewest stops
//    Queue ....................... journey history (last 10 journeys)
//    Sorting (std::sort) ......... ranking routes and crowded stations
// =============================================================================
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <map>
#include <queue>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

// ----------------------------- Constants -------------------------------------

const double INF = 1e18;
const double CHANGE_MINUTES = 5.0; // time lost when changing lines
const double STOP_MINUTES = 0.5;   // train halt at every station
const int HISTORY_SIZE = 10;       // journeys kept in the history queue

// Share of peak-hour crowd at each hour of the day (index = hour 0..23).
const double HOURLY_DEMAND[24] = {
    0.02, 0.01, 0.01, 0.01, 0.02, 0.08, 0.25, 0.55, 0.90, 1.00, 0.80, 0.55,
    0.45, 0.45, 0.45, 0.50, 0.65, 0.90, 1.00, 0.85, 0.60, 0.40, 0.20, 0.08};

// ----------------------------- Data types ------------------------------------

struct Edge {
    int to;         // station at the other end
    int line;       // metro line this connection belongs to
    double km;      // distance
    double minutes; // travel time including the halt
};

struct Line {
    string name;
    double speed;          // average speed in km/h
    bool premium;          // Airport Express has a higher fare
    vector<int> stations;  // station ids in running order
};

struct Station {
    string name;
    vector<int> lines; // lines that stop here
    int popularity;    // 1..10, used to predict crowd
};

struct Stop {
    const char* name;
    double km; // distance from the previous station
};

struct Route {
    string label;          // which search produced it, e.g. "Fastest"
    vector<int> path;      // station ids from source to destination
    vector<int> hopLine;   // line used between path[i] and path[i+1]
    double km;
    double minutes;
    int changes;
    int fare;
    int crowd;             // average crowd level 0..100
    double score;          // used for the recommendation (lower is better)
};

enum Criterion { FASTEST, SHORTEST, LEAST_CHANGES, CHEAPEST, LEAST_CROWDED, FEWEST_STOPS };
const char* CRITERION_NAMES[] = {"Fastest", "Shortest", "Least Changes",
                                 "Cheapest", "Least Crowded", "Fewest Stops"};

// ----------------------------- Global data -----------------------------------

vector<Station> stations;
vector<Line> lines;
vector<vector<Edge> > adj;                 // adjacency list: adj[station] = edges
unordered_map<string, int> stationIndex;   // hash map: normalised name -> id
queue<string> history;                     // last HISTORY_SIZE journeys

// ----------------------------- Small helpers ---------------------------------

string toStr(double value, int decimals) {
    ostringstream out;
    out << fixed << setprecision(decimals) << value;
    return out.str();
}

string toStr(int value) {
    ostringstream out;
    out << value;
    return out.str();
}

string trim(const string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

// Lowercase letters and digits only: "Dilli Haat - INA" -> "dillihaatina"
string makeKey(const string& s) {
    string key;
    for (size_t i = 0; i < s.size(); i++)
        if (isalnum((unsigned char)s[i])) key += (char)tolower((unsigned char)s[i]);
    return key;
}

// Lowercase words separated by one space: "Dilli Haat - INA" -> "dilli haat ina"
string makeWords(const string& s) {
    string out;
    bool space = false;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = s[i];
        if (isalnum(c)) {
            if (space && !out.empty()) out += ' ';
            out += (char)tolower(c);
            space = false;
        } else {
            space = true;
        }
    }
    return out;
}

string formatMinutes(double minutes) {
    int m = (int)(minutes + 0.5);
    if (m < 60) return toStr(m) + " min";
    return toStr(m / 60) + " h " + toStr(m % 60) + " min";
}

string formatHour(int hour) {
    return (hour < 10 ? "0" : "") + toStr(hour) + ":00";
}

// ----------------------------- Building the graph ----------------------------

int addStation(const string& name) {
    string key = makeKey(name);
    if (stationIndex.count(key)) return stationIndex[key]; // interchange: reuse it
    Station s;
    s.name = name;
    s.popularity = 0;
    stations.push_back(s);
    adj.push_back(vector<Edge>());
    stationIndex[key] = (int)stations.size() - 1;
    return (int)stations.size() - 1;
}

void addLine(const string& name, double speed, bool premium, const vector<Stop>& stops) {
    Line line;
    line.name = name;
    line.speed = speed;
    line.premium = premium;
    lines.push_back(line);
    int lineId = (int)lines.size() - 1;

    int previous = -1;
    for (size_t i = 0; i < stops.size(); i++) {
        int id = addStation(stops[i].name);
        stations[id].lines.push_back(lineId);
        lines[lineId].stations.push_back(id);
        if (previous != -1) {
            double km = stops[i].km;
            double minutes = km / speed * 60.0 + STOP_MINUTES;
            Edge forward = {id, lineId, km, minutes};
            Edge backward = {previous, lineId, km, minutes};
            adj[previous].push_back(forward); // undirected: store both directions
            adj[id].push_back(backward);
        }
        previous = id;
    }
}

void setPopularity(const string& name, int value) {
    stations[stationIndex[makeKey(name)]].popularity = value;
}

void buildNetwork() {
    // addLine(name, average speed km/h, premium fare?, { {station, km from previous}, ... })
    addLine("Yellow Line", 34, false, {
        {"Samaypur Badli", 0}, {"Rohini Sector 18-19", 0.8}, {"Haiderpur Badli Mor", 1.3},
        {"Jahangirpuri", 1.3}, {"Adarsh Nagar", 1.4}, {"Azadpur", 1.5}, {"Model Town", 1.4},
        {"GTB Nagar", 1.4}, {"Vishwavidyalaya", 0.8}, {"Vidhan Sabha", 1.0},
        {"Civil Lines", 1.3}, {"Kashmere Gate", 1.1}, {"Chandni Chowk", 1.1},
        {"Chawri Bazar", 1.0}, {"New Delhi", 0.8}, {"Rajiv Chowk", 1.1}, {"Patel Chowk", 1.3},
        {"Central Secretariat", 0.9}, {"Udyog Bhawan", 0.3}, {"Lok Kalyan Marg", 1.6},
        {"Jor Bagh", 1.2}, {"Dilli Haat - INA", 1.3}, {"AIIMS", 0.8}, {"Green Park", 1.0},
        {"Hauz Khas", 1.8}, {"Malviya Nagar", 1.7}, {"Saket", 0.9}, {"Qutab Minar", 1.7},
        {"Chhattarpur", 1.3}, {"Sultanpur", 1.6}, {"Ghitorni", 1.3}, {"Arjan Garh", 2.7},
        {"Guru Dronacharya", 2.3}, {"Sikanderpur", 1.0}, {"MG Road", 1.2}, {"IFFCO Chowk", 1.1},
        {"Millennium City Centre Gurugram", 1.5},
    });
    addLine("Blue Line", 32, false, {
        {"Dwarka Sector 21", 0}, {"Dwarka Sector 8", 1.0}, {"Dwarka Sector 9", 1.0},
        {"Dwarka Sector 10", 1.1}, {"Dwarka Sector 11", 0.9}, {"Dwarka Sector 12", 0.9},
        {"Dwarka Sector 13", 0.9}, {"Dwarka Sector 14", 0.9}, {"Dwarka", 1.2},
        {"Dwarka Mor", 1.0}, {"Nawada", 1.1}, {"Uttam Nagar West", 1.0},
        {"Uttam Nagar East", 0.9}, {"Janakpuri West", 1.0}, {"Janakpuri East", 1.0},
        {"Tilak Nagar", 1.0}, {"Subhash Nagar", 1.1}, {"Tagore Garden", 1.0},
        {"Rajouri Garden", 1.1}, {"Ramesh Nagar", 1.0}, {"Moti Nagar", 1.2},
        {"Kirti Nagar", 0.8}, {"Shadipur", 1.6}, {"Patel Nagar", 1.2}, {"Rajendra Place", 0.9},
        {"Karol Bagh", 1.0}, {"Jhandewalan", 1.2}, {"Ramakrishna Ashram Marg", 0.8},
        {"Rajiv Chowk", 1.0}, {"Barakhamba Road", 0.7}, {"Mandi House", 1.0},
        {"Supreme Court", 0.8}, {"Indraprastha", 0.8}, {"Yamuna Bank", 1.8},
        {"Akshardham", 1.3}, {"Mayur Vihar Phase-1", 1.8}, {"Mayur Vihar Extension", 1.2},
        {"New Ashok Nagar", 0.9}, {"Noida Sector 15", 1.1}, {"Noida Sector 16", 1.1},
        {"Noida Sector 18", 1.1}, {"Botanical Garden", 1.1}, {"Golf Course", 1.2},
        {"Noida City Centre", 1.3}, {"Noida Sector 34", 1.0}, {"Noida Sector 52", 1.3},
        {"Noida Sector 61", 1.2}, {"Noida Sector 59", 1.0}, {"Noida Sector 62", 1.2},
        {"Noida Electronic City", 1.2},
    });
    addLine("Blue Line Branch", 32, false, {
        {"Yamuna Bank", 0}, {"Laxmi Nagar", 1.3}, {"Nirman Vihar", 1.1}, {"Preet Vihar", 1.1},
        {"Karkarduma", 1.1}, {"Anand Vihar ISBT", 1.6}, {"Kaushambi", 0.8}, {"Vaishali", 1.6},
    });
    addLine("Red Line", 31, false, {
        {"Rithala", 0}, {"Rohini West", 1.4}, {"Rohini East", 1.1}, {"Pitampura", 1.0},
        {"Kohat Enclave", 1.0}, {"Netaji Subhash Place", 1.1}, {"Keshav Puram", 1.2},
        {"Kanhaiya Nagar", 0.8}, {"Inderlok", 1.2}, {"Shastri Nagar", 1.1},
        {"Pratap Nagar", 1.7}, {"Pulbangash", 0.8}, {"Tis Hazari", 0.9}, {"Kashmere Gate", 1.1},
        {"Shastri Park", 2.2}, {"Seelampur", 1.6}, {"Welcome", 1.1}, {"Shahdara", 1.2},
        {"Mansarovar Park", 1.1}, {"Jhilmil", 1.2}, {"Dilshad Garden", 0.9},
        {"Shaheed Nagar", 1.6}, {"Raj Bagh", 1.1}, {"Major Mohit Sharma Rajendra Nagar", 1.0},
        {"Shyam Park", 1.1}, {"Mohan Nagar", 1.0}, {"Arthala", 1.3}, {"Hindon River", 1.0},
        {"Shaheed Sthal New Bus Adda", 1.0},
    });
    addLine("Green Line", 33, false, {
        {"Inderlok", 0}, {"Ashok Park Main", 1.4}, {"Punjabi Bagh", 0.9}, {"Shivaji Park", 1.6},
        {"Madipur", 1.1}, {"Paschim Vihar East", 1.0}, {"Paschim Vihar West", 0.7},
        {"Peeragarhi", 1.0}, {"Udyog Nagar", 1.2}, {"Maharaja Surajmal Stadium", 1.0},
        {"Nangloi", 1.1}, {"Nangloi Railway Station", 1.0}, {"Rajdhani Park", 1.0},
        {"Mundka", 1.0},
    });
    addLine("Green Line Branch", 33, false, {
        {"Kirti Nagar", 0}, {"Satguru Ram Singh Marg", 1.1}, {"Ashok Park Main", 1.4},
    });
    addLine("Violet Line", 33, false, {
        {"Kashmere Gate", 0}, {"Lal Quila", 1.5}, {"Jama Masjid", 0.8}, {"Delhi Gate", 1.4},
        {"ITO", 1.3}, {"Mandi House", 0.8}, {"Janpath", 1.4}, {"Central Secretariat", 1.3},
        {"Khan Market", 2.1}, {"Jawaharlal Nehru Stadium", 1.4}, {"Jangpura", 0.9},
        {"Lajpat Nagar", 1.5}, {"Moolchand", 1.4}, {"Kailash Colony", 1.2},
        {"Nehru Place", 0.9}, {"Kalkaji Mandir", 1.0}, {"Govind Puri", 1.0},
        {"Harkesh Nagar Okhla", 1.5}, {"Jasola Apollo", 0.9}, {"Sarita Vihar", 1.1},
        {"Mohan Estate", 1.3}, {"Tughlakabad Station", 1.2}, {"Badarpur Border", 1.3},
        {"Sarai", 1.2}, {"NHPC Chowk", 1.0}, {"Mewala Maharajpur", 1.2},
        {"Sector 28 Faridabad", 1.0}, {"Badkal Mor", 1.1}, {"Old Faridabad", 1.6},
        {"Neelam Chowk Ajronda", 1.3}, {"Bata Chowk", 1.2}, {"Escorts Mujesar", 1.2},
        {"Sant Surdas Sihi", 2.3}, {"Raja Nahar Singh", 1.6},
    });
    addLine("Magenta Line", 35, false, {
        {"Janakpuri West", 0}, {"Dabri Mor - Janakpuri South", 1.3}, {"Dashrath Puri", 1.0},
        {"Palam", 1.4}, {"Sadar Bazaar Cantonment", 2.5}, {"Terminal 1 IGI Airport", 2.8},
        {"Shankar Vihar", 2.4}, {"Vasant Vihar", 2.4}, {"Munirka", 1.0}, {"R.K. Puram", 1.1},
        {"IIT Delhi", 1.5}, {"Hauz Khas", 1.1}, {"Panchsheel Park", 1.2}, {"Chirag Delhi", 1.3},
        {"Greater Kailash", 1.1}, {"Nehru Enclave", 1.1}, {"Kalkaji Mandir", 1.0},
        {"Okhla NSIC", 1.1}, {"Sukhdev Vihar", 1.5}, {"Jamia Millia Islamia", 1.0},
        {"Okhla Vihar", 1.0}, {"Jasola Vihar Shaheen Bagh", 1.5}, {"Kalindi Kunj", 1.8},
        {"Okhla Bird Sanctuary", 1.2}, {"Botanical Garden", 2.4},
    });
    addLine("Pink Line", 34, false, {
        {"Majlis Park", 0}, {"Azadpur", 1.5}, {"Shalimar Bagh", 1.6},
        {"Netaji Subhash Place", 1.5}, {"Shakurpur", 1.4}, {"Punjabi Bagh West", 1.6},
        {"ESI - Basaidarapur", 1.2}, {"Rajouri Garden", 1.3}, {"Mayapuri", 1.8},
        {"Naraina Vihar", 1.7}, {"Delhi Cantt", 2.3}, {"Durgabai Deshmukh South Campus", 2.1},
        {"Sir M. Vishweshwaraiah Moti Bagh", 1.4}, {"Bhikaji Cama Place", 1.0},
        {"Sarojini Nagar", 1.2}, {"Dilli Haat - INA", 1.2}, {"South Extension", 1.4},
        {"Lajpat Nagar", 1.3}, {"Vinobapuri", 1.1}, {"Ashram", 1.0},
        {"Sarai Kale Khan - Hazrat Nizamuddin", 1.2}, {"Mayur Vihar Phase-1", 2.3},
        {"Mayur Vihar Pocket 1", 1.0}, {"Trilokpuri Sanjay Lake", 1.2},
        {"East Vinod Nagar - Mayur Vihar-II", 1.0}, {"Mandawali - West Vinod Nagar", 1.1},
        {"IP Extension", 1.3}, {"Anand Vihar ISBT", 1.6}, {"Karkarduma", 1.5},
        {"Karkarduma Court", 1.0}, {"Krishna Nagar", 1.1}, {"East Azad Nagar", 1.0},
        {"Welcome", 1.6}, {"Jaffrabad", 0.9}, {"Maujpur - Babarpur", 1.1}, {"Gokulpuri", 1.2},
        {"Johri Enclave", 1.0}, {"Shiv Vihar", 0.9},
    });
    addLine("Grey Line", 30, false, {
        {"Dwarka", 0}, {"Nangli", 1.6}, {"Najafgarh", 1.6}, {"Dhansa Bus Stand", 1.1},
    });
    addLine("Airport Express", 55, true, {
        {"New Delhi", 0}, {"Shivaji Stadium", 1.9}, {"Dhaula Kuan", 6.2},
        {"Delhi Aerocity", 7.8}, {"IGI Airport T3", 3.4}, {"Dwarka Sector 21", 3.6},
        {"Yashobhoomi Dwarka Sector 25", 2.0},
    });

    // Busiest stations (1..10). Others get a value based on how many lines meet there.
    setPopularity("Rajiv Chowk", 10);       setPopularity("Kashmere Gate", 10);
    setPopularity("New Delhi", 9);          setPopularity("Chandni Chowk", 8);
    setPopularity("Hauz Khas", 8);          setPopularity("Central Secretariat", 8);
    setPopularity("Noida Sector 18", 8);    setPopularity("Botanical Garden", 8);
    setPopularity("Anand Vihar ISBT", 8);   setPopularity("Rajouri Garden", 8);
    setPopularity("Lajpat Nagar", 8);       setPopularity("Millennium City Centre Gurugram", 8);
    setPopularity("Mandi House", 7);        setPopularity("Karol Bagh", 7);
    setPopularity("Laxmi Nagar", 7);        setPopularity("Nehru Place", 7);
    setPopularity("Noida City Centre", 7);  setPopularity("Noida Sector 62", 6);
    for (size_t i = 0; i < stations.size(); i++)
        if (stations[i].popularity == 0)
            stations[i].popularity = min(10, 3 + 2 * ((int)stations[i].lines.size() - 1));
}

// ----------------------------- Trie (station search) -------------------------

struct TrieNode {
    map<char, int> next;   // letter -> index of child node
    vector<int> stationIds; // stations whose name (or a word of it) ends here
};

vector<TrieNode> trie(1); // trie[0] is the root

void trieInsert(const string& text, int stationId) {
    int node = 0;
    for (size_t i = 0; i < text.size(); i++) {
        char c = text[i];
        if (!trie[node].next.count(c)) {
            trie[node].next[c] = (int)trie.size();
            trie.push_back(TrieNode());
        }
        node = trie[node].next[c];
    }
    trie[node].stationIds.push_back(stationId);
}

// Insert the full name and every part starting at a new word, so that
// "sector 62" and "62" also find "Noida Sector 62".
void trieAddStation(int stationId) {
    string words = makeWords(stations[stationId].name);
    for (size_t i = 0; i < words.size(); i++)
        if (i == 0 || words[i - 1] == ' ') trieInsert(words.substr(i), stationId);
}

void trieCollect(int node, vector<int>& result) {
    for (size_t i = 0; i < trie[node].stationIds.size(); i++) result.push_back(trie[node].stationIds[i]);
    for (map<char, int>::iterator it = trie[node].next.begin(); it != trie[node].next.end(); ++it)
        trieCollect(it->second, result);
}

bool byName(int a, int b) { return stations[a].name < stations[b].name; }

// All stations matching the typed prefix, sorted by name.
vector<int> trieSearch(const string& prefix) {
    vector<int> result;
    string text = makeWords(prefix);
    if (text.empty()) return result;
    int node = 0;
    for (size_t i = 0; i < text.size(); i++) {
        if (!trie[node].next.count(text[i])) return result; // no match
        node = trie[node].next[text[i]];
    }
    trieCollect(node, result);
    sort(result.begin(), result.end());
    result.erase(unique(result.begin(), result.end()), result.end()); // remove duplicates
    sort(result.begin(), result.end(), byName);
    return result;
}

// ----------------------------- Crowd and fare --------------------------------

int crowdLevel(int station, int hour) {
    double popularity = 0.25 + 0.75 * stations[station].popularity / 10.0;
    int level = (int)(popularity * HOURLY_DEMAND[hour] * 100 + 0.5);
    return min(level, 100);
}

string crowdLabel(int level) {
    if (level < 25) return "Low";
    if (level < 50) return "Moderate";
    if (level < 75) return "High";
    return "Very High";
}

// Distance-based fare slabs (Rs.), similar to Delhi Metro.
int slabFare(double km) {
    if (km <= 0) return 0;
    if (km <= 2) return 11;
    if (km <= 5) return 21;
    if (km <= 12) return 32;
    if (km <= 21) return 43;
    if (km <= 32) return 54;
    return 64;
}

// Airport Express is charged separately at a premium.
int premiumFare(double km) {
    if (km <= 0) return 0;
    return min(60, 20 + 2 * (int)ceil(km));
}

// ----------------------------- Route statistics ------------------------------

// Fill in distance, time, changes, fare and crowd for a route.
void computeStats(Route& r, int hour) {
    r.km = 0;
    r.minutes = 0;
    r.changes = 0;
    double normalKm = 0, premiumKm = 0;
    for (size_t i = 0; i + 1 < r.path.size(); i++) {
        int line = r.hopLine[i];
        for (size_t j = 0; j < adj[r.path[i]].size(); j++) {
            const Edge& e = adj[r.path[i]][j];
            if (e.to == r.path[i + 1] && e.line == line) {
                r.km += e.km;
                r.minutes += e.minutes;
                if (lines[line].premium) premiumKm += e.km;
                else normalKm += e.km;
            }
        }
        if (i > 0 && line != r.hopLine[i - 1]) {
            r.changes++;
            r.minutes += CHANGE_MINUTES;
        }
    }
    r.fare = slabFare(normalKm) + premiumFare(premiumKm);
    int total = 0;
    for (size_t i = 0; i < r.path.size(); i++) total += crowdLevel(r.path[i], hour);
    r.crowd = r.path.empty() ? 0 : total / (int)r.path.size();
}

// ----------------------------- Dijkstra --------------------------------------

struct QueueItem {
    double cost;
    int station;
    int line; // line we arrived on (-1 = not boarded yet)
    bool operator>(const QueueItem& other) const { return cost > other.cost; }
};

// Weight of travelling along edge e for the chosen criterion.
double edgeCost(const Edge& e, bool lineChange, Criterion criterion, int hour) {
    double minutes = e.minutes + (lineChange ? CHANGE_MINUTES : 0);
    switch (criterion) {
        case FASTEST:       return minutes;
        case SHORTEST:      return e.km;
        case LEAST_CHANGES: return (lineChange ? 1000 : 0) + minutes;
        case CHEAPEST:      return e.km * (lines[e.line].premium ? 3 : 1);
        case LEAST_CROWDED: return minutes * (1 + crowdLevel(e.to, hour) / 50.0);
        default:            return minutes;
    }
}

// Dijkstra on (station, line) states, so that a change of line can be
// charged correctly. Returns an empty route if there is no path.
Route dijkstra(int source, int target, Criterion criterion, int hour) {
    int n = (int)stations.size();
    int slots = (int)lines.size() + 1; // slot = line + 1, slot 0 = not boarded
    vector<vector<double> > dist(n, vector<double>(slots, INF));
    vector<vector<pair<int, int> > > parent(n, vector<pair<int, int> >(slots, make_pair(-1, -1)));

    priority_queue<QueueItem, vector<QueueItem>, greater<QueueItem> > pq; // min-heap
    dist[source][0] = 0;
    QueueItem start = {0, source, -1};
    pq.push(start);
    int endLine = -2;

    while (!pq.empty()) {
        QueueItem cur = pq.top();
        pq.pop();
        if (cur.cost > dist[cur.station][cur.line + 1]) continue; // outdated entry
        if (cur.station == target) {
            endLine = cur.line;
            break;
        }
        for (size_t i = 0; i < adj[cur.station].size(); i++) {
            const Edge& e = adj[cur.station][i];
            bool lineChange = (cur.line != -1 && e.line != cur.line);
            double newCost = cur.cost + edgeCost(e, lineChange, criterion, hour);
            if (newCost < dist[e.to][e.line + 1]) {
                dist[e.to][e.line + 1] = newCost;
                parent[e.to][e.line + 1] = make_pair(cur.station, cur.line);
                QueueItem next = {newCost, e.to, e.line};
                pq.push(next);
            }
        }
    }

    Route route;
    route.label = CRITERION_NAMES[criterion];
    if (endLine == -2 || source == target) return route; // not found

    // Follow the parent links back from the target to the source.
    int s = target, l = endLine;
    while (s != -1) {
        route.path.push_back(s);
        pair<int, int> p = parent[s][l + 1];
        if (p.first != -1) route.hopLine.push_back(l);
        s = p.first;
        l = p.second;
    }
    reverse(route.path.begin(), route.path.end());
    reverse(route.hopLine.begin(), route.hopLine.end());
    computeStats(route, hour);
    return route;
}

// ----------------------------- BFS (fewest stops) ----------------------------

Route bfs(int source, int target, int hour) {
    int n = (int)stations.size();
    vector<int> previous(n, -1), previousLine(n, -1);
    vector<bool> visited(n, false);
    queue<int> q;
    q.push(source);
    visited[source] = true;
    while (!q.empty()) {
        int u = q.front();
        q.pop();
        if (u == target) break;
        for (size_t i = 0; i < adj[u].size(); i++) {
            const Edge& e = adj[u][i];
            if (!visited[e.to]) {
                visited[e.to] = true;
                previous[e.to] = u;
                previousLine[e.to] = e.line;
                q.push(e.to);
            }
        }
    }

    Route route;
    route.label = CRITERION_NAMES[FEWEST_STOPS];
    if (!visited[target] || source == target) return route;
    for (int s = target; s != -1; s = previous[s]) {
        route.path.push_back(s);
        if (previous[s] != -1) route.hopLine.push_back(previousLine[s]);
    }
    reverse(route.path.begin(), route.path.end());
    reverse(route.hopLine.begin(), route.hopLine.end());
    computeStats(route, hour);
    return route;
}

Route findRoute(int source, int target, Criterion criterion, int hour) {
    if (criterion == FEWEST_STOPS) return bfs(source, target, hour);
    return dijkstra(source, target, criterion, hour);
}

// ----------------------------- Printing --------------------------------------

// The last station of `line` in the direction from station a to station b.
string towards(int line, int a, int b) {
    const vector<int>& st = lines[line].stations;
    int posA = (int)(find(st.begin(), st.end(), a) - st.begin());
    int posB = (int)(find(st.begin(), st.end(), b) - st.begin());
    return stations[posB > posA ? st.back() : st.front()].name;
}

void printSummary(const Route& r) {
    cout << "  Time: " << formatMinutes(r.minutes) << " | Distance: " << toStr(r.km, 1)
         << " km | Stops: " << r.path.size() - 1 << " | Changes: " << r.changes
         << " | Fare: Rs. " << r.fare << " | Crowd: " << r.crowd << "% ("
         << crowdLabel(r.crowd) << ")\n";
}

// Step-by-step directions for a route.
void printDirections(const Route& r) {
    cout << "\n  " << stations[r.path.front()].name << " -> " << stations[r.path.back()].name << "\n";
    for (size_t i = 0; i + 1 < r.path.size(); i++) {
        int line = r.hopLine[i];
        if (i == 0 || line != r.hopLine[i - 1]) {
            if (i > 0) cout << "  Get off at " << stations[r.path[i]].name << " and change line.\n";
            cout << "  Board " << lines[line].name << " at " << stations[r.path[i]].name
                 << " (towards " << towards(line, r.path[i], r.path[i + 1]) << ")\n";
        }
        cout << "      " << stations[r.path[i + 1]].name << "\n";
    }
    cout << "  Get off at " << stations[r.path.back()].name << ". You have arrived.\n";
    printSummary(r);
}

void printLines(int station) {
    for (size_t i = 0; i < stations[station].lines.size(); i++) {
        if (i > 0) cout << ", ";
        cout << lines[stations[station].lines[i]].name;
    }
}

// ----------------------------- Input helpers ---------------------------------

bool inputEnded = false;

// Read one line from the keyboard. Returns false at end of input.
bool readLine(const string& prompt, string& text) {
    cout << prompt;
    if (!getline(cin, text)) {
        inputEnded = true;
        cout << "\n";
        return false;
    }
    text = trim(text);
    return true;
}

// Read a whole number between low and high. Returns -1 at end of input.
int readNumber(const string& prompt, int low, int high) {
    string text;
    while (readLine(prompt, text)) {
        char* end = 0;
        long value = strtol(text.c_str(), &end, 10);
        if (!text.empty() && *end == '\0' && value >= low && value <= high) return (int)value;
        cout << "  Please enter a number from " << low << " to " << high << ".\n";
    }
    return -1;
}

// Ask for a station name. Accepts partial names using the Trie.
// Returns the station id, or -1 if the user leaves it empty.
int readStation(const string& prompt) {
    string text;
    while (readLine(prompt, text)) {
        if (text.empty()) return -1;
        if (stationIndex.count(makeKey(text))) {                 // exact name (hash map)
            int id = stationIndex[makeKey(text)];
            cout << "  Selected: " << stations[id].name << "\n";
            return id;
        }

        vector<int> matches = trieSearch(text);
        if (matches.empty()) {
            cout << "  No station found for \"" << text << "\". Try again (or press Enter to cancel).\n";
        } else if (matches.size() == 1) {
            cout << "  Selected: " << stations[matches[0]].name << "\n";
            return matches[0];
        } else {
            size_t shown = min(matches.size(), (size_t)15);
            cout << "  Did you mean:\n";
            for (size_t i = 0; i < shown; i++)
                cout << "    " << i + 1 << ". " << stations[matches[i]].name << "\n";
            if (matches.size() > shown) cout << "    ... and " << matches.size() - shown << " more\n";
            int choice = readNumber("  Enter number (0 to type again): ", 0, (int)shown);
            if (choice > 0) return matches[choice - 1];
        }
    }
    return -1;
}

int readHour() {
    return readNumber("Departure hour (0-23, e.g. 9 for 9 AM): ", 0, 23);
}

// Ask for source, destination and hour. Returns false if cancelled.
bool readJourney(int& source, int& target, int& hour) {
    source = readStation("From station: ");
    if (source == -1) return false;
    target = readStation("To station: ");
    if (target == -1) return false;
    if (source == target) {
        cout << "  Source and destination are the same.\n";
        return false;
    }
    hour = readHour();
    return hour != -1;
}

void addToHistory(const Route& r, int hour) {
    string entry = stations[r.path.front()].name + " -> " + stations[r.path.back()].name +
                   " at " + formatHour(hour) + " (" + r.label + ", " + formatMinutes(r.minutes) +
                   ", Rs. " + toStr(r.fare) + ")";
    history.push(entry);
    if ((int)history.size() > HISTORY_SIZE) history.pop(); // drop the oldest
}

// ----------------------------- Menu options ----------------------------------

bool sameRoute(const Route& a, const Route& b) {
    return a.path == b.path && a.hopLine == b.hopLine;
}

bool byScore(const Route& a, const Route& b) { return a.score < b.score; }

// 1. Run every search, compare the routes and recommend the best one.
void planJourney() {
    int source, target, hour;
    if (!readJourney(source, target, hour)) return;

    vector<Route> routes;
    for (int c = FASTEST; c <= FEWEST_STOPS; c++) {
        Route r = findRoute(source, target, (Criterion)c, hour);
        if (r.path.empty()) continue;
        bool duplicate = false;
        for (size_t i = 0; i < routes.size(); i++) {
            if (sameRoute(routes[i], r)) {
                routes[i].label += ", " + r.label; // same path found by another search
                duplicate = true;
            }
        }
        if (!duplicate) routes.push_back(r);
    }
    if (routes.empty()) {
        cout << "  No route found.\n";
        return;
    }

    // Score in "equivalent minutes" (lower is better): each line change counts
    // as 5 extra minutes, each rupee as 0.2 minutes, each % of crowd as 0.1 minutes.
    for (size_t i = 0; i < routes.size(); i++) {
        Route& r = routes[i];
        r.score = r.minutes + 5 * r.changes + 0.2 * r.fare + 0.1 * r.crowd;
    }
    sort(routes.begin(), routes.end(), byScore);

    cout << "\nFound " << routes.size() << " different route(s):\n";
    cout << "  #  Time        Km     Stops  Changes  Fare    Crowd  Found by\n";
    for (size_t i = 0; i < routes.size(); i++) {
        const Route& r = routes[i];
        cout << "  " << left << setw(3) << i + 1 << setw(12) << formatMinutes(r.minutes)
             << setw(7) << toStr(r.km, 1) << setw(7) << r.path.size() - 1 << setw(9) << r.changes
             << setw(8) << ("Rs." + toStr(r.fare)) << setw(7) << (toStr(r.crowd) + "%") << r.label
             << right << "\n";
    }

    cout << "\nRecommended route (#1, best balance of time, changes, fare and crowd):";
    printDirections(routes[0]);
    addToHistory(routes[0], hour);

    while (routes.size() > 1) {
        int choice = readNumber("\nEnter a route number to see its directions (0 to go back): ", 0,
                                (int)routes.size());
        if (choice <= 0) break;
        printDirections(routes[choice - 1]);
    }
}

// 2. Find a route using one chosen criterion.
void routeByCriterion() {
    cout << "\nChoose what to optimise:\n";
    for (int c = FASTEST; c <= FEWEST_STOPS; c++) cout << "  " << c + 1 << ". " << CRITERION_NAMES[c] << "\n";
    int choice = readNumber("Enter choice (1-6): ", 1, 6);
    if (choice == -1) return;
    int source, target, hour;
    if (!readJourney(source, target, hour)) return;
    Route r = findRoute(source, target, (Criterion)(choice - 1), hour);
    if (r.path.empty()) {
        cout << "  No route found.\n";
        return;
    }
    cout << "\n" << r.label << " route:";
    printDirections(r);
    addToHistory(r, hour);
}

// 3. Search stations by any part of their name (Trie).
void searchStations() {
    string text;
    if (!readLine("Type part of a station name: ", text)) return;
    vector<int> matches = trieSearch(text);
    if (matches.empty()) {
        cout << "  No stations found.\n";
        return;
    }
    cout << "  " << matches.size() << " station(s) found:\n";
    for (size_t i = 0; i < matches.size(); i++) {
        cout << "    " << stations[matches[i]].name << "  [";
        printLines(matches[i]);
        cout << "]\n";
    }
}

// 4. Details of one station: lines, neighbours and crowd through the day.
void stationDetails() {
    int s = readStation("Station name: ");
    if (s == -1) return;
    cout << "\n  Station: " << stations[s].name << "\n  Lines:   ";
    printLines(s);
    cout << "\n  Interchange: " << (stations[s].lines.size() > 1 ? "Yes" : "No") << "\n";
    cout << "  Next stations:\n";
    for (size_t i = 0; i < adj[s].size(); i++) {
        const Edge& e = adj[s][i];
        cout << "    " << stations[e.to].name << " (" << lines[e.line].name << ", "
             << toStr(e.km, 1) << " km)\n";
    }
    cout << "  Expected crowd during the day:\n";
    for (int h = 6; h <= 23; h++) {
        int level = crowdLevel(s, h);
        cout << "    " << formatHour(h) << "  " << setw(3) << level << "%  "
             << string(level / 5, '#') << "\n";
    }
}

// 5. List all lines and optionally the stations of one line.
void showLines() {
    cout << "\n";
    for (size_t i = 0; i < lines.size(); i++) {
        double km = 0; // add up the edges of this line only
        for (size_t j = 0; j + 1 < lines[i].stations.size(); j++) {
            const vector<Edge>& edges = adj[lines[i].stations[j]];
            for (size_t k = 0; k < edges.size(); k++)
                if (edges[k].to == lines[i].stations[j + 1] && edges[k].line == (int)i) km += edges[k].km;
        }
        cout << "  " << setw(2) << i + 1 << ". " << left << setw(20) << lines[i].name << right
             << setw(3) << lines[i].stations.size() << " stations, " << setw(5) << toStr(km, 1)
             << " km  (" << stations[lines[i].stations.front()].name << " - "
             << stations[lines[i].stations.back()].name << ")\n";
    }
    int choice = readNumber("Enter a line number to see its stations (0 to go back): ", 0, (int)lines.size());
    if (choice <= 0) return;
    int lineId = choice - 1;
    const Line& line = lines[lineId];
    cout << "\n  " << line.name << ":\n";
    for (size_t j = 0; j < line.stations.size(); j++) {
        int s = line.stations[j];
        cout << "    " << setw(2) << j + 1 << ". " << stations[s].name;
        if (stations[s].lines.size() > 1) {
            cout << "  (change: ";
            bool first = true;
            for (size_t k = 0; k < stations[s].lines.size(); k++) {
                if (stations[s].lines[k] == lineId) continue; // skip this line itself
                cout << (first ? "" : ", ") << lines[stations[s].lines[k]].name;
                first = false;
            }
            cout << ")";
        }
        cout << "\n";
    }
}

int sortHour = 0; // hour used by byCrowd
bool byCrowd(int a, int b) { return crowdLevel(a, sortHour) > crowdLevel(b, sortHour); }

// 6. The most crowded stations at a given hour (sorted).
void crowdedStations() {
    int hour = readHour();
    if (hour == -1) return;
    vector<int> ids;
    for (size_t i = 0; i < stations.size(); i++) ids.push_back((int)i);
    sortHour = hour;
    sort(ids.begin(), ids.end(), byCrowd);
    cout << "\n  Most crowded stations at " << formatHour(hour) << ":\n";
    for (int i = 0; i < 10; i++) {
        int level = crowdLevel(ids[i], hour);
        cout << "    " << setw(2) << i + 1 << ". " << left << setw(34) << stations[ids[i]].name << right
             << setw(3) << level << "%  " << crowdLabel(level) << "\n";
    }
}

// 7. Show the journey history (oldest first).
void showHistory() {
    if (history.empty()) {
        cout << "  No journeys yet.\n";
        return;
    }
    queue<string> copy = history; // copy so the real queue is not emptied
    int number = 1;
    cout << "\n  Last " << history.size() << " journey(s):\n";
    while (!copy.empty()) {
        cout << "    " << number++ << ". " << copy.front() << "\n";
        copy.pop();
    }
}

// ----------------------------- Main ------------------------------------------

int main() {
    buildNetwork();
    for (size_t i = 0; i < stations.size(); i++) trieAddStation((int)i);

    cout << "Metro Route Navigation System - Delhi NCR\n";
    cout << lines.size() << " lines, " << stations.size() << " stations loaded.\n";

    while (!inputEnded) {
        cout << "\n----------------- MENU -----------------\n"
             << "1. Plan a journey (compare all routes)\n"
             << "2. Find a route by one criterion\n"
             << "3. Search stations\n"
             << "4. Station details and crowd\n"
             << "5. Show metro lines\n"
             << "6. Most crowded stations at an hour\n"
             << "7. Journey history\n"
             << "0. Exit\n";
        int choice = readNumber("Enter choice: ", 0, 7);
        if (choice <= 0) break;
        cout << "\n";
        switch (choice) {
            case 1: planJourney(); break;
            case 2: routeByCriterion(); break;
            case 3: searchStations(); break;
            case 4: stationDetails(); break;
            case 5: showLines(); break;
            case 6: crowdedStations(); break;
            case 7: showHistory(); break;
        }
    }
    cout << "Goodbye!\n";
    return 0;
}
