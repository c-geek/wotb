//
// Created by inso on 5/28/16.
//

#ifndef WOTB_STORY_H
#define WOTB_STORY_H

#include "Stats.h"

typedef std::pair<uint32_t, uint32_t> Link;
typedef std::pair<std::vector<uint32_t>, int32_t> Identity;

namespace libwot {

    class Story {
    public:
        Story(uint32_t sigPeriod, uint32_t sigStock, uint32_t sigValidity,
              uint32_t sigQty, float xpercent, uint32_t stepsMax, uint32_t sigPerTurnPerMember,
              float newMembersPercent);

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
        uint32_t mSigPerTurnPerMember;
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
        std::map< Link, uint32_t > mTimestamps;
        std::vector<uint32_t> mArrivals;

        /**
         * Known identities that expressed their will to join the WoT.
         * These identities are *not necessarily* members, but are subject to certifications.
         */
        std::vector<uint32_t> mIdentities;
        std::map< uint32_t, Identity > mPoolCerts;
    };
}


#endif //WOTB_STORY_H
