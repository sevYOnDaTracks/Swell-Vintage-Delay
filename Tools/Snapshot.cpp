#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
// Renders the editor to a PNG: DelaySnapshot <out.png> [preset] [theme 0-5] [colours]
// The theme and the colour panel are shown only in the picture (saved settings are left alone).
int main(int argc, char** argv) {
    swell::Licence::unlockForTesting(); // rendu hors ligne, sans fichier de licence
    juce::ScopedJuceInitialiser_GUI gui;
    DelayProcessor p;
    if (argc > 2) p.loadPreset(juce::String(argv[2]).getIntValue());
    p.prepareToPlay(48000, 256);
    juce::MidiBuffer m;
    juce::AudioBuffer<float> b(2, 256);
    for (int i = 0; i < 20; ++i) {
        for (int c = 0; c < 2; ++c)
            for (int n = 0; n < 256; ++n) b.setSample(c, n, (i == 0 && n < 40) ? .8f : 0.f);
        p.processBlock(b, m);
    }
    std::unique_ptr<juce::AudioProcessorEditor> e(p.createEditor());
    e->setSize(Panel::W, Panel::H);
    if (auto* panel = dynamic_cast<Panel*>(e->getChildComponent(0))) {
        panel->setScaleShown(1);
        if (argc > 3) panel->applyTheme(Theme::presets()[(size_t) juce::jlimit(0, 5, juce::String(argv[3]).getIntValue())], false);
        if (argc > 4) panel->showColours(true);
    }
    e->resized();
    auto image = e->createComponentSnapshot(e->getLocalBounds(), true, 2.f);
    juce::File out(juce::File::getCurrentWorkingDirectory().getChildFile(argc > 1 ? argv[1] : "snapshot.png"));
    out.deleteFile();
    juce::FileOutputStream s(out);
    juce::PNGImageFormat().writeImageToStream(image, s);
    return 0;
}
