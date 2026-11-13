// Film de l'interface, image par image, pour la vidéo de la boutique (projet Delay-Motion, assets/ui.mp4).
// Usage : DelayFilm <voix.wav> <sortie-audio.wav> [secondes] | ffmpeg -f rawvideo -pix_fmt bgra -s 2136x1292 -r 30 -i - ...
// Le plugin traite la voix en continu (120 BPM) ; les presets changent selon le montage de demo.html.
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <cstdio>

namespace {
int findPreset(const char* name) { for (int i = 0; i < presetCount; ++i) if (juce::String(presets[(size_t) i].name) == name) return i; return 0; }
void setParam(DelayProcessor& p, const char* id, float v) { auto* a = p.state.getParameter(id); a->setValueNotifyingHost(a->convertTo0to1(v)); }
}

int main(int argc, char** argv) {
    juce::ScopedJuceInitialiser_GUI gui;
    if (argc < 3) { std::fprintf(stderr, "usage: DelayFilm voix.wav sortie.wav [secondes]\n"); return 1; }
    const double seconds = argc > 3 ? juce::String(argv[3]).getDoubleValue() : 22.3;
    swell::Licence::unlockForTesting();
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::AudioFormatReader> reader(wav.createReaderFor(new juce::FileInputStream(juce::File(juce::String::fromUTF8(argv[1]))), true));
    if (reader == nullptr) { std::fprintf(stderr, "voix illisible\n"); return 1; }
    juce::AudioBuffer<float> voice(2, (int) reader->lengthInSamples);
    reader->read(&voice, 0, voice.getNumSamples(), 0, true, true);
    const double rate = reader->sampleRate;

    DelayProcessor p;
    constexpr int fps = 30;
    const int perFrame = (int) std::lround(rate / fps);
    p.prepareToPlay(rate, perFrame);
    p.loadPreset(findPreset("Quarter Note")); setParam(p, "mix", 0);
    std::unique_ptr<juce::AudioProcessorEditor> e(p.createEditor());
    e->setSize(Panel::W, Panel::H);
    Panel* panel = dynamic_cast<Panel*>(e->getChildComponent(0));
    if (panel == nullptr) return 1;
    panel->setScaleShown(1);
    panel->applyTheme(Theme::presets()[0], false);
    e->resized();

    struct Cue { double t; const char* preset; float mix; bool done = false; };
    std::vector<Cue> cues {
        { 2.0, "Quarter Note", 32 }, { 4.7, "Dotted Pong", 32 }, { 7.4, "Warm Tape", 34 }, { 10.1, "Dub Echo", 34 },
        { 12.8, "Vocal Throw", 38 }, { 15.5, "Space Orbit", 34 }, { 18.2, "Lo-Fi Wobble", 34 },
    };
    const int frames = (int) std::lround(seconds * fps);
    juce::AudioBuffer<float> out(2, frames * perFrame), b(2, perFrame);
    juce::MidiBuffer midi;
    int readPos = 0;
    for (int f = 0; f < frames; ++f) {
        const double t = (double) f / fps;
        for (auto& c : cues) if (!c.done && t >= c.t) { p.loadPreset(findPreset(c.preset)); setParam(p, "mix", c.mix); c.done = true; }
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < perFrame; ++i)
                b.setSample(ch, i, voice.getSample(std::min(ch, voice.getNumChannels() - 1), (readPos + i) % voice.getNumSamples()));
        readPos = (readPos + perFrame) % voice.getNumSamples();
        p.processBlock(b, midi);
        for (int ch = 0; ch < 2; ++ch) out.copyFrom(ch, f * perFrame, b, ch, 0, perFrame);
        EchoDisplay::filmClock = t;
        panel->filmTick();
        auto img = e->createComponentSnapshot(e->getLocalBounds(), true, 2.0f).convertedToFormat(juce::Image::ARGB);
        juce::Image::BitmapData d(img, juce::Image::BitmapData::readOnly);
        for (int y = 0; y < img.getHeight(); ++y) std::fwrite(d.getLinePointer(y), 4, (size_t) img.getWidth(), stdout);
        if (f % 60 == 0) std::fprintf(stderr, "%.0f s\n", t);
    }
    std::fflush(stdout);
    juce::File of(juce::String::fromUTF8(argv[2])); of.deleteFile();
    std::unique_ptr<juce::AudioFormatWriter> w(wav.createWriterFor(new juce::FileOutputStream(of), rate, 2, 24, {}, 0));
    w->writeFromAudioSampleBuffer(out, 0, out.getNumSamples());
    e.reset();
    return 0;
}
