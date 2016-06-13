#include "include/Story.h"

#include <iostream>
namespace libwot {

    using namespace std;

    Story::Story(uint32_t sigPeriod, uint32_t sigStock, uint32_t sigValidity, uint32_t sigQty, float xpercent,
                 uint32_t stepsMax, uint32_t sigPerTurnPerMember, float newMembersPercent, uint32_t idtyPeremption)
        : mSigPeriod(sigPeriod), mSigStock(sigStock),
          mSigValidity(sigValidity), mSigQty(sigQty), mXpercent(xpercent), mStepsMax(stepsMax),
          mNewMembersPercent(newMembersPercent), mIdtyPeremption(idtyPeremption),
          mPool(idtyPeremption, mSigValidity, sigPerTurnPerMember)
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
            mPool.addNewIdentity();
        }

        for (uint32_t i=0; i<nbMembers; i++) {
            for (uint32_t j=0; j<nbMembers; j++) {
                if (i != j) {
                    mCurrentWot->getNodeAt(i)->addLinkTo(j);
                    Link link(i, j);
                    mPool.newLink(link);
                }
            }
        }

        for (uint32_t i=0; i<nbMembers; i++) {
            if (resolveMembership(i)) {
                mCurrentMembers.push_back(i);
                mCurrentWot->getNodeAt(i)->setEnabled(true);
                mPool.setMember(i, i);
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
//            mTimestamps[Link(from, to)] = mTurn;
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
        mPool.nextTurn();
        cout << endl;
        cout << "New turn : T = " << mTurn << " : " << mCurrentMembers.size()
                    << " members, " << mPool.size() << " identities" << endl << endl;

        /****************************************************
         *  BEFORE TIME INCREMENTS, WE TAKE INTO ACCOUNT WOT VARIATIONS
         ***************************************************/

        // Check that each identity is maintained in the WoT by the links
        for (uint32_t i = 0; i < mCurrentWot->getSize(); i++){
            if (mCurrentWot->getNodeAt(i)->isEnabled()) {
                if (!hasEnoughLinks(i)) {
                    mCurrentMembers.erase(find(mCurrentMembers.begin(), mCurrentMembers.end(), i));
                    mCurrentWot->getNodeAt(i)->setEnabled(false);
                    mPool.kick(i);
                    cout << i << " : Left community" << endl;
                }
            }
        }

        /****************************************************
         *  TIME HAS INCREMENTED, WE UPDATE THE DATA VALIDTY
         ***************************************************/

        // New identities show up
        int requiredIdtyStock = (int) ceil(mNewMembersPercent * mCurrentWot->getSize());
        mPool.increaseIdtyStockUpTo(requiredIdtyStock);

        // Delete expired certifications
        for(auto it = mPool.mExpiredCerts.cbegin(); it != mPool.mExpiredCerts.cend(); it++) {
            Identity receiver = mPool.getIdentity(it->toID);
            Identity issuer = mPool.getIdentity(it->fromID);
            if (receiver.hasBeenMember && issuer.hasBeenMember) {
                if (mCurrentWot->getNodeAt(it->mLink.first)->hasLinkTo(it->mLink.second)) {
//                        cout << "Expired cert " << issuer.wotid << " -> " << receiver.wotid << endl;
                    mCurrentWot->getNodeAt(issuer.wotid)->removeLinkTo(receiver.wotid);
                }
            }
        }

        // Add new certifications to the pool
        int nbCertsAdded = mPool.produceNewCerts();
        cout << "New certs +" << nbCertsAdded << endl;

        // See which newcomers can join
        std::map< uint32_t, Identity > mNewcomers;
        std::map< uint32_t, Issuer > mIssuers;
        for (auto it = mPool.mNewIdentities.begin(); it != mPool.mNewIdentities.end(); it++){
            mIssuers[it->id].willIssue = 0;
        }
        for (auto it = mPool.mNewIdentities.begin(); it != mPool.mNewIdentities.end(); it++){
            uint32_t idty = it->id;
            Identity newcomer = mPool.getIdentity(idty);
            int32_t wotID = newcomer.wotid;
            if (newcomer.isGoodCandidate(mIdtyPeremption, mTurn)) {
                // Try to add it...
                Node* temp = mCurrentWot->addNode();
                std::vector<uint32_t > certsFromMembers;
                for (auto itc = newcomer.certs.begin(); itc != newcomer.certs.end(); itc++){
                    uint32_t issuer = *itc;
                    Identity issuerIdty = mPool.getIdentity(issuer);
                    uint32_t issuerWOTID = issuerIdty.wotid;
                    if (issuerIdty.isMember) {
                        uint32_t willIssue = mIssuers[issuer].willIssue;
//                        cout << issuer << " already issued " << mCurrentWot->getNodeAt(issuerWOTID)->getNbIssued() << " and plans to have " << willIssue << " more" << endl;
                        if (mCurrentWot->getNodeAt(issuerWOTID)->getNbIssued() + willIssue < mSigStock) {
                            mCurrentWot->getNodeAt(issuerWOTID)->addLinkTo(temp->getId());
                            certsFromMembers.push_back(issuer);
                            mIssuers[issuer].willIssue++;
                        }
                    }
                }
                bool isOK = hasEnoughLinks(temp->getId());
                isOK = isOK && hasGoodDistance(temp->getId());
                if (isOK) {
//                    cout << idty << " can join community with " << certsFromMembers.size() << " links" << endl;
                    // We add a wotID to the identity
                    mNewcomers[idty].certs = certsFromMembers;
                }
                // Remove temp data
                for (auto itc = certsFromMembers.begin(); itc != certsFromMembers.end(); itc++){
                    uint32_t issuerWOTID = mPool.getIdentity(*itc).wotid;
                    if (mCurrentWot->getNodeAt(issuerWOTID)->hasLinkTo(temp->getId())) {
                        mCurrentWot->getNodeAt(issuerWOTID)->removeLinkTo(temp->getId());
                    } else {
                        cout << "Could not undo temporary link " << (*itc) << " (" << issuerWOTID << ") -> " << temp->getId() << endl;
                    }
                }
                mCurrentWot->removeNode();
            }
        }

        // See which certifications from members to members can be taken
        vector<Cert> newInnerLinks = mPool.produceNewInnerLinks();
        for (auto link : newInnerLinks) {
            //                        cout << "priority add " << issuer << " ==> "  << wotID  << " ; "<< notSameIdty << " ; " << (issuer < mCurrentWot->getSize()) << " ; " << !mCurrentWot->getNodeAt(issuer)->hasLinkTo(wotID) << endl;
            if (mCurrentWot->getNodeAt(link.mLink.first)->getNbIssued() + mIssuers[link.fromID].willIssue < mSigStock) {
                mCurrentWot->getNodeAt(link.mLink.first)->addLinkTo(link.mLink.second);
            }
        }

        // Effectively add the newcomers
        for (auto it = mNewcomers.begin(); it != mNewcomers.end(); it++){
//            if (mTurn > 29) break;
            // Add it...
            uint32_t idtyID = (*it).first;
            Identity idty = (*it).second;
            Node* newcomer = mCurrentWot->addNode();
            int nbLinks = 0;
            for (auto itc = idty.certs.begin(); itc != idty.certs.end(); itc++){
                uint32_t issuer = *itc;
                uint32_t issuerWOTID = mPool.getIdentity(issuer).wotid;
//                if (issuer < mCurrentWot->getSize() && mCurrentWot->getNodeAt(issuer)->isEnabled()) {
//                    if (mCurrentWot->getNodeAt(issuer)->hasStockLeft()) {
//                        mCurrentWot->getNodeAt(issuer)->addLinkTo(newcomer);
//                        nbLinks++;
//                    }
//                }
                mCurrentWot->getNodeAt(issuerWOTID)->addLinkTo(newcomer);
                nbLinks++;
            }
//            cout << newcomer->getId() << "(id = " << idtyID << ") : Joined community with " << nbLinks << " links" << endl;
            mPool.setMember(idtyID, newcomer->getId());
            mCurrentMembers.push_back(idtyID);
        }

//        mCurrentWot->showTable();

        // TODO: limit at 1 certification per member per block
        // TODO: membership expiry + renew and join again (~new member)
        // TODO: do not include certifications from members being kicked
    }
}