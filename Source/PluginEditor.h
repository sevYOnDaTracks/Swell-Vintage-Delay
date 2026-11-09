#pragma once
#include "PluginProcessor.h"

// Interface colours. The face is a three-step gradient; the accent lights arcs, values and lit buttons.
struct Theme {
    juce::String name;
    juce::Colour metal1, metal2, metal3, accent;
    static Theme custom(juce::Colour face, juce::Colour accent);
    static const std::array<Theme, 5>& presets();
};

class DelayLook : public juce::LookAndFeel_V4 {
public:
    DelayLook();
    juce::Colour accent { 0xffa9d6ff };
    void setAccent(juce::Colour);
    void drawRotarySlider(juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
};

// Label above, knob, value below.
class Knob : public juce::Component {
public:
    Knob(DelayProcessor&, DelayLook&, const char* paramId, juce::String label, std::function<juce::String(float)> format, bool small);
    void resized() override;
    void paint(juce::Graphics&) override;
    juce::Slider slider;
private:
    DelayLook& look;
    juce::String label;
    std::function<juce::String(float)> format;
    bool small;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach;
};

// Row of buttons bound to one parameter (SYNC / MS, MONO / STEREO / PING-PONG), or a single on/off button (SPACE).
class Segmented : public juce::Component {
public:
    Segmented(DelayProcessor&, DelayLook&, const char* paramId, juce::StringArray labels, std::vector<float> values, float gap, bool toggle = false, float fontSize = 11.5f);
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
private:
    DelayLook& look;
    juce::StringArray labels;
    std::vector<float> values;
    float gap, fontSize;
    bool toggle;
    float current = 0;
    juce::ParameterAttachment attach;
    juce::Rectangle<float> cell(int i) const;
};

class PresetBox : public juce::Component {
public:
    explicit PresetBox(DelayLook& l) : look(l) {}
    std::function<void()> onPrev, onNext, onMenu;
    juce::String label;
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
private:
    DelayLook& look;
};

// Shows where the repeats land, left and right, over two seconds. Lights them as they play after each hit.
class EchoDisplay : public juce::Component {
public:
    EchoDisplay(DelayProcessor& p, DelayLook& l) : proc(p), look(l) {}
    void paint(juce::Graphics&) override;
    void tick();
private:
    DelayProcessor& proc;
    DelayLook& look;
    int lastOnsets = -1;
    double hitTime = -100;
};

class IconButton : public juce::Component {
public:
    enum Kind { Save, Folder, Settings, Help };
    IconButton(Kind k, DelayLook& l) : kind(k), look(l) { setMouseCursor(juce::MouseCursor::PointingHandCursor); }
    std::function<void()> onClick;
    bool active = false;
    void paint(juce::Graphics&) override;
    void mouseUp(const juce::MouseEvent& e) override { if (contains(e.getPosition()) && onClick) onClick(); }
    void mouseEnter(const juce::MouseEvent&) override { repaint(); }
    void mouseExit(const juce::MouseEvent&) override { repaint(); }
private:
    Kind kind;
    DelayLook& look;
};

// Gear panel: five ready-made colours, a free colour for the face and for the lights, and the interface size.
class ColourPanel : public juce::Component, private juce::ChangeListener {
public:
    explicit ColourPanel(DelayLook&);
    ~ColourPanel() override;
    std::function<void(const Theme&)> onTheme;
    std::function<void(float)> onScale;
    void setTheme(const Theme&, float scale);
    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
private:
    DelayLook& look;
    Theme theme;
    float scale = 1;
    int editing = 0; // 0 face, 1 lights
    juce::ColourSelector picker { juce::ColourSelector::showColourspace };
    juce::Rectangle<float> swatch(int i) const, tab(int i) const, sizeCell(int i) const;
    void changeListenerCallback(juce::ChangeBroadcaster*) override;
};

// Everything at the 1068 x 570 design size; the editor scales it.
class Panel : public juce::Component, private juce::Timer {
public:
    explicit Panel(DelayProcessor&);
    ~Panel() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    std::function<void(float)> onScale;
    void applyTheme(const Theme&, bool save);
    void setScaleShown(float s);
    void showColours(bool);
    static constexpr int W = 1068, H = 570;
private:
    DelayProcessor& proc;
    DelayLook look;
    Theme theme;
    float scale = 1;
    juce::Image logo, woodL, woodR, metal;
    std::unique_ptr<Knob> divisionKnob, msKnob, feedback, mix, spaceAmount;
    std::vector<std::unique_ptr<Knob>> advKnobs;
    std::unique_ptr<Segmented> syncSeg, modeSeg, spaceButton;
    PresetBox categoryBox { look }, presetBox { look };
    EchoDisplay echo { proc, look };
    IconButton save { IconButton::Save, look }, folder { IconButton::Folder, look }, settings { IconButton::Settings, look }, help { IconButton::Help, look };
    ColourPanel colours { look };
    std::unique_ptr<juce::FileChooser> chooser;
    juce::String hint;
    std::unique_ptr<juce::PropertiesFile> props; // colours and size, shared by every instance
    void timerCallback() override;
    void stepCategory(int);
    void stepPreset(int);
    void presetMenu();
    void categoryMenu();
    void folderMenu();
    void savePreset();
    void refreshNames();
    void renderWood();
    void renderMetal();
};

class DelayEditor : public juce::AudioProcessorEditor {
public:
    explicit DelayEditor(DelayProcessor&);
    void resized() override;
private:
    Panel panel;
    float scale = 1;
};
