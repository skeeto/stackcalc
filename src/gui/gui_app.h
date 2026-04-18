#pragma once

#include <wx/wx.h>
#include <wx/timer.h>
#include "controller.h"

class CalcPanel;
class TrailFrame;

// Main application class
class StackCalcApp : public wxApp {
public:
    bool OnInit() override;
};

// Main calculator window
class StackCalcFrame : public wxFrame {
public:
    StackCalcFrame();

    void on_toggle_trail(wxCommandEvent& e);
    void on_quit(wxCommandEvent& e);

    CalcPanel* panel() { return panel_; }
    TrailFrame* trail() { return trail_; }
    void show_trail(bool show);

private:
    CalcPanel* panel_ = nullptr;
    TrailFrame* trail_ = nullptr;
};

// Custom-painted calculator display
class CalcPanel : public wxPanel {
public:
    explicit CalcPanel(wxWindow* parent);

    sc::Controller& controller() { return ctrl_; }

    // Re-render after state change
    void redraw();

private:
    void on_paint(wxPaintEvent& e);
    void on_key_down(wxKeyEvent& e);
    void on_char(wxKeyEvent& e);
    void on_blink_tick(wxTimerEvent& e);

    void toggle_trail();

    sc::Controller ctrl_;
    wxFont mono_font_;
    wxTimer blink_timer_;
    bool cursor_visible_ = true;
};

// Separate trail window
class TrailFrame : public wxFrame {
public:
    explicit TrailFrame(wxWindow* parent);

    void update_entries(const std::vector<std::string>& entries);

private:
    void on_close(wxCloseEvent& e);

    wxTextCtrl* text_ = nullptr;
};

// IDs for menu items
enum {
    ID_ToggleTrail = wxID_HIGHEST + 1,
};

wxDECLARE_APP(StackCalcApp);
