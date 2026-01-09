// The "real work" parsing, counting, and generating deterministic top-K outputs—is contained  IN THIS FILE.

#include "analyzer.h"
#include <fstream> // We read the CSV from disk (autograder calls ingestFile with a path), so we need file streams.
#include <sstream>
#include <algorithm>

using namespace std;

// HELPER FUNCTIONS.
// Split a line by comma, trim whitespace from each field.
// Since they are designated as static, these helpers are limited to this file.
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
    return fields; // // Returns the vector of fields (may include empty strings if the row is dirty...)
}

// Check if string contains only digits (for the header detection logic ingestFile function).
// isAllDigits(): ONLY used to identify headers.
// We treat the first column as a header and omit it if if it isn't numeric.
static bool isAllDigits(const string &s)
{
    if (s.empty())
        return false;
    for (char c : s)
    {
        if (!isdigit(static_cast<unsigned char>(c))) // We cast to unsigned char here to avoid undefined or implementation-defined behavior
        {
            return false;
        }
    }
    return true;
}

// Extract hour (0-23) in this format "YYYY-MM-DD HH:MM" -> 0-23
static int parseHour(const string &datetime)
{
    size_t spacePos = datetime.find(' '); // Find the space between date and time. If missing the format is wrong.

    if (spacePos == string::npos) // Search for the ':' that separates hours from minutes in the time string.
                                  // If it's missing, the time format is broken we can't reliably read the hour.
                                  // (or the rest of the time), so we'll bail out early lol.
        return -1;

    size_t colonPos = datetime.find(':', spacePos); // Find ':' in time section
                                                    // If Missing...can't safely parse hour...invalid format
    if (colonPos == string::npos)
        return -1;

    // Take the substring up to ':' — this should be the hour ("23" from "23:59").
    string hourStr = datetime.substr(spacePos + 1, colonPos - spacePos - 1);

    size_t start = hourStr.find_first_not_of(" \t");
    if (start != string::npos)
    {
        hourStr = hourStr.substr(start);
    }

    if (hourStr.empty())
        return -1;

    try
    { // stoi can throw if the hour isn't a real number so we catch and treat it as invalid data.

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
//...................................TRIPANALYZER................................
// TripAnalyzer Implementation for ingestFile, topZones, topBusySlots...
// ingestFile() reads the file once and builds all necessary counts efficiently.
// topZones() and topBusySlots() turn those counts into deterministic top-K results
// (same input...same output order every fuckin time).

// The thing here is not not crash on a bad input, so we just skip the rows itelf.
void TripAnalyzer::ingestFile(const string &csvPath)
{
    ifstream file(csvPath); // To open CSV file or return (no crashes) if it can't be opened.
    if (!file.is_open())
        return;

    string line;
    bool firstLine = true;

    while (getline(file, line))
    {
        if (line.empty()) // Skip empty lines (no characters at all)
            continue;

        // Don't bother with lines that are just empty or full of spaces/tabs.
        // For real lines, chop off whitespace from both ends.
        // We check find_first_not_of for any actual characters, if none, skip the line.
        // Then use find_last_not_of to find where the real content ends.
        // Catches annoying cases like "   \t\r\n" without any trouble.
        size_t firstChar = line.find_first_not_of(" \t\r\n");
        size_t lastChar = line.find_last_not_of(" \t\r\n");
        if (firstChar == string::npos)
            continue;

        // Trim the line (that's for cleaning aka cleaner parsing)
        line = line.substr(firstChar, lastChar - firstChar + 1);

        // Header detection (first NON-empty line only)
        if (firstLine)
        {
            firstLine = false;

            if (line.find("TripID") != string::npos)
            {
                continue;
            }

            vector<string> fields = splitLine(line); /// Backup header check.
            if (fields.empty() || !isAllDigits(fields[0]))
            {
                continue;
            }
        }

        // Examine the line with basic if checks
        vector<string> fields = splitLine(line);

        // Validate that every row has exactly 6 columnsthe FULL.
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

        // Count THIS trip (for both ID and slots)
        zoneCount[zone]++;
        slotCount[{zone, hour}]++;
    }

    file.close(); // Done reading the file.
}

// (Students may use ANY data structure internall)
// I hope I am using the best ones here lolololololololololololol

// topZones() pulls the counts out of the hash map into a vector and sorts them.
// exactly how the spec wants by including all the tie-breaker rules.
// Same data always gives the same ordered list.
vector<ZoneCount> TripAnalyzer::topZones(int k) const
{
    vector<ZoneCount> result;
    result.reserve(zoneCount.size());

    for (const auto &pair : zoneCount)
    {
        result.push_back({pair.first, pair.second});
    }

    // Deterministic ordering:
    // 1) count descending
    // 2) zone ascending (lexicographic)

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

// topBusySlots fun: same idea as topZones(), but for (zone, hour) slots.
vector<SlotCount> TripAnalyzer::topBusySlots(int k) const
{
    vector<SlotCount> result;
    result.reserve(slotCount.size());

    // Convert the (zone, hour) counter map into a vector so we can sort by the required rules later.
    for (const auto &p : slotCount)
    {
        result.push_back({p.first.first, p.first.second, p.second});
    }

    // Sort order (deterministic):
    // - Primary: count descending (more busy = higher)
    // - Tie-breaker 1: zone name ascending (lexicographical)
    // - Tie-breaker 2: hour ascending (earlier times first)

    // This ensures consistent output for the same input data.
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
