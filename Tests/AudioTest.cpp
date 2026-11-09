#include "../Source/PluginProcessor.h"
#include <iostream>
#include <stdexcept>

void require(bool ok, const std::string& msg) { if (!ok) throw std::runtime_error(msg); }
void set(DelayProcessor& p, const char* id, float v) { auto* a = p.state.getParameter(id); a->setValueNotifyingHost(a->convertTo0to1(v)); }
constexpr int SR = 48000, BLOCK = 256;
struct Stereo { std::vector<float> l, r; };

// Renders 'seconds' of audio; in(n) gives the input sample for both channels (or inR for the right one).
template <class F, class G> Stereo run2(DelayProcessor& p, double seconds, F inL, G inR, bool prepare = true) {
    if (prepare) p.prepareToPlay(SR, BLOCK);
    juce::MidiBuffer m;
    juce::AudioBuffer<float> b(2, BLOCK);
    Stereo out;
    for (int start = 0; start < (int) (SR * seconds); start += BLOCK) {
        for (int n = 0; n < BLOCK; ++n) { b.setSample(0, n, inL(start + n)); b.setSample(1, n, inR(start + n)); }
        p.processBlock(b, m);
        for (int n = 0; n < BLOCK; ++n) {
            require(std::isfinite(b.getSample(0, n)) && std::isfinite(b.getSample(1, n)), "non-finite output");
            out.l.push_back(b.getSample(0, n)); out.r.push_back(b.getSample(1, n));
        }
    }
    return out;
}
template <class F> Stereo run(DelayProcessor& p, double seconds, F in, bool prepare = true) { return run2(p, seconds, in, in, prepare); }
float peakIn(const std::vector<float>& v, double from, double to) {
    float m = 0;
    for (int i = (int) (from * SR); i < (int) (to * SR) && i < (int) v.size(); ++i) m = std::max(m, std::abs(v[(size_t) i]));
    return m;
}
int argPeak(const std::vector<float>& v, double from, double to) {
    int best = -1; float m = 0;
    for (int i = (int) (from * SR); i < (int) (to * SR) && i < (int) v.size(); ++i) if (std::abs(v[(size_t) i]) > m) { m = std::abs(v[(size_t) i]); best = i; }
    return best;
}
double energy(const std::vector<float>& v, double from, double to) {
    double e = 0;
    for (int i = (int) (from * SR); i < (int) (to * SR) && i < (int) v.size(); ++i) e += (double) v[(size_t) i] * v[(size_t) i];
    return e;
}
// A clean, unmodulated delay for measurements.
void clean(DelayProcessor& p) {
    set(p, "mix", 100); set(p, "wow", 0); set(p, "flutter", 0); set(p, "saturation", 0); set(p, "duck", 0);
    set(p, "lowCut", 20); set(p, "highCut", 20000); set(p, "width", 100); set(p, "offset", 0); set(p, "space", 0);
}
auto click = [](int n) { return n < 8 ? .5f : 0.f; };

int main() {
    try {
        juce::ScopedJuceInitialiser_GUI gui;

        // 1. Every preset: bounded, finite, and (except the infinite ones) the repeats die away.
        for (int i = 0; i < presetCount; ++i) {
            DelayProcessor p; p.loadPreset(i); set(p, "mix", 50);
            auto out = run(p, 14, [](int n) { return n < 9600 ? (float) std::sin(n * .05) * .5f : 0.f; });
            float peak = std::max(peakIn(out.l, 0, 14), peakIn(out.r, 0, 14));
            require(peak < 2.5f, std::string("preset too loud: ") + presets[(size_t) i].name + " " + std::to_string(peak));
            if (presets[(size_t) i].feedback <= 90)
                require(peakIn(out.l, 13, 14) + peakIn(out.r, 13, 14) < .01f, std::string("repeats do not die away: ") + presets[(size_t) i].name);
        }
        std::cout << "presets OK (" << presetCount << ")\n";

        // 2. Time in ms: first echo exactly where it should be.
        {
            DelayProcessor p; clean(p); set(p, "sync", 0); set(p, "time", 250); set(p, "feedback", 0); set(p, "mode", 0);
            auto out = run(p, 1, click);
            int at = argPeak(out.l, .05, .9);
            require(std::abs(at - SR / 4) <= 6, "250 ms echo at sample " + std::to_string(at));
            require(peakIn(out.l, .5, 1) < .01f, "feedback 0 should give a single echo");
        }
        // 3. Sync follows the tempo (120 BPM without a host): 1/4 = 500 ms, 1/8D = 375 ms.
        for (auto [div, expected] : { std::pair<int, double> { 11, .5 }, { 10, .375 }, { 6, 1.0 / 6 } }) {
            DelayProcessor p; clean(p); set(p, "sync", 1); set(p, "division", (float) div); set(p, "feedback", 0); set(p, "mode", 0);
            auto out = run(p, 1.2, click);
            int at = argPeak(out.l, .05, 1.1);
            require(std::abs(at - expected * SR) <= 6, "sync division " + std::to_string(div) + " at " + std::to_string(at));
        }
        std::cout << "time / sync OK\n";

        // 4. Feedback: each repeat is 'feedback' times the previous one.
        {
            DelayProcessor p; clean(p); set(p, "sync", 0); set(p, "time", 200); set(p, "feedback", 50); set(p, "mode", 0);
            auto out = run(p, 1, click);
            float e1 = peakIn(out.l, .19, .21), e2 = peakIn(out.l, .39, .41), e3 = peakIn(out.l, .59, .61);
            require(std::abs(e2 / e1 - .5f) < .04f && std::abs(e3 / e2 - .5f) < .04f, "feedback ratio " + std::to_string(e2 / e1) + " " + std::to_string(e3 / e2));
        }
        // 5. Ping-pong: 1st repeat left only, 2nd right only, 3rd left again.
        {
            DelayProcessor p; clean(p); set(p, "sync", 0); set(p, "time", 200); set(p, "feedback", 60); set(p, "mode", 2);
            auto out = run(p, 1, click);
            require(peakIn(out.l, .19, .21) > .1f && peakIn(out.r, .19, .21) < .001f, "ping-pong 1st repeat should be left");
            require(peakIn(out.r, .39, .41) > .05f && peakIn(out.l, .39, .41) < .001f, "ping-pong 2nd repeat should be right");
            require(peakIn(out.l, .59, .61) > .02f && peakIn(out.r, .59, .61) < .001f, "ping-pong 3rd repeat should be left");
        }
        // 6. Stereo offset: the right repeat arrives 'offset' ms later.
        {
            DelayProcessor p; clean(p); set(p, "sync", 0); set(p, "time", 200); set(p, "feedback", 0); set(p, "mode", 1); set(p, "offset", 20);
            auto out = run(p, .6, click);
            int l = argPeak(out.l, .05, .5), r = argPeak(out.r, .05, .5);
            require(std::abs((r - l) - SR / 50) <= 4, "stereo offset " + std::to_string(r - l));
        }
        std::cout << "feedback / ping-pong / offset OK\n";

        // 7. Mix 0 and bypass leave the signal untouched.
        for (int mode = 0; mode < 2; ++mode) {
            DelayProcessor p; p.loadPreset(14);
            if (mode == 0) set(p, "mix", 0); else set(p, "bypass", 1);
            auto in = [](int n) { return (float) std::sin(n * .031) * .4f; };
            auto out = run(p, .5, in);
            float err = 0;
            for (int n = 0; n < (int) out.l.size(); ++n) err = std::max(err, std::abs(out.l[(size_t) n] - in(n)));
            require(err < 1e-5f, mode == 0 ? "mix 0 not transparent" : "bypass not transparent");
        }
        // 8. Feedback above 100 % sustains but stays bounded.
        {
            DelayProcessor p; p.loadPreset(0); clean(p); set(p, "feedback", 110); set(p, "time", 300); set(p, "sync", 0);
            auto out = run(p, 20, [](int n) { return n < 4800 ? (float) std::sin(n * .05) * .8f : 0.f; });
            require(peakIn(out.l, 0, 20) < 2.f, "infinite feedback not bounded");
            require(peakIn(out.l, 18, 20) > .05f, "infinite feedback should keep repeating");
        }
        // 9. Ducking: repeats are quieter while the input plays.
        {
            auto wetDuring = [](float duck) {
                DelayProcessor p; clean(p); set(p, "sync", 0); set(p, "time", 100); set(p, "feedback", 50); set(p, "mode", 0); set(p, "duck", duck);
                set(p, "mix", 100);
                auto out = run(p, 2, [](int n) { return (float) std::sin(n * .03) * .5f; });
                return energy(out.l, 1, 2);
            };
            double off = wetDuring(0), on = wetDuring(100);
            require(on < off * .1, "ducking too weak: " + std::to_string(on / off));
        }
        // 10. SPACE makes left and right repeats different, even for a mono source in mono mode.
        {
            auto sideEnergy = [](bool space) {
                DelayProcessor p; clean(p); set(p, "sync", 0); set(p, "time", 150); set(p, "feedback", 60); set(p, "mode", 0);
                set(p, "space", space ? 1.f : 0.f); set(p, "spaceAmount", 100);
                auto out = run(p, 2, click);
                double s = 0;
                for (size_t i = 0; i < out.l.size(); ++i) s += (double) (out.l[i] - out.r[i]) * (out.l[i] - out.r[i]);
                return s;
            };
            require(sideEnergy(false) < 1e-9, "mono mode should be centred without SPACE");
            require(sideEnergy(true) > 1e-3, "SPACE should spread the repeats");
        }
        std::cout << "mix / bypass / infinite / ducking / space OK\n";

        // 11. Extreme settings: nothing explodes.
        {
            juce::Random rnd(42);
            for (int k = 0; k < 30; ++k) {
                DelayProcessor p;
                for (auto* prm : p.getParameters()) prm->setValueNotifyingHost(rnd.nextFloat());
                set(p, "bypass", 0);
                auto out = run(p, 3, [&](int n) { return (float) std::sin(n * .07) * (n < 24000 ? .9f : 0.f); });
                require(peakIn(out.l, 0, 3) < 3 && peakIn(out.r, 0, 3) < 3, "random settings too loud");
            }
        }
        // 12. Session save / restore and user presets (a preset keeps your Mix).
        {
            DelayProcessor a; a.loadPreset(17); set(a, "mix", 42); set(a, "feedback", 61);
            juce::MemoryBlock mb; a.getStateInformation(mb);
            DelayProcessor b; b.setStateInformation(mb.getData(), (int) mb.getSize());
            require(std::abs(b.value("feedback") - 61) < .2f && std::abs(b.value("mix") - 42) < .2f && (int) b.value("program") == 17, "state not restored");
            auto f = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("swell-delay-test.swelldelay");
            require(a.saveUserPreset(f), "cannot save user preset");
            DelayProcessor c; set(c, "mix", 15);
            require(c.loadUserPreset(f), "cannot load user preset");
            require(std::abs(c.value("feedback") - 61) < .2f && std::abs(c.value("mix") - 15) < .2f, "user preset values wrong");
            f.deleteFile();
        }
        std::cout << "stability / state / user presets OK\nALL TESTS PASSED\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAILED: " << e.what() << "\n";
        return 1;
    }
}
