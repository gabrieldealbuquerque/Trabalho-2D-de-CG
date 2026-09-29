#pragma once

#include "game.hpp"

namespace cg {
class Controller {
public:
    explicit Controller(Game& game) : game_(game) {}
    bool keyDown(unsigned char key);
    void keyUp(unsigned char key) { game_.setKey(key, false); }
    void special(int functionNumber);
    void motion(int x);
    void mouseLeft(bool pressed, int x);
    void leave();
private:
    Game& game_;
    bool mouseKnown_ = false;
    int previousX_ = 0;
};
}
