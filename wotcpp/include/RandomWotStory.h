//
// Created by inso on 5/28/16.
//

#ifndef WOTB_RANDOMWOTSTORY_H
#define WOTB_RANDOMWOTSTORY_H

#include "Story.h"

namespace libwot {
    class RandomWotStory : public Story {
    public:
        RandomWotStory();

        ~RandomWotStory();

        void run();

        void display();
    private:
        std::vector<uint32_t> mInvitations;
    };
}


#endif //WOTB_RANDOMWOTSTORY_H
