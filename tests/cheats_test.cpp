// Cheats: refills Samus's energy, missiles and power bombs in a blank IWRAM image. No ROM needed.
#include <array>
#include <cstdint>
#include <cstdio>

#include "app/cheats.h"

namespace {
int failures = 0;
#define CHECK(c) do { if (!(c)) { std::fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #c); ++failures; } } while (0)
constexpr std::size_t kBase = 0x1310;
std::uint16_t get16(const std::array<std::uint8_t, 0x8000>& m, std::size_t at) { return std::uint16_t(m[at] | (m[at + 1] << 8)); }
void set16(std::array<std::uint8_t, 0x8000>& m, std::size_t at, std::uint16_t v) { m[at] = std::uint8_t(v); m[at + 1] = std::uint8_t(v >> 8); }
}  // namespace

int main() {
    using metrofuse::cheats::Flags;
    using metrofuse::cheats::apply;
    std::array<std::uint8_t, 0x8000> ram{};

    // Nothing enabled, or nothing owned yet: no writes.
    CHECK(!apply(ram, Flags{}));
    CHECK(!apply(ram, Flags{true, true, true}));
    CHECK(get16(ram, kBase) == 0);

    // Energy 12 of 399, missiles 3 of 50, power bombs 1 of 5.
    set16(ram, kBase + 0, 12);
    set16(ram, kBase + 2, 399);
    set16(ram, kBase + 4, 3);
    set16(ram, kBase + 6, 50);
    ram[kBase + 8] = 1;
    ram[kBase + 9] = 5;

    CHECK(apply(ram, Flags{true, false, false}));
    CHECK(get16(ram, kBase + 0) == 399);
    CHECK(get16(ram, kBase + 4) == 3);  // missiles untouched
    CHECK(ram[kBase + 8] == 1);

    CHECK(apply(ram, Flags{false, true, false}));
    CHECK(get16(ram, kBase + 4) == 50);
    CHECK(ram[kBase + 8] == 1);

    CHECK(apply(ram, Flags{false, false, true}));
    CHECK(ram[kBase + 8] == 5);

    // Already full: reports no change.
    CHECK(!apply(ram, Flags{true, true, true}));

    // Missiles not downloaded yet (max 0): energy still refills, missiles stay 0.
    set16(ram, kBase + 0, 1);
    set16(ram, kBase + 4, 0);
    set16(ram, kBase + 6, 0);
    CHECK(apply(ram, Flags{true, true, false}));
    CHECK(get16(ram, kBase + 0) == 399);
    CHECK(get16(ram, kBase + 4) == 0);

    // Neighbouring bytes are never touched.
    CHECK(ram[kBase - 1] == 0 && ram[kBase + 10] == 0);

    if (failures) std::fprintf(stderr, "%d check(s) failed\n", failures);
    else std::puts("cheats: all checks passed");
    return failures ? 1 : 0;
}
