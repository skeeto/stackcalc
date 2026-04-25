#pragma once

#include <wx/wx.h>
#include <wx/timer.h>
#include <wx/splitter.h>
#include <wx/dataview.h>
#include <atomic>
#include <functional>
#include <memory>
#include <unordered_map>
#include "calc_runner.hpp"
#include "controller.hpp"

class CalcPanel;
class TrailPanel;

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
    // Del, Esc, Alt-modified) so the focused DataView widget doesn't
    // swallow them for navigation/activation when it has focus from a
    // click-to-select. Printable characters are intentionally skipped
    // here and routed via the panel's wxEVT_CHAR bindings, where
    // unicode translation is reliable.
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
//   message_text_       wxStaticText (red, hidden when no message)
//   entry_text_         wxStaticText (mono, shows the in-progress
//                       number-entry buffer)
//   stack_ctrl_         wxDataViewListCtrl, selectable, top of stack
//                       at row 0, growing downward (1: 2: 3: ...)
// Mode line and flag indicators live on the frame's wxStatusBar.
class CalcPanel : public wxPanel {
public:
    explicit CalcPanel(wxWindow* parent);
    ~CalcPanel();

    sc::Controller& controller() { return ctrl_; }
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

    // Esc-as-meta state: after Esc is pressed, the next keystroke is
    // promoted to its M-prefixed form (Esc Tab → M-TAB, etc.) — the
    // Emacs convention, useful when the OS eats Alt-modified keys
    // (Alt+Tab is the Windows app switcher). Pressing Esc twice
    // cancels the meta state. The status bar shows "[M-]" while
    // it's true.
    bool meta_pending() const { return pending_meta_; }

    // Wired up by StackCalcFrame after the splitter creates both panes.
    void set_trail_panel(TrailPanel* tp) { trail_panel_ = tp; }

    // Send keyboard focus to a calc-input-receiving child. Used by the
    // frame after Show(); SetFocus on a wxPanel doesn't always reach the
    // right child on Windows, so we target the actual focusable widget.
    void focus_calc();

    // Re-render after state change.
    void redraw();

    // Snapshot the controller's current display into the cache, then
    // redraw. For state mutations that bypass the runner (load on
    // startup, reset, etc.). UI-thread only — must be called when
    // no worker job is in flight.
    void refresh_display();

    // Feed a string of characters to the controller, then redraw.
    // Routes through the runner like every other input.
    void dispatch_keys(const std::string& keys);

    // Submit a single special / modified KeyEvent through the runner.
    // No-op if already busy.
    void dispatch_key(const sc::KeyEvent& k);

    // Public so the frame-level char hook can dispatch into them.
    void on_key_down(wxKeyEvent& e);
    void on_char(wxKeyEvent& e);

    // Push the mode line + flag indicators into the frame's
    // wxStatusBar. Called from redraw() and from the Esc-as-meta
    // toggle paths (since pending_meta_ also feeds the indicator
    // string), and once from the frame's ctor for the initial fill.
    void update_status_bar();

    // Handler for wxEVT_DATAVIEW_ITEM_EDITING_DONE on stack/trail
    // value columns. We use CELL_EDITABLE not for actual editing but
    // to give users a native in-place text editor over the cell so
    // they can drag-select substrings and Cmd+C copy them — vetoing
    // the commit here keeps the underlying value unchanged. Public
    // so TrailPanel's ctor can bind it on `host_`.
    void on_dataview_edit_done(wxDataViewEvent& e);

    // wxEVT_DATAVIEW_ITEM_ACTIVATED handler for stack/trail. Opens
    // the value column's in-place editor for substring-copy. The
    // handler filters out Enter-driven activations (a macOS quirk)
    // by checking note_enter_event's timestamp. Public so
    // TrailPanel's ctor can bind it on `host_`.
    void on_dataview_activate(wxDataViewEvent& e);

    // Frame's CHAR_HOOK calls this just before dispatching Enter
    // to the calculator's RET path, so the DataView activation
    // handler that fires shortly after (on macOS — NSResponder
    // double-firing) can recognise the activation as Enter-driven
    // and skip it.
    void note_enter_event();
    bool enter_event_recent() const;

private:
    void on_blink_tick(wxTimerEvent& e);
    void on_overlay_tick(wxTimerEvent& e);
    void update_stack();
    void clear_selections();

    // Cmd+C / Ctrl+C handler when the stack widget owns focus. Reads
    // the selected rows and writes the values (just the values, one
    // per line, top-of-stack first) to the clipboard.
    void on_stack_copy(wxCommandEvent& e);

    // Submit `work` to the runner. on_done builds the display snapshot
    // on the worker thread (formatting huge numbers can be slow and
    // would freeze the UI if done here), then re-enters the UI via
    // wxCallAfter to install the snapshot and redraw.
    void submit_work(std::function<void()> work);

    // Submit a single KeyEvent through the runner, honouring the
    // Esc-as-meta pending state: if pending_meta_ is set, k is
    // promoted to its M-prefixed equivalent (Character('x') →
    // Modified("M-x"), Special("TAB") → Modified("M-TAB"), already-
    // Modified passes through). The pending flag is cleared and the
    // mode bar refreshed before submitting.
    void dispatch_with_meta(sc::KeyEvent k);

    // UI-thread half of the on_done flow: install snapshot, clear busy
    // state, redraw.
    void apply_snapshot_and_redraw(sc::DisplayState snapshot);

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
    wxTimer           overlay_timer_;          // delays "Computing…" overlay
    bool              busy_ = false;           // a job is in flight
    bool              computing_overlay_visible_ = false;
    bool              pending_meta_ = false;   // Esc-as-meta one-shot flag
    long long         last_enter_event_ms_ = -1000;  // see note_enter_event
    wxStaticText*         message_text_ = nullptr;  // red, hidden when empty
    wxStaticText*         entry_text_   = nullptr;  // mono entry buffer display
    wxDataViewListCtrl*   stack_ctrl_   = nullptr;
    TrailPanel*           trail_panel_  = nullptr;  // owned by the frame
};

// Right-pane trail viewer: native list control showing every trail
// entry as (tag, value) rows; the row at the trail pointer is
// rendered yellow via the shared MarkedListStore mechanism.
class TrailPanel : public wxDataViewListCtrl {
public:
    TrailPanel(wxWindow* parent, CalcPanel* host);

    // Rebuild the displayed rows from the controller's trail.
    void refresh_from_state();

    // Cmd/Ctrl+C handler: copy the values of selected trail rows
    // (just the values, one per line) to the clipboard.
    void on_trail_copy(wxCommandEvent& e);

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
