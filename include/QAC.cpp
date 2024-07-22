#include <cstdint>
#include "QAC.h"
#include "LouvainAlgorithm/src/Graph.h"

double modularityQCA(Graph& g, vector<EdgeWeight>& in, vector<EdgeWeight>& tot){ //O(n)
    double sum = 0;
    double m = g.getTotalWeight();
    for (int i = 0; i < g.getNumVertices(); i++){
        if(tot[i] > 0){
            sum += in[i] / (2 * m) - tot[i] * tot[i] / (4 * m * m);
        }
    }
    return sum;
}

void printModularity(Graph& g, int i){
    vector<EdgeWeight> tot = calcTot(g);
    vector<EdgeWeight> in = calcIn(g);
    double mod = modularityQCA(g, in, tot);
    //std::cout << "\nmodularity at timestamp "<< i <<" score: " << mod << "\n";
    printf("%f,", mod);
}

double modularityGain(Graph& g, Node i, int comm, EdgeWeight comm_weight, vector<EdgeWeight>& tot){
    double m = g.getTotalWeight();
    double n2c = comm_weight;
    double wd = g.getWeightedDegree(i);
    return (n2c - tot[comm] * wd / (2 * m)) / m;
}

void addCommunity(Graph& g, uint64_t source, uint64_t destination, std::map<uint64_t, uint64_t> mapping, std::map<uint64_t, uint64_t> revMapping){
    vector<vector<int>> community;
    vector<EdgeWeight> tot = calcTot(g);
    vector<EdgeWeight> in = calcIn(g);
    double s2d = modularityGain(g, static_cast<Node>(mapping[source]), g.getCommunity(static_cast<Node>(mapping[destination])),g.getCommunity(static_cast<Node>(mapping[destination])), tot);
    double d2s = modularityGain(g, static_cast<Node>(mapping[destination]), g.getCommunity(static_cast<Node>(mapping[source])), g.getCommunity(static_cast<Node>(mapping[source])), tot);

    if(s2d > 0 || d2s > 0){
        if (s2d > d2s){

            // check if entire community should move or only one node

            g.setCommunity(static_cast<Node>(mapping[source]), g.getCommunity(static_cast<Node>(mapping[destination])));
        }else{

            // check if entire community should move or only one node

            g.setCommunity(static_cast<Node>(mapping[destination]), g.getCommunity(static_cast<Node>(mapping[source])));
        }
    }
}

void deleteCommunity(Graph& g, uint64_t source, uint64_t destination, std::map<uint64_t, uint64_t> mapping, std::map<uint64_t, uint64_t> revMapping){

}

void copyCommunities(Graph& g, Graph& help){
    for (int i = 0; i < g.getNumVertices(); i++) {
        g.setCommunity(i,help.getCommunity(i));
    }
}

void addMapping(uint64_t source, std::map<uint64_t, uint64_t>& mapping, std::map<uint64_t, uint64_t>& revMapping, int& i){ //TODO: add to mapping if not already in it
    if(mapping.find(source) == mapping.end()){
        mapping.insert({source, i});
        revMapping.insert({i, source});
        i++;
    }
}

vector<EdgeWeight> calcTot(Graph g){
    vector<EdgeWeight> tot;
    tot.resize(g.getNumVertices());
    for (int i = 0; i < g.getNumVertices(); i++){
        tot[g.getCommunity(i)] = g.getWeightedDegree(i);
    }
    return tot;
}

vector<EdgeWeight> calcIn(Graph g){
    vector<EdgeWeight> in;
    in.resize(g.getNumVertices());
    for(int i = 0; i < g.getNumVertices(); i++){
        for(auto eit = g.getNeighbors(i).begin(); eit != g.getNeighbors(i).end(); eit++){
            if(g.getCommunity(i) == g.getCommunity(eit->first)){
                in[g.getCommunity(i)] += eit->second;
            }
        }
    }
    for (double & eit : in){
        eit = eit/2;
    }
    return in;
}
