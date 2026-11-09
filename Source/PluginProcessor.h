#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "DelayEngine.h"
#include "Presets.h"

inline juce::String utf8(const char* s) { return juce::String::fromUTF8(s); }

class DelayProcessor : public juce::AudioProcessor {
public:
    DelayProcessor();
    juce::AudioProcessorValueTreeState state;
    // Read by the editor: host tempo, the delay time in use, and a counter bumped on each input hit.
    std::atomic<float> bpm { 120 }, delaySeconds { .5f };
    std::atomic<int> onsets { 0 };

    const juce::String getName() const override { return "Swell Delay"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 12; }
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void prepareToPlay(double, int) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorParameter* getBypassParameter() const override { return state.getParameter("bypass"); }
    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override;

    int getNumPrograms() override { return presetCount; }
    int getCurrentProgram() override { return (int) value("program"); }
    void setCurrentProgram(int i) override { loadPreset(i); }
    const juce::String getProgramName(int i) override { return presetName(i); }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    // Loads a factory preset into the controls (Mix and bypass are kept).
    void loadPreset(int index);
    static juce::String presetName(int i);
    float value(const char* id) const { return state.getRawParameterValue(id)->load(); }
    void setValue(const char* id, float v);
    swell::DelayParams currentParams(double hostBpm) const;
    static float delayTimeFor(bool sync, int division, float timeMs, double hostBpm);

    // User presets: Documents/Swell Audio/Swell Delay Presets/*.swelldelay (XML).
    static juce::File userPresetFolder();
    bool saveUserPreset(const juce::File&);
    bool loadUserPreset(const juce::File&);
    juce::String userPresetName;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    void render(juce::AudioBuffer<float>&, bool bypassed);
    swell::DelayEngine engine;
    juce::AudioBuffer<float> wet;
    juce::SmoothedValue<float> dryGain, wetGain;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DelayProcessor)
};
