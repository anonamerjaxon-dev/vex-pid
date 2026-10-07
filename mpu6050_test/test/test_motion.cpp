// ═══════════════════════════════════════════════════════════════════════
//  test_motion.cpp — host-side verification of motion_detect.h
//
//  Build & run:  c++ -std=c++17 -O2 -o /tmp/test_motion test/test_motion.cpp && /tmp/test_motion
//
//  Feeds synthetic signals whose correct classification is known by
//  construction, so a threshold regression shows up as a failing assert
//  instead of as a mysterious "it doesn't detect my drop" on the bench.
// ═══════════════════════════════════════════════════════════════════════
#include "../src/motion_detect.h"

#include <cstdio>
#include <cmath>
#include <vector>
#include <string>

static int g_pass = 0, g_fail = 0;
static std::string g_case;

static void check(bool ok, const std::string& what) {
    if (ok) { g_pass++; printf("    ok   %s\n", what.c_str()); }
    else    { g_fail++; printf("    FAIL %s\n", what.c_str()); }
}
static void begin_case(const char* name) {
    g_case = name;
    printf("\n== %s ==\n", name);
}

static const float DT_MS = 5.0f;   // 200 Hz
static const float DT_S  = 0.005f;

// A synthetic scenario: a callback produces acceleration (g) and gyro (deg/s).
struct Sample { float a[3]; float g[3]; };

static md::Output run(md::Detector& det, const std::vector<Sample>& s,
                      md::Output* out_last = nullptr,
                      uint16_t* anyEvents = nullptr) {
    md::Output o;
    uint16_t acc = 0;
    for (size_t i = 0; i < s.size(); i++) {
        o = det.update(s[i].a, s[i].g, DT_MS);
        acc |= o.events;
    }
    if (out_last) *out_last = o;
    if (anyEvents) *anyEvents = acc;
    return o;
}

static Sample rest() { Sample s; s.a[0]=0; s.a[1]=0; s.a[2]=1.0f; s.g[0]=s.g[1]=s.g[2]=0; return s; }

int main() {
    printf("motion_detect.h verification @ %.0f Hz\n", 1.0f / DT_S);

    // ────────────────────────────────────────────────────────────────
    begin_case("1. Still sensor is detected as still, not as sway");
    {
        md::Detector d; d.begin(md::Config());
        std::vector<Sample> v(400, rest());          // 2.0 s
        md::Output o; uint16_t ev;
        run(d, v, &o, &ev);
        check(o.still, "still == true");
        check(!o.sway,  "sway == false");
        check(!o.shake, "shake == false");
        check(!o.freefall, "freefall == false");
        check((ev & md::EV_STILL) != 0, "EV_STILL fired");
        check((ev & md::EV_IMPACT) == 0, "no spurious impact");
        check((ev & md::EV_DROP) == 0, "no spurious drop");
    }

    // ────────────────────────────────────────────────────────────────
    begin_case("2. Drop: free-fall then landing impact");
    {
        md::Detector d; d.begin(md::Config());
        std::vector<Sample> v;
        for (int i = 0; i < 100; i++) v.push_back(rest());          // 0.5 s at rest
        // 0.30 s of weightlessness at g = 0.10
        for (int i = 0; i < 60; i++) { Sample s = rest(); s.a[2] = 0.10f; v.push_back(s); }
        // landing: one 6 g sample, then a decaying ring
        { Sample s = rest(); s.a[2] = 6.0f; v.push_back(s); }
        { Sample s = rest(); s.a[2] = 2.5f; v.push_back(s); }
        { Sample s = rest(); s.a[2] = 1.4f; v.push_back(s); }
        for (int i = 0; i < 200; i++) v.push_back(rest());
        md::Output o; uint16_t ev;
        run(d, v, &o, &ev);
        check((ev & md::EV_FREEFALL) != 0, "EV_FREEFALL fired");
        check((ev & md::EV_DROP) != 0,     "EV_DROP fired");
        check((ev & md::EV_IMPACT) != 0,   "EV_IMPACT fired on landing");
        check(o.countDrop == 1, "exactly one drop counted (got " + std::to_string(o.countDrop) + ")");
        check(o.impactG >= 5.9f, "impact peak captured >= 5.9 g (got " + std::to_string(o.impactG) + ")");
    }

    // ────────────────────────────────────────────────────────────────
    begin_case("3. Hard hit with no free-fall is an impact, not a drop");
    {
        md::Detector d; d.begin(md::Config());
        std::vector<Sample> v;
        for (int i = 0; i < 100; i++) v.push_back(rest());          // settle
        { Sample s = rest(); s.a[2] = 8.0f; v.push_back(s); }        // sharp spike
        { Sample s = rest(); s.a[2] = 3.0f; v.push_back(s); }
        for (int i = 0; i < 200; i++) v.push_back(rest());
        md::Output o; uint16_t ev;
        run(d, v, &o, &ev);
        check((ev & md::EV_IMPACT) != 0, "EV_IMPACT fired");
        check((ev & md::EV_DROP) == 0,   "EV_DROP did NOT fire");
        check((ev & md::EV_FREEFALL) == 0, "EV_FREEFALL did NOT fire");
        check(o.countImpact == 1, "exactly one impact (got " + std::to_string(o.countImpact) + ")");
        check(o.countDrop == 0, "zero drops (got " + std::to_string(o.countDrop) + ")");
    }

    // ────────────────────────────────────────────────────────────────
    begin_case("4. One physical impact rings but is counted once");
    {
        md::Detector d; d.begin(md::Config());
        std::vector<Sample> v;
        for (int i = 0; i < 100; i++) v.push_back(rest());
        // A ringing impact: 8 samples bouncing above the 2.2 g threshold.
        float ring[8] = {5.0f, 3.1f, 2.6f, 3.4f, 2.4f, 2.9f, 1.8f, 1.3f};
        for (int i = 0; i < 8; i++) { Sample s = rest(); s.a[2] = ring[i]; v.push_back(s); }
        for (int i = 0; i < 200; i++) v.push_back(rest());
        md::Output o; uint16_t ev;
        run(d, v, &o, &ev);
        check(o.countImpact == 1, "refractory collapsed the ring to 1 impact (got "
                                  + std::to_string(o.countImpact) + ")");
    }

    // ────────────────────────────────────────────────────────────────
    begin_case("5. Gentle sway at 0.8 Hz is Sway, not Shake");
    {
        md::Detector d; d.begin(md::Config());
        std::vector<Sample> v;
        for (int i = 0; i < 1600; i++) {                  // 8 s
            float t = i * DT_S;
            Sample s = rest();
            s.a[0] = 0.15f * sinf(2.0f * 3.14159265f * 0.8f * t);
            v.push_back(s);
        }
        md::Output o; uint16_t ev;
        run(d, v, &o, &ev);
        check(o.sway, "sway == true at the end");
        check(!o.shake, "shake == false");
        check((ev & md::EV_SWAY) != 0, "EV_SWAY fired");
        check(o.countSway >= 1, "sway counted");
        check(fabsf(o.swayHz - 0.8f) < 0.35f,
              "estimated frequency near 0.8 Hz (got " + std::to_string(o.swayHz) + ")");
        check(o.swayRmsG > 0.02f,
              "sway RMS above floor (got " + std::to_string(o.swayRmsG) + ")");
    }

    // ────────────────────────────────────────────────────────────────
    begin_case("6. Fast shaking at 5 Hz is Shake, not Sway");
    {
        md::Detector d; d.begin(md::Config());
        std::vector<Sample> v;
        for (int i = 0; i < 800; i++) {                   // 4 s
            float t = i * DT_S;
            Sample s = rest();
            s.a[0] = 0.80f * sinf(2.0f * 3.14159265f * 5.0f * t);
            v.push_back(s);
        }
        md::Output o; uint16_t ev;
        run(d, v, &o, &ev);
        check(o.shake, "shake == true");
        check(!o.sway, "sway == false");
        check((ev & md::EV_SHAKE) != 0, "EV_SHAKE fired");
        check(o.countShake >= 1, "shake counted");
    }

    // ────────────────────────────────────────────────────────────────
    begin_case("7. Slow tilt is not motion (no false sway/shake/impact)");
    {
        md::Detector d; d.begin(md::Config());
        std::vector<Sample> v;
        for (int i = 0; i < 1200; i++) {                  // 6 s, +-35 deg tilt
            float t = i * DT_S;
            float th = 0.6f * sinf(2.0f * 3.14159265f * 0.15f * t);
            Sample s = rest();
            s.a[0] = sinf(th); s.a[2] = cosf(th);
            v.push_back(s);
        }
        md::Output o; uint16_t ev;
        run(d, v, &o, &ev);
        check((ev & md::EV_IMPACT) == 0, "no false impact");
        check((ev & md::EV_DROP) == 0,   "no false drop");
        check(!o.shake, "no false shake");
    }

    // ────────────────────────────────────────────────────────────────
    begin_case("8. Toss in the air (long weightlessness) is not a drop");
    {
        md::Detector d; d.begin(md::Config());
        std::vector<Sample> v;
        for (int i = 0; i < 100; i++) v.push_back(rest());
        // 2.5 s weightless - longer than the 1.5 s drop window
        for (int i = 0; i < 500; i++) { Sample s = rest(); s.a[2] = 0.08f; v.push_back(s); }
        for (int i = 0; i < 100; i++) v.push_back(rest());
        md::Output o; uint16_t ev;
        run(d, v, &o, &ev);
        check((ev & md::EV_FREEFALL) != 0, "free-fall was seen");
        check((ev & md::EV_DROP) == 0, "but no drop was declared");
    }

    // ────────────────────────────────────────────────────────────────
    begin_case("9. Light tap is a Tap, not an Impact");
    {
        md::Detector d; d.begin(md::Config());
        std::vector<Sample> v;
        for (int i = 0; i < 100; i++) v.push_back(rest());
        { Sample s = rest(); s.a[2] = 1.9f; v.push_back(s); }   // above tapG, below hitG
        { Sample s = rest(); s.a[2] = 1.1f; v.push_back(s); }
        for (int i = 0; i < 200; i++) v.push_back(rest());
        md::Output o; uint16_t ev;
        run(d, v, &o, &ev);
        check((ev & md::EV_TAP) != 0, "EV_TAP fired");
        check((ev & md::EV_IMPACT) == 0, "EV_IMPACT did not fire");
        check(o.countTap == 1, "one tap (got " + std::to_string(o.countTap) + ")");
    }

    // ────────────────────────────────────────────────────────────────
    begin_case("10. Orientation independence: sway on a tilted sensor");
    {
        // Sensor tilted 40 deg about world Y and swaying along world X.
        // Body-frame gravity is then (sin t, 0, cos t) and the world X axis
        // expressed in the body frame is (cos t, 0, -sin t), so the lateral
        // sway has to be projected onto both body X and body Z.  A detector
        // that assumes "gravity is always -Z" fails this case.
        md::Detector d; d.begin(md::Config());
        std::vector<Sample> v;
        const float th = 40.0f * 3.14159265f / 180.0f;
        for (int i = 0; i < 1600; i++) {
            float t = i * DT_S;
            float lat = 0.15f * sinf(2.0f * 3.14159265f * 0.8f * t);
            Sample s;
            s.a[0] = sinf(th) + lat * cosf(th);
            s.a[1] = 0.0f;
            s.a[2] = cosf(th) - lat * sinf(th);
            s.g[0] = s.g[1] = s.g[2] = 0;
            v.push_back(s);
        }
        md::Output o; uint16_t ev;
        run(d, v, &o, &ev);
        check(o.sway, "sway detected on a tilted sensor");
        check(fabsf(o.swayHz - 0.8f) < 0.40f,
              "frequency still near 0.8 Hz (got " + std::to_string(o.swayHz) + ")");
    }

    // ────────────────────────────────────────────────────────────────
    printf("\n────────────────────────────────────────\n");
    printf("%d passed, %d failed\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
