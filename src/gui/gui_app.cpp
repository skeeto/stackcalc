#include "gui_app.h"
#include <wx/dcbuffer.h>
#include <cstdio>
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

// === StackCalcFrame ===========================================================

StackCalcFrame::StackCalcFrame()
    : wxFrame(nullptr, wxID_ANY, "stackcalc", wxDefaultPosition, wxSize(480, 640))
{
    // Native menu bar
    auto* menu_view = new wxMenu();
    menu_view->AppendCheckItem(ID_ToggleTrail, "Show &Trail\tF2");

    auto* menu_file = new wxMenu();
    menu_file->Append(wxID_EXIT, "&Quit\tCtrl+Q");

    auto* mb = new wxMenuBar();
    mb->Append(menu_file, "&File");
    mb->Append(menu_view, "&View");
    SetMenuBar(mb);

    Bind(wxEVT_MENU, &StackCalcFrame::on_toggle_trail, this, ID_ToggleTrail);
    Bind(wxEVT_MENU, &StackCalcFrame::on_quit, this, wxID_EXIT);

    // Single central panel does all the calculator drawing/input
    panel_ = new CalcPanel(this);
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(panel_, 1, wxEXPAND);
    SetSizer(sizer);

    // Trail window (hidden by default)
    trail_ = new TrailFrame(this);

    panel_->SetFocus();
}

void StackCalcFrame::on_toggle_trail(wxCommandEvent&) {
    show_trail(!trail_->IsShown());
}

void StackCalcFrame::on_quit(wxCommandEvent&) {
    Close(true);
}

void StackCalcFrame::show_trail(bool show) {
    trail_->Show(show);
    if (show) {
        // Refresh trail content from current display state
        trail_->update_entries(panel_->controller().display().trail_entries);
    }
    GetMenuBar()->Check(ID_ToggleTrail, show);
    panel_->SetFocus();
}

// === CalcPanel ================================================================

CalcPanel::CalcPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
              wxWANTS_CHARS | wxFULL_REPAINT_ON_RESIZE)
    , blink_timer_(this)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);  // for wxAutoBufferedPaintDC

    // Try to get a real monospace font
    mono_font_ = wxFont(wxFontInfo(13).FaceName("Menlo"));
    if (!mono_font_.IsOk()) {
        mono_font_ = wxFont(13, wxFONTFAMILY_TELETYPE,
                            wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    }

    Bind(wxEVT_PAINT,    &CalcPanel::on_paint,      this);
    Bind(wxEVT_KEY_DOWN, &CalcPanel::on_key_down,   this);
    Bind(wxEVT_CHAR,     &CalcPanel::on_char,       this);
    Bind(wxEVT_TIMER,    &CalcPanel::on_blink_tick, this);

    // Blink timer is only started while number entry is active — see redraw().
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
    Refresh();

    // If trail window is visible, push the latest entries
    auto* frame = static_cast<StackCalcFrame*>(GetParent());
    if (frame && frame->trail() && frame->trail()->IsShown()) {
        frame->trail()->update_entries(ctrl_.display().trail_entries);
    }
}

void CalcPanel::on_blink_tick(wxTimerEvent&) {
    cursor_visible_ = !cursor_visible_;
    Refresh();
}

void CalcPanel::on_paint(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(this);
    dc.SetFont(mono_font_);

    auto ds = ctrl_.display();
    wxSize size = GetClientSize();

    // Background
    dc.SetBackground(wxBrush(kBgColor));
    dc.Clear();

    // Use the font's line height for everything
    int line_h = dc.GetCharHeight();
    int padding = 4;

    int y = padding;

    // ---- Top: error/info message ----
    if (!ds.message.empty()) {
        dc.SetTextForeground(kErrorColor);
        dc.DrawText(ds.message, padding, y);
        y += line_h;
        // separator line
        dc.SetPen(wxPen(kSeparatorColor));
        dc.DrawLine(0, y + 1, size.GetWidth(), y + 1);
        y += 4;
    }

    // ---- Bottom: mode line ----
    int mode_y = size.GetHeight() - line_h - padding;
    {
        // separator above mode line
        dc.SetPen(wxPen(kSeparatorColor));
        dc.DrawLine(0, mode_y - 3, size.GetWidth(), mode_y - 3);

        dc.SetTextForeground(kModeColor);
        dc.DrawText(ds.mode_line, padding, mode_y);
        wxCoord mode_w, mode_h;
        dc.GetTextExtent(ds.mode_line, &mode_w, &mode_h);

        // Flags / pending prefix in yellow
        std::string flags;
        if (ds.inverse_flag)    flags += " [I]";
        if (ds.hyperbolic_flag) flags += " [H]";
        if (ds.keep_args_flag)  flags += " [K]";
        if (!ds.pending_prefix.empty()) {
            flags += " [" + ds.pending_prefix + "-]";
        }
        if (!flags.empty()) {
            dc.SetTextForeground(kFlagColor);
            dc.DrawText(flags, padding + mode_w, mode_y);
        }
    }

    // ---- Middle: stack region ----
    // Render bottom-up: build all the lines we want to draw, then draw from
    // the bottom of the available region upward so the latest entry is always
    // pinned just above the mode line.
    int stack_bottom = mode_y - 8;
    int stack_top = y;

    struct Line {
        wxString text;
        wxColour color;
        // For two-color "label + value" lines we draw twice; we represent that
        // as two consecutive Lines where the value has a label_width offset.
        // Every Line is constructed with an explicit x_offset; the default is
        // just here to satisfy the language (a default member initializer can't
        // capture a local variable, so we can't write `= padding`).
        int x_offset = 0;
    };
    std::vector<Line> lines;

    // Stack entries (numbered bottom-up)
    for (size_t i = 0; i < ds.stack_entries.size(); i++) {
        int n = ds.stack_depth - static_cast<int>(i);
        wxString label = wxString::Format("%3d:", n);
        wxString value = wxString::FromUTF8(ds.stack_entries[i].c_str());

        // Single composite line — we draw label then value at the same y
        lines.push_back({label, kLabelColor, padding});
        wxCoord lw, lh;
        dc.GetTextExtent(label + " ", &lw, &lh);
        lines.push_back({value, kValueColor, padding + lw});
        // Sentinel: empty text marks "next line"
        lines.push_back({wxString(), kBgColor, 0});
    }

    // "." marker line
    lines.push_back({"     .", kMarkerColor, padding});
    lines.push_back({wxString(), kBgColor, 0});

    // Entry text with optional cursor
    if (!ds.entry_text.empty()) {
        wxString et = wxString::FromUTF8(ds.entry_text.c_str());
        if (cursor_visible_) et += "_";
        lines.push_back({"     " + et, kEntryColor, padding});
        lines.push_back({wxString(), kBgColor, 0});
    }

    // Now draw, starting from bottom up. Lines come in groups separated by
    // empty-text sentinels; one logical row may have 1 or 2 draw spans.
    std::vector<std::vector<Line>> rows;
    std::vector<Line> cur;
    for (auto& l : lines) {
        if (l.text.empty()) {
            if (!cur.empty()) { rows.push_back(std::move(cur)); cur.clear(); }
        } else {
            cur.push_back(l);
        }
    }
    if (!cur.empty()) rows.push_back(std::move(cur));

    int row_y = stack_bottom - line_h;
    for (auto it = rows.rbegin(); it != rows.rend(); ++it) {
        if (row_y < stack_top) break;  // clip overflow at the top
        for (auto& span : *it) {
            dc.SetTextForeground(span.color);
            dc.DrawText(span.text, span.x_offset, row_y);
        }
        row_y -= line_h;
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
        redraw();
    }
    // Don't Skip — consumed
}

void CalcPanel::toggle_trail() {
    auto* frame = static_cast<StackCalcFrame*>(GetParent());
    if (frame) frame->show_trail(!frame->trail()->IsShown());
}

void CalcPanel::on_key_down(wxKeyEvent& e) {
    bool ctrl = e.ControlDown() || e.RawControlDown();
    bool alt  = e.AltDown();

    int code = e.GetKeyCode();
    bool handled = true;

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

        case WXK_F2:
            toggle_trail();
            break;

        case 'Z':
            if (ctrl) {
                ctrl_.process_key(sc::KeyEvent::character('U'));
            } else {
                handled = false;
            }
            break;

        case 'Y':
            if (ctrl) {
                ctrl_.process_key(sc::KeyEvent::character('D'));
            } else {
                handled = false;
            }
            break;

        default:
            handled = false;
            break;
    }

    if (handled) {
        redraw();
        // Don't Skip — we consumed it. EVT_CHAR won't fire.
    } else {
        e.Skip();  // let EVT_CHAR translate this key
    }
}

// === TrailFrame ===============================================================

TrailFrame::TrailFrame(wxWindow* parent)
    : wxFrame(parent, wxID_ANY, "Trail",
              wxDefaultPosition, wxSize(360, 480),
              wxDEFAULT_FRAME_STYLE)
{
    text_ = new wxTextCtrl(this, wxID_ANY, wxEmptyString,
                            wxDefaultPosition, wxDefaultSize,
                            wxTE_MULTILINE | wxTE_READONLY | wxTE_DONTWRAP);
    text_->SetFont(wxFont(wxFontInfo(12).FaceName("Menlo")));
    text_->SetBackgroundColour(kBgColor);
    text_->SetForegroundColour(kValueColor);

    auto* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(text_, 1, wxEXPAND);
    SetSizer(sizer);

    // Hide instead of destroy when user closes
    Bind(wxEVT_CLOSE_WINDOW, &TrailFrame::on_close, this);
}

void TrailFrame::on_close(wxCloseEvent&) {
    Hide();
    // Update menu check state
    if (auto* parent = static_cast<StackCalcFrame*>(GetParent())) {
        parent->GetMenuBar()->Check(ID_ToggleTrail, false);
        parent->panel()->SetFocus();
    }
}

void TrailFrame::update_entries(const std::vector<std::string>& entries) {
    wxString out;
    for (auto& e : entries) {
        out += wxString::FromUTF8(e.c_str()) + "\n";
    }
    text_->ChangeValue(out);
    // Scroll to bottom
    text_->ShowPosition(text_->GetLastPosition());
}

// === StackCalcApp =============================================================

bool StackCalcApp::OnInit() {
    auto* frame = new StackCalcFrame();
    frame->Show(true);
    return true;
}
