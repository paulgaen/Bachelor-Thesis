#include "adj_list.h"
#include "parlay/parallel.h"

#include <fstream>
#include <cinttypes>


/**
 * Checks if the given edge exists in the graph.
 * @param source node of the edge
 * @param destination node of the edge
 * @param time timestamp of the edge
 * @return true if the edge was found
 */
bool AdjList::findEdge(uint64_t source, uint64_t destination, uint64_t time){
    bool flag = false;
    edges.find_fn(time,
                  [&destination, &flag, &source](Edge &e) {
                      e.find_fn(source,
                                [&flag, &destination](std::vector<uint64_t> &d) {
                                    flag = (std::find(d.begin(), d.end(), destination) != d.end());
                                });
                  });
    return flag;
}

//has a race condition issue when insertions/deletions are occurring
/**
 * Checks if the an edge between @p source and @p destination exists in the given range of timestamps.
 * @param source node of the edge
 * @param destination node of the edge
 * @param start of the range inclusive
 * @param end end of the range exclusive
 * @return true if at least one edge was found
 * @overload
 */
bool AdjList::findEdge(uint64_t source, uint64_t destination, uint64_t start, uint64_t end){
    bool flag = false;
    auto uniqueTimesMap = genUniqueTimeMap(start, end);
    for (auto time : uniqueTimesMap) {
        flag = findEdge(source, destination, time.second);
        if (flag) break;
    }
    return flag;
}

/**
 * Inserts an edge into the given nested cuckoo map @p map.
 * @param source node of the edge
 * @param destination node of the edge
 * @param time timestamp of the edge
 * @param map
 */
void AdjList::insertEdgeDirected(uint64_t source, uint64_t destination, uint64_t time, NestedMap &map) {
    if (map.contains(time)) {
        map.update_fn(time,
                      [&source, &destination](Edge &e) {
                          e.upsert(source,
                                   [&destination](std::vector<uint64_t> &d) { d.push_back(destination); },
                                   std::vector<uint64_t>{destination});
                      });
    } else {
        Edge e;
        e.insert(source, std::vector<uint64_t>{destination});
        map.insert(time, e);
    }
}

/**
 * Checks if the given edge to be inserted is already in the graph. If not, calls insertEdgeDirected twice to insert the
 * the edge in both directions (@p source -> @p destination and @p destination -> @p source).
 * @see AdjList::insertEdgeDirected
 * @param source node of the edge
 * @param destination node of the edge
 * @param time timestamp of the edge
 */
bool AdjList::insertEdgeUndirected(uint64_t source, uint64_t destination, uint64_t time) {
    //filters out duplicates
    if (findEdge(source, destination, time)) return false;

    //insert edges from source
    insertEdgeDirected(source, destination, time, edges);
    //insert edges from destination
    insertEdgeDirected(destination, source, time, edges);
    return true;
}

/**
 * Deletes the given edge from the graph. Functions similar to insertEdgeDirected.
 * Also removes keys if their values are empty.
 * @param source node of the edge
 * @param destination node of the edge
 * @param time timestamp of the edge
 */
void AdjList::deleteEdgeDirected(uint64_t source, uint64_t destination, uint64_t time) {
    bool isDestinationEmpty = false;
    bool isEdgeEmpty = false;

    if (edges.contains(time)) {

        edges.update_fn(time, [&isDestinationEmpty, &source, &destination](Edge &e) {
            e.update_fn(source, [&isDestinationEmpty, &destination](std::vector<uint64_t> &d) {
                d.erase(std::find(d.begin(), d.end(), destination));
                if (d.empty()) isDestinationEmpty = true;
            });
        });
        //delete source node if it has no edges (destinations)
        if (isDestinationEmpty) {
            edges.find_fn(time, [&isEdgeEmpty, &source](Edge &e) {
                e.erase(source);
                //causes performance issues
                if (e.empty()) isEdgeEmpty = true;
            });
        }
        //delete timestamp if edges is empty
        if (isEdgeEmpty) {
            edges.erase(time);
            uniqueTimestamps.erase(time);
        }
    }
}

/**
 * Works similar to insertEdgeUndirected.
 * @param source node of the edge
 * @param destination node of the edge
 * @param time timestamp of the edge
 */
void AdjList::deleteEdgeUndirected(uint64_t source, uint64_t destination, uint64_t time) {
    //check if edge to be deleted exists
    if (!findEdge(source, destination, time)) return;

    //delete edges from source
    deleteEdgeDirected(source, destination, time);
    //delete edges from destination
    deleteEdgeDirected(destination, source, time);
}

/**
 * Prints all timestamps and the edge contained in them.
 */
void AdjList::printGraph() {
    std::cout << "Printing all edges:" << std::endl << std::endl;
    uint64_t count = 0;

    auto lt = edges.lock_table();

    for (const auto &innerTbl: lt) {
        Edge edgeData = innerTbl.second;
        auto lt2 = edgeData.lock_table();
        printf("Time %" PRIu64 " contains edges\n", innerTbl.first);

        for (const auto &vector: lt2) {
            for (auto &edge: vector.second) {
                printf("    - between: %" PRIu64 " and %" PRIu64 " at time %" PRIu64 "\n", vector.first, edge, innerTbl.first);
                count++;
            }
        }
        std::cout << std::endl;
    }
    std::cout << "Total number of edges: " << count << std::endl;
}


/**
 * Helper function to organize the read data.
 * @param sourceVector container for all read sources with the same command.
 * @param destinationVector container for all read destinations with the same command.
 * @param timeVector container for all read timestamps with the same command.
 * @param uniqueTimes container for all read unique timestamps with the same command.
 * @param source value to be inserted
 * @param destination value to be inserted
 * @param time value to be inserted
 */
void AdjList::fileReaderHelper(std::vector<uint64_t> &sourceVector, std::vector<uint64_t> &destinationVector,
                               std::vector<uint64_t> &timeVector, std::set<uint64_t> &uniqueTimes,
                               uint64_t source, uint64_t destination, uint64_t time){
    sourceVector.push_back(source);
    destinationVector.push_back(destination);
    timeVector.push_back(time);
    uniqueTimes.insert(time);
}

/**
 * Helper function to organise unique timestamps. Inserts or removes them from uniqueTimestamps.
 * @param uniqueTimesMap container for unique times mapped to a key that iterates up from 0
 * @param uniqueTimes values to be inserted
 * @param insert determines whether to insert or delete from uniqueTimestamps
 */
void AdjList::uniqueTimesHelper(std::unordered_map<uint64_t, uint64_t> &uniqueTimesMap, std::set<uint64_t> &uniqueTimes, bool insert){
    int i = 0;
    for (const uint64_t &time: uniqueTimes) {
        uniqueTimesMap[i] = time;
        if (insert) uniqueTimestamps.insert(time);
        i++;
    }
}


/**
 * Reads and extracts data from the file and calls functions to use the data on the graph.
 * @param path input file
 */
void AdjList::addFromFile(std::ifstream &file, int count) {
    if (file.is_open()) {
        std::string command;
        uint64_t source, destination, time;
        std::vector<uint64_t> sourceAdds{}, destinationAdds{}, timeAdds{};
        std::vector<uint64_t> sourceDels{}, destinationDels{}, timeDels{};
        std::set<uint64_t> uniqueTimesAdd, uniqueTimesDel;
        std::unordered_map<uint64_t, uint64_t> uniqueTimesAddMap, uniqueTimesDelMap;
        int i = 0;
        while (file >> command >> source >> destination >> time) {
            if(i >= count){ break;}
            if (command == "add") {
                fileReaderHelper(sourceAdds, destinationAdds, timeAdds, uniqueTimesAdd, source, destination, time);
            }
            if (command == "delete") {
                fileReaderHelper(sourceDels, destinationDels, timeDels, uniqueTimesDel, source, destination, time);
            }
            i++;
            if(file.eof()){
                break;
            }
        }

        uniqueTimesHelper(uniqueTimesAddMap, uniqueTimesAdd, true);
        uniqueTimesHelper(uniqueTimesDelMap, uniqueTimesDel, false);


        //Create new hash map, keys are source vertices and values are vectors of integer pairs (destination, time).
        //This is then filled by sortBatch function.
        libcuckoo::cuckoohash_map<uint64_t, Edge> groupedDataAdds, groupedDataDels;

        sortBatch(sourceAdds, destinationAdds, timeAdds, groupedDataAdds);
        batchOperationParlay(true, groupedDataAdds, uniqueTimesAddMap);

        sortBatch(sourceDels, destinationDels, timeDels, groupedDataDels);
        batchOperationParlay(false, groupedDataDels, uniqueTimesDelMap);

    }
}

/**
 * Iterated through @p groupedData and calls insertEdgeUndirected or deleteEdgeUndirected accordingly.
 * @param insert dictates whether to insert or delete the given data
 * @param groupedData Nested map of edges that are to be inserted/deleted
 */
void AdjList::batchOperation(bool insert, NestedMap &groupedData) {
    auto t1 = std::chrono::high_resolution_clock::now();
    auto lt = groupedData.lock_table();

    for (const auto &innerTbl: lt) {
        Edge edgeData = innerTbl.second;
        auto lt2 = edgeData.lock_table();

        for (const auto &vector: lt2) {
            for (auto &edge: vector.second) {
                if (insert) insertEdgeUndirected(vector.first, edge, innerTbl.first);
                else deleteEdgeUndirected(vector.first, edge, innerTbl.first);
            }
        }
    }
    auto t2 = std::chrono::high_resolution_clock::now();
    auto ms_int = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
    std::cout << "batchOperationCuckoo has taken " << ms_int.count() << "ms\n";
}

//doesn't terminate properly
/**
 * Works similar to batchOperation. Iterates in parallel.
 * @param insert dictates whether to insert or delete the given data
 * @param groupedData Nested cuckoo map of edges that are to be inserted/deleted
 * @param uniqueTimesMap helper parameter to iterate through @p groupedData
 */
void AdjList::batchOperationParlay(bool insert, NestedMap &groupedData, std::unordered_map<uint64_t, uint64_t> uniqueTimesMap) {
    auto t1 = std::chrono::high_resolution_clock::now();
    auto lt = groupedData.lock_table();

    parlay::parallel_for(0, uniqueTimesMap.size(), [&](uint64_t i) {
        uint64_t time = uniqueTimesMap[i];

        Edge innerTbl = lt.find(time)->second;
        auto lt2 = innerTbl.lock_table();

        for (const auto &vector: lt2) {
            for (auto &edge: vector.second) {
                if (insert) insertEdgeUndirected(vector.first, edge, time);
                else deleteEdgeUndirected(vector.first, edge, time);
            }
        }
    });
    auto t2 = std::chrono::high_resolution_clock::now();
    auto ms_int = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
    std::cout << "addBatchCuckooParlay has taken " << ms_int.count() << "ms\n";
}

/**
 * Organises the read data and calls insertEdgeDirected to insert them into @p groupedData.
 * @param sourceAdds list of source nodes
 * @param destinationAdds list of destination nodes
 * @param timeAdds list of timestamps
 * @param groupedData Nested cuckoo map and container for the organised data
 */
void AdjList::sortBatch(const std::vector<uint64_t> &sourceAdds, const std::vector<uint64_t> &destinationAdds,
                        const std::vector<uint64_t> &timeAdds, NestedMap &groupedData) {
    auto t1 = std::chrono::high_resolution_clock::now();

    // Determine the number of iterations
    size_t numIterations = timeAdds.size();

    // Group edges by time using hash map
    for (size_t i = 0; i < numIterations; ++i) {
        uint64_t source = sourceAdds[i];
        uint64_t destination = destinationAdds[i];
        uint64_t time = timeAdds[i];

        insertEdgeDirected(source, destination, time, groupedData);
    }
    auto t2 = std::chrono::high_resolution_clock::now();
    auto ms_int = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
    std::cout << "sortBatch has taken " << ms_int.count() << "ms\n";

    //printGroupedData(groupedData);
}

/**
 * Prints the edges in @p groupedData similar to printGraph.
 * @param groupedData
 */
void AdjList::printGroupedData(libcuckoo::cuckoohash_map<uint64_t, Edge> &groupedData) {
    std::cout << "Printing grouped data:" << std::endl << std::endl;
    for (auto &it: groupedData.lock_table()) {
        uint64_t source = it.first;
        Edge &DestTime = it.second;
        auto lt2 = DestTime.lock_table();


        // Print the source and its associated edges
        printf("Time %" PRIu64 " contains %" PRIu64 " edges:\n", source, DestTime.size());
        for (const auto &edge: lt2) {
            for (auto &v: edge.second) {
                printf("    - between %" PRIu64 " and %" PRIu64 "\n", edge.first, v);
            }
        }
        std::cout << std::endl;
    }
    std::cout << "-------------------------------------" << std::endl;
}

//TODO: this is very similar to the overloaded find function, test which one is faster and/or more secure (race condition)
/**
 * Applies the given function @p func to all edges within the given range.
 * @param start of the range inclusive
 * @param end of the range exclusive
 * @param func
 */
void AdjList::rangeQuery(uint64_t start, uint64_t end, void (*func)(uint64_t, uint64_t, uint64_t)) {

    auto lt = edges.lock_table();
    auto uniqueTimesMap = genUniqueTimeMap(start, end);

    for (auto &time: uniqueTimesMap) {
        uint64_t key = lt.find(time.second)->first;
        Edge edgeData = lt.find(time.second)->second;
        auto lt2 = edgeData.lock_table();

        for (const auto &vector: lt2) {
            for (auto &edge: vector.second) {
                func(key, vector.first, edge);
            }
        }
    }
}

void AdjList::memoryConsumption(NestedMap &map) {

    auto lt = map.lock_table();
    uint64_t memory=0;

    for(auto &it:lt){
        uint64_t key = it.first;
        Edge edgeData = it.second;
        auto lt2 = edgeData.lock_table();

        for(const auto &vector: lt2){
            for(auto &edge: vector.second){
                memory+=sizeof (key) + sizeof (vector.first) + sizeof (edge);
            }
        }
    }
    std::cout << "Memory consumption in Bytes:" << memory << std::endl;
}

/**
 * Generates a map of timestamps that are in the graph and within the given range.
 * @param start of the range inclusive
 * @param end of the range exclusive
 * @return the generated map
 */
std::unordered_map<uint64_t, uint64_t> AdjList::genUniqueTimeMap(uint64_t start, uint64_t end) {
    std::unordered_map<uint64_t, uint64_t> map;
    int i = 0;
    for (auto &time : uniqueTimestamps) {
        if (start < time && time < end){
            map[i] = time;
            i++;
        }
    }
    return map;
}

