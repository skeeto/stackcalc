#pragma once

#include "controller.h"

class GuiApp {
public:
    GuiApp();

    void setup_font();
    void render();

    void on_char(unsigned int codepoint);
    void on_key(int key, int scancode, int action, int mods);

private:
    void render_stack(const sc::DisplayState& ds);
    void render_mode_line(const sc::DisplayState& ds);

    sc::Controller ctrl_;
    bool show_trail_ = false;
    bool suppress_char_ = false;
};
