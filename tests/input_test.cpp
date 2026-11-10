// Headless checks for controller input: a virtual SDL gamepad is attached and driven, and the
// GBA button mask from input::Manager is compared with what the default bindings promise
// (README "Controls" table). Keyboard state cannot be faked headless, so keys are checked
// through the binding table only.
#include <SDL3/SDL.h>

#include <cstdio>
#include <map>
#include <string>

#include "app/states.h"
#include "options/input.h"

using namespace metrofuse::input;

static int failures = 0;
#define CHECK(cond)                                                                       \
    do {                                                                                  \
        if (!(cond)) {                                                                    \
            std::fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond); \
            ++failures;                                                                   \
        }                                                                                 \
    } while (0)

namespace {
constexpr std::uint16_t bit(Button b) { return std::uint16_t(1u << int(b)); }

SDL_Joystick* g_joy = nullptr;

void settle() {
    SDL_UpdateJoysticks();
    SDL_Event e;
    while (SDL_PollEvent(&e)) {}
}
void press(SDL_GamepadButton b, bool down) {
    SDL_SetJoystickVirtualButton(g_joy, b, down);
    settle();
}
void tilt(SDL_GamepadAxis a, Sint16 v) {
    SDL_SetJoystickVirtualAxis(g_joy, a, v);
    settle();
}
void release_all() {
    for (int b = 0; b < SDL_GAMEPAD_BUTTON_COUNT; ++b) SDL_SetJoystickVirtualButton(g_joy, b, false);
    // A virtual pad's raw trigger range is -32768 (released) to 32767 (pulled); 0 is half way.
    for (int a = 0; a < SDL_GAMEPAD_AXIS_COUNT; ++a)
        SDL_SetJoystickVirtualAxis(g_joy, a, a >= SDL_GAMEPAD_AXIS_LEFT_TRIGGER ? -32768 : 0);
    settle();
}

bool has_key(const Config& c, Button b, SDL_Scancode sc) {
    for (const auto& x : c.buttons[int(b)])
        if (x.kind == Binding::Kind::Key && x.code == int(sc)) return true;
    return false;
}

void keyboard_defaults() {
    const Config c = default_config();
    CHECK(has_key(c, Button::Up, SDL_SCANCODE_UP));
    CHECK(has_key(c, Button::Down, SDL_SCANCODE_DOWN));
    CHECK(has_key(c, Button::Left, SDL_SCANCODE_LEFT));
    CHECK(has_key(c, Button::Right, SDL_SCANCODE_RIGHT));
    CHECK(has_key(c, Button::A, SDL_SCANCODE_X));
    CHECK(has_key(c, Button::B, SDL_SCANCODE_Z));
    CHECK(has_key(c, Button::L, SDL_SCANCODE_A));
    CHECK(has_key(c, Button::R, SDL_SCANCODE_S));
    CHECK(has_key(c, Button::Start, SDL_SCANCODE_RETURN));
    CHECK(has_key(c, Button::Select, SDL_SCANCODE_BACKSPACE));
    // No key may drive two GBA buttons.
    std::map<int, int> uses;
    for (const auto& list : c.buttons)
        for (const auto& b : list)
            if (b.kind == Binding::Kind::Key) ++uses[b.code];
    for (const auto& [code, n] : uses) CHECK(n == 1);
}

void gamepad(Manager& m) {
    CHECK(m.gamepad_count() == 1);
    CHECK(m.poll() == 0);

    const struct { SDL_GamepadButton pad; Button gba; } buttons[] = {
        {SDL_GAMEPAD_BUTTON_SOUTH, Button::A},          {SDL_GAMEPAD_BUTTON_NORTH, Button::A},
        {SDL_GAMEPAD_BUTTON_EAST, Button::B},           {SDL_GAMEPAD_BUTTON_WEST, Button::B},
        {SDL_GAMEPAD_BUTTON_START, Button::Start},      {SDL_GAMEPAD_BUTTON_BACK, Button::Select},
        {SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, Button::L},  {SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, Button::R},
        {SDL_GAMEPAD_BUTTON_DPAD_UP, Button::Up},       {SDL_GAMEPAD_BUTTON_DPAD_DOWN, Button::Down},
        {SDL_GAMEPAD_BUTTON_DPAD_LEFT, Button::Left},   {SDL_GAMEPAD_BUTTON_DPAD_RIGHT, Button::Right},
    };
    for (const auto& t : buttons) {
        press(t.pad, true);
        if (m.poll() != bit(t.gba))
            std::fprintf(stderr, "pad button %d -> mask %03x, want %s\n", int(t.pad), m.poll(), name(t.gba));
        CHECK(m.poll() == bit(t.gba));
        press(t.pad, false);
        CHECK(m.poll() == 0);
    }

    // Left stick past the dead zone moves; a small nudge does not.
    tilt(SDL_GAMEPAD_AXIS_LEFTX, 32767);
    CHECK(m.poll() == bit(Button::Right));
    tilt(SDL_GAMEPAD_AXIS_LEFTX, -32768);
    CHECK(m.poll() == bit(Button::Left));
    tilt(SDL_GAMEPAD_AXIS_LEFTX, 6000);
    CHECK(m.poll() == 0);
    tilt(SDL_GAMEPAD_AXIS_LEFTX, 0);
    tilt(SDL_GAMEPAD_AXIS_LEFTY, -32768);
    CHECK(m.poll() == bit(Button::Up));
    tilt(SDL_GAMEPAD_AXIS_LEFTY, 32767);
    CHECK(m.poll() == bit(Button::Down));
    // Full-tilt diagonal (about 70% on each axis) aims diagonally.
    tilt(SDL_GAMEPAD_AXIS_LEFTY, -23170);
    tilt(SDL_GAMEPAD_AXIS_LEFTX, 23170);
    CHECK(m.poll() == (bit(Button::Up) | bit(Button::Right)));
    release_all();

    // Triggers act as L / R.
    tilt(SDL_GAMEPAD_AXIS_LEFT_TRIGGER, 32767);
    CHECK(m.poll() == bit(Button::L));
    tilt(SDL_GAMEPAD_AXIS_LEFT_TRIGGER, -32768);
    tilt(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, 32767);
    CHECK(m.poll() == bit(Button::R));
    release_all();

    // Held together, as in play: run right, aim with R, charge with B, jump with A.
    press(SDL_GAMEPAD_BUTTON_DPAD_RIGHT, true);
    press(SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, true);
    press(SDL_GAMEPAD_BUTTON_EAST, true);
    press(SDL_GAMEPAD_BUTTON_SOUTH, true);
    CHECK(m.poll() == (bit(Button::Right) | bit(Button::R) | bit(Button::B) | bit(Button::A)));
    release_all();

    // D-pad and stick in opposite directions cancel like real hardware.
    press(SDL_GAMEPAD_BUTTON_DPAD_LEFT, true);
    tilt(SDL_GAMEPAD_AXIS_LEFTX, 32767);
    CHECK(m.poll() == 0);
    release_all();
}

void shortcuts(Manager& m) {
    int saves = 0, loads = 0;
    metrofuse::states::Hotkeys hk;
    hk.quick_save = [&] { ++saves; };
    hk.quick_load = [&] { ++loads; };
    metrofuse::states::set_hotkeys(hk);

    // Back + Right Bumper saves, and neither Select nor R reaches the game while held.
    press(SDL_GAMEPAD_BUTTON_BACK, true);
    SDL_Event e{};
    e.type = SDL_EVENT_GAMEPAD_BUTTON_DOWN;
    e.gbutton.which = SDL_GetJoystickID(g_joy);
    e.gbutton.button = SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER;
    press(SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, true);
    CHECK(m.handle_event(e));
    CHECK(saves == 1 && loads == 0);
    CHECK((m.poll() & (bit(Button::R) | bit(Button::Select))) == 0);
    release_all();
    CHECK(m.poll() == 0);
    // Released buttons work normally again.
    press(SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, true);
    CHECK(m.poll() == bit(Button::R));
    release_all();
    metrofuse::states::set_hotkeys({});
}
}  // namespace

int main() {
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    if (!SDL_Init(SDL_INIT_GAMEPAD)) {
        std::fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    keyboard_defaults();

    SDL_VirtualJoystickDesc desc;
    SDL_INIT_INTERFACE(&desc);
    desc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
    desc.naxes = SDL_GAMEPAD_AXIS_COUNT;
    desc.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
    desc.name = "MetroFuse test pad";
    const SDL_JoystickID id = SDL_AttachVirtualJoystick(&desc);
    if (!id || !(g_joy = SDL_OpenJoystick(id))) {
        std::fprintf(stderr, "virtual gamepad: %s\n", SDL_GetError());
        return 1;
    }
    release_all();
    {
        std::map<std::string, std::string> store;
        metrofuse::SettingsIO io;
        io.get = [&](const std::string& k, const std::string& fb) {
            auto it = store.find(k);
            return it == store.end() ? fb : it->second;
        };
        io.set = [&](const std::string& k, const std::string& v) { store[k] = v; };
        io.save = [] {};
        Manager m(io);
        gamepad(m);
        shortcuts(m);
    }
    SDL_CloseJoystick(g_joy);
    SDL_DetachVirtualJoystick(id);
    SDL_Quit();
    if (failures) std::fprintf(stderr, "%d check(s) failed\n", failures);
    else std::printf("input: all checks passed\n");
    return failures ? 1 : 0;
}
