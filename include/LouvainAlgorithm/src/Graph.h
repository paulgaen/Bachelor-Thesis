#ifndef _GRAPH_H
#define _GRAPH_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <utility>
#include <cmath>
#include <algorithm>
#include <set>
#include <map>

using namespace std;

typedef double EdgeWeight;
typedef int Node;

struct Vertex {
	Vertex(): community(-1), weighted_degree(0){}
	int community;
	EdgeWeight weighted_degree;
};

typedef vector<Vertex> VertexList;
typedef vector<pair<Node, EdgeWeight> > EdgeList;
typedef vector<EdgeList> GRA;

class Graph{
	private:
		GRA graph;
		VertexList vl;
		int vsize;
		int esize;
		EdgeWeight total_weight;
	public:
		Graph();
		Graph(int num, EdgeWeight total_weight);
		Graph(istream& in);
		~Graph();
		void readGraph(istream&);
		void writeGraph();
		void addVertex(Vertex);
		void addEdge(Node, Node, const EdgeWeight&);
        void deleteEdge(Node, Node, const EdgeWeight&);
		const int& getNumVertices() const;
		const int& getNumEdges() const;
		const EdgeWeight& getTotalWeight() const;
		VertexList& getVertices();
		int getDegree(Node src);
		EdgeList& getNeighbors(Node src);
		void setCommunity(Node src, int community);
		int getCommunity(Node src);
		EdgeWeight getWeightedDegree(Node src);
		void setWeightedDegree(Node src, EdgeWeight weight);
		void resize(int size);
        vector<vector<int>> readCommunity();
};

inline
void Graph::addVertex(Vertex v){
	vl.push_back(v);
	vsize++;
}

inline
void Graph::addEdge(Node sid, Node tid, const EdgeWeight& weight) {
    int max = std::max(sid, tid);
    if (max >= graph.size()) {
        graph.resize(max + 1);
        vl.resize(max + 1);
    }
    if(vl[sid].community == -1){
        vl[sid].community = sid;
        vsize++;
    }
    if(vl[tid].community == -1){
        vl[tid].community = tid;
        vsize++;
    }
    if (std::find(graph[sid].begin(), graph[sid].end(), pair(tid, weight)) == graph[sid].end()) {
        graph[sid].emplace_back(tid, weight);
        graph[tid].emplace_back(sid, weight);
        esize++;
        esize++;
        total_weight += 2 * weight;
        vl[sid].weighted_degree += 2 * weight;
        vl[tid].weighted_degree += 2 * weight;
    }
}

inline
void Graph::deleteEdge(Node sid, Node tid, const EdgeWeight& weight){
    graph[sid].erase(
            remove_if(
                    graph[sid].begin(),
                    graph[sid].end(),
                    [tid, weight](const pair<int, int>& edge) {
                        return edge.first == tid && edge.second == weight;
                    }
            ),
            graph[sid].end()
    );
    graph[tid].erase(
            remove_if(
                    graph[tid].begin(),
                    graph[tid].end(),
                    [sid, weight](const pair<int, int>& edge) {
                        return edge.first == sid && edge.second == weight;
                    }
            ),
            graph[tid].end()
    );
    vl[sid].weighted_degree -= 2 * weight;
    vl[tid].weighted_degree -= 2 * weight;
    esize--;
    esize--;
}

inline
const int& Graph::getNumVertices() const{
	return vsize;
}

inline
const int& Graph::getNumEdges() const{
	return esize;
}

inline
const EdgeWeight& Graph::getTotalWeight() const{
	return total_weight;
}

inline
VertexList& Graph::getVertices(){
	return vl;
}

inline
int Graph::getDegree(Node src) {
	return graph[src].size();
}

inline
EdgeList& Graph::getNeighbors(Node src){
	return graph[src];
}

inline
void Graph::setCommunity(Node src, int comm){
	vl[src].community = comm;
}

inline
int Graph::getCommunity(Node src){
	return vl[src].community;
}

inline
EdgeWeight Graph::getWeightedDegree(Node src){
	return vl[src].weighted_degree;
}

inline
void Graph::setWeightedDegree(Node src, EdgeWeight weight){
	vl[src].weighted_degree = weight;
}

#endif
