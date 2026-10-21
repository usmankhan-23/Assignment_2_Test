// Temporary test driver for Question 3. NOT part of the submission.
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "ImageProcessing.h"

#include <cstdio>
#include <cmath>

#ifdef TWO_FILES
// Defined in second_tu.cpp: proves the headers can be included in two files.
int secondTranslationUnitWidth();
#endif

static int passed = 0;
static int failed = 0;

#define CHECK(cond)                                                        \
    do {                                                                   \
        if (cond) { passed++; }                                            \
        else { failed++; std::printf("FAIL line %d: %s\n", __LINE__, #cond); } \
    } while (0)

static std::string OUT = "out/";

static Pixel px(int r, int g, int b) {
    Pixel p; p.r = (uint8_t)r; p.g = (uint8_t)g; p.b = (uint8_t)b; return p;
}
static bool same(const Pixel& a, const Pixel& b) {
    return a.r == b.r && a.g == b.g && a.b == b.b;
}
static bool sameCanvas(const Canvas& a, const Canvas& b) {
    if (a.getWidth() != b.getWidth() || a.getHeight() != b.getHeight()) return false;
    for (int y = 0; y < a.getHeight(); y++)
        for (int x = 0; x < a.getWidth(); x++)
            if (!same(a.at(x, y), b.at(x, y))) return false;
    return true;
}
// A w x h canvas where every pixel has a unique colour.
static Canvas pattern(int w, int h) {
    Canvas c(w, h);
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
            c.at(x, y) = px((x * 37 + y * 11) % 256, (x * 5 + y * 91) % 256, (x + y * 7 + 13) % 256);
    return c;
}
static bool writePngOf(const std::string& path, const Canvas& c) {
    return stbi_write_png(path.c_str(), c.getWidth(), c.getHeight(), 3, c.getRawData(), c.getWidth() * 3) != 0;
}
// Load a PNG written by the code under test into a Canvas.
static Canvas readPng(const std::string& path) {
    Canvas c(0, 0);
    int w, h, n;
    unsigned char* b = stbi_load(path.c_str(), &w, &h, &n, 3);
    if (b) { c.loadFromRaw(b, w, h, w * 3); stbi_image_free(b); }
    return c;
}
static bool readFile(const std::string& path, unsigned char* buf, int max, int& size) {
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return false;
    size = (int)std::fread(buf, 1, max, f);
    std::fclose(f);
    return true;
}
static void writeFile(const std::string& path, const unsigned char* buf, int size) {
    FILE* f = std::fopen(path.c_str(), "wb");
    std::fwrite(buf, 1, size, f);
    std::fclose(f);
}
static unsigned u32(const unsigned char* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((unsigned)p[3] << 24); }
static unsigned u16(const unsigned char* p) { return p[0] | (p[1] << 8); }
// Writes c as PNG then transcodes it to BMP with the code under test.
static std::string makeBmp(const std::string& name, const Canvas& c) {
    ImageProcessing ip;
    std::string png = OUT + name + ".png";
    std::string bmp = OUT + name + ".bmp";
    writePngOf(png, c);
    if (!ip.TranscodeToBMP(png, bmp)) std::printf("makeBmp failed for %s\n", name.c_str());
    return bmp;
}
static bool closeCanvas(const Canvas& a, const Canvas& b, int tol) {
    if (a.getWidth() != b.getWidth() || a.getHeight() != b.getHeight()) return false;
    for (int y = 0; y < a.getHeight(); y++)
        for (int x = 0; x < a.getWidth(); x++) {
            const Pixel& p = a.at(x, y); const Pixel& q = b.at(x, y);
            if (std::abs(p.r - q.r) > tol || std::abs(p.g - q.g) > tol || std::abs(p.b - q.b) > tol) return false;
        }
    return true;
}

// ---------------------------------------------------------------------
static void testLifecycle() {
    Canvas c(3, 2);
    CHECK(c.getWidth() == 3 && c.getHeight() == 2 && c.getRawData() != nullptr);
    bool allBlack = true;
    for (int i = 0; i < 6; i++) if (!same(c.getRawData()[i], px(0, 0, 0))) allBlack = false;
    CHECK(allBlack);

    Canvas e1(0, 5), e2(-1, 3), e3(4, 0), e4(-2, -2);
    CHECK(e1.getWidth() == 0 && e1.getHeight() == 0 && e1.getRawData() == nullptr);
    CHECK(e2.getWidth() == 0 && e2.getHeight() == 0 && e2.getRawData() == nullptr);
    CHECK(e3.getWidth() == 0 && e3.getHeight() == 0 && e3.getRawData() == nullptr);
    CHECK(e4.getRawData() == nullptr);

    CHECK(sizeof(Pixel) == 3);

    // at(): non-const write, const read, index y*width+x
    c.at(2, 1) = px(1, 2, 3);
    const Canvas& cc = c;
    CHECK(same(cc.at(2, 1), px(1, 2, 3)));
    CHECK(same(c.getRawData()[1 * 3 + 2], px(1, 2, 3)));

    // Deep copy independence
    Canvas copy(c);
    CHECK(copy.getRawData() != c.getRawData());
    CHECK(sameCanvas(copy, c));
    copy.at(0, 0) = px(9, 9, 9);
    CHECK(same(c.at(0, 0), px(0, 0, 0)));

    Canvas emptyCopy(e1);
    CHECK(emptyCopy.getRawData() == nullptr && emptyCopy.getWidth() == 0);

    // copyFrom: different size, self, empty, repeated
    Canvas d(10, 10);
    d.copyFrom(c);
    CHECK(d.getWidth() == 3 && d.getHeight() == 2 && sameCanvas(d, c) && d.getRawData() != c.getRawData());
    d.copyFrom(d);
    CHECK(sameCanvas(d, c));
    d.copyFrom(e1);
    CHECK(d.getWidth() == 0 && d.getHeight() == 0 && d.getRawData() == nullptr);
    Canvas big = pattern(50, 40);
    for (int i = 0; i < 200; i++) { d.copyFrom(i % 2 ? big : c); }
    CHECK(sameCanvas(d, big));   // i = 199 is odd -> last copy was big
    d.copyFrom(c);
    CHECK(sameCanvas(d, c));
    Canvas e5(0, 0);
    e5.copyFrom(e1);   // empty <- empty
    CHECK(e5.getRawData() == nullptr);
}

static void testLoadFromRaw() {
    // 2x2 image, stride 8 (6 data bytes + 2 junk bytes per row)
    uint8_t raw[16] = { 10, 20, 30, 40, 50, 60, 99, 99,
                        70, 80, 90, 100, 110, 120, 99, 99 };
    Canvas c(5, 5);
    c.loadFromRaw(raw, 2, 2, 8);
    CHECK(c.getWidth() == 2 && c.getHeight() == 2);
    CHECK(same(c.at(0, 0), px(10, 20, 30)));
    CHECK(same(c.at(1, 0), px(40, 50, 60)));
    CHECK(same(c.at(0, 1), px(70, 80, 90)));
    CHECK(same(c.at(1, 1), px(100, 110, 120)));

    // Stride bigger than one row: 1x2 image taking column 0 of each row above
    c.loadFromRaw(raw, 1, 2, 8);
    CHECK(c.getWidth() == 1 && c.getHeight() == 2 && same(c.at(0, 1), px(70, 80, 90)));
    // Stride == width*3
    uint8_t raw3[9] = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    c.loadFromRaw(raw3, 1, 3, 3);
    CHECK(c.getWidth() == 1 && c.getHeight() == 3 && same(c.at(0, 2), px(7, 8, 9)));

    c.loadFromRaw(nullptr, 2, 2, 6);
    CHECK(c.getWidth() == 0 && c.getHeight() == 0 && c.getRawData() == nullptr);
    c.loadFromRaw(raw3, 0, 3, 3);
    CHECK(c.getRawData() == nullptr);
    c.loadFromRaw(raw3, 3, -1, 9);
    CHECK(c.getRawData() == nullptr);
    c.loadFromRaw(raw3, 3, 1, 9);
    CHECK(c.getWidth() == 3 && same(c.at(2, 0), px(7, 8, 9)));

    // Loading from its own buffer must be safe
    Canvas self = pattern(4, 3);
    Canvas expect(self);
    self.loadFromRaw(self.getRawData(), 4, 3, 12);
    CHECK(sameCanvas(self, expect));
}

static void testColor() {
    Canvas c(4, 1);
    c.at(0, 0) = px(100, 101, 0);
    c.at(1, 0) = px(200, 255, 1);
    c.at(2, 0) = px(3, 127, 254);
    c.at(3, 0) = px(255, 0, 128);

    Canvas b = c.scaleBrightness(1.5f);
    CHECK(same(b.at(0, 0), px(150, 152, 0)));      // 151.5+0.5=152
    CHECK(same(b.at(1, 0), px(255, 255, 2)));      // 1.5+0.5=2
    CHECK(same(b.at(2, 0), px(5, 191, 255)));      // 4.5+.5=5, 190.5+.5=191
    Canvas h = c.scaleBrightness(0.5f);
    CHECK(same(h.at(0, 0), px(50, 51, 0)));        // 50.5+.5 = 51
    CHECK(same(h.at(1, 0), px(100, 128, 1)));      // 127.5+.5=128, 0.5+.5=1
    Canvas one = c.scaleBrightness(1.0f);
    CHECK(sameCanvas(one, c));
    Canvas neg = c.scaleBrightness(-3.0f);
    CHECK(sameCanvas(neg, Canvas(4, 1)));
    Canvas huge = c.scaleBrightness(1e30f);
    CHECK(same(huge.at(0, 0), px(255, 255, 0)));
    Canvas inf = c.scaleBrightness(INFINITY);
    CHECK(same(inf.at(3, 0), px(255, 0, 255)));
    Canvas zero = c.scaleBrightness(0.0f);
    CHECK(sameCanvas(zero, Canvas(4, 1)));

    Canvas pc = c.scaleBrightness(2.0f, 0.5f, -1.0f);
    CHECK(same(pc.at(0, 0), px(200, 51, 0)));
    CHECK(same(pc.at(1, 0), px(255, 128, 0)));
    CHECK(same(pc.at(2, 0), px(6, 64, 0)));       // 63.5+.5=64

    Canvas inv = c.invertColors();
    CHECK(same(inv.at(0, 0), px(155, 154, 255)));
    CHECK(same(inv.at(3, 0), px(0, 255, 127)));
    CHECK(sameCanvas(inv.invertColors(), c));
    CHECK(same(c.at(0, 0), px(100, 101, 0)));      // original untouched

    Canvas empty(0, 0);
    CHECK(empty.scaleBrightness(2.0f).getRawData() == nullptr);
    CHECK(empty.invertColors().getRawData() == nullptr);
}

static void testComposition() {
    Canvas a(2, 1); a.at(0, 0) = px(255, 0, 0); a.at(1, 0) = px(254, 0, 0);
    Canvas b(1, 3); b.at(0, 0) = px(0, 255, 0); b.at(0, 1) = px(0, 254, 0); b.at(0, 2) = px(0, 253, 0);

    Canvas s = a.stitchHorizontal(b);
    CHECK(s.getWidth() == 3 && s.getHeight() == 3);
    CHECK(same(s.at(0, 0), px(255, 0, 0)) && same(s.at(1, 0), px(254, 0, 0)));
    CHECK(same(s.at(2, 0), px(0, 255, 0)) && same(s.at(2, 2), px(0, 253, 0)));
    CHECK(same(s.at(0, 1), px(0, 0, 0)) && same(s.at(1, 2), px(0, 0, 0)));

    Canvas s2 = b.stitchHorizontal(a);   // taller on left
    CHECK(s2.getWidth() == 3 && s2.getHeight() == 3);
    CHECK(same(s2.at(1, 0), px(255, 0, 0)) && same(s2.at(1, 1), px(0, 0, 0)) && same(s2.at(2, 2), px(0, 0, 0)));

    Canvas v = a.stackVertical(b);
    CHECK(v.getWidth() == 2 && v.getHeight() == 4);
    CHECK(same(v.at(0, 0), px(255, 0, 0)) && same(v.at(1, 0), px(254, 0, 0)));
    CHECK(same(v.at(0, 1), px(0, 255, 0)) && same(v.at(0, 3), px(0, 253, 0)));
    CHECK(same(v.at(1, 1), px(0, 0, 0)) && same(v.at(1, 3), px(0, 0, 0)));

    Canvas v2 = b.stackVertical(a);
    CHECK(v2.getWidth() == 2 && v2.getHeight() == 4);
    CHECK(same(v2.at(1, 0), px(0, 0, 0)) && same(v2.at(0, 3), px(255, 0, 0)) && same(v2.at(1, 3), px(254, 0, 0)));

    Canvas empty(0, 0);
    CHECK(sameCanvas(a.stitchHorizontal(empty), a));
    CHECK(sameCanvas(empty.stackVertical(b), b));
    CHECK(a.stitchHorizontal(a).getWidth() == 4);   // self-stitch
}

static void testCropPaste() {
    Canvas p = pattern(5, 4);
    Canvas cr = p.crop(1, 2, 3, 2);
    CHECK(cr.getWidth() == 3 && cr.getHeight() == 2);
    bool ok = true;
    for (int y = 0; y < 2; y++) for (int x = 0; x < 3; x++) if (!same(cr.at(x, y), p.at(x + 1, y + 2))) ok = false;
    CHECK(ok);
    CHECK(sameCanvas(p.crop(0, 0, 5, 4), p));
    CHECK(same(p.crop(4, 3, 1, 1).at(0, 0), p.at(4, 3)));

    // Paste fully inside
    Canvas base(4, 4);
    Canvas src(2, 2);
    src.at(0, 0) = px(1, 1, 1); src.at(1, 0) = px(2, 2, 2); src.at(0, 1) = px(3, 3, 3); src.at(1, 1) = px(4, 4, 4);
    base.paste(src, 1, 1);
    CHECK(same(base.at(1, 1), px(1, 1, 1)) && same(base.at(2, 2), px(4, 4, 4)) && same(base.at(0, 0), px(0, 0, 0)) && same(base.at(3, 3), px(0, 0, 0)));

    // Negative offsets (clip top-left)
    Canvas b2(3, 3);
    b2.paste(src, -1, -1);
    CHECK(same(b2.at(0, 0), px(4, 4, 4)) && same(b2.at(1, 0), px(0, 0, 0)) && same(b2.at(0, 1), px(0, 0, 0)));
    // Beyond bottom-right
    Canvas b3(3, 3);
    b3.paste(src, 2, 2);
    CHECK(same(b3.at(2, 2), px(1, 1, 1)) && same(b3.at(1, 1), px(0, 0, 0)));
    // Fully outside: nothing changes
    Canvas b4(3, 3);
    b4.paste(src, 10, -10); b4.paste(src, -5, 0); b4.paste(src, 2000000000, 2000000000); b4.paste(src, -2147483647, 0);
    CHECK(sameCanvas(b4, Canvas(3, 3)));
    // Source bigger than destination
    Canvas b5(2, 2);
    Canvas bigSrc = pattern(6, 6);
    b5.paste(bigSrc, -2, -3);
    CHECK(same(b5.at(0, 0), bigSrc.at(2, 3)) && same(b5.at(1, 1), bigSrc.at(3, 4)));
    // Pasting empty canvas / onto empty canvas
    Canvas b6(2, 2); b6.paste(Canvas(0, 0), 0, 0);
    CHECK(sameCanvas(b6, Canvas(2, 2)));
    Canvas emptyDest(0, 0); emptyDest.paste(src, 0, 0);
    CHECK(emptyDest.getRawData() == nullptr);

    // Transparent key: skip only exact 3-channel match
    Canvas dest(3, 1);
    dest.at(0, 0) = px(7, 7, 7); dest.at(1, 0) = px(7, 7, 7); dest.at(2, 0) = px(7, 7, 7);
    Canvas keySrc(3, 1);
    keySrc.at(0, 0) = px(255, 0, 255);   // equals key -> skipped
    keySrc.at(1, 0) = px(255, 0, 254);   // only 2 channels match -> copied
    keySrc.at(2, 0) = px(0, 0, 255);
    dest.paste(keySrc, 0, 0, px(255, 0, 255));
    CHECK(same(dest.at(0, 0), px(7, 7, 7)));
    CHECK(same(dest.at(1, 0), px(255, 0, 254)));
    CHECK(same(dest.at(2, 0), px(0, 0, 255)));
    Canvas dest2(2, 1);
    dest2.paste(keySrc, -1, 0, px(255, 0, 255));
    CHECK(same(dest2.at(0, 0), px(255, 0, 254)) && same(dest2.at(1, 0), px(0, 0, 255)));
}

static void testGeometry() {
    Canvas p = pattern(3, 2);
    Canvas fh = p.flipHorizontal();
    Canvas fv = p.flipVertical();
    Canvas r = p.rotate90Clockwise();
    bool okH = true, okV = true, okR = true;
    for (int y = 0; y < 2; y++) for (int x = 0; x < 3; x++) {
        if (!same(fh.at(3 - 1 - x, y), p.at(x, y))) okH = false;
        if (!same(fv.at(x, 2 - 1 - y), p.at(x, y))) okV = false;
    }
    CHECK(okH && okV);
    CHECK(r.getWidth() == 2 && r.getHeight() == 3);
    for (int y = 0; y < 3; y++) for (int x = 0; x < 2; x++)
        if (!same(r.at(x, y), p.at(y, 2 - 1 - x))) okR = false;
    CHECK(okR);
    // Visual sanity: top-left of rotated = old bottom-left; top-right = old top-left
    CHECK(same(r.at(0, 0), p.at(0, 1)) && same(r.at(1, 0), p.at(0, 0)));
    // Four rotations = original, two flips = original
    CHECK(sameCanvas(r.rotate90Clockwise().rotate90Clockwise().rotate90Clockwise(), p));
    CHECK(sameCanvas(fh.flipHorizontal(), p) && sameCanvas(fv.flipVertical(), p));
    Canvas line = pattern(5, 1);
    CHECK(line.rotate90Clockwise().getWidth() == 1 && line.rotate90Clockwise().getHeight() == 5);
    CHECK(Canvas(0, 0).rotate90Clockwise().getRawData() == nullptr);
}

// Straightforward reference convolution used to check applyFilter.
static Canvas refFilter(const Canvas& src, const float* w, int k) {
    int r = k / 2, W = src.getWidth(), H = src.getHeight();
    Canvas out(W, H);
    for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) {
        float s[3] = { 0, 0, 0 };
        for (int i = 0; i < k; i++) for (int j = 0; j < k; j++) {
            int cx = x + j - r; if (cx < 0) cx = 0; if (cx > W - 1) cx = W - 1;
            int cy = y + i - r; if (cy < 0) cy = 0; if (cy > H - 1) cy = H - 1;
            const Pixel& p = src.at(cx, cy);
            s[0] += w[i * k + j] * p.r; s[1] += w[i * k + j] * p.g; s[2] += w[i * k + j] * p.b;
        }
        uint8_t o[3];
        for (int c = 0; c < 3; c++) o[c] = s[c] < 0 ? 0 : (s[c] > 255 ? 255 : (uint8_t)(s[c] + 0.5f));
        out.at(x, y) = px(o[0], o[1], o[2]);
    }
    return out;
}

static void testFilter() {
    Canvas p = pattern(6, 5);
    float identity[9] = { 0, 0, 0, 0, 1, 0, 0, 0, 0 };
    CHECK(sameCanvas(p.applyFilter(identity, 3), p));

    float box[9]; for (int i = 0; i < 9; i++) box[i] = 1.0f / 9.0f;
    CHECK(closeCanvas(p.applyFilter(box, 3), refFilter(p, box, 3), 1));

    // Hand-computed edge clamp: 3x1 image [0, 90, 180], box 3x3.
    Canvas row(3, 1);
    row.at(0, 0) = px(0, 0, 0); row.at(1, 0) = px(90, 90, 90); row.at(2, 0) = px(180, 180, 180);
    Canvas rb = row.applyFilter(box, 3);
    // x=0: cols clamp to {0,0,1} -> (0+0+90)*3/9 = 30 ; x=1: (0+90+180)/3 = 90 ; x=2: (90+180+180)/3 = 150
    CHECK(closeCanvas(rb, rb, 0));
    CHECK(std::abs(rb.at(0, 0).r - 30) <= 1 && std::abs(rb.at(1, 0).r - 90) <= 1 && std::abs(rb.at(2, 0).r - 150) <= 1);

    // Non-symmetric kernel: proves no kernel flip. Weight at (i=1, j=2) picks the RIGHT neighbour.
    float right[9] = { 0, 0, 0, 0, 0, 1, 0, 0, 0 };
    Canvas rr = row.applyFilter(right, 3);
    CHECK(rr.at(0, 0).r == 90 && rr.at(1, 0).r == 180 && rr.at(2, 0).r == 180);
    // Weight at (i=0, j=1) picks the neighbour ABOVE.
    Canvas col = p.crop(0, 0, 1, 5);
    float up[9] = { 0, 1, 0, 0, 0, 0, 0, 0, 0 };
    Canvas cu = col.applyFilter(up, 3);
    CHECK(same(cu.at(0, 0), col.at(0, 0)) && same(cu.at(0, 3), col.at(0, 2)));

    // 1x1 kernel, clamps above 255, negative -> 0
    float two = 2.0f;
    CHECK(closeCanvas(p.applyFilter(&two, 1), p.scaleBrightness(2.0f), 1));
    float minus = -1.0f;
    CHECK(sameCanvas(p.applyFilter(&minus, 1), Canvas(6, 5)));

    // Edge detection (has negative weights), 5x5 and 7x7 kernels on a 2x2 image
    float edge[9] = { -1, -1, -1, -1, 8, -1, -1, -1, -1 };
    CHECK(closeCanvas(p.applyFilter(edge, 3), refFilter(p, edge, 3), 1));
    float k5[25]; for (int i = 0; i < 25; i++) k5[i] = (i % 3 - 1) * 0.3f + 0.04f;
    CHECK(closeCanvas(p.applyFilter(k5, 5), refFilter(p, k5, 5), 1));
    Canvas tiny = pattern(2, 2);
    float k7[49]; for (int i = 0; i < 49; i++) k7[i] = 1.0f / 49.0f;
    CHECK(closeCanvas(tiny.applyFilter(k7, 7), refFilter(tiny, k7, 7), 1));
    Canvas single = pattern(1, 1);
    CHECK(closeCanvas(single.applyFilter(box, 3), single, 1));

    // Kernel overload gives the same result and does NOT free the weights
    Kernel kern;
    kern.size = 3;
    kern.weights = new float[9];
    for (int i = 0; i < 9; i++) kern.weights[i] = edge[i];
    CHECK(sameCanvas(p.applyFilter(kern), p.applyFilter(edge, 3)));
    CHECK(kern.weights[4] == 8.0f);     // still readable
    delete[] kern.weights;               // caller frees it (ASan would catch a double free)

    CHECK(sameCanvas(p, pattern(6, 5)));   // source unchanged
}

static void testBmpWriter() {
    // Widths 1..5 give padding 1, 2, 3, 0, 1  ((4 - (w*3)%4) % 4)
    int expectedPad[6] = { 0, 1, 2, 3, 0, 1 };
    for (int w = 1; w <= 5; w++) {
        Canvas c = pattern(w, 3);
        std::string bmp = makeBmp("w" + std::to_string(w), c);
        static unsigned char buf[4096];
        int size = 0;
        CHECK(readFile(bmp, buf, sizeof(buf), size));
        int rowBytes = w * 3 + expectedPad[w];
        CHECK(rowBytes % 4 == 0);
        CHECK(size == 54 + rowBytes * 3);
        CHECK(buf[0] == 'B' && buf[1] == 'M');
        CHECK(u32(buf + 2) == (unsigned)size);
        CHECK(u16(buf + 6) == 0 && u16(buf + 8) == 0);
        CHECK(u32(buf + 10) == 54);
        CHECK(u32(buf + 14) == 40);
        CHECK((int)u32(buf + 18) == w && (int)u32(buf + 22) == 3);
        CHECK(u16(buf + 26) == 1 && u16(buf + 28) == 24);
        CHECK(u32(buf + 30) == 0);
        CHECK(u32(buf + 34) == (unsigned)(rowBytes * 3));
        CHECK(u32(buf + 38) == 2835 && u32(buf + 42) == 2835);
        CHECK(u32(buf + 46) == 0 && u32(buf + 50) == 0);
        bool pixelsOk = true, padOk = true;
        for (int fileRow = 0; fileRow < 3; fileRow++) {
            int y = 3 - 1 - fileRow;           // bottom-up
            const unsigned char* row = buf + 54 + fileRow * rowBytes;
            for (int x = 0; x < w; x++) {
                const Pixel& p = c.at(x, y);
                if (row[x * 3] != p.b || row[x * 3 + 1] != p.g || row[x * 3 + 2] != p.r) pixelsOk = false;
            }
            for (int k = w * 3; k < rowBytes; k++) if (row[k] != 0) padOk = false;
        }
        CHECK(pixelsOk);
        CHECK(padOk);
    }
}

static void testPipelines() {
    ImageProcessing ip;
    Canvas a = pattern(5, 3);          // odd width -> padding
    Canvas b = pattern(2, 6);
    std::string bmpA = makeBmp("A", a);
    std::string bmpB = makeBmp("B", b);

    // BMP read round-trip through ProcessSingleImage(1.0, false)
    CHECK(ip.ProcessSingleImage(bmpA, OUT + "same.png", 1.0f, false));
    CHECK(sameCanvas(readPng(OUT + "same.png"), a));

    CHECK(ip.ProcessSingleImage(bmpA, OUT + "bright.png", 1.7f, false));
    CHECK(closeCanvas(readPng(OUT + "bright.png"), a.scaleBrightness(1.7f), 1));
    CHECK(ip.ProcessSingleImage(bmpA, OUT + "inv.png", 1.0f, true));
    CHECK(sameCanvas(readPng(OUT + "inv.png"), a.invertColors()));
    CHECK(ip.ProcessSingleImage(bmpA, OUT + "both.png", 1.3f, true));   // brightness THEN invert
    CHECK(closeCanvas(readPng(OUT + "both.png"), a.scaleBrightness(1.3f).invertColors(), 1));
    CHECK(ip.ProcessSingleImage(bmpA, OUT + "rgb.png", 2.0f, 0.5f, 0.0f, false));
    CHECK(closeCanvas(readPng(OUT + "rgb.png"), a.scaleBrightness(2.0f, 0.5f, 0.0f), 1));
    CHECK(ip.ProcessSingleImage(bmpA, OUT + "rgbinv.png", 2.0f, 0.5f, 0.0f, true));
    CHECK(closeCanvas(readPng(OUT + "rgbinv.png"), a.scaleBrightness(2.0f, 0.5f, 0.0f).invertColors(), 1));

    CHECK(ip.StitchImages(bmpA, bmpB, OUT + "h.png", true));
    CHECK(sameCanvas(readPng(OUT + "h.png"), a.stitchHorizontal(b)));
    CHECK(ip.StitchImages(bmpA, bmpB, OUT + "v.png", false));
    CHECK(sameCanvas(readPng(OUT + "v.png"), a.stackVertical(b)));

    CHECK(ip.CropAndSave(bmpA, OUT + "crop.png", 1, 1, 3, 2));
    CHECK(sameCanvas(readPng(OUT + "crop.png"), a.crop(1, 1, 3, 2)));
    CHECK(ip.CropAndSave(bmpA, OUT + "cropall.png", 0, 0, 5, 3));
    CHECK(sameCanvas(readPng(OUT + "cropall.png"), a));
    CHECK(!ip.CropAndSave(bmpA, OUT + "x.png", 0, 0, 0, 1));
    CHECK(!ip.CropAndSave(bmpA, OUT + "x.png", 0, 0, 1, -1));
    CHECK(!ip.CropAndSave(bmpA, OUT + "x.png", -1, 0, 2, 2));
    CHECK(!ip.CropAndSave(bmpA, OUT + "x.png", 0, -1, 2, 2));
    CHECK(!ip.CropAndSave(bmpA, OUT + "x.png", 4, 0, 2, 1));     // sticks out right
    CHECK(!ip.CropAndSave(bmpA, OUT + "x.png", 0, 2, 1, 2));     // sticks out bottom
    CHECK(!ip.CropAndSave(bmpA, OUT + "x.png", 2147483647, 0, 2147483647, 1));

    CHECK(ip.PasteImage(bmpA, bmpB, OUT + "paste.png", 3, -2));
    Canvas expectPaste(a); expectPaste.paste(b, 3, -2);
    CHECK(sameCanvas(readPng(OUT + "paste.png"), expectPaste));
    Pixel key = b.at(0, 3);
    CHECK(ip.PasteImage(bmpA, bmpB, OUT + "pastekey.png", 1, -1, key.r, key.g, key.b));
    Canvas expectKey(a); expectKey.paste(b, 1, -1, key);
    CHECK(sameCanvas(readPng(OUT + "pastekey.png"), expectKey));
    CHECK(!sameCanvas(expectKey, expectPaste));

    CHECK(ip.TransformGeometry(bmpA, OUT + "fh.png", 0));
    CHECK(sameCanvas(readPng(OUT + "fh.png"), a.flipHorizontal()));
    CHECK(ip.TransformGeometry(bmpA, OUT + "fv.png", 1));
    CHECK(sameCanvas(readPng(OUT + "fv.png"), a.flipVertical()));
    CHECK(ip.TransformGeometry(bmpA, OUT + "rot.png", 2));
    CHECK(sameCanvas(readPng(OUT + "rot.png"), a.rotate90Clockwise()));
    CHECK(!ip.TransformGeometry(bmpA, OUT + "x.png", 3));
    CHECK(!ip.TransformGeometry(bmpA, OUT + "x.png", -1));

    float* sharpen = new float[9];
    float sh[9] = { 0, -1, 0, -1, 5, -1, 0, -1, 0 };
    for (int i = 0; i < 9; i++) sharpen[i] = sh[i];
    CHECK(ip.ApplyConvolution(bmpA, OUT + "sharp.png", sharpen, 3));
    CHECK(closeCanvas(readPng(OUT + "sharp.png"), a.applyFilter(sh, 3), 1));
    CHECK(sharpen[4] == 5.0f);
    delete[] sharpen;
    float w1 = 0.5f;
    CHECK(ip.ApplyConvolution(bmpA, OUT + "k1.png", &w1, 1));
    CHECK(!ip.ApplyConvolution(bmpA, OUT + "x.png", sh, 2));
    CHECK(!ip.ApplyConvolution(bmpA, OUT + "x.png", sh, 0));
    CHECK(!ip.ApplyConvolution(bmpA, OUT + "x.png", sh, -3));
    CHECK(!ip.ApplyConvolution(bmpA, OUT + "x.png", nullptr, 3));

    // JPG input (made by make_fixtures.py) -> BMP -> PNG
    CHECK(ip.TranscodeToBMP("fixtures/j.jpg", OUT + "j.bmp"));
    CHECK(ip.ProcessSingleImage(OUT + "j.bmp", OUT + "j.png", 1.0f, false));
    Canvas jpgBack = readPng(OUT + "j.png");
    CHECK(jpgBack.getWidth() == 7 && jpgBack.getHeight() == 4);
    // Compare with what stb itself decodes from the JPG
    CHECK(sameCanvas(jpgBack, readPng("fixtures/j.jpg")));

    // Grayscale and RGBA PNGs are converted to 3 channels by stbi_load(..., 3)
    CHECK(ip.TranscodeToBMP("fixtures/gray.png", OUT + "gray.bmp"));
    CHECK(ip.ProcessSingleImage(OUT + "gray.bmp", OUT + "gray_out.png", 1.0f, false));
    Canvas gray = readPng(OUT + "gray_out.png");
    CHECK(gray.getWidth() == 3 && gray.getHeight() == 2 && same(gray.at(2, 1), px(250, 250, 250)) && same(gray.at(1, 0), px(50, 50, 50)));
    CHECK(ip.TranscodeToBMP("fixtures/rgba.png", OUT + "rgba.bmp"));
    CHECK(ip.ProcessSingleImage(OUT + "rgba.bmp", OUT + "rgba_out.png", 1.0f, false));
    Canvas rgba = readPng(OUT + "rgba_out.png");
    CHECK(rgba.getWidth() == 2 && same(rgba.at(0, 0), px(10, 20, 30)) && same(rgba.at(1, 0), px(40, 50, 60)));

    // Repeated operations (leaks would show up under ASan)
    for (int i = 0; i < 30; i++) {
        CHECK(ip.TransformGeometry(bmpB, OUT + "loop.png", i % 3));
        CHECK(ip.StitchImages(bmpB, bmpA, OUT + "loop2.png", i % 2 == 0));
    }
}

static void testFailures() {
    ImageProcessing ip;
    Canvas a = pattern(5, 3);
    std::string good = makeBmp("good", a);
    std::string nodir = "no_such_dir/out.png";

    // Missing / non-image inputs
    CHECK(!ip.TranscodeToBMP("missing.png", OUT + "m.bmp"));
    writeFile(OUT + "text.png", (const unsigned char*)"hello world", 11);
    CHECK(!ip.TranscodeToBMP(OUT + "text.png", OUT + "m.bmp"));
    CHECK(!ip.ProcessSingleImage("missing.bmp", OUT + "x.png", 1.0f, false));
    CHECK(!ip.ProcessSingleImage("missing.bmp", OUT + "x.png", 1, 1, 1, false));
    CHECK(!ip.StitchImages(good, "missing.bmp", OUT + "x.png", true));
    CHECK(!ip.StitchImages("missing.bmp", good, OUT + "x.png", false));
    CHECK(!ip.CropAndSave("missing.bmp", OUT + "x.png", 0, 0, 1, 1));
    CHECK(!ip.PasteImage(good, "missing.bmp", OUT + "x.png", 0, 0));
    CHECK(!ip.PasteImage("missing.bmp", good, OUT + "x.png", 0, 0, 1, 2, 3));
    CHECK(!ip.TransformGeometry("missing.bmp", OUT + "x.png", 0));
    float id = 1.0f;
    CHECK(!ip.ApplyConvolution("missing.bmp", OUT + "x.png", &id, 1));
    // PNG passed where BMP expected
    CHECK(!ip.ProcessSingleImage(OUT + "good.png", OUT + "x.png", 1.0f, false));

    // Output cannot be written
    CHECK(!ip.TranscodeToBMP(OUT + "good.png", nodir));
    CHECK(!ip.ProcessSingleImage(good, nodir, 1.0f, false));
    CHECK(!ip.ProcessSingleImage(good, nodir, 1, 1, 1, true));
    CHECK(!ip.StitchImages(good, good, nodir, true));
    CHECK(!ip.CropAndSave(good, nodir, 0, 0, 1, 1));
    CHECK(!ip.PasteImage(good, good, nodir, 0, 0));
    CHECK(!ip.PasteImage(good, good, nodir, 0, 0, 1, 2, 3));
    CHECK(!ip.TransformGeometry(good, nodir, 2));
    CHECK(!ip.ApplyConvolution(good, nodir, &id, 1));

    // Malformed BMPs, all made from a valid file
    static unsigned char buf[4096];
    int size = 0;
    readFile(good, buf, sizeof(buf), size);
    auto mutate = [&](const std::string& name, int offset, unsigned char value) {
        static unsigned char copy[4096];
        for (int i = 0; i < size; i++) copy[i] = buf[i];
        copy[offset] = value;
        writeFile(OUT + name, copy, size);
        return OUT + name;
        };
    CHECK(!ip.ProcessSingleImage(mutate("magic.bmp", 0, 'X'), OUT + "x.png", 1, false));
    CHECK(!ip.ProcessSingleImage(mutate("bpp32.bmp", 28, 32), OUT + "x.png", 1, false));
    CHECK(!ip.ProcessSingleImage(mutate("comp.bmp", 30, 1), OUT + "x.png", 1, false));
    CHECK(!ip.ProcessSingleImage(mutate("negh.bmp", 25, 0xFF), OUT + "x.png", 1, false));  // height < 0
    CHECK(!ip.ProcessSingleImage(mutate("negw.bmp", 21, 0xFF), OUT + "x.png", 1, false));  // width < 0
    {   // width 0
        static unsigned char c[4096]; for (int i = 0; i < size; i++) c[i] = buf[i];
        c[18] = c[19] = c[20] = c[21] = 0; writeFile(OUT + "w0.bmp", c, size);
        CHECK(!ip.ProcessSingleImage(OUT + "w0.bmp", OUT + "x.png", 1, false));
    }
    {   // huge width/height but tiny file: must fail without a giant allocation
        static unsigned char c[4096]; for (int i = 0; i < size; i++) c[i] = buf[i];
        c[18] = 0xFF; c[19] = 0xFF; c[20] = 0xFF; c[21] = 0x7F;
        c[22] = 0xFF; c[23] = 0xFF; c[24] = 0xFF; c[25] = 0x7F;
        writeFile(OUT + "huge.bmp", c, size);
        CHECK(!ip.ProcessSingleImage(OUT + "huge.bmp", OUT + "x.png", 1, false));
    }
    {   // data offset beyond end of file
        static unsigned char c[4096]; for (int i = 0; i < size; i++) c[i] = buf[i];
        c[10] = 0xFF; c[11] = 0xFF; c[12] = 0; c[13] = 0;
        writeFile(OUT + "offset.bmp", c, size);
        CHECK(!ip.ProcessSingleImage(OUT + "offset.bmp", OUT + "x.png", 1, false));
    }
    writeFile(OUT + "trunc.bmp", buf, size - 1);                  // one byte short
    CHECK(!ip.ProcessSingleImage(OUT + "trunc.bmp", OUT + "x.png", 1, false));
    writeFile(OUT + "short.bmp", buf, 30);                        // shorter than header
    CHECK(!ip.ProcessSingleImage(OUT + "short.bmp", OUT + "x.png", 1, false));
    writeFile(OUT + "empty.bmp", buf, 0);
    CHECK(!ip.ProcessSingleImage(OUT + "empty.bmp", OUT + "x.png", 1, false));

    {   // Valid BMP whose pixel data starts at a non-54 offset (gap of 10 bytes)
        static unsigned char c[4096];
        for (int i = 0; i < 54; i++) c[i] = buf[i];
        for (int i = 54; i < 64; i++) c[i] = 0xAB;
        for (int i = 54; i < size; i++) c[i + 10] = buf[i];
        c[10] = 64;
        writeFile(OUT + "gap.bmp", c, size + 10);
        CHECK(ip.ProcessSingleImage(OUT + "gap.bmp", OUT + "gap.png", 1.0f, false));
        CHECK(sameCanvas(readPng(OUT + "gap.png"), a));
    }
    {   // Extra trailing bytes are fine
        static unsigned char c[4096];
        for (int i = 0; i < size; i++) c[i] = buf[i];
        c[size] = 1; c[size + 1] = 2;
        writeFile(OUT + "extra.bmp", c, size + 2);
        CHECK(ip.ProcessSingleImage(OUT + "extra.bmp", OUT + "extra.png", 1.0f, false));
    }
}

int main() {
    testLifecycle();
    testLoadFromRaw();
    testColor();
    testComposition();
    testCropPaste();
    testGeometry();
    testFilter();
    testBmpWriter();
    testPipelines();
    testFailures();
#ifdef TWO_FILES
    CHECK(secondTranslationUnitWidth() == 7);
    std::printf("(two-file link test ran)\n");
#endif

    // Optional: a real photo, if one is passed in via sample.png / sample.jpg
    ImageProcessing ip;
    CHECK(ip.TranscodeToBMP("fixtures/sample.jpg", OUT + "sample.bmp"));
    {
        float blur[25]; for (int i = 0; i < 25; i++) blur[i] = 1.0f / 25.0f;
        CHECK(ip.ApplyConvolution(OUT + "sample.bmp", OUT + "sample_blur.png", blur, 5));
        CHECK(ip.TransformGeometry(OUT + "sample.bmp", OUT + "sample_rot.png", 2));
        CHECK(ip.ProcessSingleImage(OUT + "sample.bmp", OUT + "sample_neg.png", 1.2f, true));
        std::printf("(sample image processed)\n");
    }

    std::printf("passed: %d  failed: %d\n", passed, failed);
    return failed == 0 ? 0 : 1;
}
