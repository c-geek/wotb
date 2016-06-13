#include "include/RandomWotStory.h"

#include <iostream>
#include <chrono>
#include <math.h>

#include <boost/random/uniform_int.hpp>
#include <boost/random/linear_congruential.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/generator_iterator.hpp>


namespace libwot {

    using namespace std;
    using namespace boost;

    uint32_t REQUIRED_SIG_TO_BE_A_MEMBER = 5;
    uint32_t DISTANCE_MAX = 5;
    float PERCENT_OF_SENTRIES_TO_REACH = 0.9;

    uint32_t NB_YEARS = 8;
    uint32_t NB_TURN_PER_YEARS = 12;
    uint32_t NB_TURNS = NB_YEARS * NB_TURN_PER_YEARS;

    uint32_t SIG_STOCK = 24;
    uint32_t SIG_VALIDITY_DURATION = 900;
    uint32_t SIG_PER_TURN_PER_MEMBER = floor(SIG_STOCK / NB_TURN_PER_YEARS);
    uint32_t IDENTITY_PEREMPTION = NB_TURNS; // Infinity
    float NEW_IDENTITIES_STOCK = 0.5; // We always have a X percent of new identities available

    RandomWotStory::RandomWotStory()
            : Story(0, SIG_STOCK, SIG_VALIDITY_DURATION, REQUIRED_SIG_TO_BE_A_MEMBER, PERCENT_OF_SENTRIES_TO_REACH,
                    DISTANCE_MAX, SIG_PER_TURN_PER_MEMBER, NEW_IDENTITIES_STOCK, IDENTITY_PEREMPTION) {
    }

    RandomWotStory::~RandomWotStory() {

    }

    void showDurationMS(std::chrono::high_resolution_clock::time_point t1) {
        std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count() / 1000;
//        cout << duration << "ms" << endl;
    }

    void RandomWotStory::run() {
        boost::random::minstd_rand rng;         // produces randomness out of thin air
        // see pseudo-random number generators

        // Create the initial WoT
        initialize(mSigQty + 1);
        mCurrentWot->showTable();

        // Now make the WoT live
        for (uint32_t t=0; t<NB_TURNS; t++) {

            // Time mesurements
            std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();

            boost::random::uniform_int_distribution<> nodes(0, mCurrentWot->getNbNodes()-1);
//            for (auto it=mCurrentMembers.begin(); it != mCurrentMembers.end(); it++){
//                if (t % 3 == 0) {
//                    uint32_t identity = addIdentity();
//                    addLink(*it, identity);
//                }
//                for (uint32_t i=0; i < (uint32_t)(mSigStock/mSigValidity); i++) {
//                    addLink(*it, (uint32_t)(nodes(rng)));
//                }
//            }
            nextTurn();
            showDurationMS(t2);
            if (mCurrentMembers.size() == 0) {
                cout << "Community died" << endl;
                break;
            }
        }

        // Finally we display the resulting WoT
        mCurrentWot->showTable();
    }

    void RandomWotStory::display() {

    }

}