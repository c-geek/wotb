#include "include/Story.h"

#include <boost/random/uniform_int.hpp>
#include <boost/random/linear_congruential.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>

#include <iostream>
namespace libwot {

    using namespace std;

    Story::Story(uint32_t sigPeriod, uint32_t sigStock, uint32_t sigValidity, uint32_t sigQty, float xpercent,
                 uint32_t stepsMax, uint32_t sigPerTurnPerMember, float newMembersPercent)
        : mSigPeriod(sigPeriod), mSigStock(sigStock),
          mSigValidity(sigValidity), mSigQty(sigQty), mXpercent(xpercent), mStepsMax(stepsMax),
          mSigPerTurnPerMember(sigPerTurnPerMember), mNewMembersPercent(newMembersPercent)
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
            mIdentities.push_back(i);
            mPoolCerts[i].first = std::vector<uint32_t>();
            mPoolCerts[i].second = i;
        }

        for (uint32_t i=0; i<nbMembers; i++) {
            for (uint32_t j=0; j<nbMembers; j++) {
                if (i != j) {
                    mCurrentWot->getNodeAt(i)->addLinkTo(j);
                    mTimestamps[Link(i, j)] = 0;
                    mPoolCerts[j].first.push_back(i);
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
        // We initialize at -mSigPeriod so that the very first certification issued by this identity won't be too recent
        mNodesLatestCerts[nextIndex] = -mSigPeriod;
        mArrivals.push_back(mTurn);
        return nextIndex;
    }

    uint32_t Story::addLink(uint32_t from, uint32_t to) {

        if (mNodesLatestCerts[from] + mSigPeriod > mTurn) {
            cout << from << " -> " << to << " : Latest certification is too recent" << endl;;
        }

        if (mCurrentWot->getNodeAt(from)->addLinkTo(to)) {
            mTimestamps[Link(from, to)] = mTurn;
            mNodesLatestCerts[from] = mTurn;
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

    bool Story::hasEnoughLinks(uint32_t identity) {
        Node* node = mCurrentWot->getNodeAt(identity);
        if (node->getNbLinks() < mSigQty) {
//            cout << identity << " : not enough links" << endl;
            return false;
        }
        return true;
    }

    bool Story::hasGoodDistance(uint32_t identity) {
        if (mCurrentWot->computeDistance(identity, sentriesRule(mCurrentMembers.size()+1), mStepsMax, mXpercent)
                .isOutdistanced) {
//            cout << identity << " : outdistanced" << endl;
            return false;
        }
        return true;
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

        /****************************************************
         *  BEFORE TIME INCREMENTS, WE TAKE INTO ACCOUNT WOT VARIATIONS
         ***************************************************/

        // Check that each identity is maintained in the WoT by the links
        for (int i = 0; i < mCurrentWot->getSize(); ++i) {
            if (mCurrentWot->getNodeAt(i)->isEnabled()) {
                if (!hasEnoughLinks(i)) {
                    mCurrentMembers.erase(find(mCurrentMembers.begin(), mCurrentMembers.end(), i));
                    mCurrentWot->getNodeAt(i)->setEnabled(false);
                    cout << i << " : Left community" << endl;
                }
            }
        }

//        for (auto it = mArrivals.begin(); it != mArrivals.end(); it++) {
//            if ((mTurn - *it) % 6 == 0) {
//                uint32_t index = it - mArrivals.begin();
//                bool resolve = resolveMembership(index);
//                auto isMember = mCurrentWot->getNodeAt(index)->isEnabled();
//                // If he was a member but not anymore
//                if (isMember && !resolve) {
//                    mCurrentMembers.erase(find(mCurrentMembers.begin(), mCurrentMembers.end(), index));
//                    mCurrentWot->getNodeAt(index)->setEnabled(false);
//                    cout << index << " : Left community" << endl;
//                }
//                    // If he was not a member but now he is
//                else if (!isMember && resolve) {
//                    mCurrentMembers.push_back(index);
//                    mCurrentWot->getNodeAt(index)->setEnabled(true);
//                    cout << index << " : Joined community" << endl;
//                }
//            }
//        }
        mStats.addWot(mCurrentWot);
        mTurn++;
        cout << endl;
        cout << "New turn : T = " << mTurn << " : " << mCurrentMembers.size()
                    << " members, " << mIdentities.size() << " identities" << endl << endl;

        /****************************************************
         *  TIME HAS INCREMENTED, WE UPDATE THE DATA VALIDTY
         ***************************************************/

        // New identities show up
        uint32_t newIdentities = ceil(mIdentities.size() * mNewMembersPercent);
        for (uint32_t i=0; i< newIdentities; i++) {
            uint32_t idty = mIdentities.size();
            mIdentities.push_back(idty);
            mPoolCerts[idty].first = std::vector<uint32_t>();
            mPoolCerts[idty].second = -1;
        }

        // Delete expired certifications
        for(auto it = mTimestamps.cbegin(); it != mTimestamps.cend();) {
            if (it->second + mSigValidity < mTurn) {
                uint32_t receiverID = it->first.first;
                uint32_t issuerID = it->first.second;
                Identity receiver = mPoolCerts[receiverID];
                Identity issuer = mPoolCerts[issuerID];
                if (receiver.second > -1 && issuer.second > -1) {
                    uint32_t idtyWoTID = receiver.second;
                    uint32_t issuerWotID = issuer.second;
                    if (mCurrentWot->getNodeAt(issuerWotID)->hasLinkTo(idtyWoTID)) {
                        mCurrentWot->getNodeAt(issuerWotID)->removeLinkTo(idtyWoTID);
                    }
                }
                mTimestamps.erase(it++);
                vector<uint32_t >::iterator cert = find(receiver.first.begin(), receiver.first.end(), issuerID);
                if (cert != receiver.first.end()) {
                    receiver.first.erase(cert);
                }
            }
            else {
                ++it;
            }
        }

        boost::random::minstd_rand rng;         // produces randomness out of thin air

        // Add new certifications to the pool
        boost::random::uniform_int_distribution<> identities(0, mIdentities.size() - 1);
        for (auto it = mCurrentMembers.begin(); it != mCurrentMembers.end(); it++){
            uint32_t member = *it;
            for (uint32_t i = 0; i < mSigPerTurnPerMember; i++) {
                uint32_t randomIdty = (uint32_t)(identities(rng));
                // We don't certify ourself
                if (randomIdty != member) {
                    bool notFound = find(mPoolCerts[randomIdty].first.begin(), mPoolCerts[randomIdty].first.end(), member) == mPoolCerts[randomIdty].first.end();
                    if (notFound) {
                        mPoolCerts[randomIdty].first.push_back(member);
                        // We remember the date of each link
                        mTimestamps[Link(member, randomIdty)] = mTurn;
                    }
                }
            }
        }

        // See which identities can join
        std::vector<uint32_t> idetitiesToAdd;
        for (auto it = mIdentities.begin(); it != mIdentities.end(); it++){
            uint32_t idty = *it;
            int32_t wotID = mPoolCerts[idty].second;
            bool isNotAMemberYet = wotID == -1;
            if (isNotAMemberYet) {
                // Try to add it...
                Node* temp = mCurrentWot->addNode();
                std::vector<uint32_t > certsFromMembers;
                for (auto itc = mPoolCerts[idty].first.begin(); itc != mPoolCerts[idty].first.end(); itc++){
                    uint32_t issuer = *itc;
                    if (issuer < mCurrentWot->getSize() && mCurrentWot->getNodeAt(issuer)->isEnabled()) {
                        if (mCurrentWot->getNodeAt(issuer)->hasStockLeft()) {
                            mCurrentWot->getNodeAt(issuer)->addLinkTo(temp);
                            certsFromMembers.push_back(issuer);
                        }
                    }
                }
                bool isOK = hasEnoughLinks(temp->getId());
                isOK = isOK && hasGoodDistance(temp->getId());
                if (isOK) {
                    // We add a wotID to the identity
                    idetitiesToAdd.push_back(idty);
                }
                // Remove temp data
                for (auto itc = certsFromMembers.begin(); itc != certsFromMembers.end(); itc++){
                    mCurrentWot->getNodeAt(*itc)->removeLinkTo(temp->getId());
                }
                mCurrentWot->removeNode();
            }
        }

        // See which certifications from members to members can be taken
        for (auto it = mIdentities.begin(); it != mIdentities.end(); it++){
            uint32_t idty = *it;
            int32_t wotID = mPoolCerts[idty].second;
            bool isAMember = wotID > -1;
            if (isAMember) {
                // Try to add certifications..
                std::vector<uint32_t > certsFromMembers;
                for (auto itc = mPoolCerts[idty].first.begin(); itc != mPoolCerts[idty].first.end(); itc++){
                    uint32_t issuer = *itc;
                    bool notSameIdty = issuer != wotID;
                    if (notSameIdty && issuer < mCurrentWot->getSize() && mCurrentWot->getNodeAt(issuer)->isEnabled() && !mCurrentWot->getNodeAt(issuer)->hasLinkTo(wotID)) {
                        if (mCurrentWot->getNodeAt(issuer)->hasStockLeft()) {
                            mCurrentWot->getNodeAt(issuer)->addLinkTo(wotID);
                            certsFromMembers.push_back(issuer);
                        }
                    }
                }
            }
        }

        // Effectively add the newcomers
        for (auto it = idetitiesToAdd.begin(); it != idetitiesToAdd.end(); it++){
            // Add it...
            uint32_t idty = *it;
            Node*newcomer = mCurrentWot->addNode();
            for (auto itc = mPoolCerts[idty].first.begin(); itc != mPoolCerts[idty].first.end(); itc++){
                uint32_t issuer = *itc;
                if (issuer < mCurrentWot->getSize() && mCurrentWot->getNodeAt(issuer)->isEnabled()) {
                    if (mCurrentWot->getNodeAt(issuer)->hasStockLeft()) {
                        mCurrentWot->getNodeAt(issuer)->addLinkTo(newcomer);
                    }
                }
            }
            mPoolCerts[idty].second = newcomer->getId();
            mCurrentMembers.push_back(newcomer->getId());
        }

        // TODO: limit at 1 certification per member per block
        // TODO: membership expiry + renew and join again (~new member)
        // TODO: do not include certifications from members being kicked
    }
}