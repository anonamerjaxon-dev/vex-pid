// ═══════════════════════════════════════════════════════════════════════
//  motion_detect.h — real-time motion pattern detection for MPU-6000 family
//
//  WHY THIS RUNS ON THE ESP32 AND NOT IN THE BROWSER
//  -------------------------------------------------
//  The loop samples the sensor at 200 Hz but only streams 33 lines/s, so
//  83% of the samples never leave the chip.  A hit or a landing impact is
//  a transient lasting roughly 5-25 ms: at 33 Hz (30 ms between samples)
//  it is usually missed outright, and when it is caught the peak is
//  attenuated by 2-4x.  Detection therefore has to happen at the full
//  200 Hz, which is why this module lives in the firmware.
//
//  Pure C++ on purpose: no Arduino headers, no dynamic allocation, no
//  floating point library calls in the hot path.  That lets the exact same
//  code run in a host-side unit test (test/test_motion.cpp) against
//  synthetic drop / hit / sway / shake signals, so the thresholds are
//  verified before they ever reach hardware.
//
//  UNITS: acceleration is in g (1 g = 9.80665 m/s^2) with GRAVITY INCLUDED,
//         so a stationary sensor reads |a| = 1.0 g.  Gyro is in deg/s.
// ═══════════════════════════════════════════════════════════════════════
#pragma once

#include <math.h>
#include <stdint.h>

namespace md {

// ── Event bits ────────────────────────────────────────────────────────
// EV_* in `Output::events` are TRANSIENT: set only on the sample where the
// event fires.  The dashboard latches them.  The stable "is it happening
// right now" state is in the bool members (freefall/sway/still/shake).
enum : uint16_t {
    EV_FREEFALL = 1u << 0,   // entered weightlessness
    EV_DROP     = 1u << 1,   // free-fall followed by a landing impact
    EV_IMPACT   = 1u << 2,   // sharp high-g transient
    EV_TAP      = 1u << 3,   // light, very short knock
    EV_SWAY     = 1u << 4,   // sustained low-frequency oscillation (started)
    EV_SHAKE    = 1u << 5,   // fast bidirectional motion (started)
    EV_STILL    = 1u << 6,   // came to rest
    EV_MOVED    = 1u << 7,   // left the at-rest state
};

// ── Tunables ──────────────────────────────────────────────────────────
// Every threshold carries the reason it has the value it does.  The
// physical justification matters more than the number: these are the
// values a re-tune should start from, not magic constants.
struct Config {
    // Nominal sample rate the filter coefficients are designed for.
    float sampleHz = 200.0f;

    // ── Free-fall / drop ──────────────────────────────────────────────
    // Laptop hard-drive active protection parks the heads below ~0.5 g
    // held for 50-100 ms; that is the canonical tuned free-fall window.
    float freefallG     = 0.45f;
    float freefallMinMs = 60.0f;
    // A real drop lands hard.  2.5 g is roughly what a 30 cm drop onto a
    // desk produces on a bare PCB; lighter contacts stay below it.
    float impactG       = 2.50f;
    // A fall from a hand lasts 0.15-0.6 s.  Much longer than this and the
    // low-g reading was really a toss/spin, not a drop.
    float dropWindowMs  = 1500.0f;

    // ── Impact / hit ──────────────────────────────────────────────────
    float hitG          = 2.20f;
    // Rise rate in g per second.  At 200 Hz one sample is 5 ms, so a
    // 1 g -> 5 g edge is 800 g/s.  Vibration on a table is far slower.
    float hitJerkGps    = 250.0f;
    // One mechanical impact rings for tens of ms and would otherwise be
    // counted 3-6 times as the signal bounces around the threshold.
    float impactRefractMs = 180.0f;

    // ── Sway ──────────────────────────────────────────────────────────
    // Human postural / hand sway sits at 0.1-1 Hz; deliberate gentle
    // "swaying" of a held object lands around 0.3-2 Hz.  0.25 Hz is the
    // high-pass corner that removes slow drift and gravity leakage.
    float swayHighpassHz = 0.25f;
    float swayLowpassHz  = 2.50f;
    float swayWindowMs   = 4000.0f;
    // Sensor noise floor: the MPU at +/-16 g has ~0.0005 g LSB, but
    // filtered residual sits near 5 mg.  20 mg clears it comfortably.
    float swayRmsG       = 0.020f;
    // Min crossings must stay CONSISTENT with the band, because the
    // frequency estimate is crossings/(2*window).  Requiring N crossings
    // silently imposes a floor of N/(2*window) Hz: the old value of 6 over
    // 3 s meant 1.0 Hz, which made the 0.25 Hz bottom of the band
    // unreachable.  3 crossings over 4 s = 0.375 Hz floor, leaving 3 as a
    // pure noise guard.
    int   swayMinCross   = 3;

    // ── Still ─────────────────────────────────────────────────────────
    float stillG       = 0.020f;   // |a| within this of 1.000 g
    float stillGyroDps = 1.50f;    // and every axis below this
    float stillMinMs   = 400.0f;

    // ── Shake ─────────────────────────────────────────────────────────
    // Shaking is the same measurement as sway but fast and large; the
    // separation is frequency, so it gets its own pass band.
    float shakeHighpassHz = 2.50f;
    float shakeLowpassHz  = 15.0f;
    float shakeWindowMs   = 600.0f;
    float shakeRmsG       = 0.20f;
    // Same consistency rule: 3 crossings over 0.6 s = 2.5 Hz, matching
    // shakeHighpassHz.  The old 8 implied a 6.7 Hz floor and, combined with
    // the filter start-up transient, produced a one-shot false positive.
    int   shakeMinCross   = 3;

    // ── Tap ───────────────────────────────────────────────────────────
    float tapG      = 1.50f;    // above this ...
    float tapMaxMs  = 60.0f;    // ... for no longer than this ...
    float tapQuietG = 1.25f;    // ... then back under this within tapMaxMs
};

// ── Per-sample result ─────────────────────────────────────────────────
struct Output {
    // live states
    bool freefall = false;
    bool sway     = false;
    bool still    = false;
    bool shake    = false;

    // measurements
    float aMag    = 1.0f;   // |a| in g
    float jerk    = 0.0f;   // d|a|/dt in g/s
    float impactG = 0.0f;   // peak |a| of the most recent impact
    float swayHz  = 0.0f;   // dominant oscillation frequency
    float swayRmsG = 0.0f;  // RMS of the horizontal, band-passed signal
    float shakeRmsG = 0.0f;

    uint16_t events = 0;    // transient bits fired THIS sample

    // lifetime counters (saturating, so they never wrap in a demo)
    uint32_t countFreefall = 0;
    uint32_t countDrop     = 0;
    uint32_t countImpact   = 0;
    uint32_t countTap      = 0;
    uint32_t countSway     = 0;
    uint32_t countShake    = 0;
    uint32_t countStill    = 0;
};

// ═══════════════════════════════════════════════════════════════════════
//  Detector
// ═══════════════════════════════════════════════════════════════════════
class Detector {
public:
    void begin(const Config& cfg) {
        m_cfg = cfg;

        const float dt = 1.0f / (cfg.sampleHz > 0.0f ? cfg.sampleHz : 200.0f);

        // One-pole coefficient for a high-pass, y = a*(y_prev + x - x_prev)
        m_hpSway  = coefHp(cfg.swayHighpassHz,  dt);
        m_lpSway  = coefLp(cfg.swayLowpassHz,   dt);
        m_hpShake = coefHp(cfg.shakeHighpassHz, dt);
        m_lpShake = coefLp(cfg.shakeLowpassHz,  dt);
        // Gravity direction tracker: slow, because tilting must not look
        // like translation and vice versa.
        m_lpGrav  = coefLp(0.60f, dt);
        // Jerk is differentiated raw; a light low-pass keeps 5 ms sample
        // quantisation from dominating the derivative.
        m_lpJerk  = coefLp(35.0f, dt);

        reset();
    }

    void reset() {
        m_lpAx = m_lpAy = m_lpAz = 0.0f;
        m_gravValid = false;
        // X1/X2 are the previous input of each horizontal channel,
        // P1/P2 the previous output.
        m_hpSwayX1 = m_hpSwayX2 = m_hpSwayP1 = m_hpSwayP2 = 0.0f;
        m_lpSwayX1 = m_lpSwayX2 = 0.0f;
        m_hpShakeX1 = m_hpShakeX2 = m_hpShakeP1 = m_hpShakeP2 = 0.0f;
        m_lpShakeX1 = m_lpShakeX2 = 0.0f;
        m_rmsSway1 = m_rmsSway2 = 0.0f;
        m_rmsShake1 = m_rmsShake2 = 0.0f;
        m_signSway1 = m_signSway2 = 0;
        m_signShake1 = m_signShake2 = 0;
        for (int i = 0; i < XRING; i++) { m_crossSway[i] = 0; m_crossShake[i] = 0; }
        m_crossSwayHead = m_crossSwayTail = 0;
        m_crossShakeHead = m_crossShakeTail = 0;
        m_prevAMag = -1.0f;
        m_jerk = 0.0f;

        m_ffActive = false;  m_ffStartMs = 0;  m_lowSinceMs = 0;
        m_impactPeak = 0.0f;
        m_impactRefractUntilMs = 0; m_impactWindowUntilMs = 0;
        m_tapAboveSince = 0; m_tapActive = false;

        m_stillSinceMs = 0;  m_wasStill = false;
        m_swayActive = false; m_shakeActive = false;
        m_nowMs = 0;

        m_out = Output();
    }

    // Call once per AHRS sample.  a[3] in g (gravity included), g[3] in deg/s.
    const Output& update(const float a[3], const float g[3], float dtMs) {
        uint32_t nowMs = m_nowMs + (uint32_t)(dtMs + 0.5f);
        m_nowMs = nowMs;
        m_out.events = 0;

        // ── 1. Raw magnitudes ─────────────────────────────────────────
        float ax = a[0], ay = a[1], az = a[2];
        float aMag = sqrtf(ax*ax + ay*ay + az*az);
        m_out.aMag = aMag;

        // jerk = d|a|/dt, in g/s.  dtMs is ~5 ms.
        if (m_prevAMag >= 0.0f && dtMs > 0.01f) {
            float raw = (aMag - m_prevAMag) * 1000.0f / dtMs;
            m_jerk += m_lpJerk * (raw - m_jerk);
        }
        m_prevAMag = aMag;
        m_out.jerk = m_jerk;

        // ── 2. Gravity direction (body frame, unit) ───────────────────
        // A slow low-pass of the acceleration vector points along gravity.
        // Using the direction rather than assuming "down is -Z" is what
        // makes the horizontal channels correct at any orientation.
        if (!m_gravValid) {
            m_lpAx = ax; m_lpAy = ay; m_lpAz = az;
            m_gravValid = true;
        } else {
            m_lpAx += m_lpGrav * (ax - m_lpAx);
            m_lpAy += m_lpGrav * (ay - m_lpAy);
            m_lpAz += m_lpGrav * (az - m_lpAz);
        }
        float gn = sqrtf(m_lpAx*m_lpAx + m_lpAy*m_lpAy + m_lpAz*m_lpAz);
        float e1x = 0.0f, e1y = 0.0f, e1z = 0.0f;
        float e2x = 0.0f, e2y = 0.0f, e2z = 0.0f;
        bool haveFrame = false;
        if (gn > 0.20f) {                    // |a| near 0 in free-fall: no frame
            float ux = m_lpAx / gn, uy = m_lpAy / gn, uz = m_lpAz / gn;
            // Build a horizontal basis.  Cross with body +Z; if gravity is
            // nearly parallel to +Z the cross product degenerates, so fall
            // back to body +X.
            float rx = uy * 1.0f - uz * 0.0f;
            float ry = uz * 0.0f - ux * 1.0f;
            float rz = ux * 0.0f - uy * 0.0f;
            float rn = sqrtf(rx*rx + ry*ry + rz*rz);
            if (rn < 0.10f) {
                rx = uy * 0.0f - uz * 0.0f;   // cross(u, +X)
                ry = uz * 1.0f - ux * 0.0f;
                rz = ux * 0.0f - uy * 1.0f;
                rn = sqrtf(rx*rx + ry*ry + rz*rz);
            }
            if (rn > 1e-6f) {
                e1x = rx / rn; e1y = ry / rn; e1z = rz / rn;
                e2x = uy * e1z - uz * e1y;
                e2y = uz * e1x - ux * e1z;
                e2z = ux * e1y - uy * e1x;
                haveFrame = true;
            }
        }

        // Projection of a onto the horizontal plane.  Because e1 and e2 are
        // perpendicular to the tracked gravity direction, most of the
        // gravity term is already gone; the band-pass removes the rest.
        float h1 = 0.0f, h2 = 0.0f;
        if (haveFrame) {
            h1 = ax * e1x + ay * e1y + az * e1z;
            h2 = ax * e2x + ay * e2y + az * e2z;
        }

        // ── 3. Sway band (0.25 - 2.5 Hz) ──────────────────────────────
        float sp1 = hpf(m_hpSway, h1, m_hpSwayP1, m_hpSwayX1);
        float sp2 = hpf(m_hpSway, h2, m_hpSwayP2, m_hpSwayX2);
        float s1 = lpf(m_lpSway, sp1, m_lpSwayX1);
        float s2 = lpf(m_lpSway, sp2, m_lpSwayX2);

        // ── 4. Shake band (2.5 - 15 Hz) ───────────────────────────────
        float kp1 = hpf(m_hpShake, h1, m_hpShakeP1, m_hpShakeX1);
        float kp2 = hpf(m_hpShake, h2, m_hpShakeP2, m_hpShakeX2);
        float k1 = lpf(m_lpShake, kp1, m_lpShakeX1);
        float k2 = lpf(m_lpShake, kp2, m_lpShakeX2);

        // ── 5. Running RMS (one-pole on the square) ───────────────────
        // 0.5 s time constant on the power estimate.
        const float rmsA = 0.02f;
        m_rmsSway1  += rmsA * (s1*s1 - m_rmsSway1);
        m_rmsSway2  += rmsA * (s2*s2 - m_rmsSway2);
        m_rmsShake1 += rmsA * (k1*k1 - m_rmsShake1);
        m_rmsShake2 += rmsA * (k2*k2 - m_rmsShake2);
        float swayRms  = sqrtf(m_rmsSway1 + m_rmsSway2);
        float shakeRms = sqrtf(m_rmsShake1 + m_rmsShake2);
        m_out.swayRmsG  = swayRms;
        m_out.shakeRmsG = shakeRms;

        // ── 6. Zero-crossing frequency estimate ───────────────────────
        float swayDb  = 0.15f * swayRms  + 0.0005f;
        float shakeDb = 0.15f * shakeRms + 0.0005f;
        countCrossing(s1, swayDb,  m_signSway1,  m_crossSway,  m_crossSwayHead,  m_crossSwayTail,  nowMs);
        countCrossing(s2, swayDb,  m_signSway2,  m_crossSway,  m_crossSwayHead,  m_crossSwayTail,  nowMs);
        countCrossing(k1, shakeDb, m_signShake1, m_crossShake, m_crossShakeHead, m_crossShakeTail, nowMs);
        countCrossing(k2, shakeDb, m_signShake2, m_crossShake, m_crossShakeHead, m_crossShakeTail, nowMs);

        int swayCross  = prune(m_crossSway,  m_crossSwayHead,  m_crossSwayTail,
                               nowMs, (uint32_t)m_cfg.swayWindowMs);
        int shakeCross = prune(m_crossShake, m_crossShakeHead, m_crossShakeTail,
                               nowMs, (uint32_t)m_cfg.shakeWindowMs);

        // f = crossings / (2 * windowSeconds)
        float swayHz = 0.0f;
        if (m_cfg.swayWindowMs > 1.0f)
            swayHz = (float)swayCross / (2.0f * m_cfg.swayWindowMs * 0.001f);
        m_out.swayHz = swayHz;

        bool swayNow = (swayCross >= m_cfg.swayMinCross) &&
                       (swayRms  >= m_cfg.swayRmsG) &&
                       (swayHz  >= m_cfg.swayHighpassHz) &&
                       (swayHz  <= m_cfg.swayLowpassHz + 0.5f) &&
                       !m_ffActive;
        bool shakeNow = (shakeCross >= m_cfg.shakeMinCross) &&
                        (shakeRms  >= m_cfg.shakeRmsG);

        // Shake outranks sway: the same motion cannot be both.
        if (shakeNow) swayNow = false;

        if (swayNow && !m_swayActive)  { m_out.events |= EV_SWAY;  m_out.countSway++; }
        if (shakeNow && !m_shakeActive) { m_out.events |= EV_SHAKE; m_out.countShake++; }
        m_swayActive  = swayNow;
        m_shakeActive = shakeNow;
        m_out.sway  = swayNow;
        m_out.shake = shakeNow;

        // ── 7. Still ──────────────────────────────────────────────────
        bool quiet = (fabsf(aMag - 1.0f) <= m_cfg.stillG) &&
                     (fabsf(g[0]) < m_cfg.stillGyroDps) &&
                     (fabsf(g[1]) < m_cfg.stillGyroDps) &&
                     (fabsf(g[2]) < m_cfg.stillGyroDps);
        if (quiet) {
            if (m_stillSinceMs == 0) m_stillSinceMs = nowMs;
        } else {
            m_stillSinceMs = 0;
        }
        bool stillNow = m_stillSinceMs != 0 &&
                        (nowMs - m_stillSinceMs >= (uint32_t)m_cfg.stillMinMs);
        if (stillNow && !m_wasStill) { m_out.events |= EV_STILL; m_out.countStill++; }
        if (!stillNow && m_wasStill) { m_out.events |= EV_MOVED; }
        m_wasStill = stillNow;
        m_out.still = stillNow;

        // ── 8. Free-fall -> drop ──────────────────────────────────────
        bool lowG = aMag < m_cfg.freefallG;
        if (!m_ffActive) {
            if (lowG) {
                if (m_lowSinceMs == 0) m_lowSinceMs = nowMs;
                if (nowMs - m_lowSinceMs >= (uint32_t)m_cfg.freefallMinMs) {
                    m_ffActive = true;
                    m_ffStartMs = m_lowSinceMs;
                    m_out.events |= EV_FREEFALL;
                    m_out.countFreefall++;
                }
            } else {
                m_lowSinceMs = 0;
            }
        } else {
            // In free-fall.  Look for the landing.
            if (aMag > m_cfg.impactG) {
                m_impactPeak = aMag;
                m_out.impactG = aMag;
                m_out.events |= EV_DROP | EV_IMPACT;
                m_out.countDrop++;
                m_out.countImpact++;
                m_ffActive = false;
                m_lowSinceMs = 0;
                m_impactRefractUntilMs = nowMs + (uint32_t)m_cfg.impactRefractMs;
                m_impactWindowUntilMs  = nowMs + 120u;
            } else if (nowMs - m_ffStartMs > (uint32_t)m_cfg.dropWindowMs) {
                // Weightless for too long to be a drop - it was a toss.
                m_ffActive = false;
                m_lowSinceMs = 0;
            }
        }
        m_out.freefall = m_ffActive;

        // ── 9. Impact / tap (only outside the drop path) ──────────────
        bool inRefract = nowMs < m_impactRefractUntilMs;
        if (aMag > m_cfg.hitG && m_jerk > m_cfg.hitJerkGps && !inRefract) {
            m_out.events |= EV_IMPACT;
            m_out.countImpact++;
            m_out.impactG = aMag;
            m_impactPeak = aMag;
            m_impactRefractUntilMs = nowMs + (uint32_t)m_cfg.impactRefractMs;
            m_impactWindowUntilMs  = nowMs + 120u;
        }
        // Track the true peak for ~120 ms after the trigger so the reported
        // figure is the peak, not whichever sample happened to cross first.
        if (nowMs < m_impactWindowUntilMs && aMag > m_impactPeak) {
            m_impactPeak = aMag;
            m_out.impactG = aMag;
        }

        // Tap: a light, very short knock.  It must rise above tapG and fall
        // back below tapQuietG quickly - that is what separates a tap from
        // a sustained push or from ordinary handling.
        if (!m_tapActive) {
            if (aMag > m_cfg.tapG && !inRefract) {
                if (m_tapAboveSince == 0) m_tapAboveSince = nowMs;
                m_tapActive = true;
            }
        } else {
            if (aMag > m_cfg.tapQuietG) {
                if (nowMs - m_tapAboveSince > (uint32_t)m_cfg.tapMaxMs) {
                    m_tapActive = false;    // too long to be a tap
                    m_tapAboveSince = 0;
                }
            } else {
                if (aMag > m_cfg.hitG) {
                    // already reported as an impact above
                } else {
                    m_out.events |= EV_TAP;
                    m_out.countTap++;
                }
                m_tapActive = false;
                m_tapAboveSince = 0;
            }
        }

        // ── 10. Publish ───────────────────────────────────────────────
        // The counters live in m_out and persist across calls; only the
        // per-sample event bits are cleared at the top of update().
        return m_out;
    }

    const Config& config() const { return m_cfg; }
    const Output& last() const { return m_out; }

private:
    static const int XRING = 128;

    static float coefLp(float fc, float dt) {
        return 1.0f - expf(-2.0f * 3.14159265f * fc * dt);
    }
    static float coefHp(float fc, float dt) {
        return expf(-2.0f * 3.14159265f * fc * dt);
    }
    static float hpf(float a, float x, float& xPrev, float& yPrev) {
        float y = a * (yPrev + x - xPrev);
        xPrev = x; yPrev = y;
        return y;
    }
    static float lpf(float a, float x, float& yPrev) {
        yPrev += a * (x - yPrev);
        return yPrev;
    }

    // Register a zero crossing of `v` into the ring, with hysteresis.
    // A fixed tiny deadband lets band-passed noise rattle across zero and
    // inflate the crossing count, which reads out as a too-high frequency.
    // Scaling the deadband to the running RMS is the standard Schmitt
    // trigger fix and keeps the frequency estimate honest.
    void countCrossing(float v, float deadband, int& sign,
                       uint32_t* ring, int& head, int& tail, uint32_t nowMs) {
        int s = (v > deadband) ? 1 : ((v < -deadband) ? -1 : 0);
        if (s != 0) {
            if (sign != 0 && s != sign) {
                ring[head] = nowMs;
                head = (head + 1) % XRING;
                if (head == tail) tail = (tail + 1) % XRING;   // overwrite oldest
            }
            sign = s;
        }
    }

    // Drop crossings older than `windowMs`, return how many remain.
    int prune(uint32_t* ring, int head, int& tail, uint32_t nowMs, uint32_t windowMs) {
        while (tail != head) {
            uint32_t age = nowMs - ring[tail];
            if (age <= windowMs) break;
            tail = (tail + 1) % XRING;
        }
        int n = head - tail;
        if (n < 0) n += XRING;
        return n;
    }

    Config m_cfg;

    // filter coefficients
    float m_hpSway = 0, m_lpSway = 0, m_hpShake = 0, m_lpShake = 0;
    float m_lpGrav = 0, m_lpJerk = 0;

    // gravity tracker
    float m_lpAx = 0, m_lpAy = 0, m_lpAz = 0;
    bool  m_gravValid = false;

    // sway filter state
    float m_hpSwayX1 = 0, m_hpSwayX2 = 0, m_hpSwayP1 = 0, m_hpSwayP2 = 0;
    float m_lpSwayX1 = 0, m_lpSwayX2 = 0;
    // shake filter state
    float m_hpShakeX1 = 0, m_hpShakeX2 = 0, m_hpShakeP1 = 0, m_hpShakeP2 = 0;
    float m_lpShakeX1 = 0, m_lpShakeX2 = 0;

    float m_rmsSway1 = 0, m_rmsSway2 = 0, m_rmsShake1 = 0, m_rmsShake2 = 0;
    int   m_signSway1 = 0, m_signSway2 = 0, m_signShake1 = 0, m_signShake2 = 0;

    uint32_t m_crossSway[XRING];
    uint32_t m_crossShake[XRING];
    int m_crossSwayHead = 0, m_crossSwayTail = 0;
    int m_crossShakeHead = 0, m_crossShakeTail = 0;

    float m_prevAMag = -1.0f;
    float m_jerk = 0.0f;

    bool     m_ffActive = false;
    uint32_t m_ffStartMs = 0;
    uint32_t m_lowSinceMs = 0;
    uint32_t m_impactRefractUntilMs = 0;
    uint32_t m_impactWindowUntilMs = 0;
    float    m_impactPeak = 0.0f;

    bool     m_tapActive = false;
    uint32_t m_tapAboveSince = 0;

    uint32_t m_stillSinceMs = 0;
    bool     m_wasStill = false;
    bool     m_swayActive = false, m_shakeActive = false;

    uint32_t m_nowMs = 0;
    Output   m_out;
};

} // namespace md
