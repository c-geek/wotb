#include "include/RandomWotStory.h"

#include <iostream>
#include <math.h>

#include <boost/random/uniform_int.hpp>
#include <boost/random/linear_congruential.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/generator_iterator.hpp>


namespace libwot {

    using namespace std;
    using namespace boost;

    uint32_t NB_YEARS = 160;
    uint32_t NB_TURN_PER_YEARS = 12;
    uint32_t NB_TURNS = NB_YEARS*NB_TURN_PER_YEARS;

    RandomWotStory::RandomWotStory()
            : Story(0, 16*NB_TURN_PER_YEARS, NB_TURN_PER_YEARS, 3, 0.9, 3) {
    }

    RandomWotStory::~RandomWotStory() {

    }

    void RandomWotStory::run() {
        boost::random::minstd_rand rng;         // produces randomness out of thin air
        // see pseudo-random number generators

        initialize(4);
        for (uint32_t t=0; t<NB_TURNS; t++) {
            boost::random::uniform_int_distribution<> nodes(0, mCurrentWot->getNbNodes()-1);
            for (auto it=mCurrentMembers.begin(); it != mCurrentMembers.end(); it++){
                if (t % 3 == 0) {
                    uint32_t identity = addIdentity();
                    addLink(*it, identity);
                }
                for (uint32_t i=0; i < (uint32_t)(mSigStock/mSigValidity); i++) {
                    addLink(*it, (uint32_t)(nodes(rng)));
                }
            }
            nextTurn();
            if (mCurrentMembers.size() == 0) {
                cout << "Community died" << endl;
                break;
            }
        }
    }

    void RandomWotStory::display() {

    }

}