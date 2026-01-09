// this is a standard library headers required for structuring the TripAnalyzer class,
// The TripAnalyzer class and the fundamental data structures are defined in this header.
// It serves as the agreement between the autograder and our implementation.
// Nothing here should be dependent on the specifics of the input or output formats.
// Only STL components allowed by course constraints are used.

#pragma once
#include <string>
#include <vector>
#include <unordered_map> // hash tables for fast average O(1) operations
#include <map>

// Holds the total number of trips for a single pickup zone.
// just data with no logeic and topZones() uses this struct as its return type.
// Maps each pickup zone to its total number of trips.
// Used for fast aggregation of zone-level statistics.
struct ZoneCount
{
    std::string zone;
    long long count;
};

// Shows the level of activity in a particular pickup zone at a given time by Combines zone + hour + trip count.
struct SlotCount
{
    std::string zone;
    int hour; // 0–23
    long long count;
};

// this for main analyzer class for trip data analysis.
class TripAnalyzer
{
public:
    // Parse Trips.csv, skip the dirty rows and for that it will not crash
    void ingestFile(const std::string &csvPath);

    // Top K zones: count desc, zone asc
    std::vector<ZoneCount> topZones(int k = 10) const;

    // Top K slots: count desc, zone asc, hour asc
    std::vector<SlotCount> topBusySlots(int k = 10) const;

private:
    // Zone -> total trip count like an ID
    std::unordered_map<std::string, long long> zoneCount;

    // (Zone, Hour) -> trip count (Zone ID + hour slot)
    // Using map with pair as key for simplicity
    std::map<std::pair<std::string, int>, long long> slotCount;
};
