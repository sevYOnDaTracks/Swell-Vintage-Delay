#include "../Source/PluginProcessor.h"
#include <iostream>
// Offline demo renderer: DelayDemo <voice.wav> <outDir>
// Runs the real DelayProcessor (same code as the VST3) on a dry vocal with several presets, at 120 BPM.
// Each file: dry phrase, 0.6 s pause, the same phrase through the delay, then the repeats.

namespace {
struct Shot { const char* preset; float mix; };
const Shot shots[] = {
    { "Quarter Note", 32 }, { "Dotted Pong", 32 }, { "Warm Tape", 34 }, { "Dub Echo", 34 },
    { "Vocal Throw", 38 },  { "Rap Doubler", 35 }, { "Space Orbit", 34 }, { "Lo-Fi Wobble", 34 },
};
int findPreset(const char* name) { for (int i = 0; i < presetCount; ++i) if (juce::String(presets[(size_t) i].name) == name) return i; return 0; }
void setParam(DelayProcessor& p, const char* id, float v) { auto* a = p.state.getParameter(id); a->setValueNotifyingHost(a->convertTo0to1(v)); }

juce::AudioBuffer<float> process(const juce::AudioBuffer<float>& dry, int preset, float mix, double sr, double tail) {
    DelayProcessor p;
    p.loadPreset(preset); setParam(p, "mix", mix);
    const int block = 256, total = dry.getNumSamples() + (int) (tail * sr);
    p.prepareToPlay(sr, block);
    juce::AudioBuffer<float> out(2, total), b(2, block);
    juce::MidiBuffer m;
    for (int start = 0; start < total; start += block) {
        int n = std::min(block, total - start);
        b.clear();
        for (int c = 0; c < 2; ++c)
            if (start < dry.getNumSamples()) b.copyFrom(c, 0, dry, std::min(c, dry.getNumChannels() - 1), start, std::min(n, dry.getNumSamples() - start));
        juce::AudioBuffer<float> view(b.getArrayOfWritePointers(), 2, n);
        p.processBlock(view, m);
        for (int c = 0; c < 2; ++c) out.copyFrom(c, start, b, c, 0, n);
    }
    return out;
}
void append(juce::AudioBuffer<float>& dst, int& pos, const juce::AudioBuffer<float>& src) {
    for (int c = 0; c < 2; ++c) dst.copyFrom(c, pos, src, std::min(c, src.getNumChannels() - 1), 0, src.getNumSamples());
    pos += src.getNumSamples();
}
float write(const juce::File& f, juce::AudioBuffer<float>& buf, double sr) {
    float peak = buf.getMagnitude(0, buf.getNumSamples());
    if (peak > .89f) buf.applyGain(.89f / peak); // keep at least 1 dB headroom
    f.deleteFile();
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::AudioFormatWriter> w(wav.createWriterFor(new juce::FileOutputStream(f), sr, 2, 24, {}, 0));
    if (w) w->writeFromAudioSampleBuffer(buf, 0, buf.getNumSamples());
    return peak;
}
}

int main(int argc, char** argv) {
    juce::ScopedJuceInitialiser_GUI gui;
    if (argc < 3) { std::cerr << "usage: DelayDemo <voice.wav> <outDir>\n"; return 1; }
    juce::AudioFormatManager fm; fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> r(fm.createReaderFor(juce::File(argv[1])));
    if (!r) { std::cerr << "cannot read voice\n"; return 1; }
    const double sr = r->sampleRate;
    juce::AudioBuffer<float> dry((int) r->numChannels, (int) r->lengthInSamples);
    r->read(&dry, 0, dry.getNumSamples(), 0, true, true);
    juce::File dir(argv[2]); dir.createDirectory();
    const int gap = (int) (.6 * sr);
    juce::AudioBuffer<float> chain(2, dry.getNumSamples() + gap + (int) std::size(shots) * (dry.getNumSamples() + (int) (3.3 * sr)));
    chain.clear();
    int chainPos = 0;
    append(chain, chainPos, dry); chainPos += gap;
    int index = 1;
    for (auto& s : shots) {
        int pr = findPreset(s.preset);
        auto wet = process(dry, pr, s.mix, sr, 4.0);
        juce::AudioBuffer<float> ab(2, dry.getNumSamples() + gap + wet.getNumSamples());
        ab.clear();
        int pos = 0;
        append(ab, pos, dry); pos += gap; append(ab, pos, wet);
        juce::String name = juce::String(index).paddedLeft('0', 2) + "-" + juce::String(s.preset).replaceCharacter(' ', '-').replaceCharacter('/', '-') + "-AVANT-APRES.wav";
        float peak = write(dir.getChildFile(name), ab, sr);
        auto shortWet = process(dry, pr, s.mix, sr, 3.0);
        append(chain, chainPos, shortWet); chainPos += (int) (.3 * sr);
        std::cout << name << "  (mix " << s.mix << " %, peak " << juce::Decibels::gainToDecibels(peak) << " dBFS)\n";
        ++index;
    }
    juce::AudioBuffer<float> trimmed(2, chainPos);
    for (int c = 0; c < 2; ++c) trimmed.copyFrom(c, 0, chain, c, 0, chainPos);
    write(dir.getChildFile("00-DEMO-TOUS-LES-PRESETS.wav"), trimmed, sr);
    return 0;
}
