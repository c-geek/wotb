#include "include/Stats.h"

#include <iostream>
#include <algorithm>
#include <iomanip>
#include <cmath>
#include <random>
#include <fstream>

namespace libwot {

    using namespace std;
    using namespace boost;

    Stats::Stats() {
    }

    Stats::~Stats(){

    }

    void Stats::addWot(WebOfTrust* wot) {
        Graph graph = Graph(wot->getNbNodes());

        for (int i=0; i<wot->getNbNodes(); i++) {
            Node* node = wot->getNodeAt(i);
            vertex_t v1 = vertex(i, graph);
            for (int j=0; j<node->getNbLinks(); j++)
            {
                uint32_t certIndex = wot->getNodeIndex(node->getLinkAt(j));
                vertex_t v2 = vertex(certIndex, graph);
                add_edge(v2, v1, graph);
            }
        }
        mGraphs.push_back(graph);
    }
}