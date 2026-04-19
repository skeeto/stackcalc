#pragma once

#include <wx/wx.h>
#include <wx/timer.h>
#include <wx/richtext/richtextctrl.h>
#include <unordered_map>
#include "controller.h"

class CalcPanel;
class TrailFrame;
class EditArea;

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

// Hosts the calculator display: a wxRichTextCtrl up top for the
// (selectable, copyable) stack, and a custom-painted EditArea below
// for the dynamic UI elements (`.` marker, blinking entry line,
// mode line). All keyboard input is handled here.
class CalcPanel : public wxPanel {
public:
    explicit CalcPanel(wxWindow* parent);

    sc::Controller& controller() { return ctrl_; }
    EditArea* edit_area() { return edit_area_; }
    bool cursor_visible() const { return cursor_visible_; }
    const wxFont& mono_font() const { return mono_font_; }

    // Re-render after state change.
    void redraw();

    // Feed a string of characters to the controller, then redraw.
    void dispatch_keys(const std::string& keys);

    // Public so the frame-level char hook can dispatch into them.
    void on_key_down(wxKeyEvent& e);
    void on_char(wxKeyEvent& e);

private:
    void on_blink_tick(wxTimerEvent& e);
    void update_stack();
    void toggle_trail();

    sc::Controller    ctrl_;
    wxFont            mono_font_;
    wxTimer           blink_timer_;
    bool              cursor_visible_ = true;
    wxRichTextCtrl*   stack_ctrl_ = nullptr;
    EditArea*         edit_area_  = nullptr;
};

// Custom-painted bottom subpanel: separator line, "." marker,
// blinking entry line, separator line, mode line. Keeps a back
// pointer to its CalcPanel so it can read the controller state and
// the cursor-blink phase.
class EditArea : public wxPanel {
public:
    EditArea(wxWindow* parent, CalcPanel* host);

    // Height that this control wants in pixels (computed from the
    // monospace font metrics).
    int desired_height() const { return desired_height_; }

private:
    void on_paint(wxPaintEvent& e);

    CalcPanel* host_;
    int        desired_height_ = 0;
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
