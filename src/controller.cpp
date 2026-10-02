#include "controller.hpp"
#include <cctype>

namespace cg {
bool Controller::keyDown(unsigned char key) {
    key = static_cast<unsigned char>(std::tolower(key));
    switch (key) {
    case 27:
        return false;
    case 'r':
        game_.reset();
        mouseKnown_ = false;
        break;
    case 'p':
        game_.togglePause();
        break;
    case 'h':
        game_.toggleHelp();
        break;
    default:
        game_.setKey(key, true);
    }
    return true;
}

void Controller::special(int functionNumber) {
    if (functionNumber == 1) {
        game_.toggleHelp();
    } else {
        game_.toggleDemo(functionNumber);
    }
}

void Controller::motion(int x) {
    if (mouseKnown_) {
        game_.aim(static_cast<double>(x) - previousX_);
    }
    previousX_ = x;
    mouseKnown_ = true;
}

void Controller::mouseLeft(bool pressed, int x) {
    motion(x);
    game_.setFiring(pressed);
}

void Controller::leave() {
    game_.clearInput();
    mouseKnown_ = false;
}
}
