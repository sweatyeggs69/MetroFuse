#pragma once
// MetroFuse launcher theme: deep navy space, icy cyan chrome, orb purple and a pale gold accent, after the
// colors of the Metroid Fusion logo. Original artwork only:
// everything is drawn procedurally; the font is Fredoka (SIL OFL 1.1, see assets/fonts/OFL.txt).

#include <imgui.h>

namespace metrofuse::theme {

// Shared palette so other panels (options, pause menu) match the launcher.
struct Palette {
    ImVec4 navy;         // window background, deep navy (logo outline)
    ImVec4 navy_light;   // panels / frames
    ImVec4 navy_hover;
    ImVec4 cyan;         // primary buttons, accents (logo lettering)
    ImVec4 cyan_hover;
    ImVec4 cyan_active;
    ImVec4 cyan_soft;    // headers, selection
    ImVec4 gold;         // highlights, focus (logo bolt)
    ImVec4 text;         // cool white
    ImVec4 text_dim;
    ImVec4 text_on_cyan;  // dark navy text for cyan buttons
    ImVec4 good;         // verified / ok
    ImVec4 bad;          // errors
    ImVec4 purple;       // background glow (logo orb)
    ImVec4 silver;       // subtle borders (logo ring)
};
const Palette& palette();
inline ImU32 u32(const ImVec4& c, float alpha_mul = 1.0f) {
    return ImGui::ColorConvertFloat4ToU32(ImVec4(c.x, c.y, c.z, c.w * alpha_mul));
}

// Loads the fonts and applies the style. ui_scale is the display content scale (1.0, 2.0, ...).
// Call once after ImGui::CreateContext() and before the first NewFrame().
void apply(float ui_scale);

ImFont* font_body();   // ~20 px
ImFont* font_title();  // ~44 px
ImFont* font_small();  // ~16 px
ImFont* font_button(); // ~23 px semibold

// Background: navy-to-purple gradient with a soft orb glow, filling [min, max].
void draw_backdrop(ImDrawList* dl, ImVec2 min, ImVec2 max);
// Large rounded button; hover and focus show a gold outline.
// primary = cyan, otherwise navy-light.
bool big_button(const char* label, ImVec2 size, bool primary = true);
// Text centered horizontally in the current window.
void centered_text(const char* text, ImFont* font = nullptr, ImVec4 color = ImVec4(0, 0, 0, 0));

}  // namespace metrofuse::theme
