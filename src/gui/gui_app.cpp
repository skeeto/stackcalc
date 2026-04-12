#include "gui_app.h"
#include "imgui.h"
#include <GLFW/glfw3.h>
#include <cmath>
#include <cstdio>

GuiApp::GuiApp() {}

void GuiApp::setup_font() {
    ImGuiIO& io = ImGui::GetIO();
    // Try system monospace fonts, fall back to ImGui default
    ImFont* font = io.Fonts->AddFontFromFileTTF(
        "/System/Library/Fonts/Menlo.ttc", 16.0f);
    if (!font) {
        io.Fonts->AddFontDefault();
    }
}

void GuiApp::render() {
    auto ds = ctrl_.display();

    // Full-window borderless ImGui window
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::Begin("Calculator", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollbar);

    // Message area (errors in red)
    if (!ds.message.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
        ImGui::TextUnformatted(ds.message.c_str());
        ImGui::PopStyleColor();
        ImGui::Separator();
    }

    // Reserve space for mode line at bottom
    float mode_line_height = ImGui::GetFrameHeightWithSpacing() +
                             ImGui::GetStyle().ItemSpacing.y;
    float stack_height = ImGui::GetContentRegionAvail().y - mode_line_height;

    // Stack region (scrollable)
    ImGui::BeginChild("Stack", ImVec2(0, stack_height));
    render_stack(ds);
    ImGui::EndChild();

    // Mode line
    ImGui::Separator();
    render_mode_line(ds);

    ImGui::End();

    // Optional trail window (toggle with F2)
    if (show_trail_) {
        ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Trail", &show_trail_)) {
            for (auto& entry : ds.trail_entries) {
                ImGui::TextUnformatted(entry.c_str());
            }
            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                ImGui::SetScrollHereY(1.0f);
        }
        ImGui::End();
    }
}

void GuiApp::render_stack(const sc::DisplayState& ds) {
    const ImVec4 label_color(0.5f, 0.5f, 0.5f, 1.0f);
    const ImVec4 value_color(1.0f, 1.0f, 1.0f, 1.0f);
    const ImVec4 entry_color(0.4f, 1.0f, 0.4f, 1.0f);
    const ImVec4 marker_color(0.5f, 0.5f, 0.5f, 1.0f);

    // Stack entries numbered bottom-up (like Emacs calc)
    for (int i = 0; i < (int)ds.stack_entries.size(); i++) {
        int n = ds.stack_depth - i;
        char label[32];
        std::snprintf(label, sizeof(label), "%3d:", n);

        ImGui::PushStyleColor(ImGuiCol_Text, label_color);
        ImGui::TextUnformatted(label);
        ImGui::PopStyleColor();
        ImGui::SameLine(0, 0);
        ImGui::PushStyleColor(ImGuiCol_Text, value_color);
        ImGui::Text(" %s", ds.stack_entries[i].c_str());
        ImGui::PopStyleColor();
    }

    // "." marker (edit point)
    ImGui::PushStyleColor(ImGuiCol_Text, marker_color);
    ImGui::TextUnformatted("     .");
    ImGui::PopStyleColor();

    // Entry text with blinking cursor
    if (!ds.entry_text.empty()) {
        bool show_cursor = std::fmod(ImGui::GetTime(), 1.0) < 0.5;
        ImGui::PushStyleColor(ImGuiCol_Text, entry_color);
        if (show_cursor)
            ImGui::Text("     %s_", ds.entry_text.c_str());
        else
            ImGui::Text("     %s", ds.entry_text.c_str());
        ImGui::PopStyleColor();
    }

    // Auto-scroll to bottom
    ImGui::SetScrollHereY(1.0f);
}

void GuiApp::render_mode_line(const sc::DisplayState& ds) {
    const ImVec4 mode_color(0.4f, 0.8f, 1.0f, 1.0f);
    const ImVec4 flag_color(1.0f, 1.0f, 0.4f, 1.0f);

    ImGui::PushStyleColor(ImGuiCol_Text, mode_color);
    ImGui::TextUnformatted(ds.mode_line.c_str());
    ImGui::PopStyleColor();

    // Flags and pending prefix indicator
    std::string flags;
    if (ds.inverse_flag) flags += " [I]";
    if (ds.hyperbolic_flag) flags += " [H]";
    if (ds.keep_args_flag) flags += " [K]";
    if (!ds.pending_prefix.empty()) flags += " [" + ds.pending_prefix + "-]";

    if (!flags.empty()) {
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, flag_color);
        ImGui::TextUnformatted(flags.c_str());
        ImGui::PopStyleColor();
    }
}

void GuiApp::on_key(int key, [[maybe_unused]] int scancode,
                    int action, int mods) {
    if (action != GLFW_PRESS && action != GLFW_REPEAT) return;

    bool ctrl = mods & GLFW_MOD_CONTROL;
    bool alt  = mods & GLFW_MOD_ALT;

    // Enter, Backspace, Tab, Delete, Escape, F-keys do NOT generate
    // GLFW char callbacks, so they must NOT set suppress_char_.
    // Only Space (and Ctrl/Alt combos below) produce char events to suppress.

    switch (key) {
        case GLFW_KEY_ENTER:
        case GLFW_KEY_KP_ENTER:
            if (alt)
                ctrl_.process_key(sc::KeyEvent::modified("M-RET"));
            else
                ctrl_.process_key(sc::KeyEvent::special("RET"));
            return;

        case GLFW_KEY_BACKSPACE:
            // Try as edit-backspace first; if not in entry mode, drop
            if (!ctrl_.process_key(sc::KeyEvent::character('\b')))
                ctrl_.process_key(sc::KeyEvent::special("DEL"));
            return;

        case GLFW_KEY_DELETE:
            ctrl_.process_key(sc::KeyEvent::special("DEL"));
            return;

        case GLFW_KEY_TAB:
            if (alt)
                ctrl_.process_key(sc::KeyEvent::modified("M-TAB"));
            else
                ctrl_.process_key(sc::KeyEvent::special("TAB"));
            return;

        case GLFW_KEY_SPACE:
            suppress_char_ = true;  // Space generates a ' ' char event
            ctrl_.process_key(sc::KeyEvent::special("SPC"));
            return;

        case GLFW_KEY_ESCAPE:
            if (ctrl_.input().active())
                ctrl_.input().cancel();
            return;

        case GLFW_KEY_F2:
            show_trail_ = !show_trail_;
            return;

        default:
            break;
    }

    // Ctrl combos (map to Emacs calc keys)
    if (ctrl) {
        suppress_char_ = true;
        switch (key) {
            case GLFW_KEY_Z: ctrl_.process_key(sc::KeyEvent::character('U')); return;
            case GLFW_KEY_Y: ctrl_.process_key(sc::KeyEvent::character('D')); return;
            default: return;
        }
    }

    // Suppress char callback for Alt combos
    if (alt) {
        suppress_char_ = true;
    }
}

void GuiApp::on_char(unsigned int codepoint) {
    if (suppress_char_) {
        suppress_char_ = false;
        return;
    }
    if (codepoint > 0 && codepoint < 128) {
        ctrl_.process_key(sc::KeyEvent::character(static_cast<char>(codepoint)));
    }
}
