#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <map>

struct ZoneCount
{
    std::string zone;
    long long count;
};

struct SlotCount
{
    std::string zone;
    int hour;
    long long count;
};

class TripAnalyzer
{
public:
    void ingestFile(const std::string &csvPath);
    std::vector<ZoneCount> topZones(int k = 10) const;
    std::vector<SlotCount> topBusySlots(int k = 10) const;

private:
    std::unordered_map<std::string, long long> zoneCount;
    std::map<std::pair<std::string, int>, long long> slotCount;
};
