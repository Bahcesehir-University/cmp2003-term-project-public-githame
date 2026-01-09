// this is a standard library headers required for structuring the TripAnalyzer class,
// The TripAnalyzer class and the fundamental data structures are defined in this header.
// It serves as the agreement between the autograder and our implementation.
// Nothing here should be dependent on the specifics of the input or output formats.
// Only STL components allowed by course constraints are used.
// Public interface that the autograder expects.

#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <map>

// Total number of trips for a single pickup zone (PickupZoneID).
// Holds the total number of trips for a single pickup zone.
// just data with no logeic and topZones() uses this struct as its return type.
// Maps each pickup zone to its total number of trips.
// Used for fast aggregation of zone-level statistics.
struct ZoneCount
{
    std::string zone;
    long long count;
};

// Total number of trips for a (zone, hour) slot.
// Shows the level of activity in a particular pickup zone at a given time by Combines zone + hour + trip count.
struct SlotCount
{
    std::string zone;
    int hour;
    long long count;
};

// This is the main analyzer class for trip data, it reads the CSV, aggregates counts, returns top-k results.
class TripAnalyzer
{
public:
    // Reads a CSV file from disk and updates internal counters.
    // Must be robust: skip malformed rows and never crash.
    void ingestFile(const std::string &csvPath);

    // Top K zones sorted by:
    // 1count descending 2zone ascending.
    std::vector<ZoneCount> topZones(int k = 10) const;

    // Top K (zone, hour) slots sorted by:
    // 1count descending, 2zone ascending, 3hour ascending.
    std::vector<SlotCount> topBusySlots(int k = 10) const;

private:
    // zone to total trips count like an ID
    std::unordered_map<std::string, long long> zoneCount;

    // (zone, hour) to trips count (Zone ID + hour slot)
    // Using map with pair as key for simplicity (no custom hash needed)
    std::map<std::pair<std::string, int>, long long> slotCount;
};
