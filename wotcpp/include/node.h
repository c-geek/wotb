#ifndef NODE_H
#define NODE_H

#include <cstdint>
#include <cstddef>
#include <vector>

namespace libwot {

  class WebOfTrust;

  class Node {

    public :

      Node(WebOfTrust* wot, uint32_t id);
      ~Node();

      bool isEnabled() {return mEnabled;};
      uint32_t getNbLinks() {return mCert.size();};
      Node* getLinkAt(uint32_t index) {return mCert.at(index);};
      uint32_t getNbIssued() {return mNbIssued;};
      uint32_t getId() { return mId; };


      Node* setEnabled(bool enable);
      std::vector<Node*> getLinks() {return mCert;};

      bool hasStockLeft();
      bool addLinkTo(uint32_t to);
      bool addLinkTo(Node* to);
      bool hasLinkTo(uint32_t to);
      bool hasLinkTo(Node* to);
      bool hasLinkFrom(uint32_t from);
      bool hasLinkFrom(Node* from);
      void removeLinkTo(uint32_t to);

    private :

      WebOfTrust *mWot;
      bool mEnabled;
      std::vector<Node*> mCert;
      uint32_t mNbIssued;
      uint32_t mId;
  };
}

#endif
