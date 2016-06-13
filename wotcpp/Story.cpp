#include "include/Story.h"

#include <boost/random/uniform_int.hpp>
#include <boost/random/linear_congruential.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>

#include <iostream>
namespace libwot {

    using namespace std;

    boost::random::minstd_rand rng;         // produces randomness out of thin air

    Story::Story(uint32_t sigPeriod, uint32_t sigStock, uint32_t sigValidity, uint32_t sigQty, float xpercent,
                 uint32_t stepsMax, uint32_t sigPerTurnPerMember, float newMembersPercent, uint32_t idtyPeremption)
        : mSigPeriod(sigPeriod), mSigStock(sigStock),
          mSigValidity(sigValidity), mSigQty(sigQty), mXpercent(xpercent), mStepsMax(stepsMax),
          mSigPerTurnPerMember(sigPerTurnPerMember), mNewMembersPercent(newMembersPercent), mIdtyPeremption(idtyPeremption)
    {
        mStats = Stats();
        mCurrentWot = NULL;
        mTurn = 0;
    }

    Story::~Story(){

    }

    struct SortableIdentity {
        int nbCerts = 0;
        uint32_t idtyID;
    };
    struct lessCertQuantity {
        inline bool operator() (const SortableIdentity& struct1, const SortableIdentity& struct2) {
            return (struct1.nbCerts < struct2.nbCerts);
        }
    };

    void Story::initialize(uint32_t nbMembers) {
        mCurrentWot = new WebOfTrust(mSigStock);
        for (uint32_t i=0; i<nbMembers; i++) {
            mCurrentWot->addNode();
            mIdentities.push_back(i);
            mPoolCerts[i].wotid = i;
            mPoolCerts[i].hasBeenMember = true;
            mPoolCerts[i].isMember = true;
        }

        for (uint32_t i=0; i<nbMembers; i++) {
            for (uint32_t j=0; j<nbMembers; j++) {
                if (i != j) {
                    mCurrentWot->getNodeAt(i)->addLinkTo(j);
                    mTimestamps[Link(i, j)] = 0;
                    mPoolCerts[j].certs.push_back(i);
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
         *  BEFORE TIME INCREMENTS, WE TAKE INTO ACCOUNT WOT VARIATIONS
         ***************************************************/

        // Check that each identity is maintained in the WoT by the links
        for (auto it = mIdentities.begin(); it != mIdentities.end(); it++){
            Identity idty = mPoolCerts[*it];
            if (mCurrentWot->getNodeAt(idty.wotid)->isEnabled()) {
                if (!hasEnoughLinks(idty.wotid)) {
//        mCurrentWot->showTable();
                    mCurrentMembers.erase(find(mCurrentMembers.begin(), mCurrentMembers.end(), *it));
                    mCurrentWot->getNodeAt(idty.wotid)->setEnabled(false);
                    mPoolCerts[*it].isMember = false;
                    mPoolCerts[*it].hasBeenMember = true;
                    cout << idty.wotid << " : Left community" << endl;
                }
            }
        }

        /****************************************************
         *  TIME HAS INCREMENTED, WE UPDATE THE DATA VALIDTY
         ***************************************************/

        // New identities show up
        int requiredIdtyStock = (int) ceil(mNewMembersPercent * mCurrentWot->getSize());
        int currentStock = 0;
        for (auto it = mIdentities.begin(); it != mIdentities.end(); it++){
            if (mPoolCerts[*it].isGoodCandidate(mIdtyPeremption, mTurn)) {
                currentStock++;
            }
        }
        int qtyToCreate = requiredIdtyStock - currentStock;
        for (uint32_t i = 0; i < qtyToCreate; i++) {
            uint32_t idty = mIdentities.size();
            mIdentities.push_back(idty);
            mPoolCerts[idty].wotid = 0; // Has to be changed when added to the WoT
            mPoolCerts[idty].created_on = mTurn;
//            cout << idty << " : new identity showing up" << endl;
        }

        // Delete expired certifications
        for(auto it = mTimestamps.cbegin(); it != mTimestamps.cend();) {
            if (it->second + mSigValidity < mTurn) {
                uint32_t receiverID = it->first.first;
                uint32_t issuerID = it->first.second;
                Identity receiver = mPoolCerts[receiverID];
                Identity issuer = mPoolCerts[issuerID];
                if (receiver.hasBeenMember && issuer.hasBeenMember) {
                    if (mCurrentWot->getNodeAt(issuer.wotid)->hasLinkTo(receiver.wotid)) {
//                        cout << "Expired cert " << issuer.wotid << " -> " << receiver.wotid << endl;
                        mCurrentWot->getNodeAt(issuer.wotid)->removeLinkTo(receiver.wotid);
                    }
                }
                mTimestamps.erase(it++);
                vector<uint32_t >::iterator cert = find(receiver.certs.begin(), receiver.certs.end(), issuerID);
                if (cert != receiver.certs.end()) {
                    receiver.certs.erase(cert);
                }
            }
            else {
                ++it;
            }
        }

        // Get a vector of identities ordered by quantity of certs, DESC
        vector<SortableIdentity> sortables;
        for (auto it = mIdentities.begin(); it != mIdentities.end(); it++){
            SortableIdentity idty;
            idty.idtyID = *it;
            idty.nbCerts = mPoolCerts[*it].certs.size();
            sortables.push_back(idty);
        }
        sort(sortables.begin(), sortables.end(), lessCertQuantity());

        // Add new certifications to the pool
        int nbCertsAdded = 0;
        for (auto it = mCurrentMembers.begin(); it != mCurrentMembers.end(); it++){
            Identity idty = mPoolCerts[*it];
            uint32_t member = idty.wotid;
            boost::random::uniform_int_distribution<> identities(0, member);
            for (uint32_t i = 0; i < mSigPerTurnPerMember; i++) {
                bool validIdenty = false;
                int maxTries = 3, tries = 0;
                uint32_t randomIdty;
                do {
                    SortableIdentity sortable = sortables[(uint32_t)(identities(rng))];
                    randomIdty = sortable.idtyID;
                    validIdenty = randomIdty != member && !mPoolCerts[randomIdty].isExpired(mIdtyPeremption, mTurn);
                    tries++;
                } while (!validIdenty && tries <= maxTries);
//                cout << "Try cert " << member << " -> " << randomIdty << endl;
                bool notFound = find(mPoolCerts[randomIdty].certs.begin(), mPoolCerts[randomIdty].certs.end(), member) == mPoolCerts[randomIdty].certs.end();
                if (notFound && validIdenty) {
                    mPoolCerts[randomIdty].certs.push_back(member);
                    random_shuffle(mPoolCerts[randomIdty].certs.begin(), mPoolCerts[randomIdty].certs.end());
                    // We remember the date of each link
//                    cout << "New cert " << member << " -> " << randomIdty << endl;
                    mTimestamps[Link(member, randomIdty)] = mTurn;
                    nbCertsAdded++;
                }
            }
        }
        cout << "New certs +" << nbCertsAdded << endl;

        // See which newcomers can join
        std::map< uint32_t, Identity > mNewcomers;
        std::map< uint32_t, Issuer > mIssuers;
        for (auto it = mIdentities.begin(); it != mIdentities.end(); it++){
            mIssuers[*it].willIssue = 0;
        }
        for (auto it = mIdentities.begin(); it != mIdentities.end(); it++){
            uint32_t idty = *it;
            int32_t wotID = mPoolCerts[idty].wotid;
            if (mPoolCerts[idty].isGoodCandidate(mIdtyPeremption, mTurn)) {
                // Try to add it...
                Node* temp = mCurrentWot->addNode();
                std::vector<uint32_t > certsFromMembers;
                for (auto itc = mPoolCerts[idty].certs.begin(); itc != mPoolCerts[idty].certs.end(); itc++){
                    uint32_t issuer = *itc;
                    uint32_t issuerWOTID = mPoolCerts[issuer].wotid;
                    if (mPoolCerts[issuer].isMember) {
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
                    if (mCurrentWot->getNodeAt(mPoolCerts[*itc].wotid)->hasLinkTo(temp->getId())) {
                        mCurrentWot->getNodeAt(mPoolCerts[*itc].wotid)->removeLinkTo(temp->getId());
                    } else {
                        cout << "Could not undo temporary link " << (*itc) << " (" << mPoolCerts[*itc].wotid << ") -> " << temp->getId() << endl;
                    }
                }
                mCurrentWot->removeNode();
            }
        }

        // See which certifications from members to members can be taken

//        cout << ">>>>>>>>>>>>> BEFORE GETTING INNER CERTS <<<<<<<<<<<<<<<<<";

//        mCurrentWot->showTable();

        for (auto it = sortables.begin(); it != sortables.end(); it++){
            SortableIdentity sortable = (*it);
//            cout <<  " Try to get certs for " << sortable.idtyID << endl;
            uint32_t idty = sortable.idtyID;
            int32_t wotID = mPoolCerts[idty].wotid;
            if (mPoolCerts[idty].isMember) {
                // Try to add certifications..
                for (auto itc = mPoolCerts[idty].certs.begin(); itc != mPoolCerts[idty].certs.end(); itc++){
                    uint32_t issuer = *itc;
                    uint32_t issuerWoTID = mPoolCerts[issuer].wotid;
                    bool notSameIdty = issuerWoTID != wotID;
                    if (notSameIdty && mPoolCerts[issuer].isMember && mCurrentWot->getNodeAt(issuerWoTID)->isEnabled() && !mCurrentWot->getNodeAt(issuerWoTID)->hasLinkTo(wotID)) {
//                        cout << "priority add " << issuer << " ==> "  << wotID  << " ; "<< notSameIdty << " ; " << (issuer < mCurrentWot->getSize()) << " ; " << !mCurrentWot->getNodeAt(issuer)->hasLinkTo(wotID) << endl;
                        if (mCurrentWot->getNodeAt(issuerWoTID)->getNbIssued() + mIssuers[issuer].willIssue < mSigStock) {
                            mCurrentWot->getNodeAt(issuerWoTID)->addLinkTo(wotID);
                        }
                    }
                }
            }
        }

//        cout << ">>>>>>>>>>>>> AFTER GETTING INNER CERTS <<<<<<<<<<<<<<<<<";

//        mCurrentWot->showTable();

        // Effectively add the newcomers
        for (auto it = mNewcomers.begin(); it != mNewcomers.end(); it++){
//            if (mTurn > 29) break;
            // Add it...
            uint32_t idtyID = (*it).first;
            Identity idty = (*it).second;
            Node*newcomer = mCurrentWot->addNode();
            int nbLinks = 0;
            for (auto itc = idty.certs.begin(); itc != idty.certs.end(); itc++){
                uint32_t issuer = *itc;
                uint32_t issuerWOTID = mPoolCerts[issuer].wotid;
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
            mPoolCerts[idtyID].wotid = newcomer->getId();
            mPoolCerts[idtyID].isMember = true;
            mPoolCerts[idtyID].hasBeenMember = true;
            mCurrentMembers.push_back(idtyID);
        }

//        mCurrentWot->showTable();

        // TODO: limit at 1 certification per member per block
        // TODO: membership expiry + renew and join again (~new member)
        // TODO: do not include certifications from members being kicked
    }
}