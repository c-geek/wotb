#include "include/Stats.h"

#include <iostream>
#include <algorithm>
#include <iomanip>
#include <cmath>
#include <random>
#include <fstream>
#include "include/gnuplot-iostream.h"
#include <boost/property_map/shared_array_property_map.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/betweenness_centrality.hpp>

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
                uint32_t certIndex = node->getLinkAt(j)->getId();
                vertex_t v2 = vertex(certIndex, graph);
                add_edge(v2, v1, graph);
            }
        }
        mGraphs.push_back(graph);
    }

    void Stats::addData(uint32_t nbMembers, uint32_t nbIdentities) {
        mNbIdentities.push_back(nbIdentities);
        mNbMembers.push_back(nbMembers);
    }

    void Stats::renderStats() {
        uint32_t max = 0;
        for (auto it = mNbMembers.begin(); it != mNbMembers.end(); it++) {
            if (*it > max)
                max = *it;
        }
        for (auto it = mNbIdentities.begin(); it != mNbIdentities.end(); it++) {
            if (*it > max)
                max = *it;
        }

        /*for (auto it = mGraphs.begin(); it != mGraphs.end(); it++) {
            shared_array_property_map<double, property_map<Graph, vertex_index_t>::const_type>
                    centrality_map(num_vertices(*it), get(vertex_index, *it));

            brandes_betweenness_centrality(*it, centrality_map);
        }*/

        Gnuplot gp;
        // Don't forget to put "\n" at the end of each line!
        gp << "set xrange [0:" << mNbMembers.size() << "]\n";
        gp << "set yrange [0:" << max << "]\n";
        // '-' means read from stdin.  The send1d() function sends data to gnuplot's stdin.
        gp << "plot '-' with lines title 'Members', '-' with lines title 'Identities'\n";
        gp.send1d(mNbMembers);
        gp.send1d(mNbIdentities);
    }

}