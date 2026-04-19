#pragma once

#include <wx/wx.h>
#include <wx/timer.h>
#include <unordered_map>
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
    void on_about(wxCommandEvent& e);

    // Single dispatcher for every menu item that just feeds keystrokes
    // to the controller. Looks up event.GetId() in menu_dispatch_.
    void on_menu_dispatch(wxCommandEvent& e);

    CalcPanel* panel() { return panel_; }
    TrailFrame* trail() { return trail_; }
    void show_trail(bool show);

private:
    void build_menus();

    // Add a menu item whose only action is to dispatch a string of
    // keystrokes through the controller. The label gets formatted as
    // "Name (k e y s)". Pass empty `keys` for an inert label.
    void add_action(wxMenu* menu, const wxString& name, const char* keys);

    CalcPanel* panel_ = nullptr;
    TrailFrame* trail_ = nullptr;

    // Map of menu-item ID → keystroke string for on_menu_dispatch.
    std::unordered_map<int, std::string> menu_dispatch_;
    int next_menu_id_;
};

// Custom-painted calculator display
class CalcPanel : public wxPanel {
public:
    explicit CalcPanel(wxWindow* parent);

    sc::Controller& controller() { return ctrl_; }

    // Re-render after state change
    void redraw();

    // Feed a string of characters to the controller, then redraw.
    // Used by menu items so they exercise the same dispatch path as
    // typing the keystrokes directly.
    void dispatch_keys(const std::string& keys);

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
    // Dispatching menu items (the ones that just feed keystrokes) live
    // in a contiguous range starting here; on_menu_dispatch handles them
    // all via the menu_dispatch_ table.
    ID_DispatchBase = wxID_HIGHEST + 100,
    ID_DispatchEnd  = wxID_HIGHEST + 999,
};

wxDECLARE_APP(StackCalcApp);
