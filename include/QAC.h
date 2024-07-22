//
// Created by paulg on 15.06.2024.
//
#include <fstream>
#include <map>
#include "LouvainAlgorithm/src/Graph.h"

#ifndef TEMPUS_QAC_H
#define TEMPUS_QAC_H
void addCommunity(Graph& g, uint64_t source, uint64_t destination, std::map<uint64_t, uint64_t> mapping, std::map<uint64_t, uint64_t> revMapping);
void deleteCommunity(Graph& g, uint64_t source, uint64_t destination, std::map<uint64_t, uint64_t> mapping, std::map<uint64_t, uint64_t> revMapping);
void addMapping(uint64_t source, std::map<uint64_t, uint64_t>& mapping, std::map<uint64_t, uint64_t>& revMapping, int& i);
void printModularity(Graph& g, int i);
void copyCommunities(Graph& g, Graph& help);
vector<EdgeWeight> calcTot(Graph g);
vector<EdgeWeight> calcIn(Graph g);
#endif //TEMPUS_QAC_H
