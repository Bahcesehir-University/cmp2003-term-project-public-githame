// This is our main place, like the engine room, here we take data from CSV and rank it.

#include "analyzer.h"
#include <fstream>
#include <sstream>
#include <algorithm>
using namespace std;

// HELPERS AND PARSING FUNCTIONS

// Split a line by comma, trim whitespace from each field.
// splitLine(): takes one CSV line and splits it by commas.
static vector<string> splitLine(const string &line)
{
    vector<string> fields;
    stringstream ss(line);
    string field;

    while (getline(ss, field, ','))
    {
        size_t start = field.find_first_not_of(" \t\r\n");
        size_t end = field.find_last_not_of(" \t\r\n");

        if (start != string::npos)
        {
            fields.push_back(field.substr(start, end - start + 1));
        }
        else
        {
            fields.push_back("");
        }
    }
    return fields;
}

// Check if string contains only digits (for header detection).
// isAllDigits(): ONLY used to identify headers.
// We treat the first column as a header and omit it if it isn't numeric.
static bool isAllDigits(const string &s)
{
    if (s.empty())
        return false;
    for (char c : s)
    {
        if (!isdigit(static_cast<unsigned char>(c)))
        {
            return false;
        }
    }
    return true;
}

// Digs into the "YYYY-MM-DD HH:MM" string to find JUST the hour.
static int parseHour(const string &datetime)
{
    size_t spacePos = datetime.find(' ');
    if (spacePos == string::npos)
        return -1;

    size_t colonPos = datetime.find(':', spacePos);
    if (colonPos == string::npos)
        return -1;

    string hourStr = datetime.substr(spacePos + 1, colonPos - spacePos - 1);

    size_t start = hourStr.find_first_not_of(" \t");
    if (start != string::npos)
    {
        hourStr = hourStr.substr(start);
    }

    if (hourStr.empty())
        return -1;

    try
    {
        int hour = stoi(hourStr);
        if (hour < 0 || hour > 23)
            return -1;
        return hour;
    }
    catch (...)
    {
        return -1;
    }
}

// TRIP ANALYZER PART

void TripAnalyzer::ingestFile(const string &csvPath)
{
    ifstream file(csvPath);
    if (!file.is_open())
        return;

    string line;
    bool firstLine = true;

    while (getline(file, line))
    {
        // Skip empty lines (no characters at all)
        if (line.empty())
            continue;

        // Don't bother with lines that are just empty or full of spaces/tabs.
        // We check find_first_not_of for any actual characters, if none, skip the line.
        // Then use find_last_not_of to find where the real content ends.
        size_t firstChar = line.find_first_not_of(" \t\r\n");
        size_t lastChar = line.find_last_not_of(" \t\r\n");
        if (firstChar == string::npos)
            continue;

        // Trim the line (that's for cleaning aka cleaner parsing)
        line = line.substr(firstChar, lastChar - firstChar + 1);

        // Skip the very first line if it contains TripID (the headers of it).
        if (firstLine)
        {
            firstLine = false;

            if (line.find("TripID") != string::npos)
            {
                continue;
            }

            vector<string> fields = splitLine(line);
            if (fields.empty() || !isAllDigits(fields[0]))
            {
                continue;
            }
        }

        // Pull the data we need out of the line.
        vector<string> fields = splitLine(line);

        // Validate that every row has exactly 6 columns the FULL.
        // Schema (by index):
        // [0] TripID
        // [1] PickupZoneID          we use this
        // [2] DropoffZoneID
        // [3] PickupDateTime        we use this
        // [4] DistanceKm
        // [5] FareAmount
        //
        // Why require all 6 when we only need columns 1 and 3 for our analysis?
        // A row with fewer than 6 fields is considered malformed or incomplete,
        // even if the fields we care about are present.
        if (fields.size() < 6)
            continue;

        string zone = fields[1];
        if (zone.empty())
            continue;

        int hour = parseHour(fields[3]);
        if (hour < 0)
            continue;

        // tally things up:
        zoneCount[zone]++;         // This zone just got another trip.
        slotCount[{zone, hour}]++; // This specific zone at this specific hour just got another trip to tally.
    }

    file.close();
}

// topZones() pulls the counts out of the hash map into a vector and sorts them.
// exactly how the spec wants by including all the tie-breaker rules.
// Same data always gives the same ordered list.
std::vector<ZoneCount> TripAnalyzer::topZones(int k) const
{
    vector<ZoneCount> result;
    result.reserve(zoneCount.size());

    // Dump the map data into a list, flat structure so we can sort it.
    for (const auto &pair : zoneCount)
    {
        result.push_back({pair.first, pair.second});
    }

    // For Tie breakers
    // 1The higher count wins 2If counts are equal, the lexicographically smaller zone get priority to come first.
    sort(result.begin(), result.end(),
         [](const ZoneCount &a, const ZoneCount &b)
         {
             if (a.count != b.count)
                 return a.count > b.count;
             return a.zone < b.zone;
         });

    // Keep only the top K results (or all of them if fewer exist yani).
    if ((int)result.size() > k)
    {
        result.resize(k);
    }

    return result;
}

// K is being returned busiest time slots.
// topBusySlots: same idea as topZones(), but for (zone, hour) slots.
std::vector<SlotCount> TripAnalyzer::topBusySlots(int k) const
{
    vector<SlotCount> result;
    result.reserve(slotCount.size());

    // Convert the (zone, hour) counter map into a vector so we can sort by the required rules later.
    for (const auto &p : slotCount)
    {
        result.push_back({p.first.first, p.first.second, p.second});
    }

    // this is a tie breaker for sloting first, then Zone Name, then Hour.
    // Sort order (deterministic):
    // - Primary: count descending (more busy = higher)
    // - Tie-breaker 1: zone name ascending (lexicographical)
    // - Tie-breaker 2: hour ascending (earlier times first)
    sort(result.begin(), result.end(),
         [](const SlotCount &a, const SlotCount &b)
         {
             if (a.count != b.count)
                 return a.count > b.count;
             if (a.zone != b.zone)
                 return a.zone < b.zone;
             return a.hour < b.hour;
         });

    if ((int)result.size() > k)
    {
        result.resize(k);
    }

    return result;
}
