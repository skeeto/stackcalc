#include "gui_app.hpp"
#include "persistence.hpp"
#include <wx/dcbuffer.h>
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/fontenum.h>
#include <wx/stdpaths.h>
#include <cstdio>
#include <sstream>
#include <string>

// --- Color palette (matches old ImGui look) ---
static const wxColour kBgColor       (26, 26, 26);
static const wxColour kSeparatorColor(64, 64, 64);
static const wxColour kLabelColor    (128, 128, 128);
static const wxColour kValueColor    (255, 255, 255);
static const wxColour kMarkerColor   (128, 128, 128);
static const wxColour kEntryColor    (102, 255, 102);
static const wxColour kModeColor     (102, 204, 255);
static const wxColour kFlagColor     (255, 255, 102);
static const wxColour kErrorColor    (255, 77, 77);

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

    // Splitter hosts the calculator on the left and the trail on the right.
    splitter_ = new wxSplitterWindow(
        this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxSP_LIVE_UPDATE | wxSP_THIN_SASH | wxBORDER_NONE);
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
    // Focus has to be set after the frame is shown — on Windows, SetFocus
    // before Show() is silently ignored and the OS picks its own default
    // child (often the splitter), leaving keystrokes nowhere to land.
    // CallAfter defers until the event loop is running and the frame is
    // realized.
    CallAfter([this] { panel_->focus_calc(); });

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
            panel_->redraw();
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
    auto add_special = [&](wxMenu* m, const wxString& label, const char* name) {
        int id = next_menu_id_++;
        m->Append(id, label);
        Bind(wxEVT_MENU, [this, name](wxCommandEvent&) {
            try { panel_->controller().process_key(sc::KeyEvent::special(name)); }
            catch (...) {}
            panel_->redraw();
            panel_->focus_calc();
        }, id);
    };
    auto add_modified = [&](wxMenu* m, const wxString& label, const char* name) {
        int id = next_menu_id_++;
        m->Append(id, label);
        Bind(wxEVT_MENU, [this, name](wxCommandEvent&) {
            try { panel_->controller().process_key(sc::KeyEvent::modified(name)); }
            catch (...) {}
            panel_->redraw();
            panel_->focus_calc();
        }, id);
    };
    add_special (edit, "&Drop (Back)",            "DEL");
    add_special (edit, "S&wap (Tab)",             "TAB");
    add_modified(edit, "Roll Up (Alt+Tab)",       "M-TAB");
    add_modified(edit, "Last Args (Alt+Enter)",   "M-RET");
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
        panel_->controller().reset();
        panel_->redraw();
        panel_->SetFocus();
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
    // prevents the wxRichTextCtrl from swallowing them when it has
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
    wxMessageBox(
        "stackcalc — RPN calculator\n\n"
        "An Emacs M-x calc clone with arbitrary-precision arithmetic.\n"
        "See manual.md in the source tree for the full reference.",
        "About stackcalc", wxOK | wxICON_INFORMATION, this);
}

void StackCalcFrame::on_quit(wxCommandEvent&) {
    Close(true);
}

// === CalcPanel ================================================================

CalcPanel::CalcPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
              wxWANTS_CHARS | wxFULL_REPAINT_ON_RESIZE)
    , blink_timer_(this)
{
    SetBackgroundColour(kBgColor);

    // Monospace font, used by both subwidgets. Try platform-appropriate
    // face names in order; wxFont silently substitutes when a face is
    // missing (still IsOk(), just wrong-looking), so verify against the
    // system font list before committing. Falls back to the TELETYPE
    // family if nothing on the list is installed.
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

    // Top: custom-painted area for message + entry line.
    top_bar_ = new TopBar(this, this);

    // Middle: native rich-text widget for the stack — selectable + Ctrl+C.
    stack_ctrl_ = new wxRichTextCtrl(
        this, wxID_ANY, wxEmptyString,
        wxDefaultPosition, wxDefaultSize,
        wxRE_READONLY | wxRE_MULTILINE | wxBORDER_NONE);
    stack_ctrl_->SetFont(mono_font_);
    stack_ctrl_->SetBackgroundColour(kBgColor);
    stack_ctrl_->SetForegroundColour(kValueColor);
    stack_ctrl_->SetEditable(false);
    // (Tried installing a hidden wxCaret here to suppress the dark
    // caret on macOS, but it caused a reactivation crash on Cmd+Tab
    // back into the app — wxRichTextCtrl draws its own caret and
    // doesn't tolerate a foreign zero-size wxCaret being substituted.
    // Live with the visual quirk for now.)

    // Bottom: custom-painted mode line.
    mode_bar_ = new ModeBar(this, this);

    auto* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(top_bar_,    0, wxEXPAND);
    sizer->Add(stack_ctrl_, 1, wxEXPAND);
    sizer->Add(mode_bar_,   0, wxEXPAND);
    SetSizer(sizer);

    // Calculator key handling. Bind on the panel itself, AND on the
    // stack widget — when the user clicks into the stack to start a
    // selection, focus stays there, but subsequent typing still needs
    // to reach the calculator. on_key_down skips unhandled keys, so the
    // rich text widget's defaults (Ctrl+A, Ctrl+C, arrow keys, etc.)
    // still work for selection.
    Bind(wxEVT_KEY_DOWN, &CalcPanel::on_key_down,   this);
    Bind(wxEVT_CHAR,     &CalcPanel::on_char,       this);
    Bind(wxEVT_TIMER,    &CalcPanel::on_blink_tick, this);
    stack_ctrl_->Bind(wxEVT_KEY_DOWN, &CalcPanel::on_key_down, this);
    stack_ctrl_->Bind(wxEVT_CHAR,     &CalcPanel::on_char,     this);

    update_stack();
    // Blink timer is only started while number entry is active — see redraw().
}

void CalcPanel::dispatch_keys(const std::string& keys) {
    for (char c : keys) {
        try {
            ctrl_.process_key(sc::KeyEvent::character(c));
        } catch (...) { /* defensive — Controller already handles errors */ }
    }
    redraw();
    focus_calc();
}

// Drop focus onto the actual focusable child. wxPanel itself can be
// focused but that doesn't help on Windows, where the OS-managed focus
// after Show() prefers a real control. Targeting stack_ctrl_ keeps
// keystrokes flowing through the CHAR_HOOK + EVT_CHAR pipeline.
void CalcPanel::focus_calc() {
    if (stack_ctrl_) stack_ctrl_->SetFocus();
}

void CalcPanel::redraw() {
    // Start/stop the cursor-blink timer based on whether entry is active,
    // so an idle calculator wakes up zero times per second.
    bool active = ctrl_.input().active();
    if (active && !blink_timer_.IsRunning()) {
        cursor_visible_ = true;
        blink_timer_.Start(500);
    } else if (!active && blink_timer_.IsRunning()) {
        blink_timer_.Stop();
        cursor_visible_ = true;
    }

    update_stack();
    top_bar_->Refresh();
    mode_bar_->Refresh();
    if (trail_panel_) trail_panel_->refresh_from_state();
}

void CalcPanel::on_blink_tick(wxTimerEvent&) {
    cursor_visible_ = !cursor_visible_;
    // Cursor lives on the entry line in the top bar now.
    top_bar_->Refresh();
}

void CalcPanel::update_stack() {
    auto ds = ctrl_.display();

    stack_ctrl_->Freeze();
    stack_ctrl_->Clear();

    // Top-of-stack rendered at the TOP of the widget, growing downward.
    // ds.stack_entries[back()] is the top; iterate in reverse and number
    // 1, 2, 3, ... from there.
    int n = static_cast<int>(ds.stack_entries.size());
    for (int j = 0; j < n; ++j) {
        int idx = n - 1 - j;        // entry index in stack_entries
        int label = j + 1;          // 1 = top of stack

        stack_ctrl_->BeginTextColour(kLabelColor);
        stack_ctrl_->WriteText(wxString::Format("%3d:  ", label));
        stack_ctrl_->EndTextColour();

        stack_ctrl_->BeginTextColour(kValueColor);
        stack_ctrl_->WriteText(wxString::FromUTF8(ds.stack_entries[idx].c_str()));
        stack_ctrl_->EndTextColour();

        if (j + 1 < n) stack_ctrl_->Newline();
    }

    stack_ctrl_->Thaw();
    // Top of the stack is at line 0; scroll there so the user always
    // sees the most recent entry just below the entry line.
    stack_ctrl_->ShowPosition(0);
}

// === TopBar: custom-painted top region (message + entry) =====================

TopBar::TopBar(wxWindow* parent, CalcPanel* host)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
              wxFULL_REPAINT_ON_RESIZE)
    , host_(host)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(kBgColor);

    wxClientDC dc(this);
    dc.SetFont(host_->mono_font());
    int line_h = dc.GetCharHeight();
    int pad = FromDIP(4);
    int sep_gap = FromDIP(3);
    // Two text rows: message + entry-with-marker. Plus separator under
    // the entry line that visually anchors the stack right below.
    desired_height_ = pad + line_h + line_h + sep_gap + pad;
    SetMinSize(wxSize(-1, desired_height_));

    Bind(wxEVT_PAINT, &TopBar::on_paint, this);
}

void TopBar::on_paint(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(this);
    dc.SetFont(host_->mono_font());
    dc.SetBackground(wxBrush(kBgColor));
    dc.Clear();

    auto ds = host_->controller().display();
    wxSize size = GetClientSize();
    int line_h = dc.GetCharHeight();
    int padding = FromDIP(4);
    int sep_gap = FromDIP(3);

    // ---- Row 0: message (red) — only when present ----
    int msg_y = padding;
    if (!ds.message.empty()) {
        dc.SetTextForeground(kErrorColor);
        dc.DrawText(ds.message, padding, msg_y);
    }

    // ---- Row 1: "." marker + entry text (with blinking cursor) ----
    int entry_y = msg_y + line_h;
    dc.SetTextForeground(kMarkerColor);
    dc.DrawText("  .  ", padding, entry_y);  // align with stack labels
    if (!ds.entry_text.empty()) {
        wxCoord marker_w, marker_h;
        dc.GetTextExtent("  .  ", &marker_w, &marker_h);
        wxString et = wxString::FromUTF8(ds.entry_text.c_str());
        if (host_->cursor_visible()) et += "_";
        dc.SetTextForeground(kEntryColor);
        dc.DrawText(et, padding + marker_w, entry_y);
    }

    // ---- Separator below the entry line, anchoring the stack ----
    int sep_y = entry_y + line_h + sep_gap / 2;
    dc.SetPen(wxPen(kSeparatorColor));
    dc.DrawLine(0, sep_y, size.GetWidth(), sep_y);
}

// === ModeBar: custom-painted bottom region (mode line + flags) ===============

ModeBar::ModeBar(wxWindow* parent, CalcPanel* host)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
              wxFULL_REPAINT_ON_RESIZE)
    , host_(host)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(kBgColor);

    wxClientDC dc(this);
    dc.SetFont(host_->mono_font());
    int line_h = dc.GetCharHeight();
    int pad = FromDIP(4);
    int sep_gap = FromDIP(3);
    desired_height_ = sep_gap + pad + line_h + pad;
    SetMinSize(wxSize(-1, desired_height_));

    Bind(wxEVT_PAINT, &ModeBar::on_paint, this);
}

void ModeBar::on_paint(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(this);
    dc.SetFont(host_->mono_font());
    dc.SetBackground(wxBrush(kBgColor));
    dc.Clear();

    auto ds = host_->controller().display();
    wxSize size = GetClientSize();
    int line_h = dc.GetCharHeight();
    int padding = FromDIP(4);
    int sep_gap = FromDIP(3);

    // Separator above the mode line (anchors the stack visually).
    dc.SetPen(wxPen(kSeparatorColor));
    dc.DrawLine(0, sep_gap / 2, size.GetWidth(), sep_gap / 2);

    int mode_y = size.GetHeight() - line_h - padding;
    dc.SetTextForeground(kModeColor);
    dc.DrawText(ds.mode_line, padding, mode_y);
    wxCoord mode_w, mode_h;
    dc.GetTextExtent(ds.mode_line, &mode_w, &mode_h);

    std::string flags;
    if (ds.inverse_flag)    flags += " [I]";
    if (ds.hyperbolic_flag) flags += " [H]";
    if (ds.keep_args_flag)  flags += " [K]";
    if (!ds.pending_prefix.empty()) flags += " [" + ds.pending_prefix + "-]";
    if (!flags.empty()) {
        dc.SetTextForeground(kFlagColor);
        dc.DrawText(flags, padding + mode_w, mode_y);
    }
}

void CalcPanel::on_char(wxKeyEvent& e) {
    int uc = e.GetUnicodeKey();
    if (uc != WXK_NONE && uc >= 0x20 && uc < 0x7f) {
        try {
            ctrl_.process_key(sc::KeyEvent::character(static_cast<char>(uc)));
        } catch (...) {
            // Controller swallows its own errors via message_; defensive
        }
        clear_selections();
        redraw();
    }
    // Don't Skip — consumed
}

void CalcPanel::on_key_down(wxKeyEvent& e) {
    bool ctrl = e.ControlDown() || e.RawControlDown();
    bool alt  = e.AltDown();

    int code = e.GetKeyCode();
    bool handled = true;

    // Ctrl/Cmd-modified keys are reserved for the OS and menu
    // accelerators (Cmd+Q quit, Cmd+W close, Cmd+Tab app switcher,
    // Cmd+C copy, etc.). Only the two we deliberately mapped onto
    // Ctrl modifiers — Z and Y for undo/redo — are claimed here;
    // everything else falls through. Without this guard, e.g.
    // Cmd+Tab on macOS would be processed as our SWAP, which is
    // wrong and (during reactivation) appears to confuse macOS.
    if (ctrl) {
        if (code == 'Z')      ctrl_.process_key(sc::KeyEvent::character('U'));
        else if (code == 'Y') ctrl_.process_key(sc::KeyEvent::character('D'));
        else { e.Skip(); return; }
        clear_selections();
        redraw();
        return;
    }

    switch (code) {
        case WXK_RETURN:
        case WXK_NUMPAD_ENTER:
            if (alt) ctrl_.process_key(sc::KeyEvent::modified("M-RET"));
            else     ctrl_.process_key(sc::KeyEvent::special("RET"));
            break;

        case WXK_BACK:
            // try edit-backspace; if not consumed (no entry active), drop
            if (!ctrl_.process_key(sc::KeyEvent::character('\b')))
                ctrl_.process_key(sc::KeyEvent::special("DEL"));
            break;

        case WXK_DELETE:
            ctrl_.process_key(sc::KeyEvent::special("DEL"));
            break;

        case WXK_TAB:
            if (alt) ctrl_.process_key(sc::KeyEvent::modified("M-TAB"));
            else     ctrl_.process_key(sc::KeyEvent::special("TAB"));
            break;

        case WXK_SPACE:
            ctrl_.process_key(sc::KeyEvent::special("SPC"));
            break;

        case WXK_ESCAPE:
            if (ctrl_.input().active()) ctrl_.input().cancel();
            break;

        default:
            handled = false;
            break;
    }

    if (handled) {
        clear_selections();
        redraw();
        // Don't Skip — we consumed it. EVT_CHAR won't fire.
    } else {
        e.Skip();  // let EVT_CHAR translate this key
    }
}

void CalcPanel::clear_selections() {
    if (stack_ctrl_)  stack_ctrl_->SelectNone();
    if (trail_panel_) trail_panel_->SelectNone();
}

// === TrailPanel: right-pane trail viewer =====================================

TrailPanel::TrailPanel(wxWindow* parent, CalcPanel* host)
    : wxRichTextCtrl(parent, wxID_ANY, wxEmptyString,
                     wxDefaultPosition, wxDefaultSize,
                     wxRE_READONLY | wxRE_MULTILINE | wxBORDER_NONE)
    , host_(host)
{
    SetFont(host->mono_font());
    SetBackgroundColour(kBgColor);
    SetForegroundColour(kValueColor);
    SetEditable(false);

    // Forward keystrokes to the calculator so typing still works while
    // the trail widget has focus from a click-to-select. Same pattern as
    // the stack widget.
    Bind(wxEVT_KEY_DOWN, &CalcPanel::on_key_down, host_);
    Bind(wxEVT_CHAR,     &CalcPanel::on_char,     host_);

    refresh_from_state();
}

void TrailPanel::refresh_from_state() {
    auto ds = host_->controller().display();

    Freeze();
    Clear();

    // Each row: "  tag: value" (2-space indent), or "> tag: value" for
    // the entry at the trail pointer (rendered yellow for emphasis).
    int n = static_cast<int>(ds.trail_entries.size());
    int ptr = ds.trail_pointer;
    int ptr_line_pos = -1;
    for (int i = 0; i < n; ++i) {
        bool is_pointer = (i == ptr);
        if (is_pointer) ptr_line_pos = GetLastPosition();

        BeginTextColour(is_pointer ? kFlagColor : kLabelColor);
        WriteText(is_pointer ? "> " : "  ");
        EndTextColour();

        BeginTextColour(is_pointer ? kFlagColor : kValueColor);
        WriteText(wxString::FromUTF8(ds.trail_entries[i].c_str()));
        EndTextColour();

        if (i + 1 < n) Newline();
    }

    Thaw();

    // Keep the trail-pointer line visible after navigation.
    if (ptr_line_pos >= 0) {
        ShowPosition(ptr_line_pos);
    } else {
        ShowPosition(GetLastPosition());
    }
}

// === StackCalcApp =============================================================

bool StackCalcApp::OnInit() {
    // Drives wxStandardPaths::GetUserDataDir() to a "stackcalc" subdir.
    SetAppName("stackcalc");
    auto* frame = new StackCalcFrame();
    frame->Show(true);
    return true;
}
