#include "gui_app.hpp"
#include "persistence.hpp"
#include "version.hpp"
#include <wx/accel.h>
#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/fontenum.h>
#include <wx/stdpaths.h>
#include <algorithm>
#include <cstdio>
#include <sstream>
#include <string>

// Two color constants survive the native-widget migration:
//   kAccentColor — dark amber, used for placeholder rows in stack/
//                  trail (via MarkedListStore), the "Computing…"
//                  overlay row, and the trail-pointer row.
//                  Distinguishes "not an actual number" output and
//                  "current entry" highlighting. Chosen to read
//                  against both light (white) and dark backgrounds —
//                  pure yellow is unreadable on white. Combined with
//                  Bold for extra emphasis.
//   kErrorColor  — red, used for the message line (wxStaticText
//                  foreground when a calculator error is set). Reads
//                  on both light and dark backgrounds.
// Everything else (kBgColor / kSeparatorColor / kLabelColor /
// kValueColor / kMarkerColor / kEntryColor / kModeColor / the old
// kFlagColor yellow) was a dark-theme override of platform defaults;
// with native widgets throughout, the platform handles light/dark
// mode and selection state correctly without us telling it.
static const wxColour kAccentColor(204, 102,   0);  // dark amber
static const wxColour kErrorColor (200,  40,  40);  // dark red

// Custom store for the stack and trail widgets. wxDataViewListCtrl's
// built-in store doesn't expose per-row attributes, but we want some
// rows rendered in flag color rather than the platform default text
// color: formatter-emitted "<integer: N digits>" placeholders, the
// stack's "Computing…" overlay row, and the trail's pointer row.
// Subclass and override GetAttrByRow, keying on the row's wxUIntPtr
// data slot — which AppendItem(row, data) and GetItemData(item)
// already manage as part of the store's public API.
class MarkedListStore : public wxDataViewListStore {
public:
    bool GetAttrByRow(unsigned int row, unsigned int /*col*/,
                      wxDataViewItemAttr& attr) const override {
        if (row >= m_data.size()) return false;
        // 1 = "render with accent + bold"; 0 = platform default.
        if (m_data[row]->GetData() == 1) {
            attr.SetColour(kAccentColor);
            attr.SetBold(true);
            return true;
        }
        return false;
    }
};

// === Session persistence (per-platform user data dir) ========================

namespace {

// Returns the per-user data directory (created if needed) plus the state
// filename. Locations:
//   macOS    ~/Library/Application Support/stackcalc/state.scl
//   Windows  %APPDATA%\stackcalc\state.scl
//   Linux    $XDG_DATA_HOME/stackcalc/state.scl  (default ~/.local/share/...)
wxString state_file_path() {
    wxString dir = wxStandardPaths::Get().GetUserDataDir();
    if (!wxFileName::DirExists(dir)) {
        wxFileName::Mkdir(dir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
    }
    return dir + wxFileName::GetPathSeparator() + "state.scl";
}

// Read entire file with wxFile (native handles, no stdio).
bool read_state_file(const wxString& path, std::string& out) {
    if (!wxFileName::FileExists(path)) return false;
    wxFile f;
    if (!f.Open(path, wxFile::read)) return false;
    wxFileOffset len = f.Length();
    if (len < 0) return false;
    out.assign(static_cast<size_t>(len), '\0');
    ssize_t got = f.Read(out.data(), len);
    return got == len;
}

// Atomically-ish overwrite the state file via wxFile (no stdio).
bool write_state_file(const wxString& path, const std::string& contents) {
    wxFile f;
    if (!f.Create(path, /*overwrite=*/true)) return false;
    return f.Write(contents.data(), contents.size()) == contents.size();
}

// Pull "(window W H S)" out of saved contents (the persistence module
// itself ignores this form as unknown). Returns true if found and parsed.
bool parse_window_line(const std::string& s, int& w, int& h, int& sash) {
    size_t pos = s.find("(window ");
    if (pos == std::string::npos) return false;
    return std::sscanf(s.c_str() + pos, "(window %d %d %d)",
                       &w, &h, &sash) == 3;
}

} // anon namespace

// === StackCalcFrame ===========================================================

// Format a dispatch-key string for display: insert spaces between chars,
// and double up '&' so wxWidgets doesn't interpret it as a mnemonic.
static wxString format_keys_for_display(const char* keys) {
    wxString out;
    for (size_t i = 0; keys[i]; ++i) {
        if (i) out += ' ';
        if (keys[i] == '&') out += "&&";
        else out += keys[i];
    }
    return out;
}

void StackCalcFrame::add_action(wxMenu* menu, const wxString& name,
                                const char* keys) {
    wxString label = name;
    if (keys && *keys) {
        label += " (";
        label += format_keys_for_display(keys);
        label += ")";
    }
    int id = next_menu_id_++;
    menu->Append(id, label);
    menu_dispatch_[id] = keys ? keys : "";
}

void StackCalcFrame::on_menu_dispatch(wxCommandEvent& e) {
    auto it = menu_dispatch_.find(e.GetId());
    if (it == menu_dispatch_.end() || it->second.empty()) return;
    panel_->dispatch_keys(it->second);
}

StackCalcFrame::StackCalcFrame()
    : wxFrame(nullptr, wxID_ANY, "stackcalc", wxDefaultPosition, wxDefaultSize),
      next_menu_id_(ID_DispatchBase)
{
    SetClientSize(FromDIP(wxSize(960, 640)));
    Centre();

    // Window icon. On Windows this picks the per-window title-bar /
    // taskbar / Alt-Tab icon out of the EXE's APPICON resource (see
    // stackcalc.rc). On macOS/Linux wxICON's resource path doesn't
    // apply — macOS reads CFBundleIconFile from Info.plist for the
    // Dock/Cmd+Tab/Finder icon, and there's no per-window icon
    // concept in the Cocoa frame.
#ifdef __WXMSW__
    SetIcon(wxICON(APPICON));
#endif

    // Splitter hosts the calculator on the left and the trail on the right.
    // wxSP_3D gives a visible native sash on every platform —
    // wxSP_THIN_SASH renders the sash as a 1-pixel hairline that's
    // technically draggable but invisible (especially on macOS
    // NSSplitView with light-mode backgrounds). Live update so the
    // panes resize while dragging.
    splitter_ = new wxSplitterWindow(
        this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxSP_LIVE_UPDATE | wxSP_3D | wxBORDER_NONE);
    splitter_->SetMinimumPaneSize(FromDIP(150));
    splitter_->SetSashGravity(0.5);  // share resize equally between calc and trail

    panel_ = new CalcPanel(splitter_);
    trail_ = new TrailPanel(splitter_, panel_);
    panel_->set_trail_panel(trail_);

    // The sash position passed here is interpreted relative to the
    // splitter's current width — which is still its default at
    // construction time. We re-set it below after layout.
    splitter_->SplitVertically(panel_, trail_);

    auto* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(splitter_, 1, wxEXPAND);
    SetSizer(sizer);

    build_menus();

    // Status bar at the bottom of the frame: field 0 (proportional)
    // shows the mode line ("12 Deg"), field 1 (proportional, narrower)
    // shows flag indicators ([I][H][K][d-][M-]). CalcPanel pushes
    // the text via update_status_bar() on every refresh.
    {
        auto* sb = CreateStatusBar(2);
        const int widths[] = { -3, -1 };  // 3:1 split
        sb->SetStatusWidths(2, widths);
    }

    // Focus has to be set after the frame is shown — on Windows, SetFocus
    // before Show() is silently ignored and the OS picks its own default
    // child (often the splitter), leaving keystrokes nowhere to land.
    // CallAfter defers until the event loop is running and the frame is
    // realized.
    CallAfter([this] { panel_->focus_calc(); });

    // Push the initial mode/flags into the status bar now that both
    // the bar and the panel exist.
    panel_->update_status_bar();

    // Try to restore saved session. If we got a window size and sash from
    // the saved state, use those; otherwise fall back to centering and a
    // 50/50 sash split via CallAfter (so the splitter has its real width
    // by the time we set the sash position).
    int saved_w = -1, saved_h = -1, saved_sash = -1;
    {
        std::string contents;
        if (read_state_file(state_file_path(), contents)) {
            std::istringstream in(contents);
            sc::persistence::load(in, panel_->controller());
            // refresh_display (not just redraw) — load mutated the
            // controller, but our cached snapshot is still the empty
            // state from the panel's constructor.
            panel_->refresh_display();
            parse_window_line(contents, saved_w, saved_h, saved_sash);
        }
    }
    if (saved_w > 0 && saved_h > 0) {
        SetClientSize(saved_w, saved_h);
    }
    if (saved_sash <= 0) {
        // Default 50/50 split (deferred so the splitter has its real width).
        CallAfter([this] {
            splitter_->SetSashPosition(splitter_->GetClientSize().GetWidth() / 2);
        });
    } else {
        CallAfter([this, saved_sash] { splitter_->SetSashPosition(saved_sash); });
    }

    // Save session on close.
    Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent& e) {
        std::ostringstream out;
        sc::persistence::save(out, panel_->controller());
        out << "(window " << GetClientSize().GetWidth()
            << " "        << GetClientSize().GetHeight()
            << " "        << splitter_->GetSashPosition() << ")\n";
        write_state_file(state_file_path(), out.str());
        e.Skip();  // continue normal close handling
    });
}

void StackCalcFrame::build_menus() {
    auto* mb = new wxMenuBar();

    // ---- File ----
    auto* file = new wxMenu();
    file->Append(ID_Reset, "&Reset Calculator\tCtrl+R");
    file->AppendSeparator();
    // Use the platform's idiomatic close shortcut. wx maps Ctrl+Q to
     // Cmd+Q automatically on macOS; on Windows the standard is Alt+F4.
#ifdef __WXMSW__
    file->Append(wxID_EXIT, "&Quit\tAlt+F4");
#else
    file->Append(wxID_EXIT, "&Quit\tCtrl+Q");
#endif
    mb->Append(file, "&File");

    // ---- Edit ----
    // Stack manipulation + undo/redo + modifier flags. Items with real
    // wx accelerators (Ctrl/F-keys) use \t syntax; bare-letter alternates
    // are shown in parens for discoverability.
    auto* edit = new wxMenu();
    add_action(edit, "&Undo\tCtrl+Z",        "U");
    add_action(edit, "&Redo\tCtrl+Y",        "D");
    edit->AppendSeparator();
    // Stack ops: avoid wx accelerators here because they would compete
    // with our CHAR_HOOK for the actual keystrokes (Back/Tab/Alt+Tab/
    // Alt+Enter) and break their context-aware behavior (e.g., Backspace
    // edits the entry when active, drops otherwise). Show the keystroke
    // in parens like the other menu items, and bind a per-item lambda
    // that synthesizes the right Special/Modified KeyEvent on click.
    // Menu-driven commands route through the runner just like keyboard
    // input, so they respect the same async + cancel pipeline.
    auto add_special = [&](wxMenu* m, const wxString& label, const char* name) {
        int id = next_menu_id_++;
        m->Append(id, label);
        Bind(wxEVT_MENU, [this, name](wxCommandEvent&) {
            panel_->dispatch_key(sc::KeyEvent::special(name));
        }, id);
    };
    auto add_modified = [&](wxMenu* m, const wxString& label, const char* name) {
        int id = next_menu_id_++;
        m->Append(id, label);
        Bind(wxEVT_MENU, [this, name](wxCommandEvent&) {
            panel_->dispatch_key(sc::KeyEvent::modified(name));
        }, id);
    };
    add_special (edit, "&Drop (Back)",            "DEL");
    add_special (edit, "S&wap (Tab)",             "TAB");
    add_modified(edit, "Roll Top Three Up (Esc Tab / Alt+Tab)",      "M-TAB");
    add_modified(edit, "Recall Last Args (Esc Enter / Alt+Enter)",   "M-RET");
    edit->AppendSeparator();
    add_action(edit, "Toggle Inverse Flag",     "I");
    add_action(edit, "Toggle Hyperbolic Flag",  "H");
    add_action(edit, "Toggle Keep-Args Flag",   "K");
    mb->Append(edit, "&Edit");

    // ---- Math (basic arithmetic + constants) ----
    auto* math = new wxMenu();
    add_action(math, "Add",            "+");
    add_action(math, "Subtract",       "-");
    add_action(math, "Multiply",       "*");
    add_action(math, "Divide",         "/");
    add_action(math, "Power",          "^");
    add_action(math, "Modulo",         "%");
    add_action(math, "Integer Divide", "\\");
    math->AppendSeparator();
    add_action(math, "Negate",        "n");
    add_action(math, "Reciprocal",    "&");
    add_action(math, "Square Root",   "Q");
    add_action(math, "Square",        "IQ");
    add_action(math, "Absolute Value","A");
    math->AppendSeparator();
    add_action(math, "Floor",   "F");
    add_action(math, "Ceiling", "IF");
    add_action(math, "Round",   "R");
    add_action(math, "Truncate","IR");
    math->AppendSeparator();
    auto* constants = new wxMenu();
    add_action(constants, "Pi",                  "P");
    add_action(constants, "e",                   "HP");
    add_action(constants, "Gamma (Euler-Masch.)","IP");
    add_action(constants, "Phi (golden ratio)",  "HIP");
    math->AppendSubMenu(constants, "&Constants");
    mb->Append(math, "&Math");

    // ---- Scientific ----
    auto* sci = new wxMenu();
    auto* trig = new wxMenu();
    add_action(trig, "sin",     "S");
    add_action(trig, "cos",     "C");
    add_action(trig, "tan",     "T");
    trig->AppendSeparator();
    add_action(trig, "arcsin",  "IS");
    add_action(trig, "arccos",  "IC");
    add_action(trig, "arctan",  "IT");
    trig->AppendSeparator();
    add_action(trig, "sinh",    "HS");
    add_action(trig, "cosh",    "HC");
    add_action(trig, "tanh",    "HT");
    trig->AppendSeparator();
    add_action(trig, "arsinh",  "IHS");
    add_action(trig, "arcosh",  "IHC");
    add_action(trig, "artanh",  "IHT");
    trig->AppendSeparator();
    add_action(trig, "atan2(y,x)", "fT");
    sci->AppendSubMenu(trig, "&Trigonometric");

    auto* logs = new wxMenu();
    add_action(logs, "ln",          "L");
    add_action(logs, "exp",         "E");
    add_action(logs, "log10",       "HL");
    add_action(logs, "10^x",        "IHL");
    add_action(logs, "log_b(a)",    "B");
    add_action(logs, "b^a (alog)",  "IB");
    add_action(logs, "expm1 (e^x-1)","fE");
    add_action(logs, "lnp1 (ln(1+x))","fL");
    sci->AppendSubMenu(logs, "&Logarithmic");

    auto* combo = new wxMenu();
    add_action(combo, "Factorial",       "!");
    add_action(combo, "Double Factorial","kd");
    add_action(combo, "Choose (n,m)",    "kc");
    add_action(combo, "Permutations",    "Hkc");
    sci->AppendSubMenu(combo, "Combina&torics");

    sci->AppendSeparator();
    add_action(sci, "Integer Log (floor log_b(a))", "fI");
    add_action(sci, "Gamma Function",               "fg");
    mb->Append(sci, "&Scientific");

    // ---- Number Theory ----
    auto* nt = new wxMenu();
    add_action(nt, "GCD", "kg");
    add_action(nt, "LCM", "kl");
    nt->AppendSeparator();
    add_action(nt, "Random in [0, n)", "kr");
    nt->AppendSeparator();
    add_action(nt, "Next Prime",     "kn");
    add_action(nt, "Previous Prime", "Ikn");
    add_action(nt, "Prime Test",     "kp");
    add_action(nt, "Prime Factors",  "kf");
    add_action(nt, "Euler's Totient","kt");
    nt->AppendSeparator();
    add_action(nt, "Extended GCD",   "ke");
    add_action(nt, "Modular Power",  "km");
    mb->Append(nt, "&Number Theory");

    // ---- Vector ----
    auto* vec = new wxMenu();
    auto* vc = new wxMenu();
    add_action(vc, "Index Vector [1..n]", "vx");
    add_action(vc, "Identity Matrix",     "vi");
    add_action(vc, "Diagonal Matrix",     "vd");
    vec->AppendSubMenu(vc, "&Construct");

    auto* vm = new wxMenu();
    add_action(vm, "Length",            "vl");
    add_action(vm, "Reverse",           "vv");
    add_action(vm, "Sort Ascending",    "vr");
    add_action(vm, "Sort Descending",   "Ivr");
    add_action(vm, "Head",              "vh");
    add_action(vm, "Tail",              "Ivh");
    add_action(vm, "Prepend (cons)",    "vk");
    add_action(vm, "Transpose",         "vt");
    vec->AppendSubMenu(vm, "&Manipulate");

    auto* vr = new wxMenu();
    add_action(vr, "Sum",     "V+");
    add_action(vr, "Product", "V*");
    add_action(vr, "Maximum", "VX");
    add_action(vr, "Minimum", "VN");
    vec->AppendSubMenu(vr, "&Reduce");

    auto* la = new wxMenu();
    add_action(la, "Dot Product",    "VO");
    add_action(la, "Cross Product",  "VC");
    add_action(la, "Determinant",    "VD");
    add_action(la, "Trace",          "VT");
    add_action(la, "Matrix Inverse", "v&");
    vec->AppendSubMenu(la, "&Linear Algebra");
    mb->Append(vec, "&Vector");

    // ---- Bitwise ----
    auto* bit = new wxMenu();
    add_action(bit, "AND", "ba");
    add_action(bit, "OR",  "bo");
    add_action(bit, "XOR", "bx");
    add_action(bit, "NOT", "bn");
    bit->AppendSeparator();
    add_action(bit, "Left Shift",    "bl");
    add_action(bit, "Right Shift",   "br");
    add_action(bit, "Rotate Left",   "bt");
    add_action(bit, "Rotate Right",  "Ibt");
    bit->AppendSeparator();
    add_action(bit, "Clip to Word Size",      "bc");
    add_action(bit, "Set Word Size from Top", "bw");
    mb->Append(bit, "&Bitwise");

    // ---- Variables ----
    // Each item leaves the controller in "waiting for variable name"
    // state (mode line shows e.g. "store ?"). The user then presses a
    // letter to complete the operation.
    auto* var = new wxMenu();
    add_action(var, "&Store...",        "ss");
    add_action(var, "Store and Po&p...","st");
    add_action(var, "&Recall...",       "sr");
    add_action(var, "&Exchange...",     "sx");
    add_action(var, "&Unstore...",      "su");
    var->AppendSeparator();
    add_action(var, "Add To...",        "s+");
    add_action(var, "Subtract From...", "s-");
    add_action(var, "Multiply Into...", "s*");
    add_action(var, "Divide Into...",   "s/");
    var->AppendSeparator();
    auto* qstore = new wxMenu();
    auto* qrecall = new wxMenu();
    static const char qkeys[10][3] = {"t0","t1","t2","t3","t4","t5","t6","t7","t8","t9"};
    static const char rkeys[10][3] = {"r0","r1","r2","r3","r4","r5","r6","r7","r8","r9"};
    for (int i = 0; i < 10; ++i) {
        add_action(qstore,  wxString::Format("q%d", i), qkeys[i]);
        add_action(qrecall, wxString::Format("q%d", i), rkeys[i]);
    }
    var->AppendSubMenu(qstore,  "&Quick Store");
    var->AppendSubMenu(qrecall, "Q&uick Recall");
    mb->Append(var, "Va&riables");

    // ---- Modes ----
    auto* mode = new wxMenu();
    auto* ang = new wxMenu();
    add_action(ang, "Degrees", "md");
    add_action(ang, "Radians", "mr");
    mode->AppendSubMenu(ang, "&Angular");

    auto* fmt = new wxMenu();
    add_action(fmt, "Normal",       "dn");
    add_action(fmt, "Fixed-Point",  "df");
    add_action(fmt, "Scientific",   "ds");
    add_action(fmt, "Engineering",  "de");
    mode->AppendSubMenu(fmt, "Display &Format");

    auto* rad = new wxMenu();
    add_action(rad, "Binary",      "d2");
    add_action(rad, "Octal",       "d8");
    add_action(rad, "Decimal",     "d0");
    add_action(rad, "Hexadecimal", "d6");
    add_action(rad, "Custom from Top of Stack", "dr");
    mode->AppendSubMenu(rad, "&Radix");

    auto* cpx = new wxMenu();
    add_action(cpx, "(re, im) Pair", "dc");
    add_action(cpx, "re + im*i",     "di");
    add_action(cpx, "re + im*j",     "dj");
    mode->AppendSubMenu(cpx, "&Complex Display");

    mode->AppendSeparator();
    add_action(mode, "Toggle Fraction Mode", "mf");
    add_action(mode, "Toggle Symbolic Mode", "ms");
    add_action(mode, "Toggle Infinite Mode", "mi");
    add_action(mode, "Toggle Digit Grouping","dg");
    add_action(mode, "Toggle Leading Zeros", "dz");
    mode->AppendSeparator();
    add_action(mode, "Set Precision from Top", "p");
    add_action(mode, "Set Word Size from Top", "bw");
    mb->Append(mode, "&Modes");

    // ---- Trail ----
    // The trail panel is always visible on the right; these items move
    // the trail pointer or yank/kill at the pointer.
    auto* trail = new wxMenu();
    add_action(trail, "&Yank Selected onto Stack", "ty");
    trail->AppendSeparator();
    add_action(trail, "Go to &First",   "t[");
    add_action(trail, "Go to &Last",    "t]");
    add_action(trail, "Pre&vious Entry","tp");
    add_action(trail, "&Next Entry",    "tn");
    trail->AppendSeparator();
    add_action(trail, "&Delete Entry",  "tk");
    mb->Append(trail, "&Trail");

    // ---- Help ----
    auto* help = new wxMenu();
    help->Append(wxID_ABOUT, "&About stackcalc");
    mb->Append(help, "&Help");

    SetMenuBar(mb);

    // One handler covers all dispatch IDs; specific ones get their own.
    Bind(wxEVT_MENU, &StackCalcFrame::on_quit,        this, wxID_EXIT);
    Bind(wxEVT_MENU, &StackCalcFrame::on_about,       this, wxID_ABOUT);
    Bind(wxEVT_MENU, [this](wxCommandEvent&) {
        // Reset bypasses the runner (it's a single fast mutation), but
        // we still have to honour the busy guard — otherwise it'd race
        // with an in-flight worker job touching the same controller.
        if (panel_->is_busy()) return;
        panel_->controller().reset();
        panel_->refresh_display();
        panel_->focus_calc();
    }, ID_Reset);
    Bind(wxEVT_MENU, &StackCalcFrame::on_menu_dispatch,
         this, ID_DispatchBase, ID_DispatchEnd);

    // Special-key catcher (see on_char_hook for the rationale).
    Bind(wxEVT_CHAR_HOOK, &StackCalcFrame::on_char_hook, this);
}

void StackCalcFrame::on_char_hook(wxKeyEvent& e) {
    // Ctrl combinations: let menu accelerators (Ctrl+Z/Y/Q) and the rich
    // text widget's defaults (Ctrl+A/C) win.
    if (e.ControlDown() || e.RawControlDown()) {
        e.Skip();
        return;
    }

    int code = e.GetKeyCode();

    // Function keys: reserved for future menu accelerators; pass through.
    if (code >= WXK_F1 && code <= WXK_F12) {
        e.Skip();
        return;
    }

    // Calculator special keys. These are unambiguous virtual key codes,
    // unaffected by the unicode-translation issue that affects shifted
    // printable characters in CHAR_HOOK on macOS. Catching them here
    // prevents the focused DataView from swallowing them when it has
    // focus from a click-to-select. Alt+Enter / Alt+Tab go through too
    // (the panel's on_key_down checks the alt modifier).
    if (code == WXK_RETURN || code == WXK_NUMPAD_ENTER ||
        code == WXK_BACK   || code == WXK_DELETE       ||
        code == WXK_TAB    || code == WXK_SPACE        ||
        code == WXK_ESCAPE) {
        panel_->on_key_down(e);
        return;
    }

    // Printable characters: skip and let the normal EVT_CHAR routing
    // (via the bindings on stack_ctrl_ and the panel) deliver them
    // with unicode translation already done.
    e.Skip();
}

void StackCalcFrame::on_about(wxCommandEvent&) {
    // Use wide string literals (via wxT) for any text containing
    // non-ASCII characters. wxString's narrow-string constructor
    // defaults to wxConvLibc, which on Windows is the system "ANSI"
    // code page (Windows-1252) — that mangles UTF-8 bytes into
    // mojibake. Wide literals are converted by the compiler (with
    // /utf-8 on MSVC), so wx receives wchar_t directly with no
    // runtime conversion.
    // wxT() is a token-pasting macro that uses double-indirection so a
    // macro argument (STACKCALC_VERSION) gets expanded BEFORE L is
    // pasted onto its value — i.e. wxT(STACKCALC_VERSION) becomes
    // L"0.1.0", not LSTACKCALC_VERSION. That's the standard way to
    // splice a generated string macro into wide literal concatenation.
    wxMessageBox(
        wxT("stackcalc ") wxT(STACKCALC_VERSION) wxT(" — RPN calculator\n\n")
        wxT("An Emacs M-x calc clone with arbitrary-precision arithmetic.\n")
        wxT("See manual.md in the source tree for the full reference.\n\n")
        wxT("https://github.com/skeeto/stackcalc\n\n")
        wxT("Built with:\n")
        wxT("  • GMP 6.3.0           LGPL v3 (or GPL v2)\n")
        wxT("  • MPFR 4.2.2          LGPL v3 or later\n")
        wxT("  • wxWidgets 3.2.10    wxWindows Library Licence v3.1"),
        wxT("About stackcalc"), wxOK | wxICON_INFORMATION, this);
}

void StackCalcFrame::on_quit(wxCommandEvent&) {
    Close(true);
}

// === CalcPanel ================================================================

CalcPanel::CalcPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
              wxWANTS_CHARS | wxFULL_REPAINT_ON_RESIZE)
{
    // Monospace font for the entry display and the stack widget. Try
    // platform-appropriate face names in order; wxFont silently
    // substitutes when a face is missing (still IsOk(), just
    // wrong-looking), so verify against the system font list before
    // committing. Falls back to the TELETYPE family if nothing on
    // the list is installed.
    for (const wxString& face :
         { "Menlo", "Consolas", "DejaVu Sans Mono", "Courier New" }) {
        if (!wxFontEnumerator::IsValidFacename(face)) continue;
        wxFont f(wxFontInfo(13).FaceName(face));
        if (f.IsOk()) { mono_font_ = f; break; }
    }
    if (!mono_font_.IsOk()) {
        mono_font_ = wxFont(13, wxFONTFAMILY_TELETYPE,
                            wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    }

    // Top: two single-line wxStaticText widgets. The message line is
    // hidden when there's nothing to say; the entry line displays the
    // in-progress number-entry buffer and is otherwise empty. No
    // hand-painted blinking cursor anymore — the text appearing as
    // the user types IS the visual cue that input is being captured.
    // Reserve a single line of vertical space for the message even
    // when no message is showing — otherwise transient notifications
    // ("Probably prime", "Cancelled — operation undone") push the
    // stack down whenever they appear and pull it back up when they
    // clear, which is disorienting. Set the min size from the GUI
    // font's line height once at construction.
    message_text_ = new wxStaticText(this, wxID_ANY, wxEmptyString);
    message_text_->SetForegroundColour(kErrorColor);
    {
        wxClientDC dc(this);
        message_text_->SetMinSize(wxSize(-1, dc.GetCharHeight()));
    }

    entry_text_ = new wxStaticText(this, wxID_ANY, wxEmptyString);
    entry_text_->SetFont(mono_font_);

    // Middle: native list view for the stack. Two columns (index,
    // value), platform-default look (NSTableView on macOS, ListView
    // on Windows, GtkTreeView on Linux), native row-based selection
    // and copy.
    stack_ctrl_ = new wxDataViewListCtrl(
        this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxDV_MULTIPLE | wxDV_ROW_LINES | wxDV_NO_HEADER | wxBORDER_NONE);

    // Swap in our custom store BEFORE adding columns. AssociateModel
    // transfers ownership through wx refcounting; the built-in store
    // installed by wxDataViewListCtrl's ctor is DecRef'd and
    // destroyed.
    {
        auto* store = new MarkedListStore();
        stack_ctrl_->AssociateModel(store);
        store->DecRef();
    }

    // Set the monospace font on the control BEFORE adding columns.
    // wxWidgets' Cocoa DataView captures each column's NSCell font
    // from the control's font at the moment AppendTextColumn runs
    // (src/osx/cocoa/dataview.mm:444); a SetFont after the fact
    // doesn't retroactively update existing cells.
    stack_ctrl_->SetFont(mono_font_);

    // Two columns: right-aligned fixed-width index (CELL_INERT —
    // never editable, never selectable into the cell), left-aligned
    // stretching value (CELL_EDITABLE — double-click pops up the
    // native in-place editor with the full value text, the user can
    // drag-select any substring and Cmd+C / Ctrl+C copies it; we
    // veto the edit on commit so the actual stack value is never
    // changed by this affordance — see on_dataview_edit_done below).
    stack_ctrl_->AppendTextColumn(wxEmptyString, wxDATAVIEW_CELL_INERT,
        FromDIP(56), wxALIGN_RIGHT, 0);
    stack_ctrl_->AppendTextColumn(wxEmptyString, wxDATAVIEW_CELL_EDITABLE,
        -1, wxALIGN_LEFT, wxDATAVIEW_COL_RESIZABLE);
    stack_ctrl_->Bind(wxEVT_DATAVIEW_ITEM_EDITING_DONE,
                      &CalcPanel::on_dataview_edit_done, this);
    // Per-platform binding for opening the in-place editor:
    //   macOS: LEFT_DCLICK. wxEVT_DATAVIEW_ITEM_ACTIVATED ALSO
    //     fires from Enter on macOS (NSTableView consumes Enter at
    //     the NSResponder level before wx's CHAR_HOOK gets a chance,
    //     so we can't even detect it to filter), and using it would
    //     hijack the calculator's RET keystroke.
    //   Windows / generic: ITEM_ACTIVATED. The generic backend
    //     consumes the second click of a fast double-click in its
    //     selection logic, so LEFT_DCLICK requires three clicks
    //     instead of two — but ITEM_ACTIVATED works in one
    //     double-click and Enter is properly absorbed by CHAR_HOOK
    //     before reaching it.
#ifdef __WXOSX__
    stack_ctrl_->Bind(wxEVT_LEFT_DCLICK,
                      &CalcPanel::on_dataview_dclick, this);
#else
    stack_ctrl_->Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED,
                      &CalcPanel::on_dataview_activate, this);
#endif

    // Right-click context menu — Copy / Copy Substring / Select All.
    stack_ctrl_->Bind(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU,
                      &CalcPanel::on_stack_context_menu, this);

    // Mode line + flag indicators live on the frame's wxStatusBar
    // (created in StackCalcFrame's ctor); no panel-level child
    // widget for them anymore.

    auto* sizer = new wxBoxSizer(wxVERTICAL);
    int pad = FromDIP(4);
    sizer->Add(message_text_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, pad);
    sizer->Add(entry_text_,   0, wxEXPAND | wxLEFT | wxRIGHT,         pad);
    sizer->Add(stack_ctrl_,   1, wxEXPAND | wxTOP,                    pad);
    SetSizer(sizer);

    // Calculator key handling. Bind on the panel itself, AND on the
    // stack widget — when the user clicks into the stack to make a
    // row selection, focus stays there, but subsequent typing still
    // needs to reach the calculator. The frame's CHAR_HOOK
    // intercepts special keys (RET / TAB / etc.) before DataView can
    // consume them for navigation/activation.
    Bind(wxEVT_KEY_DOWN, &CalcPanel::on_key_down,   this);
    Bind(wxEVT_CHAR,     &CalcPanel::on_char,       this);
    // Single timer now (the blink timer is gone with the custom-
    // painted entry line). Distinct ID is unnecessary but kept for
    // local clarity.
    constexpr int kOverlayTimerId = 30002;
    overlay_timer_.SetOwner(this, kOverlayTimerId);
    Bind(wxEVT_TIMER, &CalcPanel::on_overlay_tick, this, kOverlayTimerId);
    stack_ctrl_->Bind(wxEVT_KEY_DOWN, &CalcPanel::on_key_down, this);
    stack_ctrl_->Bind(wxEVT_CHAR,     &CalcPanel::on_char,     this);

    // Cmd/Ctrl+C: copy selected stack values (just the values, one
    // per line) to the clipboard. wxACCEL_CTRL maps to Cmd on macOS.
    // wxDataViewCtrl handles Cmd+A (select all) natively.
    stack_ctrl_->Bind(wxEVT_MENU, &CalcPanel::on_stack_copy,
                      this, wxID_COPY);
    {
        wxAcceleratorEntry copy_entry(wxACCEL_CTRL, (int)'C', wxID_COPY);
        wxAcceleratorTable copy_accel(1, &copy_entry);
        stack_ctrl_->SetAcceleratorTable(copy_accel);
    }

    // Initial display snapshot so paint code has something to read
    // before the first command runs.
    cached_display_ = ctrl_.display();
    update_stack();
    // Blink timer is only started while number entry is active — see redraw().
}

CalcPanel::~CalcPanel() {
    // Disarm callbacks first. The runner_ destructor (which fires next
    // when members tear down) will cancel and join the worker; if the
    // worker's last job calls on_done, the captured shared_ptr<bool>
    // alive flag is now false and the lambda bails out without
    // touching `this`. Same protection for any wxCallAfter already
    // queued — it'll see alive==false when the UI loop processes it.
    alive_->store(false);
}

void CalcPanel::dispatch_keys(const std::string& keys) {
    // Used by menu items that "press a sequence of keys for you"
    // (e.g. Math > Add → "+"). Routes through the runner so
    // long-running menu commands are still cancellable.
    if (busy_) return;
    submit_work([this, keys]{
        for (char c : keys) {
            try { ctrl_.process_key(sc::KeyEvent::character(c)); }
            catch (...) { /* controller already absorbs std::exception */ }
        }
    });
}

void CalcPanel::dispatch_key(const sc::KeyEvent& k) {
    if (busy_) return;
    // Menu commands always dispatch the literal key the menu specified
    // — they don't honour Esc-as-meta, since clicking a menu while
    // meta is pending shouldn't silently mutate the dispatched key.
    // Clear the pending state so the user isn't stuck in meta mode
    // after taking a menu action.
    if (pending_meta_) { pending_meta_ = false; update_status_bar(); }
    submit_work([this, k]{
        try { ctrl_.process_key(k); }
        catch (...) { /* controller absorbs std::exception */ }
    });
}

void CalcPanel::dispatch_with_meta(sc::KeyEvent k) {
    if (pending_meta_) {
        pending_meta_ = false;
        update_status_bar();
        // Promote to M-prefixed form. Already-Modified events fall
        // through unchanged (no double-meta).
        if (k.type == sc::KeyEvent::Char) {
            k = sc::KeyEvent::modified(std::string("M-") + k.ch);
        } else if (k.type == sc::KeyEvent::Special) {
            k = sc::KeyEvent::modified("M-" + k.name);
        }
    }
    submit_work([this, k]{
        try { ctrl_.process_key(k); }
        catch (...) { /* controller absorbs std::exception */ }
    });
}

// Helper: submit work through the runner. Returns immediately; the
// on_done callback (worker thread) builds the display snapshot and
// then marshals to the UI via wxCallAfter for the actual repaint.
//
// Both the work AND the formatting are cancellable:
//
//   - work() runs in the runner's own Section (set up by CalcRunner).
//     Ctrl+G during compute trips the next GMP allocation.
//
//   - display() is wrapped in a fresh Section here. Ctrl+G during
//     formatting trips an mpz_get_str allocation. On cancel, we undo
//     the just-completed work and reuse the pre-submit snapshot
//     (captured into pre_snapshot) — no re-format needed, so the
//     post-cancel UI update is instant.
//
// pre_snapshot is the cached display from BEFORE submitting; the
// stack at that moment matches the begin_command snapshot the
// controller's undo machinery saved, so undo + pre_snapshot keeps
// the displayed state and the actual stack consistent.
//
// The alive flag (captured by value) gates both the worker callback
// and the UI-thread callback against `this` having been destroyed
// while the job was in flight (e.g. user closes the app mid-calc).
void CalcPanel::submit_work(std::function<void()> work) {
    busy_ = true;
    overlay_timer_.StartOnce(150);  // delay before "Computing…" overlay
    auto alive        = alive_;
    auto snapshot     = std::make_shared<sc::DisplayState>();
    auto pre_snapshot = std::make_shared<sc::DisplayState>(cached_display_);
    runner_.submit(
        [this, work = std::move(work)] { work(); },
        [this, alive, snapshot, pre_snapshot](
                sc::CalcRunner::Outcome, std::exception_ptr) {
            if (!alive->load()) return;

            // Format the new state. The runner cleared the cancel
            // flag before invoking on_done, so this Section starts
            // clean — but a fresh Ctrl+G arriving NOW will set it
            // and trip mpz_get_str's allocations.
            bool format_cancelled = false;
            {
                sc::cancel::Section s;
                try {
                    *snapshot = ctrl_.display();
                } catch (const sc::CancelledException&) {
                    format_cancelled = true;
                }
            }

            if (format_cancelled) {
                // User asked to abort while formatting. Undo the
                // work() so the stack matches pre_snapshot, then use
                // pre_snapshot directly (no second format pass).
                sc::cancel::reset();
                try { ctrl_.process_key(sc::KeyEvent::character('U')); }
                catch (...) {}
                *snapshot = *pre_snapshot;
                snapshot->message = "Cancelled — operation undone";
            }

            CallAfter([this, alive, snapshot]{
                if (!alive->load()) return;
                apply_snapshot_and_redraw(std::move(*snapshot));
            });
        });
}

void CalcPanel::refresh_display() {
    cached_display_ = ctrl_.display();
    redraw();
}

void CalcPanel::apply_snapshot_and_redraw(sc::DisplayState snapshot) {
    busy_ = false;
    overlay_timer_.Stop();
    computing_overlay_visible_ = false;
    cached_display_ = std::move(snapshot);
    clear_selections();
    redraw();
}

void CalcPanel::on_overlay_tick(wxTimerEvent&) {
    if (busy_) {
        computing_overlay_visible_ = true;
        update_stack();  // repaints the stack widget with overlay text
    }
}

// Drop focus onto the actual focusable child. wxPanel itself can be
// focused but that doesn't help on Windows, where the OS-managed focus
// after Show() prefers a real control. Targeting stack_ctrl_ keeps
// keystrokes flowing through the CHAR_HOOK + EVT_CHAR pipeline.
void CalcPanel::focus_calc() {
    if (stack_ctrl_) stack_ctrl_->SetFocus();
}

void CalcPanel::redraw() {
    // Message line: always laid out at one line tall (the min size is
    // set in the ctor). Just update the text — empty string when
    // there's no message — so transient notifications appear in
    // place without reflowing the rest of the panel.
    const auto& ds = cached_display_;
    message_text_->SetLabel(wxString::FromUTF8(ds.message.c_str()));

    // Entry line: shows the in-progress number-entry buffer (e.g.
    // "127" while typing). Empty when nothing is being entered.
    entry_text_->SetLabel(wxString::FromUTF8(ds.entry_text.c_str()));

    update_stack();
    update_status_bar();
    if (trail_panel_) trail_panel_->refresh_from_state();
}

void CalcPanel::update_status_bar() {
    auto* frame = wxGetTopLevelParent(this);
    if (!frame) return;
    auto* sb = static_cast<wxFrame*>(frame)->GetStatusBar();
    if (!sb) return;

    const auto& ds = cached_display_;

    // Field 0: mode line ("12 Deg", "12 Rad", etc.)
    sb->SetStatusText(wxString::FromUTF8(ds.mode_line.c_str()), 0);

    // Field 1: flag indicators in [bracket] form. Includes the
    // GUI-side meta-prefix indicator (Esc-as-meta one-shot, separate
    // from the controller's pending_prefix) at the end.
    std::string flags;
    if (ds.inverse_flag)            flags += " [I]";
    if (ds.hyperbolic_flag)         flags += " [H]";
    if (ds.keep_args_flag)          flags += " [K]";
    if (!ds.pending_prefix.empty()) flags += " [" + ds.pending_prefix + "-]";
    if (pending_meta_)              flags += " [M-]";
    sb->SetStatusText(wxString::FromUTF8(flags.c_str()), 1);
}

void CalcPanel::update_stack() {
    stack_ctrl_->Freeze();
    stack_ctrl_->DeleteAllItems();

    if (computing_overlay_visible_) {
        // While a long calculation is running, replace the stack
        // contents with a single yellow "Computing…" row so the
        // user has visual feedback that input is being held.
        // Restored when refresh_cache_and_redraw fires.
        wxVector<wxVariant> row;
        row.push_back(wxVariant(wxString()));
        row.push_back(wxVariant(
            wxString(wxT("Computing…  (Ctrl+G to cancel)"))));
        stack_ctrl_->AppendItem(row, /*data=*/1);  // 1 = render yellow
    } else {
        // Top-of-stack rendered at the TOP of the widget, growing
        // downward. cached_display_.stack_entries[back()] is the top;
        // iterate in reverse so row 0 = top of stack.
        const auto& entries = cached_display_.stack_entries;
        int n = static_cast<int>(entries.size());

        for (int j = 0; j < n; ++j) {
            int idx = n - 1 - j;
            int label = j + 1;
            const std::string& s = entries[idx];

            // Yellow attribute when the formatter emitted a placeholder
            // ("<integer: N digits>"). Real values never start with '<'.
            wxUIntPtr marker =
                (!s.empty() && s.front() == '<' && s.back() == '>') ? 1 : 0;

            wxVector<wxVariant> row;
            row.push_back(wxVariant(wxString::Format("%3d:", label)));
            row.push_back(wxVariant(wxString::FromUTF8(s.c_str())));
            stack_ctrl_->AppendItem(row, marker);
        }
    }

    stack_ctrl_->Thaw();

    // Scroll so row 0 (top of stack) is visible. Skipped on empty
    // stack to avoid passing an invalid row to RowToItem.
    if (stack_ctrl_->GetItemCount() > 0) {
        stack_ctrl_->EnsureVisible(stack_ctrl_->RowToItem(0));
    }
}

void CalcPanel::on_char(wxKeyEvent& e) {
    int uc = e.GetUnicodeKey();
    if (uc == WXK_NONE || uc < 0x20 || uc >= 0x7f) return;  // ignore controls

    if (busy_) return;  // only the cancel keystroke (handled in on_key_down) reaches here

    dispatch_with_meta(sc::KeyEvent::character(static_cast<char>(uc)));
    // Don't Skip — consumed
}

void CalcPanel::on_key_down(wxKeyEvent& e) {
    bool ctrl = e.ControlDown() || e.RawControlDown();
    bool alt  = e.AltDown();

    int code = e.GetKeyCode();
    bool handled = true;

    // Ctrl+G is always available — even (especially) while a
    // calculation is in flight. Sets the cancel flag; the worker's
    // next GMP allocation throws CancelledException.
    if (ctrl && code == 'G') {
        runner_.request_cancel();
        return;
    }

    // While the worker is busy, ignore every other keystroke. Letting
    // them queue would buffer indefinitely if the user mashes keys
    // during a long calc. They get their attention via the
    // "Computing…" overlay (after 150 ms).
    if (busy_) {
        e.Skip();  // let menu accelerators (e.g. Cmd+Q) still work
        return;
    }

    // Ctrl/Cmd-modified keys are reserved for the OS and menu
    // accelerators (Cmd+Q quit, Cmd+W close, Cmd+Tab app switcher,
    // Cmd+C copy, etc.). Only the two we deliberately mapped onto
    // Ctrl modifiers — Z and Y for undo/redo — are claimed here;
    // everything else falls through.
    if (ctrl) {
        if (code == 'Z') { dispatch_with_meta(sc::KeyEvent::character('U')); return; }
        if (code == 'Y') { dispatch_with_meta(sc::KeyEvent::character('D')); return; }
        e.Skip();
        return;
    }

    switch (code) {
        case WXK_RETURN:
        case WXK_NUMPAD_ENTER: {
            auto k = alt ? sc::KeyEvent::modified("M-RET")
                         : sc::KeyEvent::special("RET");
            dispatch_with_meta(k);
            break;
        }
        case WXK_BACK:
            // Backspace is special: it can mean "edit the entry" OR
            // "drop top of stack" depending on whether entry is
            // active. Both go through the runner; meta-prefix
            // doesn't apply to either.
            if (pending_meta_) { pending_meta_ = false; update_status_bar(); }
            submit_work([this]{
                if (!ctrl_.process_key(sc::KeyEvent::character('\b')))
                    ctrl_.process_key(sc::KeyEvent::special("DEL"));
            });
            break;
        case WXK_DELETE:
            dispatch_with_meta(sc::KeyEvent::special("DEL"));
            break;
        case WXK_TAB: {
            auto k = alt ? sc::KeyEvent::modified("M-TAB")
                         : sc::KeyEvent::special("TAB");
            dispatch_with_meta(k);
            break;
        }
        case WXK_SPACE:
            dispatch_with_meta(sc::KeyEvent::special("SPC"));
            break;
        case WXK_ESCAPE:
            // Esc has two roles. If number entry is active, cancel
            // it (and clear any pending meta — single press to clean
            // up). Otherwise, toggle the Esc-as-meta one-shot: the
            // next non-Esc key will be M-prefixed (Emacs convention,
            // workaround for Alt+Tab being eaten by the Windows app
            // switcher). A second Esc cancels the meta state.
            if (cached_display_.input_active) {
                if (pending_meta_) { pending_meta_ = false; }
                submit_work([this]{
                    if (ctrl_.input().active()) ctrl_.input().cancel();
                });
            } else {
                pending_meta_ = !pending_meta_;
                update_status_bar();
            }
            break;
        default:
            handled = false;
            break;
    }

    if (handled) {
        // Don't Skip — we consumed it. EVT_CHAR won't fire. The
        // submitted work's on_done callback will repaint via
        // refresh_cache_and_redraw.
    } else {
        e.Skip();  // let EVT_CHAR translate this key
    }
}

void CalcPanel::clear_selections() {
    if (stack_ctrl_)  stack_ctrl_->UnselectAll();
    if (trail_panel_) trail_panel_->UnselectAll();
    // Pull focus off whichever DataView child might own it, onto
    // the panel. The panel has wxWANTS_CHARS and the same
    // on_char / on_key_down bindings, so subsequent typing routes
    // back through the calculator regardless of which list got
    // clicked.
    if (FindFocus() != this) SetFocus();
}

void CalcPanel::on_stack_copy(wxCommandEvent&) {
    wxDataViewItemArray sel;
    stack_ctrl_->GetSelections(sel);
    if (sel.empty()) return;

    // Build text in row order (top-of-stack first since that's how
    // rows are sorted in the widget).
    std::vector<int> rows;
    rows.reserve(sel.size());
    for (const auto& item : sel) {
        rows.push_back(stack_ctrl_->ItemToRow(item));
    }
    std::sort(rows.begin(), rows.end());

    wxString text;
    for (size_t i = 0; i < rows.size(); ++i) {
        if (i) text += wxT("\n");
        // Column 1 is the value (column 0 is the index label).
        text += stack_ctrl_->GetTextValue(rows[i], 1);
    }

    if (wxTheClipboard->Open()) {
        wxTheClipboard->SetData(new wxTextDataObject(text));
        wxTheClipboard->Close();
    }
}

void CalcPanel::on_dataview_edit_done(wxDataViewEvent& e) {
    // The CELL_EDITABLE columns on stack_ctrl_ and trail_panel_
    // exist purely so the user can pop up the native in-place
    // editor over a value cell, drag-select a substring inside it,
    // and Cmd+C to copy. The actual stack/trail isn't editable from
    // the GUI — vetoing the commit reverts whatever the user typed.
    e.Veto();
}

void CalcPanel::on_stack_context_menu(wxDataViewEvent& e) {
    // Native idiom: if the user right-clicks a row that isn't in
    // the current selection, switch the selection to just that row
    // before showing the menu — so menu actions operate on what was
    // clicked, not whatever was selected before.
    auto item = e.GetItem();
    if (item.IsOk() && !stack_ctrl_->IsSelected(item)) {
        stack_ctrl_->UnselectAll();
        stack_ctrl_->Select(item);
    }

    wxMenu menu;
    menu.Append(wxID_COPY, wxT("Copy\tCtrl+C"));
    auto* substring = menu.Append(wxID_ANY,
                                  wxT("Copy Value as Substring…"));
    menu.AppendSeparator();
    menu.Append(wxID_SELECTALL, wxT("Select All\tCtrl+A"));

    // Disable items that have nothing to act on.
    menu.Enable(wxID_COPY, stack_ctrl_->GetSelectedItemsCount() > 0);
    menu.Enable(substring->GetId(), item.IsOk());
    menu.Enable(wxID_SELECTALL, stack_ctrl_->GetItemCount() > 0);

    menu.Bind(wxEVT_MENU, &CalcPanel::on_stack_copy, this, wxID_COPY);
    menu.Bind(wxEVT_MENU, [this, item](wxCommandEvent&) {
        if (auto* col = stack_ctrl_->GetColumn(1))
            stack_ctrl_->EditItem(item, col);
    }, substring->GetId());
    menu.Bind(wxEVT_MENU, [this](wxCommandEvent&) {
        stack_ctrl_->SelectAll();
    }, wxID_SELECTALL);

    PopupMenu(&menu);
}

void CalcPanel::on_dataview_activate(wxDataViewEvent& e) {
    // Bound only on non-macOS platforms (see ctor for per-platform
    // rationale). On Windows / generic the activation event fires
    // on mouse double-click only — Enter is absorbed by the frame's
    // CHAR_HOOK before DataView sees it.
    auto* ctrl = static_cast<wxDataViewCtrl*>(e.GetEventObject());
    if (!ctrl) return;
    auto item = e.GetItem();
    if (!item.IsOk()) return;
    auto* edit_col = ctrl->GetColumn(1);  // value column
    if (!edit_col) return;
    ctrl->EditItem(item, edit_col);
}

void CalcPanel::on_dataview_dclick(wxMouseEvent& e) {
    // Bound only on macOS (see ctor for per-platform rationale).
    // Mouse double-click bypasses the NSTableView Enter activation
    // we can't otherwise filter.
    auto* ctrl = static_cast<wxDataViewCtrl*>(e.GetEventObject());
    if (!ctrl) { e.Skip(); return; }
    wxDataViewItem item;
    wxDataViewColumn* hit_col = nullptr;
    ctrl->HitTest(e.GetPosition(), item, hit_col);
    if (!item.IsOk()) { e.Skip(); return; }
    auto* edit_col = ctrl->GetColumn(1);  // value column
    if (!edit_col) { e.Skip(); return; }
    ctrl->EditItem(item, edit_col);
}

// === TrailPanel: right-pane trail viewer =====================================

TrailPanel::TrailPanel(wxWindow* parent, CalcPanel* host)
    : wxDataViewListCtrl(parent, wxID_ANY,
                         wxDefaultPosition, wxDefaultSize,
                         wxDV_MULTIPLE | wxDV_ROW_LINES |
                         wxDV_NO_HEADER | wxBORDER_NONE)
    , host_(host)
{
    // Same custom store as the stack — one row attribute (yellow vs
    // platform default) keyed on the wxUIntPtr data slot.
    {
        auto* store = new MarkedListStore();
        AssociateModel(store);
        store->DecRef();
    }

    // Font BEFORE the column (Cocoa captures column NSCell font from
    // the control's font at AppendTextColumn time).
    SetFont(host->mono_font());

    // Two columns: tag (operation name — fixed width, INERT) and
    // value (formatted result — stretches, EDITABLE for substring-
    // copy via the native in-place editor; commits are vetoed —
    // see CalcPanel::on_dataview_edit_done).
    AppendTextColumn(wxEmptyString, wxDATAVIEW_CELL_INERT,
                     FromDIP(72), wxALIGN_LEFT, 0);
    AppendTextColumn(wxEmptyString, wxDATAVIEW_CELL_EDITABLE,
                     -1, wxALIGN_LEFT, wxDATAVIEW_COL_RESIZABLE);
    Bind(wxEVT_DATAVIEW_ITEM_EDITING_DONE,
         &CalcPanel::on_dataview_edit_done, host_);
    // See stack_ctrl_'s same binding for the per-platform rationale.
#ifdef __WXOSX__
    Bind(wxEVT_LEFT_DCLICK,
         &CalcPanel::on_dataview_dclick, host_);
#else
    Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED,
         &CalcPanel::on_dataview_activate, host_);
#endif

    // Right-click context menu — same shape as stack.
    Bind(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU,
         &TrailPanel::on_context_menu, this);

    // Forward keystrokes to the calculator so typing still works
    // while the trail widget has focus from click-to-select. Frame
    // CHAR_HOOK already handles RET/TAB/etc.
    Bind(wxEVT_KEY_DOWN, &CalcPanel::on_key_down, host_);
    Bind(wxEVT_CHAR,     &CalcPanel::on_char,     host_);

    // Cmd/Ctrl+C: copy selected trail entries (formatted "tag:
    // value", one per line) to the clipboard. Same accelerator
    // pattern as the stack widget.
    Bind(wxEVT_MENU, &TrailPanel::on_trail_copy, this, wxID_COPY);
    {
        wxAcceleratorEntry copy_entry(wxACCEL_CTRL, (int)'C', wxID_COPY);
        wxAcceleratorTable copy_accel(1, &copy_entry);
        SetAcceleratorTable(copy_accel);
    }

    refresh_from_state();
}

void TrailPanel::refresh_from_state() {
    const auto& ds = host_->display_cache();

    Freeze();
    DeleteAllItems();

    int n = static_cast<int>(ds.trail_entries.size());
    int ptr = ds.trail_pointer;
    for (int i = 0; i < n; ++i) {
        const auto& entry = ds.trail_entries[i];
        wxVector<wxVariant> row;
        row.push_back(wxVariant(wxString::FromUTF8(entry.tag.c_str())));
        row.push_back(wxVariant(wxString::FromUTF8(entry.value.c_str())));
        // marker=1 → MarkedListStore renders the row in accent + bold.
        AppendItem(row, /*data=*/(i == ptr) ? 1 : 0);
    }

    Thaw();

    // Keep the trail-pointer row visible after navigation; otherwise
    // scroll to the bottom (most recent entry).
    if (n > 0) {
        int target = (ptr >= 0 && ptr < n) ? ptr : (n - 1);
        EnsureVisible(RowToItem(target));
    }
}

void TrailPanel::on_context_menu(wxDataViewEvent& e) {
    auto item = e.GetItem();
    if (item.IsOk() && !IsSelected(item)) {
        UnselectAll();
        Select(item);
    }

    wxMenu menu;
    menu.Append(wxID_COPY, wxT("Copy\tCtrl+C"));
    auto* substring = menu.Append(wxID_ANY,
                                  wxT("Copy Value as Substring…"));
    menu.AppendSeparator();
    menu.Append(wxID_SELECTALL, wxT("Select All\tCtrl+A"));

    menu.Enable(wxID_COPY, GetSelectedItemsCount() > 0);
    menu.Enable(substring->GetId(), item.IsOk());
    menu.Enable(wxID_SELECTALL, GetItemCount() > 0);

    menu.Bind(wxEVT_MENU, &TrailPanel::on_trail_copy, this, wxID_COPY);
    menu.Bind(wxEVT_MENU, [this, item](wxCommandEvent&) {
        if (auto* col = GetColumn(1)) EditItem(item, col);
    }, substring->GetId());
    menu.Bind(wxEVT_MENU, [this](wxCommandEvent&) {
        SelectAll();
    }, wxID_SELECTALL);

    PopupMenu(&menu);
}

void TrailPanel::on_trail_copy(wxCommandEvent&) {
    wxDataViewItemArray sel;
    GetSelections(sel);
    if (sel.empty()) return;

    std::vector<int> rows;
    rows.reserve(sel.size());
    for (const auto& item : sel) rows.push_back(ItemToRow(item));
    std::sort(rows.begin(), rows.end());

    // Reproduce the old single-string "tag: value" format on copy
    // (or just value when tag is empty), regardless of column
    // layout. This is what most users want when pasting trail
    // entries elsewhere — the bare value alone loses the context.
    wxString text;
    for (size_t i = 0; i < rows.size(); ++i) {
        if (i) text += wxT("\n");
        wxString tag   = GetTextValue(rows[i], 0);
        wxString value = GetTextValue(rows[i], 1);
        text += tag.IsEmpty() ? value : (tag + wxT(": ") + value);
    }
    if (wxTheClipboard->Open()) {
        wxTheClipboard->SetData(new wxTextDataObject(text));
        wxTheClipboard->Close();
    }
}

// === StackCalcApp =============================================================

bool StackCalcApp::OnInit() {
    // Set the global "current" narrow-string conversion to UTF-8.
    // Note: this does NOT affect wxString(const char*) constructors —
    // those bind to wxConvLibc at compile time. It only affects the
    // various wx APIs that explicitly consult wxConvCurrent (some
    // legacy printf paths, env-var helpers, etc.). For known-UTF-8
    // literals containing non-ASCII characters, use wxT("...") /
    // L"..." instead so wx never has to reinterpret narrow bytes at
    // all. Kept here as defense-in-depth.
    wxConvCurrent = &wxConvUTF8;

    // Drives wxStandardPaths::GetUserDataDir() to a "stackcalc" subdir.
    SetAppName("stackcalc");
    auto* frame = new StackCalcFrame();
    frame->Show(true);
    return true;
}
