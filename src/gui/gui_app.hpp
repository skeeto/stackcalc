#pragma once

#include <wx/wx.h>
#include <wx/timer.h>
#include <wx/splitter.h>
#include <wx/richtext/richtextctrl.h>
#include <atomic>
#include <functional>
#include <memory>
#include <unordered_map>
#include "calc_runner.hpp"
#include "controller.hpp"

class CalcPanel;
class TrailPanel;
class TopBar;
class ModeBar;

// Main application class
class StackCalcApp : public wxApp {
public:
    bool OnInit() override;
};

// Main calculator window. Hosts a wxSplitterWindow whose left pane is
// the CalcPanel (entry/stack/mode) and right pane is the TrailPanel.
class StackCalcFrame : public wxFrame {
public:
    StackCalcFrame();

    void on_quit(wxCommandEvent& e);
    void on_about(wxCommandEvent& e);

    // Frame-level keystroke hook: catches special keys (Enter, Tab, Back,
    // Del, Esc, Alt-modified) so the wxRichTextCtrl doesn't swallow them
    // when it has focus from a click-to-select. Printable characters are
    // intentionally skipped here and routed via the panel's wxEVT_CHAR
    // bindings, where unicode translation is reliable.
    void on_char_hook(wxKeyEvent& e);

    // Single dispatcher for every menu item that just feeds keystrokes
    // to the controller. Looks up event.GetId() in menu_dispatch_.
    void on_menu_dispatch(wxCommandEvent& e);

    CalcPanel*  panel() { return panel_; }
    TrailPanel* trail() { return trail_; }

private:
    void build_menus();

    // Add a menu item whose only action is to dispatch a string of
    // keystrokes through the controller. The label gets formatted as
    // "Name (k e y s)". Pass empty `keys` for an inert label.
    void add_action(wxMenu* menu, const wxString& name, const char* keys);

    wxSplitterWindow* splitter_ = nullptr;
    CalcPanel*        panel_ = nullptr;
    TrailPanel*       trail_ = nullptr;

    // Map of menu-item ID → keystroke string for on_menu_dispatch.
    std::unordered_map<int, std::string> menu_dispatch_;
    int next_menu_id_;
};

// Hosts the calculator display, top to bottom:
//   TopBar          custom-painted: message + "." marker + entry line
//   wxRichTextCtrl  selectable stack — top of stack at the TOP, growing
//                   downward (1: just below entry, 2:, 3:, …)
//   ModeBar         custom-painted: mode line + flag indicators
class CalcPanel : public wxPanel {
public:
    explicit CalcPanel(wxWindow* parent);
    ~CalcPanel();

    sc::Controller& controller() { return ctrl_; }
    TopBar*  top_bar()  { return top_bar_; }
    ModeBar* mode_bar() { return mode_bar_; }
    bool cursor_visible() const { return cursor_visible_; }
    const wxFont& mono_font() const { return mono_font_; }

    // Cached display snapshot — paint code reads from here instead of
    // calling controller().display() directly, since the controller
    // may be mid-mutation on the worker thread when paint fires.
    // Updated on the UI thread after every CalcRunner job completes.
    const sc::DisplayState& display_cache() const { return cached_display_; }

    // True iff the worker is currently running a calculation. While
    // busy, the UI accepts only the cancel keystroke (Ctrl+G) and
    // ignores other input. After kComputingOverlayDelayMs, paint code
    // shows a "Computing…" overlay instead of the (frozen) stack.
    bool is_busy() const { return busy_; }
    bool computing_overlay_visible() const { return computing_overlay_visible_; }

    // Wired up by StackCalcFrame after the splitter creates both panes.
    void set_trail_panel(TrailPanel* tp) { trail_panel_ = tp; }

    // Send keyboard focus to a calc-input-receiving child. Used by the
    // frame after Show(); SetFocus on a wxPanel doesn't always reach the
    // right child on Windows, so we target the actual focusable widget.
    void focus_calc();

    // Re-render after state change.
    void redraw();

    // Feed a string of characters to the controller, then redraw.
    // Routes through the runner like every other input.
    void dispatch_keys(const std::string& keys);

    // Submit a single special / modified KeyEvent through the runner.
    // No-op if already busy.
    void dispatch_key(const sc::KeyEvent& k);

    // Public so the frame-level char hook can dispatch into them.
    void on_key_down(wxKeyEvent& e);
    void on_char(wxKeyEvent& e);

private:
    void on_blink_tick(wxTimerEvent& e);
    void on_overlay_tick(wxTimerEvent& e);
    void update_stack();
    void clear_selections();

    // Submit `work` to the runner. on_done re-enters the UI thread
    // via wxCallAfter to refresh cached_display_ and redraw.
    void submit_work(std::function<void()> work);

    // Refresh cached_display_ from ctrl_ and trigger a redraw. Must
    // be called on the UI thread, only when the runner is idle.
    void refresh_cache_and_redraw();

    // Liveness flag for runner callbacks. Captured by value (shared_ptr
    // copy) into every on_done lambda; checked before touching `this`.
    // Cleared in our destructor before runner_ shuts down. Declared
    // first so it's destroyed last, after every captured shared_ptr.
    std::shared_ptr<std::atomic<bool>> alive_ =
        std::make_shared<std::atomic<bool>>(true);

    sc::Controller    ctrl_;
    sc::CalcRunner    runner_;
    sc::DisplayState  cached_display_;
    wxFont            mono_font_;
    wxTimer           blink_timer_;
    wxTimer           overlay_timer_;          // delays "Computing…" overlay
    bool              cursor_visible_ = true;
    bool              busy_ = false;           // a job is in flight
    bool              computing_overlay_visible_ = false;
    TopBar*           top_bar_   = nullptr;
    wxRichTextCtrl*   stack_ctrl_ = nullptr;
    ModeBar*          mode_bar_  = nullptr;
    TrailPanel*       trail_panel_ = nullptr;  // owned by the frame
};

// Custom-painted top subpanel: error/info message (when present) and
// the "." marker + blinking entry line. Two rows tall (height fixed
// for stable layout).
class TopBar : public wxPanel {
public:
    TopBar(wxWindow* parent, CalcPanel* host);
    int desired_height() const { return desired_height_; }
private:
    void on_paint(wxPaintEvent& e);
    CalcPanel* host_;
    int        desired_height_ = 0;
};

// Custom-painted bottom subpanel: mode line + flag/pending-prefix
// indicators. One row tall.
class ModeBar : public wxPanel {
public:
    ModeBar(wxWindow* parent, CalcPanel* host);
    int desired_height() const { return desired_height_; }
private:
    void on_paint(wxPaintEvent& e);
    CalcPanel* host_;
    int        desired_height_ = 0;
};

// Right-pane trail viewer: native rich-text widget showing every trail
// entry with a marker on the entry at the trail pointer.
class TrailPanel : public wxRichTextCtrl {
public:
    TrailPanel(wxWindow* parent, CalcPanel* host);

    // Rebuild the displayed text from the controller's trail.
    void refresh_from_state();

private:
    CalcPanel* host_;
};

// IDs for menu items
enum {
    ID_Reset = wxID_HIGHEST + 1,
    // Dispatching menu items (the ones that just feed keystrokes) live
    // in a contiguous range starting here; on_menu_dispatch handles them
    // all via the menu_dispatch_ table.
    ID_DispatchBase = wxID_HIGHEST + 100,
    ID_DispatchEnd  = wxID_HIGHEST + 999,
};

wxDECLARE_APP(StackCalcApp);
