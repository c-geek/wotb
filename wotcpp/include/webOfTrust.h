#ifndef WEB_OF_TRUST_H
#define WEB_OF_TRUST_H

#include <cstdint>
#include <vector>
#include <string>

#include "node.h"
#include "distanceResult.h"


namespace libwot {

  class WebOfTrust {

    public :

      static WebOfTrust* createRandom(uint32_t nbMembers, uint32_t maxCertStock);
      static WebOfTrust* readFromDisk(std::string filename);
      WebOfTrust(uint32_t maxCert);
      ~WebOfTrust();

      void setMaxCert(uint32_t maxCert) { mMaxCert = maxCert; };
      uint32_t getNbNodes();
      uint32_t getMaxCert() {return mMaxCert;};
      uint32_t getNodeIndex(Node* node);
      uint32_t getSize() { return mNodes.size();};
      Node* getNodeAt(uint32_t index) { return mNodes.at(index);};
      Node* getRandomNode();

      Node* addNode();
      void removeNode();

      void reset();

      WebOfTrust* showTable();
      WebOfTrust* showGraphviz();

      bool writeToDisk(std::string filename);

      DistanceResult computeDistance(uint32_t member, uint32_t d_min, uint32_t k_max, double x_percent);
      uint32_t checkMatches(uint32_t m1, int distance, uint32_t distanceMax, bool *wotChecked, uint32_t i, uint32_t nbSuccessYet);


    private :

      std::vector<Node*> mNodes;	
      uint32_t mMaxCert;
  };
}

#endif
