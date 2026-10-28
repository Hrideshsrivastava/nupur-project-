#include "metro/MetroData.h"

namespace metro::data {

MetroGraph buildDelhiMetro() {
    MetroGraph g;

    // ── Lines: code, name, colour, average speed (km/h), premium fare ───────
    const int yellow  = g.addLine("YL",  "Yellow Line",       {255, 204,   0}, 34);
    const int blue    = g.addLine("BL",  "Blue Line",         { 30, 136, 229}, 32);
    const int blueBr  = g.addLine("BLb", "Blue Line Branch",  {100, 181, 246}, 32);
    const int red     = g.addLine("RD",  "Red Line",          {229,  57,  53}, 31);
    const int green   = g.addLine("GN",  "Green Line",        { 67, 160,  71}, 33);
    const int greenBr = g.addLine("GNb", "Green Line Branch", {129, 199, 132}, 33);
    const int violet  = g.addLine("VT",  "Violet Line",       {171,  71, 188}, 33);
    const int magenta = g.addLine("MG",  "Magenta Line",      {216,  27,  96}, 35);
    const int pink    = g.addLine("PK",  "Pink Line",         {240,  98, 146}, 34);
    const int aqua    = g.addLine("AQ",  "Aqua Line",         {  0, 188, 212}, 38);
    const int grey    = g.addLine("GY",  "Grey Line",         {158, 158, 158}, 30);
    const int airport = g.addLine("AE",  "Airport Express",   {255, 143,   0}, 55, true);

    // ── Stations in running order: {name, km from previous station} ─────────
    g.buildLine(yellow, {
        {"Samaypur Badli", 0}, {"Rohini Sector 18-19", 0.8}, {"Haiderpur Badli Mor", 1.3},
        {"Jahangirpuri", 1.3}, {"Adarsh Nagar", 1.4}, {"Azadpur", 1.5}, {"Model Town", 1.4},
        {"GTB Nagar", 1.4}, {"Vishwavidyalaya", 0.8}, {"Vidhan Sabha", 1.0},
        {"Civil Lines", 1.3}, {"Kashmere Gate", 1.1}, {"Chandni Chowk", 1.1},
        {"Chawri Bazar", 1.0}, {"New Delhi", 0.8}, {"Rajiv Chowk", 1.1}, {"Patel Chowk", 1.3},
        {"Central Secretariat", 0.9}, {"Udyog Bhawan", 0.3}, {"Lok Kalyan Marg", 1.6},
        {"Jor Bagh", 1.2}, {"Dilli Haat - INA", 1.3}, {"AIIMS", 0.8}, {"Green Park", 1.0},
        {"Hauz Khas", 1.8}, {"Malviya Nagar", 1.7}, {"Saket", 0.9}, {"Qutab Minar", 1.7},
        {"Chhattarpur", 1.3}, {"Sultanpur", 1.6}, {"Ghitorni", 1.3}, {"Arjan Garh", 2.7},
        {"Guru Dronacharya", 2.3}, {"Sikanderpur", 1.0}, {"MG Road", 1.2},
        {"IFFCO Chowk", 1.1}, {"Millennium City Centre Gurugram", 1.5},
    });

    g.buildLine(blue, {
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

    g.buildLine(blueBr, {
        {"Yamuna Bank", 0}, {"Laxmi Nagar", 1.3}, {"Nirman Vihar", 1.1}, {"Preet Vihar", 1.1},
        {"Karkarduma", 1.1}, {"Anand Vihar ISBT", 1.6}, {"Kaushambi", 0.8}, {"Vaishali", 1.6},
    });

    g.buildLine(red, {
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

    g.buildLine(green, {
        {"Inderlok", 0}, {"Ashok Park Main", 1.4}, {"Punjabi Bagh", 0.9}, {"Shivaji Park", 1.6},
        {"Madipur", 1.1}, {"Paschim Vihar East", 1.0}, {"Paschim Vihar West", 0.7},
        {"Peeragarhi", 1.0}, {"Udyog Nagar", 1.2}, {"Maharaja Surajmal Stadium", 1.0},
        {"Nangloi", 1.1}, {"Nangloi Railway Station", 1.0}, {"Rajdhani Park", 1.0},
        {"Mundka", 1.0},
    });

    g.buildLine(greenBr, {
        {"Kirti Nagar", 0}, {"Satguru Ram Singh Marg", 1.1}, {"Ashok Park Main", 1.4},
    });

    g.buildLine(violet, {
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

    g.buildLine(magenta, {
        {"Janakpuri West", 0}, {"Dabri Mor - Janakpuri South", 1.3}, {"Dashrath Puri", 1.0},
        {"Palam", 1.4}, {"Sadar Bazaar Cantonment", 2.5}, {"Terminal 1 IGI Airport", 2.8},
        {"Shankar Vihar", 2.4}, {"Vasant Vihar", 2.4}, {"Munirka", 1.0}, {"R.K. Puram", 1.1},
        {"IIT Delhi", 1.5}, {"Hauz Khas", 1.1}, {"Panchsheel Park", 1.2}, {"Chirag Delhi", 1.3},
        {"Greater Kailash", 1.1}, {"Nehru Enclave", 1.1}, {"Kalkaji Mandir", 1.0},
        {"Okhla NSIC", 1.1}, {"Sukhdev Vihar", 1.5}, {"Jamia Millia Islamia", 1.0},
        {"Okhla Vihar", 1.0}, {"Jasola Vihar Shaheen Bagh", 1.5}, {"Kalindi Kunj", 1.8},
        {"Okhla Bird Sanctuary", 1.2}, {"Botanical Garden", 2.4},
    });

    g.buildLine(pink, {
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

    g.buildLine(aqua, {
        {"Noida Sector 51", 0}, {"Noida Sector 50", 1.3}, {"Noida Sector 76", 1.1},
        {"Noida Sector 101", 1.2}, {"Noida Sector 81", 1.3}, {"NSEZ", 1.1},
        {"Noida Sector 83", 1.2}, {"Noida Sector 137", 1.6}, {"Noida Sector 142", 1.7},
        {"Noida Sector 143", 1.2}, {"Noida Sector 144", 1.1}, {"Noida Sector 145", 1.2},
        {"Noida Sector 146", 1.3}, {"Noida Sector 147", 1.4}, {"Noida Sector 148", 1.2},
        {"Knowledge Park II", 2.5}, {"Pari Chowk", 1.9}, {"Alpha 1", 1.2}, {"Delta 1", 1.3},
        {"GNIDA Office", 1.5}, {"Depot Station", 1.6},
    });

    g.buildLine(grey, {
        {"Dwarka", 0}, {"Nangli", 1.6}, {"Najafgarh", 1.6}, {"Dhansa Bus Stand", 1.1},
    });

    g.buildLine(airport, {
        {"New Delhi", 0}, {"Shivaji Stadium", 1.9}, {"Dhaula Kuan", 6.2},
        {"Delhi Aerocity", 7.8}, {"IGI Airport T3", 3.4}, {"Dwarka Sector 21", 3.6},
        {"Yashobhoomi Dwarka Sector 25", 2.0},
    });

    // ── Pedestrian skywalks between stations with different names ───────────
    g.addWalkTransfer("Noida Sector 52", "Noida Sector 51", 300, 5);
    g.addWalkTransfer("Dhaula Kuan", "Durgabai Deshmukh South Campus", 1000, 12);
    g.addWalkTransfer("Punjabi Bagh", "Punjabi Bagh West", 250, 5);

    // ── Popularity (1..10) of the busiest hubs; others get a default ────────
    const std::pair<const char*, int> hotspots[] = {
        {"Rajiv Chowk", 10}, {"Kashmere Gate", 10}, {"New Delhi", 9},
        {"Chandni Chowk", 8}, {"Hauz Khas", 8}, {"Central Secretariat", 8},
        {"Noida Sector 18", 8}, {"Botanical Garden", 8}, {"Anand Vihar ISBT", 8},
        {"Rajouri Garden", 8}, {"Lajpat Nagar", 8}, {"Millennium City Centre Gurugram", 8},
        {"Mandi House", 7}, {"Dilli Haat - INA", 7}, {"Karol Bagh", 7},
        {"Vishwavidyalaya", 7}, {"Laxmi Nagar", 7}, {"Nehru Place", 7},
        {"Janakpuri West", 7}, {"Azadpur", 7}, {"Netaji Subhash Place", 7},
        {"Noida City Centre", 7}, {"Yamuna Bank", 6}, {"Saket", 6},
        {"Noida Sector 62", 6}, {"IFFCO Chowk", 6}, {"Dwarka Sector 21", 6},
        {"IGI Airport T3", 5}, {"Welcome", 6}, {"Inderlok", 6},
        {"Mayur Vihar Phase-1", 6}, {"Karkarduma", 6}, {"Kalkaji Mandir", 6},
        {"MG Road", 6},
    };
    for (const auto& [name, footfall] : hotspots) g.setFootfall(name, footfall);
    g.finalizeFootfall();

    return g;
}

} // namespace metro::data
