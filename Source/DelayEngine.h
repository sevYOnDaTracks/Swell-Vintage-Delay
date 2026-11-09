#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace swell {
constexpr float kPi = 3.14159265358979f;

enum DelayMode { Mono = 0, Stereo = 1, PingPong = 2 };

// Note values offered when SYNC is on, in quarter notes (beats).
struct Division { const char* name; float beats; };
inline constexpr std::array<Division, 18> divisions = {{
    { "1/64", 1.f / 16 }, { "1/32T", 1.f / 12 }, { "1/32", 1.f / 8 }, { "1/16T", 1.f / 6 }, { "1/32D", 3.f / 16 }, { "1/16", 1.f / 4 },
    { "1/8T", 1.f / 3 },  { "1/16D", 3.f / 8 },  { "1/8", 1.f / 2 },  { "1/4T", 2.f / 3 },  { "1/8D", 3.f / 4 },   { "1/4", 1.f },
    { "1/2T", 4.f / 3 },  { "1/4D", 3.f / 2 },   { "1/2", 2.f },      { "1/1T", 8.f / 3 },  { "1/2D", 3.f },       { "1/1", 4.f },
}};
inline constexpr float maxDelaySeconds = 4.f;

struct DelayParams {
    float delaySec = .5f;
    float feedback = .45f;       // 0 .. 1.1 (above 1 = infinite, held by the soft clipper)
    float lowCut = 150, highCut = 6000;
    float wow = .15f, flutter = .1f, saturation = .2f; // 0..1
    float duck = 0, duckReleaseMs = 250;
    float width = 1;             // 0..2
    float offsetMs = 0;          // right line offset in Stereo mode
    int mode = PingPong;
    bool space = false;
    float spaceAmount = .6f;     // 0..1
};

// RBJ biquad, used for the low / high cut inside the feedback loop.
struct Biquad {
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    void set(bool high, float f, float sr) {
        f = std::clamp(f, 10.f, sr * .45f);
        float w = 2 * kPi * f / sr, c = std::cos(w), alpha = std::sin(w) / (2 * .7071f), a0 = 1 + alpha;
        if (high) { b0 = (1 + c) / 2 / a0; b1 = -(1 + c) / a0; }
        else      { b0 = (1 - c) / 2 / a0; b1 = (1 - c) / a0; }
        b2 = b0; a1 = -2 * c / a0; a2 = (1 - alpha) / a0;
    }
    float process(float x) { float y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; }
    void reset() { z1 = z2 = 0; }
};

// Circular buffer read with 4-point Hermite interpolation (smooth for modulated / gliding times).
struct DelayLine {
    std::vector<float> buf; int w = 0, mask = 0;
    void prepare(int samples) { int n = 1; while (n < samples) n <<= 1; buf.assign((size_t) n, 0.f); mask = n - 1; w = 0; }
    void clear() { std::fill(buf.begin(), buf.end(), 0.f); }
    void push(float x) { buf[(size_t) w] = x; w = (w + 1) & mask; }
    float read(float d) const {
        float pos = (float) w - d;
        int i = (int) std::floor(pos);
        float f = pos - (float) i;
        float xm1 = buf[(size_t) ((i - 1) & mask)], x0 = buf[(size_t) (i & mask)], x1 = buf[(size_t) ((i + 1) & mask)], x2 = buf[(size_t) ((i + 2) & mask)];
        float c1 = .5f * (x1 - xm1), c2 = xm1 - 2.5f * x0 + 2 * x1 - .5f * x2, c3 = .5f * (x2 - xm1) + 1.5f * (x0 - x1);
        return ((c3 * f + c2) * f + c1) * f + x0;
    }
};

struct Allpass {
    std::vector<float> buf; int i = 0; float g = .6f;
    void prepare(int n) { buf.assign((size_t) std::max(1, n), 0.f); i = 0; }
    void clear() { std::fill(buf.begin(), buf.end(), 0.f); }
    float process(float x) {
        float d = buf[(size_t) i], y = d - g * x;
        buf[(size_t) i] = x + g * y;
        if (++i >= (int) buf.size()) i = 0;
        return y;
    }
};

class DelayEngine {
public:
    // Written by the audio thread for the editor's echo display.
    int onsetCount = 0;

    void prepare(double sampleRate, int) {
        sr = (float) sampleRate;
        int maxSamples = (int) (sr * (maxDelaySeconds + .2f)) + 16;
        lineL.prepare(maxSamples); lineR.prepare(maxSamples);
        const float msL[4] { 4.7f, 7.3f, 11.1f, 15.9f }, msR[4] { 5.3f, 8.1f, 12.3f, 17.1f };
        for (int k = 0; k < 4; ++k) { apL[(size_t) k].prepare((int) (msL[k] * sr / 1000)); apR[(size_t) k].prepare((int) (msR[k] * sr / 1000)); }
        timeSmooth = 1 - std::exp(-1 / (.06f * sr));
        paramSmooth = 1 - std::exp(-1 / (.02f * sr));
        reset();
    }

    void reset() {
        lineL.clear(); lineR.clear();
        for (auto* f : { &hpL, &hpR, &lpL, &lpR }) f->reset();
        for (auto& a : apL) a.clear();
        for (auto& a : apR) a.clear();
        dSm = p.delaySec; offSm = p.offsetMs / 1000; spaceSm = p.space ? p.spaceAmount : 0;
        env = fastEnv = slowEnv = 0; sinceOnset = 1 << 20;
        wowPh = flPh = panPh = 0;
        primed = true;
    }

    void setParams(const DelayParams& np) {
        p = np;
        p.delaySec = std::clamp(p.delaySec, .001f, maxDelaySeconds);
        if (primed) { dSm = p.delaySec; offSm = p.offsetMs / 1000; spaceSm = p.space ? p.spaceAmount : 0; primed = false; }
        hpL.set(true, p.lowCut, sr); hpR.set(true, p.lowCut, sr);
        lpL.set(false, p.highCut, sr); lpR.set(false, p.highCut, sr);
    }

    void process(const float* inL, const float* inR, float* wetL, float* wetR, int n) {
        const float maxD = sr * (maxDelaySeconds + .1f);
        const float k = 1 + p.saturation * 6;
        // Level make-up on the echoes we hear (not in the loop), so saturation does not make them quieter.
        const float mk = k > 1.001f ? .3f / (std::tanh(.3f * k) / k) : 1.f;
        const float fb = p.feedback;
        const float duckRel = std::exp(-1 / (std::max(.02f, p.duckReleaseMs / 1000) * sr)), duckAtt = std::exp(-1 / (.003f * sr));
        const float fastRel = std::exp(-1 / (.03f * sr)), slowRel = std::exp(-1 / (.4f * sr));
        const float spaceTarget = p.space ? p.spaceAmount : 0;
        const float twoPi = 2 * kPi;
        for (int s = 0; s < n; ++s) {
            const float xl = inL[s], xr = inR[s];
            dSm += (p.delaySec - dSm) * timeSmooth;
            offSm += (p.offsetMs / 1000 - offSm) * timeSmooth;
            spaceSm += (spaceTarget - spaceSm) * paramSmooth;

            // Tape wow (slow drift) and flutter (fast shimmer) move the read head.
            wowPh += .55f / sr; if (wowPh > 1) wowPh -= 1;
            flPh += 6.3f / sr;  if (flPh > 1) flPh -= 1;
            float wowL = std::sin(twoPi * wowPh) * .7f + std::sin(twoPi * wowPh * 1.73f + 1.3f) * .3f;
            float wowR = std::sin(twoPi * wowPh + .9f) * .7f + std::sin(twoPi * wowPh * 1.73f + 2.1f) * .3f;
            float flL = std::sin(twoPi * flPh), flR = std::sin(twoPi * flPh + 1.7f);
            float modL = (p.wow * wowL * .004f + p.flutter * flL * .0004f) * sr;
            float modR = (p.wow * wowR * .004f + p.flutter * flR * .0004f) * sr;
            float base = dSm * sr;
            float dL = std::clamp(base + modL + p.wow * .004f * sr, 4.f, maxD);
            float dR = std::clamp(base + (p.mode == Stereo ? offSm * sr : 0.f) + modR + p.wow * .004f * sr, 4.f, maxD);

            float yl = shape(lpL.process(hpL.process(lineL.read(dL))), k);
            float yr = shape(lpR.process(hpR.process(lineR.read(dR))), k);
            float ol, orr;
            if (p.mode == Mono) {
                lineL.push(safe(.5f * (xl + xr) + yl * fb));
                lineR.push(0);
                ol = orr = yl * mk;
            } else if (p.mode == Stereo) {
                lineL.push(safe(xl + yl * fb));
                lineR.push(safe(xr + yr * fb));
                ol = yl * mk; orr = yr * mk;
            } else {
                lineL.push(safe(.5f * (xl + xr) + yr * fb));
                lineR.push(safe(yl * fb));
                ol = yl * mk; orr = yr * mk;
            }

            // Stereo width on the echoes (mid / side).
            float mid = .5f * (ol + orr), side = .5f * (ol - orr) * p.width;
            ol = mid + side; orr = mid - side;

            // SPACE: each repeat drifts to a new place in the stereo field, plus a short diffusion.
            if (spaceSm > .001f) {
                float dl = ol, dr = orr;
                for (auto& a : apL) dl = a.process(dl);
                for (auto& a : apR) dr = a.process(dr);
                ol = ol * (1 - .45f * spaceSm) + dl * .75f * spaceSm;
                orr = orr * (1 - .45f * spaceSm) + dr * .75f * spaceSm;
                panPh += 1 / (2.6f * std::max(.02f, dSm) * sr); if (panPh > 1) panPh -= 1;
                float pan = std::sin(twoPi * panPh) * .9f * spaceSm;  // -0.9 .. 0.9
                float a = (pan + 1) * kPi / 4;
                ol *= std::cos(a) * 1.4142f; orr *= std::sin(a) * 1.4142f;
            } else {
                panPh = 0;
            }

            // Ducking: the echoes step back while the input plays, and return in the gaps.
            float level = std::max(std::abs(xl), std::abs(xr));
            env = level > env ? level + (env - level) * duckAtt : level + (env - level) * duckRel;
            float g = 1 - .9f * p.duck * std::min(1.f, env / .2f);
            wetL[s] = ol * g; wetR[s] = orr * g;

            // Onset detector for the display.
            fastEnv = std::max(level, fastEnv * fastRel);
            slowEnv = std::max(level * .5f, slowEnv * slowRel);
            ++sinceOnset;
            if (fastEnv > .03f && fastEnv > slowEnv * 2.2f && sinceOnset > (int) (.12f * sr)) { ++onsetCount; sinceOnset = 0; slowEnv = fastEnv; }
        }
    }

private:
    DelayParams p;
    float sr = 48000, timeSmooth = .001f, paramSmooth = .001f;
    DelayLine lineL, lineR;
    Biquad hpL, hpR, lpL, lpR;
    std::array<Allpass, 4> apL, apR;
    float dSm = .5f, offSm = 0, spaceSm = 0, env = 0, fastEnv = 0, slowEnv = 0;
    float wowPh = 0, flPh = 0, panPh = 0;
    int sinceOnset = 0;
    bool primed = true;

    // Unity gain for quiet signals (the loop gain stays = feedback), rounds off the loud peaks.
    static float shape(float x, float k) { return k > 1.001f ? std::tanh(k * x) / k : x; }
    // Keeps the loop bounded, even with feedback above 100 %.
    static float safe(float x) { return 1.5f * std::tanh(x / 1.5f); }
};
} // namespace swell
