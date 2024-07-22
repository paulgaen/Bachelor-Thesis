//
// Created by paulg on 29.12.2023.
//

#ifndef TEMPUS_LOUVAIN_H
#define TEMPUS_LOUVAIN_H

#include "Graph.h"

string input_graphfile;
int input_para_counter = 1;
float EPSILON = 0.000001;
int	PASS_NUM = 100;

void LouvainAlgorithm(Graph& g, vector<vector<int> >& community, float EPSILON, int PASS_NUM);
void writeCommunity(vector<vector<int> >& community);

#endif //TEMPUS_LOUVAIN_H
