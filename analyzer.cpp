#include "analyzer.h"
#include <fstream>
#include <sstream>
#include <algorithm>
using namespace std;

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
            fields.push_back(field.substr(start, end - start + 1));
        else
            fields.push_back("");
    }
    return fields;
}

static bool isAllDigits(const string &s)
{
    if (s.empty())
        return false;
    for (char c : s)
    {
        if (!isdigit(static_cast<unsigned char>(c)))
            return false;
    }
    return true;
}

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
        hourStr = hourStr.substr(start);

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

void TripAnalyzer::ingestFile(const string &csvPath)
{
    ifstream file(csvPath);
    if (!file.is_open())
        return;

    string line;
    bool firstLine = true;

    while (getline(file, line))
    {
        if (line.empty())
            continue;

        size_t firstChar = line.find_first_not_of(" \t\r\n");
        size_t lastChar = line.find_last_not_of(" \t\r\n");
        if (firstChar == string::npos)
            continue;

        line = line.substr(firstChar, lastChar - firstChar + 1);

        if (firstLine)
        {
            firstLine = false;

            if (line.find("TripID") != string::npos)
                continue;

            vector<string> fields = splitLine(line);
            if (fields.empty() || !isAllDigits(fields[0]))
                continue;
        }

        vector<string> fields = splitLine(line);

        if (fields.size() < 6)
            continue;

        string zone = fields[1];
        if (zone.empty())
            continue;

        int hour = parseHour(fields[3]);
        if (hour < 0)
            continue;

        zoneCount[zone]++;
        slotCount[{zone, hour}]++;
    }

    file.close();
}

vector<ZoneCount> TripAnalyzer::topZones(int k) const
{
    vector<ZoneCount> result;
    result.reserve(zoneCount.size());

    for (const auto &p : zoneCount)
        result.push_back({p.first, p.second});

    sort(result.begin(), result.end(),
         [](const ZoneCount &a, const ZoneCount &b)
         {
             if (a.count != b.count)
                 return a.count > b.count;
             return a.zone < b.zone;
         });

    if ((int)result.size() > k)
        result.resize(k);

    return result;
}

vector<SlotCount> TripAnalyzer::topBusySlots(int k) const
{
    vector<SlotCount> result;
    result.reserve(slotCount.size());

    for (const auto &p : slotCount)
        result.push_back({p.first.first, p.first.second, p.second});

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
        result.resize(k);

    return result;
}
