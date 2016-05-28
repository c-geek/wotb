#include "include/Story.h"

#include <iostream>
namespace libwot {

    using namespace std;

    Story::Story(uint32_t sigPeriod, uint32_t sigStock, uint32_t sigValidity, uint32_t sigQty, uint32_t xpercent, uint32_t stepsMax)
        : mSigPeriod(sigPeriod), mSigStock(sigStock),
          mSigValidity(sigValidity), mSigQty(sigQty), mXpercent(xpercent), mStepsMax(stepsMax)
    {
        mStats = Stats();
        mCurrentWot = NULL;
        mTurn = 0;
    }

    Story::~Story(){

    }

    void Story::initialize(uint32_t nbMembers) {
        mCurrentWot = new WebOfTrust(mSigStock);
        for (uint32_t i=0; i<nbMembers; i++) {
            mCurrentWot->addNode();
            mNodesLatestCerts.push_back(0);
        }

        for (uint32_t i=0; i<nbMembers; i++) {
            for (uint32_t j=0; j<nbMembers; j++) {
                if (i != j) {
                    mCurrentWot->getNodeAt(i)->addLinkTo(j);
                    mTimestamps[Link(i, j)] = 0;
                }
            }
        }

        for (uint32_t i=0; i<nbMembers; i++) {
            if (resolveMembership(i)) {
                mCurrentMembers.push_back(i);
                mCurrentWot->getNodeAt(i)->setEnabled(true);
            }
            else {
                cout << i << " : Could not join on initialization" << endl;
            }
        }
    }

    uint32_t Story::addIdentity() {
        uint32_t  nextIndex = mCurrentWot->getNbNodes();
        mCurrentWot->addNode();
        mNodesLatestCerts.push_back(0);
        return nextIndex;
    }

    uint32_t Story::addLink(uint32_t from, uint32_t to) {

        if (mNodesLatestCerts[from] + mSigPeriod > mTurn) {
            cout << from << " -> " << to << " : Latest certification is too recent" << endl;;
        }

        if (mCurrentWot->getNodeAt(from)->addLinkTo(to)) {
            addToCheckedNodes(to);
            mTimestamps[Link(from, to)] = mTurn;
        }
        else {
            //cout << from << " -> " << to << " : Could not add link" << endl;;
        }
    }

    void Story::addToCheckedNodes(uint32_t nodeIndex) {

        if (find (mCheckedNodes.begin(), mCheckedNodes.end(), nodeIndex) == mCheckedNodes.end()){
            mCheckedNodes.push_back(nodeIndex);
        }
    }

    uint32_t Story::sentriesRule(uint32_t nbMembers) {
        if (nbMembers > 100000)
            return 20;
        else if (nbMembers > 10000)
            return 12;
        else if (nbMembers > 1000)
            return 6;
        else if (nbMembers > 100)
            return 4;
        else if (nbMembers > 10)
            return 2;

        return 1;
    }

    bool Story::resolveMembership(uint32_t identity) {
        Node* node = mCurrentWot->getNodeAt(identity);
        if (node->getNbLinks() < mSigQty) {
            cout << identity << " : not enough links, not a member anymore" << endl;
            return false;
        }

        if (mCurrentWot->computeDistance(identity, sentriesRule(mCurrentMembers.size()+1), mStepsMax, mXpercent)
                .isOutdistanced) {
            cout << identity << " : outdistanced, not a member anymore" << endl;
            return false;
        }
        return true;
    }

    void Story::nextTurn() {
        for(auto it = mTimestamps.cbegin(); it != mTimestamps.cend();) {
            if (it->second + mSigValidity < mTurn) {
                mCurrentWot->getNodeAt(it->first.first)->removeLinkTo(it->first.second);
                mTimestamps.erase(it++);
                addToCheckedNodes(it->first.second);
            }
            else {
                ++it;
            }
        }

        for (auto it = mCheckedNodes.begin(); it != mCheckedNodes.end(); it++) {
            bool resolve = resolveMembership(*it);
            auto isMember = find(mCurrentMembers.begin(), mCurrentMembers.end(), *it);
            // If he was a member but not anymore
            if (isMember != mCurrentMembers.end() && !resolve) {
                mCurrentMembers.erase(isMember);
                mCurrentWot->getNodeAt(*it)->setEnabled(false);
                cout << *it << " : Left community" << endl;
            }
                // If he was not a member but now he is
            else if (isMember == mCurrentMembers.end() && resolve) {
                mCurrentMembers.push_back(*it);
                mCurrentWot->getNodeAt(*it)->setEnabled(true);
                cout << *it << " : Joined community" << endl;
            }
        }
        mCheckedNodes.clear();
        mStats.addWot(mCurrentWot);
        mTurn++;
        cout << "New turn : " << mTurn << " : " << mCurrentMembers.size()+1
                    << " members, " << mCurrentWot->getNbNodes()+1 << " identities" << endl;
    }
}