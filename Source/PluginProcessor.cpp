#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace {
using Range = juce::NormalisableRange<float>;
Range skewed(float lo, float hi, float centre) { Range r(lo, hi); r.setSkewForCentre(centre); return r; }
void addFloat(juce::AudioProcessorValueTreeState::ParameterLayout& l, const char* id, const char* name, Range r, float def, const char* unit = "") {
    l.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { id, 1 }, name, r, def, juce::AudioParameterFloatAttributes().withLabel(unit)));
}
}

DelayProcessor::DelayProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true).withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      state(*this, nullptr, "SwellDelay", layout()) {}

juce::String DelayProcessor::presetName(int i) {
    const auto& p = presets[(size_t) juce::jlimit(0, presetCount - 1, i)];
    return utf8(p.category) + " / " + utf8(p.name);
}

juce::AudioProcessorValueTreeState::ParameterLayout DelayProcessor::layout() {
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    juce::StringArray names, divs, modes { "Mono", utf8("St\xc3\xa9r\xc3\xa9o"), "Ping-Pong" };
    for (int i = 0; i < presetCount; ++i) names.add(presetName(i));
    for (auto& d : swell::divisions) divs.add(d.name);
    const auto& p = presets[0];
    l.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID { "program", 1 }, "Preset", names, 0));
    l.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID { "sync", 1 }, "Sync", p.sync));
    l.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID { "division", 1 }, "Time (Sync)", divs, p.division));
    addFloat(l, "time", "Time (ms)", skewed(1, 2000, 300), p.timeMs, "ms");
    addFloat(l, "feedback", "Feedback", Range(0, 110, .1f), p.feedback, "%");
    addFloat(l, "mix", "Mix", Range(0, 100, .1f), 30, "%");
    addFloat(l, "lowCut", "Low Cut", skewed(20, 2000, 200), p.lowCut, "Hz");
    addFloat(l, "highCut", "High Cut", skewed(1000, 20000, 5000), p.highCut, "Hz");
    addFloat(l, "wow", "Wow", Range(0, 100, .1f), p.wow, "%");
    addFloat(l, "flutter", "Flutter", Range(0, 100, .1f), p.flutter, "%");
    addFloat(l, "saturation", "Saturation", Range(0, 100, .1f), p.saturation, "%");
    addFloat(l, "duck", "Ducking", Range(0, 100, .1f), p.duck, "%");
    addFloat(l, "duckRelease", "Ducking Release", skewed(20, 1000, 200), p.duckRelease, "ms");
    addFloat(l, "width", "Width", Range(0, 200, .1f), p.width, "%");
    addFloat(l, "offset", "Offset R", Range(-50, 50, .1f), p.offset, "ms");
    l.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID { "mode", 1 }, "Mode", modes, p.mode));
    l.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID { "space", 1 }, "Space", p.space));
    addFloat(l, "spaceAmount", "Space Amount", Range(0, 100, .1f), p.spaceAmount, "%");
    l.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID { "bypass", 1 }, "Bypass", false));
    return l;
}

bool DelayProcessor::isBusesLayoutSupported(const BusesLayout& b) const {
    auto in = b.getMainInputChannelSet(), out = b.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo()) return false;
    return in == out || (in == juce::AudioChannelSet::mono() && out == juce::AudioChannelSet::stereo());
}

void DelayProcessor::setValue(const char* id, float v) {
    auto* p = state.getParameter(id);
    p->beginChangeGesture(); p->setValueNotifyingHost(p->convertTo0to1(v)); p->endChangeGesture();
}

void DelayProcessor::loadPreset(int index) {
    index = juce::jlimit(0, presetCount - 1, index);
    const auto& p = presets[(size_t) index];
    setValue("program", (float) index);
    setValue("sync", p.sync ? 1.f : 0.f); setValue("division", (float) p.division); setValue("time", p.timeMs);
    setValue("feedback", p.feedback); setValue("lowCut", p.lowCut); setValue("highCut", p.highCut);
    setValue("wow", p.wow); setValue("flutter", p.flutter); setValue("saturation", p.saturation);
    setValue("duck", p.duck); setValue("duckRelease", p.duckRelease);
    setValue("width", p.width); setValue("offset", p.offset); setValue("mode", (float) p.mode);
    setValue("space", p.space ? 1.f : 0.f); setValue("spaceAmount", p.spaceAmount);
    userPresetName = {};
}

float DelayProcessor::delayTimeFor(bool sync, int division, float timeMs, double hostBpm) {
    if (!sync) return timeMs / 1000;
    double tempo = juce::jlimit(20.0, 999.0, hostBpm > 0 ? hostBpm : 120.0);
    float s = swell::divisions[(size_t) juce::jlimit(0, (int) swell::divisions.size() - 1, division)].beats * (float) (60 / tempo);
    return std::min(s, swell::maxDelaySeconds);
}

swell::DelayParams DelayProcessor::currentParams(double hostBpm) const {
    swell::DelayParams r;
    r.delaySec = delayTimeFor(value("sync") > .5f, (int) value("division"), value("time"), hostBpm);
    r.feedback = value("feedback") / 100;
    r.lowCut = value("lowCut"); r.highCut = value("highCut");
    r.wow = value("wow") / 100; r.flutter = value("flutter") / 100; r.saturation = value("saturation") / 100;
    r.duck = value("duck") / 100; r.duckReleaseMs = value("duckRelease");
    r.width = value("width") / 100; r.offsetMs = value("offset");
    r.mode = (int) value("mode");
    r.space = value("space") > .5f; r.spaceAmount = value("spaceAmount") / 100;
    return r;
}

void DelayProcessor::prepareToPlay(double sr, int block) {
    engine.prepare(sr, block);
    engine.setParams(currentParams(bpm.load()));
    engine.reset();
    wet.setSize(2, std::max(1, block));
    dryGain.reset(sr, .03); wetGain.reset(sr, .03);
    float m = value("mix") / 100;
    bool off = value("bypass") > .5f;
    dryGain.setCurrentAndTargetValue(off ? 1.f : std::min(1.f, 2 * (1 - m)));
    wetGain.setCurrentAndTargetValue(off ? 0.f : std::min(1.f, 2 * m));
}

void DelayProcessor::processBlock(juce::AudioBuffer<float>& b, juce::MidiBuffer&) {
    if (! licence->isUnlocked()) { b.clear(); return; }
    render(b, false);
}
void DelayProcessor::processBlockBypassed(juce::AudioBuffer<float>& b, juce::MidiBuffer&) { render(b, true); }

void DelayProcessor::render(juce::AudioBuffer<float>& b, bool bypassed) {
    juce::ScopedNoDenormals guard;
    const int n = b.getNumSamples(), inCh = getTotalNumInputChannels(), outCh = getTotalNumOutputChannels();
    if (n == 0) return;
    if (wet.getNumSamples() < n) wet.setSize(2, n, false, false, true);
    for (int c = inCh; c < outCh; ++c) b.copyFrom(c, 0, b, 0, 0, n); // mono in, stereo out

    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto t = pos->getBpm()) bpm.store((float) *t);
    auto params = currentParams(bpm.load());
    delaySeconds.store(params.delaySec);
    engine.setParams(params);
    engine.process(b.getReadPointer(0), b.getReadPointer(outCh > 1 ? 1 : 0), wet.getWritePointer(0), wet.getWritePointer(1), n);
    onsets.store(engine.onsetCount);

    const float m = value("mix") / 100;
    const bool off = bypassed || value("bypass") > .5f;
    dryGain.setTargetValue(off ? 1.f : std::min(1.f, 2 * (1 - m)));
    wetGain.setTargetValue(off ? 0.f : std::min(1.f, 2 * m));
    auto* wl = wet.getReadPointer(0);
    auto* wr = wet.getReadPointer(1);
    for (int i = 0; i < n; ++i) {
        float d = dryGain.getNextValue(), w = wetGain.getNextValue();
        if (outCh > 1) {
            b.setSample(0, i, b.getSample(0, i) * d + wl[i] * w);
            b.setSample(1, i, b.getSample(1, i) * d + wr[i] * w);
        } else
            b.setSample(0, i, b.getSample(0, i) * d + .5f * (wl[i] + wr[i]) * w);
    }
}

void DelayProcessor::getStateInformation(juce::MemoryBlock& block) {
    auto t = state.copyState();
    t.setProperty("userPreset", userPresetName, nullptr);
    if (auto xml = t.createXml()) copyXmlToBinary(*xml, block);
}

void DelayProcessor::setStateInformation(const void* data, int size) {
    auto xml = getXmlFromBinary(data, size);
    if (xml && xml->hasTagName(state.state.getType())) {
        auto t = juce::ValueTree::fromXml(*xml);
        userPresetName = t.getProperty("userPreset", "").toString();
        state.replaceState(t);
    }
}

juce::File DelayProcessor::userPresetFolder() {
    return juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("Swell Audio").getChildFile("Swell Delay Presets");
}

bool DelayProcessor::saveUserPreset(const juce::File& f) {
    auto t = state.copyState();
    t.removeProperty("userPreset", nullptr);
    auto xml = t.createXml();
    if (!xml) return false;
    f.getParentDirectory().createDirectory();
    if (!xml->writeTo(f)) return false;
    userPresetName = f.getFileNameWithoutExtension();
    return true;
}

bool DelayProcessor::loadUserPreset(const juce::File& f) {
    auto xml = juce::XmlDocument::parse(f);
    if (!xml || !xml->hasTagName(state.state.getType())) return false;
    auto t = juce::ValueTree::fromXml(*xml);
    for (auto* p : getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(p)) {
            auto id = ranged->getParameterID();
            if (id == "mix" || id == "bypass") continue; // a preset never changes the level you set
            for (auto child : t)
                if (child.getProperty("id").toString() == id) setValue(id.toRawUTF8(), (float) child.getProperty("value"));
        }
    userPresetName = f.getFileNameWithoutExtension();
    return true;
}

juce::AudioProcessorEditor* DelayProcessor::createEditor() { return swell::LicenceGuard::attach(new DelayEditor(*this)); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new DelayProcessor(); }
