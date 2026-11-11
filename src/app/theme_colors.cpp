// MetroFuse's colors for the shared launcher theme (theme.h): deep navy surfaces, icy cyan accent
// and a pale gold focus ring, after the Metroid Fusion logo. This is the only theme file that
// differs between ports.
#include "app/theme.h"

namespace metrofuse::theme {

namespace {
ImVec4 rgb(int r, int g, int b, float a = 1.0f) { return ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, a); }
}  // namespace

const Palette& palette() {
    static const Palette p{
        rgb(10, 20, 48),            // surface
        rgb(15, 28, 61),            // card
        rgb(22, 38, 77),            // raised
        rgb(32, 51, 95),            // high
        rgb(150, 180, 225, 0.16f),  // line
        rgb(238, 247, 255),         // ink
        rgb(156, 184, 216),         // ink_muted
        rgb(102, 128, 163),         // ink_faint
        rgb(64, 186, 234),          // accent
        rgb(140, 218, 248),         // accent_hover
        rgb(30, 142, 204),          // accent_active
        rgb(64, 186, 234, 0.15f),   // accent_soft
        rgb(8, 22, 58),             // on_accent
        rgb(226, 214, 150),         // highlight
        rgb(52, 211, 153),          // good
        rgb(248, 113, 113),         // bad
        rgb(72, 52, 150),           // glow
    };
    return p;
}

}  // namespace metrofuse::theme
