// metrofuse_icon: renders the MetroFuse app icon (original artwork: a cyan "MF" monogram on deep navy)
// into an .iconset folder of PNGs for iconutil. No external image libraries.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iterator>
#include <fstream>
#include <string>
#include <vector>

namespace {

struct Rgba { float r, g, b, a; };

Rgba mix(Rgba a, Rgba b, float t) {
    return {a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t, a.a + (b.a - a.a) * t};
}
Rgba over(Rgba dst, Rgba src) {  // src over dst, straight alpha
    float a = src.a + dst.a * (1 - src.a);
    if (a <= 0) return {0, 0, 0, 0};
    return {(src.r * src.a + dst.r * dst.a * (1 - src.a)) / a, (src.g * src.a + dst.g * dst.a * (1 - src.a)) / a,
            (src.b * src.a + dst.b * dst.a * (1 - src.a)) / a, a};
}

// Signed distance to a box of half-size (hx, hy) centered at (cx, cy) and rotated by `rot`,
// with corners rounded by r. Negative inside.
float sd_box(float x, float y, float cx, float cy, float hx, float hy, float rot, float r) {
    float c = std::cos(rot), s = std::sin(rot);
    float px = (x - cx) * c + (y - cy) * s, py = -(x - cx) * s + (y - cy) * c;
    float qx = std::fabs(px) - hx + r, qy = std::fabs(py) - hy + r;
    float ox = std::max(qx, 0.0f), oy = std::max(qy, 0.0f);
    return std::hypot(ox, oy) + std::min(std::max(qx, qy), 0.0f) - r;
}
// Box from (x0,y0) to (x1,y1), axis aligned.
float sd_rect(float x, float y, float x0, float y0, float x1, float y1, float r) {
    return sd_box(x, y, (x0 + x1) / 2, (y0 + y1) / 2, (x1 - x0) / 2, (y1 - y0) / 2, 0, r);
}
// Capsule of thickness t along the segment a-b (round ends, so two meeting bars form a clean V).
float sd_capsule(float x, float y, float ax, float ay, float bx, float by, float t) {
    float dx = bx - ax, dy = by - ay;
    float h = std::clamp(((x - ax) * dx + (y - ay) * dy) / (dx * dx + dy * dy), 0.0f, 1.0f);
    return std::hypot(x - ax - dx * h, y - ay - dy * h) - t / 2;
}

// The "MF" monogram, blocky like the MetroFuse wordmark. Unit coordinates.
float sd_monogram(float u, float v) {
    const float t = 0.078f, r = 0.012f, top = 0.34f, bot = 0.66f;
    const float m0 = 0.20f, m1 = 0.475f, mid = (m0 + m1) / 2, f0 = 0.525f;
    float d = sd_rect(u, v, m0, top, m0 + t, bot, r);                          // M left stem
    d = std::min(d, sd_rect(u, v, m1 - t, top, m1, bot, r));                   // M right stem
    d = std::min(d, sd_capsule(u, v, m0 + t * 0.5f, top + t * 0.5f, mid, 0.545f, t));  // M diagonals
    d = std::min(d, sd_capsule(u, v, m1 - t * 0.5f, top + t * 0.5f, mid, 0.545f, t));
    d = std::min(d, sd_rect(u, v, f0, top, f0 + t, bot, r));                   // F stem
    d = std::min(d, sd_rect(u, v, f0, top, 0.80f, top + t, r));                // F top bar
    d = std::min(d, sd_rect(u, v, f0, 0.465f, 0.755f, 0.465f + t * 0.9f, r));  // F middle bar
    return d;
}

// Superellipse "squircle" mask like macOS icons, in unit coordinates [0,1].
bool in_squircle(float u, float v) {
    float x = std::fabs(u - 0.5f) / 0.40f, y = std::fabs(v - 0.5f) / 0.40f;
    return std::pow(x, 5.0f) + std::pow(y, 5.0f) <= 1.0f;
}

// Original artwork: a deep navy tile with a purple glow and a cyan-to-white "MF" monogram with a
// navy outline, matching the MetroFuse wordmark and the launcher theme.
Rgba shade(float u, float v) {
    Rgba out{0, 0, 0, 0};
    if (!in_squircle(u, v)) return out;
    Rgba top{0.04f, 0.08f, 0.19f, 1}, bottom{0.14f, 0.11f, 0.36f, 1};
    out = mix(top, bottom, v);
    // Soft purple glow behind the letters.
    float g = std::hypot(u - 0.5f, (v - 0.55f) * 1.2f);
    out = over(out, {0.40f, 0.30f, 0.80f, std::max(0.0f, 0.45f - g * 1.3f)});
    // Monogram: navy outline, then the gradient fill (cyan on the left to near white on the right).
    float d = sd_monogram(u, v);
    const float outline = 0.016f;
    if (d < outline) out = over(out, {0.24f, 0.22f, 0.52f, 1});
    if (d < 0) {
        float k = std::clamp((u - 0.20f) / 0.60f, 0.0f, 1.0f);
        Rgba fill = mix({0.18f, 0.76f, 0.89f, 1}, {0.93f, 0.99f, 1.0f, 1}, k);
        // Slight top highlight, like the wordmark's sheen.
        fill = mix(fill, {1, 1, 1, 1}, std::max(0.0f, 0.42f - v) * 0.6f);
        out = over(out, fill);
    }
    return out;
}

std::uint32_t crc32(const std::uint8_t* d, size_t n, std::uint32_t c = 0xFFFFFFFF) {
    static std::uint32_t table[256];
    static bool init = false;
    if (!init) {
        for (std::uint32_t i = 0; i < 256; ++i) {
            std::uint32_t v = i;
            for (int k = 0; k < 8; ++k) v = (v & 1) ? 0xEDB88320 ^ (v >> 1) : v >> 1;
            table[i] = v;
        }
        init = true;
    }
    for (size_t i = 0; i < n; ++i) c = table[(c ^ d[i]) & 0xFF] ^ (c >> 8);
    return c;
}

void be32(std::vector<std::uint8_t>& v, std::uint32_t x) {
    for (int i = 3; i >= 0; --i) v.push_back(std::uint8_t(x >> (8 * i)));
}

// Minimal PNG writer: RGBA8, zlib "stored" (uncompressed) blocks.
bool write_png(const std::string& path, int w, int h, const std::vector<std::uint8_t>& rgba) {
    std::vector<std::uint8_t> raw;
    for (int y = 0; y < h; ++y) {
        raw.push_back(0);
        raw.insert(raw.end(), rgba.begin() + y * w * 4, rgba.begin() + (y + 1) * w * 4);
    }
    std::vector<std::uint8_t> z = {0x78, 0x01};
    std::uint32_t a = 1, b = 0;
    for (std::uint8_t c : raw) { a = (a + c) % 65521; b = (b + a) % 65521; }
    for (size_t off = 0; off < raw.size(); off += 65535) {
        size_t n = std::min<size_t>(65535, raw.size() - off);
        z.push_back(off + n >= raw.size() ? 1 : 0);
        z.push_back(std::uint8_t(n));
        z.push_back(std::uint8_t(n >> 8));
        z.push_back(std::uint8_t(~n));
        z.push_back(std::uint8_t(~n >> 8));
        z.insert(z.end(), raw.begin() + off, raw.begin() + off + n);
    }
    be32(z, (b << 16) | a);

    std::vector<std::uint8_t> png = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
    auto chunk = [&](const char* type, const std::vector<std::uint8_t>& data) {
        be32(png, std::uint32_t(data.size()));
        size_t start = png.size();
        png.insert(png.end(), type, type + 4);
        png.insert(png.end(), data.begin(), data.end());
        be32(png, ~crc32(&png[start], png.size() - start));
    };
    std::vector<std::uint8_t> ihdr;
    be32(ihdr, std::uint32_t(w));
    be32(ihdr, std::uint32_t(h));
    ihdr.insert(ihdr.end(), {8, 6, 0, 0, 0});
    chunk("IHDR", ihdr);
    chunk("IDAT", z);
    chunk("IEND", {});
    std::ofstream f(path, std::ios::binary);
    f.write(reinterpret_cast<const char*>(png.data()), std::streamsize(png.size()));
    return bool(f);
}

std::vector<std::uint8_t> render(int size) {
    const int ss = 4;
    std::vector<std::uint8_t> px(size_t(size) * size * 4);
    for (int y = 0; y < size; ++y)
        for (int x = 0; x < size; ++x) {
            float r = 0, g = 0, b = 0, a = 0;
            for (int sy = 0; sy < ss; ++sy)
                for (int sx = 0; sx < ss; ++sx) {
                    Rgba c = shade((x + (sx + 0.5f) / ss) / size, (y + (sy + 0.5f) / ss) / size);
                    r += c.r * c.a; g += c.g * c.a; b += c.b * c.a; a += c.a;
                }
            float n = float(ss * ss);
            size_t i = (size_t(y) * size + x) * 4;
            px[i + 3] = std::uint8_t(std::lround(a / n * 255));
            if (a > 0) {
                px[i + 0] = std::uint8_t(std::lround(std::min(1.0f, r / a) * 255));
                px[i + 1] = std::uint8_t(std::lround(std::min(1.0f, g / a) * 255));
                px[i + 2] = std::uint8_t(std::lround(std::min(1.0f, b / a) * 255));
            }
        }
    return px;
}

}  // namespace

// Encodes a PNG to memory by writing it to a temp file path; simpler: reuse write_png's logic.
std::vector<std::uint8_t> png_bytes(int px) {
    std::string tmp = "metrofuse_icon_tmp_" + std::to_string(px) + ".png";
    if (!write_png(tmp, px, px, render(px))) return {};
    std::ifstream f(tmp, std::ios::binary);
    std::vector<std::uint8_t> b((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    f.close();
    std::remove(tmp.c_str());
    return b;
}

// Windows .ico with PNG-compressed entries (supported since Windows Vista).
bool write_ico(const std::string& path) {
    const int sizes[] = {16, 24, 32, 48, 64, 128, 256};
    std::vector<std::vector<std::uint8_t>> imgs;
    for (int s : sizes) imgs.push_back(png_bytes(s));
    std::vector<std::uint8_t> out;
    auto le16 = [&](unsigned v) { out.push_back(std::uint8_t(v)); out.push_back(std::uint8_t(v >> 8)); };
    auto le32 = [&](std::uint32_t v) { for (int i = 0; i < 4; ++i) out.push_back(std::uint8_t(v >> (8 * i))); };
    le16(0);
    le16(1);
    le16(unsigned(imgs.size()));
    std::uint32_t offset = 6 + 16 * std::uint32_t(imgs.size());
    for (size_t i = 0; i < imgs.size(); ++i) {
        out.push_back(std::uint8_t(sizes[i] >= 256 ? 0 : sizes[i]));
        out.push_back(std::uint8_t(sizes[i] >= 256 ? 0 : sizes[i]));
        out.push_back(0);
        out.push_back(0);
        le16(1);
        le16(32);
        le32(std::uint32_t(imgs[i].size()));
        le32(offset);
        offset += std::uint32_t(imgs[i].size());
    }
    for (auto& img : imgs) out.insert(out.end(), img.begin(), img.end());
    std::ofstream f(path, std::ios::binary);
    f.write(reinterpret_cast<const char*>(out.data()), std::streamsize(out.size()));
    return bool(f);
}

int main(int argc, char** argv) {
    if (argc == 3 && std::string(argv[1]) == "--ico") return write_ico(argv[2]) ? 0 : 1;
    if (argc == 4 && std::string(argv[1]) == "--png") {
        int px = std::atoi(argv[2]);
        return px > 0 && write_png(argv[3], px, px, render(px)) ? 0 : 1;
    }
    if (argc < 2 || argv[1][0] == '-') {
        std::fprintf(stderr, "usage: metrofuse_icon <out.iconset> | --ico <out.ico> | --png <size> <out.png>\n");
        return 2;
    }
    std::string dir = argv[1];
    const int sizes[] = {16, 32, 128, 256, 512};
    for (int s : sizes) {
        for (int scale = 1; scale <= 2; ++scale) {
            int px = s * scale;
            std::string name = dir + "/icon_" + std::to_string(s) + "x" + std::to_string(s) +
                               (scale == 2 ? "@2x" : "") + ".png";
            if (!write_png(name, px, px, render(px))) {
                std::fprintf(stderr, "could not write %s\n", name.c_str());
                return 1;
            }
        }
    }
    return 0;
}
