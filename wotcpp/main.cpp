#include <iostream>
#include "include/RandomWotStory.h"
#include "include/log.h"

#define WOT_FILE "wot.bin"

int main(int argc, char **argv) {
  using namespace std;
  using namespace libwot;

  RandomWotStory story;

  story.run();
  story.display();
}
