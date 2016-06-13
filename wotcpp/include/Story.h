//
// Created by inso on 5/28/16.
//

#ifndef WOTB_STORY_H
#define WOTB_STORY_H

#include "Stats.h"

#include <iostream>
#include <boost/random/uniform_int.hpp>
#include <boost/random/linear_congruential.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>

using namespace std;

typedef std::pair<uint32_t, uint32_t> Link;

struct Issuer {
    uint32_t willIssue = 0;
};

class Cert {

public:

    Cert(){}

    Cert(Link link, uint32_t mTurn, uint32_t from, uint32_t to) :
            mLink(link),
            created_on(mTurn), fromID(from), toID(to) {}

    Link mLink;
    uint32_t created_on = 0;
    uint32_t fromID = 0;
    uint32_t toID = 0;
    bool written = false;
};

struct Identity {
    std::vector<uint32_t> certs;
    uint32_t id;
    uint32_t wotid;
    uint32_t created_on = 0;
    bool hasBeenMember;
    bool isMember;

    bool isGoodCandidate(uint32_t idtyPeremption, uint32_t currentTime) {
        return !hasBeenMember && !isExpired(idtyPeremption, currentTime);
    }

    bool isExpired(uint32_t idtyPeremption, uint32_t currentTime) {
        return currentTime > created_on + idtyPeremption;
    }
};

struct SortableIdentity {
    int nbCerts = 0;
    uint32_t idtyID;
};

struct lessCertQuantity {
    inline bool operator() (const SortableIdentity& struct1, const SortableIdentity& struct2) {
        return (struct1.nbCerts < struct2.nbCerts);
    }
};

class IdentityPool {

    boost::random::minstd_rand rng;         // produces randomness out of thin air
    uint32_t mIdtyPeremption = 0;
    uint32_t mSigValidity = 0;
    uint32_t mSigPerTurnPerMember = 0;
    uint32_t mTurn = 0;
    std::map< Link, Cert > mCerts;
    std::map< uint32_t, Identity > mPoolCerts;
    std::map< uint32_t, uint32_t > mWoT2Identity;

    // Temp data
    vector<SortableIdentity> sortables;

public:
    std::vector<Cert> mExpiredCerts;
    std::vector<Identity> mNewIdentities;

public:
    IdentityPool(uint32_t idtyPeremption, uint32_t sigValidity, uint32_t sigPerTurnPerMember) :
            mIdtyPeremption(idtyPeremption),
            mSigValidity(sigValidity),
            mSigPerTurnPerMember(sigPerTurnPerMember) {
    }

    Identity getIdentity(uint32_t id) {
        return mPoolCerts[id];
    }

    unsigned long size() {
        return mPoolCerts.size();
    }

    void addNewIdentity() {
        uint32_t id = (uint32_t) mPoolCerts.size();
        mPoolCerts[id].created_on = mTurn;
        mPoolCerts[id].id = id;
        mPoolCerts[id].wotid = 0;
        mPoolCerts[id].hasBeenMember = false;
        mPoolCerts[id].isMember = false;
    }

    void newLink(Link link) {
        uint32_t fromID = mWoT2Identity[link.first];
        uint32_t toID = mWoT2Identity[link.second];
        Identity to = mPoolCerts[toID];
        to.certs.push_back(link.first);
        mCerts[Link(link.first, link.second)] = Cert(link, mTurn, fromID, toID);
    }

    void setMember(uint32_t idty, uint32_t wotid) {
        mPoolCerts[idty].wotid = wotid;
        mPoolCerts[idty].hasBeenMember = true;
        mPoolCerts[idty].isMember = true;
        mWoT2Identity[wotid] = idty;
    }

    void kick(uint32_t wotid) {
        uint32_t idty = mWoT2Identity[wotid];
        if (!mPoolCerts[idty].isMember) {
            throw "Cannot kick a non member";
        }
        if (!mPoolCerts[idty].hasBeenMember) {
            throw "Cannot kick someone who has never been a member";
        }
        mPoolCerts[idty].isMember = false;
    }

    void increaseIdtyStockUpTo(int requiredIdtyStock) {
        int currentStock = this->getCurrentCandidatesStock();
        int qtyToCreate = requiredIdtyStock - currentStock;
        for (uint32_t i = 0; i < qtyToCreate; i++) {
            this->addNewIdentity();
        }
        mNewIdentities.clear();
        for (auto pair : mPoolCerts) {
            if (!pair.second.hasBeenMember) {
                mNewIdentities.push_back(pair.second);
            }
        }
    }

    void nextTurn() {
        // TODO: wot data actualization
        mTurn++;
        // Delete expired certifications
        mExpiredCerts.clear();
        for (auto it = mCerts.begin(); it != mCerts.end(); it++) {
            Cert cert = it->second;
            if (cert.created_on + mSigValidity < mTurn) {
                Identity receiver = mPoolCerts[cert.toID];
                std::vector<uint32_t>::iterator found = std::find(receiver.certs.begin(), receiver.certs.end(), cert.fromID);
                if (found != receiver.certs.end()) {
                    receiver.certs.erase(found);
                }
                mExpiredCerts.push_back(cert);
                mCerts.erase(it);
            }
        }
    }

    /**
     * Randomly share new certifications, but prefer identities with few certifications count.
     */
    int produceNewCerts() {
        int nbCertsAdded = 0;
        sortables.clear();
        sortables = this->getIdentitiesSortedByLessCertificationsCount();
        for (auto it = mPoolCerts.begin(); it != mPoolCerts.end(); it++){
            Identity issuer = it->second;
            uint32_t member = it->first;
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
                    Link link(issuer.wotid, mPoolCerts[randomIdty].wotid);
                    mCerts[link] = Cert(link, mTurn, member, randomIdty);
                    nbCertsAdded++;
                }
            }
        }
        return nbCertsAdded;
    }

    std::vector<SortableIdentity> getIdentitiesSortedByLessCertificationsCount() {
        std::vector<SortableIdentity> sortables;
        for (auto it = mPoolCerts.begin(); it != mPoolCerts.end(); it++){
            SortableIdentity idty;
            idty.idtyID = it->first;
            idty.nbCerts = it->second.certs.size();
            sortables.push_back(idty);
        }
        sort(sortables.begin(), sortables.end(), lessCertQuantity());
        return sortables;
    }

    vector<Cert> produceNewInnerLinks() {
        vector<Cert> newLinks;
        for (auto it = sortables.begin(); it != sortables.end(); it++){
            SortableIdentity sortable = (*it);
//            cout <<  " Try to get certs for " << sortable.idtyID << endl;
            uint32_t idty = sortable.idtyID;
            Identity receiverIdty = mPoolCerts[idty];
            int32_t wotID = receiverIdty.wotid;
            if (receiverIdty.isMember) {
                // Try to add certifications..
                for (auto itc = receiverIdty.certs.begin(); itc != receiverIdty.certs.end(); itc++){
                    uint32_t issuer = *itc;
                    Identity issuerIdty = mPoolCerts[issuer];
                    uint32_t issuerWoTID = issuerIdty.wotid;
                    bool notSameIdty = issuerWoTID != wotID;
                    if (notSameIdty && issuerIdty.isMember) {
                        newLinks.push_back(Cert(Link(issuerWoTID, wotID), 0, issuer, idty));
                    }
                }
            }
        }
        return newLinks;
    }

private:

    int getCurrentCandidatesStock() {
        int currentStock = 0;
        for (auto it = mPoolCerts.begin(); it != mPoolCerts.end(); it++){
            Identity idty = (*it).second;
            if (idty.isGoodCandidate(mIdtyPeremption, mTurn)) {
                currentStock++;
            }
        }
        return currentStock;
    }
};

namespace libwot {

    class Story {
    public:
        Story(uint32_t sigPeriod, uint32_t sigStock, uint32_t sigValidity,
              uint32_t sigQty, float xpercent, uint32_t stepsMax, uint32_t sigPerTurnPerMember,
              float newMembersPercent, uint32_t idtyPeremption);

        virtual ~Story();

        virtual void run() = 0;
        virtual void display() = 0;
        void initialize(uint32_t nbMembers);
        uint32_t addIdentity();
        uint32_t addLink(uint32_t from, uint32_t to);

        bool hasEnoughLinks(uint32_t identity);
        bool hasGoodDistance(uint32_t identity);
        bool resolveMembership(uint32_t identity);
        uint32_t sentriesRule(uint32_t nbMembers);
        void nextTurn();

    protected:
        void addToCheckedNodes(uint32_t nodeIndex);

        float mNewMembersPercent;
        uint32_t mIdtyPeremption;
        uint32_t mSigPeriod;
        uint32_t mSigStock;
        uint32_t mSigValidity;
        uint32_t mSigQty;
        float mXpercent;
        uint32_t mStepsMax;
        uint32_t mTurn;
        Stats mStats;
        WebOfTrust* mCurrentWot;
        std::vector<uint32_t> mCurrentMembers;
        std::map< uint32_t, int32_t > mNodesLatestCerts;
        std::vector<uint32_t> mArrivals;

        IdentityPool mPool;
    };
}


#endif //WOTB_STORY_H
