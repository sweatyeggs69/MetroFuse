#include "app/licenses.h"

#include <cstdio>

extern const unsigned char metrofuse_text_license[];
extern const unsigned metrofuse_text_license_size;
extern const unsigned char metrofuse_text_notices[];
extern const unsigned metrofuse_text_notices_size;
extern const unsigned char metrofuse_text_dmsans_ofl[];
extern const unsigned metrofuse_text_dmsans_ofl_size;

namespace metrofuse::licenses {

namespace {
std::string_view view(const unsigned char* p, unsigned n) { return {reinterpret_cast<const char*>(p), n}; }
}  // namespace

std::string_view metrofuse() { return view(metrofuse_text_license, metrofuse_text_license_size); }
std::string_view notices() { return view(metrofuse_text_notices, metrofuse_text_notices_size); }
std::string_view font_ofl() { return view(metrofuse_text_dmsans_ofl, metrofuse_text_dmsans_ofl_size); }

void print_all() {
    for (std::string_view t : {metrofuse(), notices(), font_ofl()}) {
        std::fwrite(t.data(), 1, t.size(), stdout);
        std::fputs("\n\n", stdout);
    }
}

}  // namespace metrofuse::licenses
