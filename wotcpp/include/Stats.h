#ifndef STATS_H
#define STATS_H

#include <cstdint>
#include <vector>
#include <string>
#include <boost/graph/adjacency_list.hpp>

#include "webOfTrust.h"
typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS> Graph;
typedef boost::graph_traits<Graph>::vertex_descriptor vertex_t;
typedef boost::graph_traits<Graph>::edge_descriptor edge_t;


namespace libwot {

  class Stats {

    public :
      Stats();
      ~Stats();

      void addWot(WebOfTrust* wot);
      void addData(uint32_t nbMembers, uint32_t nbIdentities);
      void renderStats();

    private :
      std::vector<uint32_t> mNbMembers;
      std::vector<uint32_t> mNbIdentities;
      std::vector< Graph > mGraphs;
  };
}

#endif
