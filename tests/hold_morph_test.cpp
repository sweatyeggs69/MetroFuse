// Hold Down to morph: Down is dropped for one frame once it has been held long enough while Samus
// is crouching with the Morph Ball, and only once per hold.
#include <cstdio>
#include <vector>

#include "app/hold_morph.h"

using namespace metrofuse;

static int failures = 0;
static void check(bool ok, const char* what) {
    if (!ok) {
        std::printf("FAIL: %s\n", what);
        ++failures;
    }
}

int main() {
    std::vector<std::uint8_t> iwram(0x8000, 0);
    const std::size_t pose = game::kSamusDataAddress + 1 - 0x03000000u;
    const std::size_t suit = game::kEquipmentAddress + 0xC - 0x03000000u;
    const std::uint16_t down = hold_morph::kDown;
    const int frames = hold_morph::frames_for_ms(300);
    check(frames == 18, "300 ms is 18 frames");

    // Crouching without the Morph Ball: never fires.
    iwram[pose] = std::uint8_t(game::kPoseCrouching);
    hold_morph::Assist a;
    for (int i = 0; i < 100; ++i) check(a.step(down, iwram, frames) == down, "no morph ball, Down stays held");

    // With the Morph Ball: Down drops for exactly one frame after the hold time.
    iwram[suit] = std::uint8_t(game::kSuitMorphBall);
    a.reset();
    int dropped_at = -1, drops = 0;
    for (int i = 0; i < 100; ++i) {
        if (!(a.step(down, iwram, frames) & down)) {
            ++drops;
            if (dropped_at < 0) dropped_at = i;
        }
    }
    check(drops == 1, "fires once per hold");
    check(dropped_at == frames - 1, "fires when the hold time is reached");

    // Releasing and holding again fires again.
    a.step(0, iwram, frames);
    drops = 0;
    for (int i = 0; i < 100; ++i) drops += !(a.step(down, iwram, frames) & down);
    check(drops == 1, "fires again after Down is let go");

    // Not crouching (standing, midair): never fires.
    for (unsigned p : {0u, 3u, 4u, 13u}) {
        iwram[pose] = std::uint8_t(p);
        a.reset();
        for (int i = 0; i < 100; ++i) check(a.step(down, iwram, frames) == down, "only while crouching");
    }

    // Diagonal aim (L held): never fires.
    iwram[pose] = std::uint8_t(game::kPoseCrouching);
    a.reset();
    const std::uint16_t aim = std::uint16_t(down | hold_morph::kL);
    for (int i = 0; i < 100; ++i) check(a.step(aim, iwram, frames) == aim, "not while L is held");

    // Other buttons pass through untouched.
    a.reset();
    check(a.step(0x0003, iwram, frames) == 0x0003, "other buttons untouched");

    if (failures == 0) std::printf("hold_morph: all checks passed\n");
    return failures == 0 ? 0 : 1;
}
