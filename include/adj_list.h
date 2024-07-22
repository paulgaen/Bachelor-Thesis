#ifndef TEMPUS_ADJ_LIST_H
#define TEMPUS_ADJ_LIST_H

#include <set>
#include "libcuckoo/cuckoohash_map.hh"

typedef libcuckoo::cuckoohash_map<uint64_t, libcuckoo::cuckoohash_map<uint64_t, std::vector<uint64_t>>> NestedMap;
typedef libcuckoo::cuckoohash_map<uint64_t, std::vector<uint64_t>> Edge;

class AdjList{
public:
    void addFromFile(std::ifstream &file, int count);
    void printGraph();
    NestedMap edges;
    bool insertEdgeUndirected(uint64_t source, uint64_t destination, uint64_t time);
    void deleteEdgeUndirected(uint64_t source, uint64_t destination, uint64_t time);

private:
    //time < source < list of destinations>>
    std::set<uint64_t> uniqueTimestamps;


    bool findEdge(uint64_t source, uint64_t destination, uint64_t time);
    bool findEdge(uint64_t source, uint64_t destination, uint64_t start, uint64_t end);
    static void insertEdgeDirected(uint64_t source, uint64_t destination, uint64_t time, NestedMap &map);
    void deleteEdgeDirected(uint64_t source, uint64_t destination, uint64_t time);
    void batchOperation(bool insert, NestedMap &groupedData);
    void batchOperationParlay(bool insert, NestedMap &groupedData, std::unordered_map<uint64_t, uint64_t> uniqueTimesMap);
    static void sortBatch(const std::vector<uint64_t>& sourceAdds, const std::vector<uint64_t>& destinationAdds,
                          const std::vector<uint64_t>& timeAdds, NestedMap &groupedData);
    static void printGroupedData(NestedMap &groupedData);
    void rangeQuery(uint64_t start, uint64_t end, void (*func)(uint64_t,uint64_t,uint64_t));
    void memoryConsumption (NestedMap &map);
    static void fileReaderHelper(std::vector<uint64_t> &sourceVector, std::vector<uint64_t> &destinationVector,
                          std::vector<uint64_t> &timeVector, std::set<uint64_t> &uniqueTimes, uint64_t source,
                          uint64_t destination, uint64_t time);
    void uniqueTimesHelper(std::unordered_map<uint64_t, uint64_t> &uniqueTimesMap, std::set<uint64_t> &uniqueTimes, bool insert);
    std::unordered_map<uint64_t, uint64_t> genUniqueTimeMap(uint64_t start, uint64_t end);
};

#endif //TEMPUS_ADJ_LIST_H
