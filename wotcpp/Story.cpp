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
        Node* node = mCurrentWot->addNode();
        node->setEnabled(false);
        mNodesLatestCerts.push_back(0);
        mArrivals.push_back(mTurn);
        return nextIndex;
    }

    uint32_t Story::addLink(uint32_t from, uint32_t to) {

        if (mNodesLatestCerts[from] + mSigPeriod > mTurn) {
            cout << from << " -> " << to << " : Latest certification is too recent" << endl;;
        }

        if (mCurrentWot->getNodeAt(from)->addLinkTo(to)) {
            mTimestamps[Link(from, to)] = mTurn;
        }
        else {
            //cout << from << " -> " << to << " : Could not add link" << endl;;
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
            cout << identity << " : not enough links" << endl;
            return false;
        }

        if (mCurrentWot->computeDistance(identity, sentriesRule(mCurrentMembers.size()+1), mStepsMax, mXpercent)
                .isOutdistanced) {
            cout << identity << " : outdistanced" << endl;
            return false;
        }
        return true;
    }

    void Story::nextTurn() {
        for(auto it = mTimestamps.cbegin(); it != mTimestamps.cend();) {
            if (it->second + mSigValidity < mTurn) {
                mCurrentWot->getNodeAt(it->first.first)->removeLinkTo(it->first.second);
                mTimestamps.erase(it++);
            }
            else {
                ++it;
            }
        }

        for (auto it = mArrivals.begin(); it != mArrivals.end(); it++) {
            if ((mTurn - *it) % 6 == 0) {
                uint32_t index = it - mArrivals.begin();
                bool resolve = resolveMembership(index);
                auto isMember = mCurrentWot->getNodeAt(index)->isEnabled();
                // If he was a member but not anymore
                if (isMember && !resolve) {
                    mCurrentMembers.erase(find(mCurrentMembers.begin(), mCurrentMembers.end(), *it));
                    mCurrentWot->getNodeAt(index)->setEnabled(false);
                    cout << index << " : Left community" << endl;
                }
                    // If he was not a member but now he is
                else if (!isMember && resolve) {
                    mCurrentMembers.push_back(index);
                    mCurrentWot->getNodeAt(index)->setEnabled(true);
                    cout << index << " : Joined community" << endl;
                }
            }
        }
        mStats.addWot(mCurrentWot);
        mTurn++;
        cout << "New turn : " << mTurn << " : " << mCurrentMembers.size()+1
                    << " members, " << mCurrentWot->getNbNodes()+1 << " identities" << endl;
    }
}