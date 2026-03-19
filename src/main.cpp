#include "logger.h"

int main() {
    Logger logger("out.log");

    for (int i = 0; i < 10000; i++) {
        logger.log("hello " + std::to_string(i));
    }

    return 0;
}
