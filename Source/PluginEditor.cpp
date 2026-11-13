#include "PluginEditor.h"
#include "BinaryData.h"
#include "VintageUI.h"

namespace {
const juce::Colour ink { 0xffeadfc6 }, dim { 0xffa89f8c }, muted { 0xff6e675a }, well { 0xff0f0f10 }, lineDark { 0xff0b0b0c }, offBtn { 0xff141415 };
constexpr float X0 = 34; // left edge of the metal face (after the wooden cheek)

juce::Font uiFont(float size, bool bold = false, float kerning = 0) {
#if JUCE_WINDOWS
    const char* face = "Segoe UI";
#else
    const char* face = "Helvetica Neue";
#endif
    return juce::Font(juce::FontOptions(face, size, bold ? juce::Font::bold : juce::Font::plain)).withExtraKerningFactor(kerning);
}
void drawLabel(juce::Graphics& g, const juce::String& s, juce::Rectangle<float> r, float size, juce::Colour c, juce::Justification j = juce::Justification::centred, float kerning = 0, bool bold = false) {
    g.setColour(c); g.setFont(uiFont(size, bold, kerning)); g.drawText(s, r, j, false);
}
juce::String hz(float v) { return v >= 1000 ? juce::String(v / 1000, 1) + " kHz" : juce::String(juce::roundToInt(v)) + " Hz"; }
juce::String pct(float v) { return juce::String(juce::roundToInt(v)) + " %"; }
juce::String ms(float v) { return juce::String(juce::roundToInt(v)) + " ms"; }
// Recessed black well (displays, preset boxes).
void drawWell(juce::Graphics& g, juce::Rectangle<float> r, float radius = 4) {
    g.setColour(juce::Colours::white.withAlpha(.05f)); g.drawRoundedRectangle(r.translated(0, 1), radius, 1);
    g.setColour(juce::Colour(0xff0b0b0c)); g.fillRoundedRectangle(r, radius);
    g.setColour(juce::Colours::black); g.drawRoundedRectangle(r, radius, 1);
    juce::ColourGradient inner(juce::Colours::black.withAlpha(.75f), 0, r.getY(), juce::Colours::transparentBlack, 0, r.getY() + 8, false);
    g.setGradientFill(inner); g.fillRoundedRectangle(r.reduced(1), radius);
}
}

//============================================================================== Theme
Theme Theme::custom(juce::Colour face, juce::Colour accent) {
    return { "Personnalis" + juce::String::fromUTF8("\xc3\xa9") + "e", face, face.withMultipliedBrightness(.78f), face.withMultipliedBrightness(.55f), accent };
}
const std::array<Theme, 6>& Theme::presets() {
    static const std::array<Theme, 6> list {{
        { "Vintage",                           juce::Colour(0xff29282c), juce::Colour(0xff1e1d21), juce::Colour(0xff141316), juce::Colour(0xffffb547) },
        { "Bleu nuit",                         juce::Colour(0xff232d3b), juce::Colour(0xff1a2230), juce::Colour(0xff121924), juce::Colour(0xffa9d6ff) },
        { "Vert anglais",                      juce::Colour(0xff223029), juce::Colour(0xff19241f), juce::Colour(0xff111a16), juce::Colour(0xffe8dcc0) },
        { "Bordeaux",                          juce::Colour(0xff32202a), juce::Colour(0xff271820), juce::Colour(0xff1b1016), juce::Colour(0xfff2d8b0) },
        { juce::String::fromUTF8("Acier & cr\xc3\xa8me"), juce::Colour(0xff3b3d41), juce::Colour(0xff2f3135), juce::Colour(0xff232528), juce::Colour(0xfff3e3c0) },
        { "Noir & blanc",                      juce::Colour(0xff2a2a2b), juce::Colour(0xff202021), juce::Colour(0xff171718), juce::Colour(0xffffffff) },
    }};
    return list;
}

//============================================================================== Look
DelayLook::DelayLook() { setAccent(accent); }
void DelayLook::setAccent(juce::Colour c) {
    accent = c;
    setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff1a1a1b));
    setColour(juce::PopupMenu::textColourId, ink);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, accent);
    setColour(juce::PopupMenu::highlightedTextColourId, juce::Colour(0xff111111));
    setColour(juce::PopupMenu::headerTextColourId, dim);
}

void DelayLook::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float start, float end, juce::Slider& s) {
    vui::knob(g, { (float) x, (float) y, (float) w, (float) h }, pos, start, end, s.getProperties()["small"], s.getProperties()["bipolar"], accent);
}

//============================================================================== Knob
Knob::Knob(DelayProcessor& p, DelayLook& l, const char* id, juce::String lb, std::function<juce::String(float)> f, bool sm)
    : look(l), label(std::move(lb)), format(std::move(f)), small(sm) {
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters(juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    slider.setMouseDragSensitivity(220);
    slider.setVelocityModeParameters(.6, 1, .05, true, juce::ModifierKeys::shiftModifier);
    slider.getProperties().set("small", small);
    slider.getProperties().set("bipolar", juce::String(id) == "offset");
    slider.onValueChange = [this] { repaint(); };
    addAndMakeVisible(slider);
    attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.state, id, slider);
    if (auto* param = p.state.getParameter(id)) slider.setDoubleClickReturnValue(true, param->convertFrom0to1(param->getDefaultValue()));
}
void Knob::resized() {
    auto r = getLocalBounds();
    r.removeFromTop(small ? 18 : 30); r.removeFromBottom(small ? 18 : 40);
    int s = std::min(r.getWidth(), r.getHeight());
    slider.setBounds(r.withSizeKeepingCentre(s, s));
}
void Knob::paint(juce::Graphics& g) {
    auto r = getLocalBounds().toFloat();
    if (small) {
        drawLabel(g, label, r.removeFromTop(18), 12.5f, ink);
        vui::glowText(g, format((float) slider.getValue()), r.removeFromBottom(18), vui::font(12.5f), look.accent);
        return;
    }
    drawLabel(g, label.toUpperCase(), r.removeFromTop(30), 21, ink, juce::Justification::centred, .14f, true);
    auto box = r.removeFromBottom(40).withSizeKeepingCentre(std::min(150.f, r.getWidth() - 10), 30);
    vui::lcd(g, box);
    vui::glowText(g, format((float) slider.getValue()), box, vui::mono(17), look.accent);
}

//============================================================================== Segmented
Segmented::Segmented(DelayProcessor& p, DelayLook& l, const char* id, juce::StringArray lb, std::vector<float> v, float gp, bool tg, float fs)
    : look(l), labels(std::move(lb)), values(std::move(v)), gap(gp), fontSize(fs), toggle(tg),
      attach(*p.state.getParameter(id), [this](float x) { current = x; repaint(); }, nullptr) {
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
    attach.sendInitialUpdate();
}
juce::Rectangle<float> Segmented::cell(int i) const {
    float n = (float) labels.size(), w = ((float) getWidth() - gap * (n - 1)) / n;
    return { (float) i * (w + gap), 0, w, (float) getHeight() };
}
void Segmented::paint(juce::Graphics& g) {
    for (int i = 0; i < labels.size(); ++i) {
        auto r = cell(i).reduced(gap > 0 ? .5f : 1.f, 1.f);
        bool on = toggle ? current > .5f : std::abs(current - values[(size_t) i]) < .5f;
        vui::raised(g, r, on, 4);
        if (on) {
            auto lamp = juce::Rectangle<float>(r.getX() + 9, r.getCentreY() - 3, 6, 6);
            g.setColour(look.accent.withAlpha(.3f)); g.fillEllipse(lamp.expanded(3));
            g.setColour(look.accent); g.fillEllipse(lamp);
        } else {
            g.setColour(juce::Colours::black.withAlpha(.7f)); g.fillEllipse(r.getX() + 9, r.getCentreY() - 3, 6, 6);
        }
        drawLabel(g, labels[i], r.withTrimmedLeft(14), fontSize, on ? ink : muted, juce::Justification::centred, .2f, on);
    }
}
void Segmented::mouseDown(const juce::MouseEvent& e) {
    if (toggle) { attach.setValueAsCompleteGesture(current > .5f ? 0.f : 1.f); return; }
    for (int i = 0; i < labels.size(); ++i)
        if (cell(i).contains(e.position)) attach.setValueAsCompleteGesture(values[(size_t) i]);
}

//============================================================================== PresetBox
void PresetBox::paint(juce::Graphics& g) { vui::presetDisplay(g, getLocalBounds().toFloat().reduced(.5f), label, look.accent); }
void PresetBox::mouseDown(const juce::MouseEvent& e) {
    if (e.x < 34) { if (onPrev) onPrev(); }
    else if (e.x > getWidth() - 34) { if (onNext) onNext(); }
    else if (onMenu) onMenu();
}

//============================================================================== Echo display
void EchoDisplay::tick() {
    int o = proc.onsets.load();
    const double now = clock();
    if (o != lastOnsets) { if (lastOnsets >= 0) hitTime = now; lastOnsets = o; }
    // Tape runs faster for short delays, like a real echo machine's head spacing.
    const float d = juce::jlimit(.03f, 2.f, proc.delaySeconds.load());
    if (lastTick > 0) reelAngle += (float) (now - lastTick) * juce::jlimit(.8f, 9.f, .9f / d);
    reelAngle = std::fmod(reelAngle, juce::MathConstants<float>::twoPi);
    lastTick = now;
    repaint();
}

namespace {
void drawReel(juce::Graphics& g, juce::Point<float> c, float R, float angle, float pack) {
    using juce::MathConstants;
    // Tape pack seen through the flange.
    g.setColour(juce::Colour(0xff2a1a10)); g.fillEllipse(c.x - R * pack, c.y - R * pack, R * pack * 2, R * pack * 2);
    g.setColour(juce::Colour(0xff4a2f1c).withAlpha(.6f));
    for (float rr = R * .36f; rr < R * pack; rr += 2.2f) g.drawEllipse(c.x - rr, c.y - rr, rr * 2, rr * 2, .5f);
    // Chrome flange with three windows.
    juce::Path flange; flange.addEllipse(c.x - R, c.y - R, R * 2, R * 2);
    for (int i = 0; i < 3; ++i) {
        float a0 = angle + (float) i * MathConstants<float>::twoPi / 3 + .32f, a1 = a0 + 1.45f;
        juce::Path win; win.addPieSegment(c.x - R * .86f, c.y - R * .86f, R * 1.72f, R * 1.72f, a0, a1, .44f);
        flange.addPath(win);
    }
    flange.setUsingNonZeroWinding(false);
    g.setColour(juce::Colours::black.withAlpha(.6f)); g.fillEllipse(c.x - R + 2, c.y - R + 4, R * 2, R * 2);
    juce::ColourGradient metal(juce::Colour(0xffe2e2e2), c.x - R * .5f, c.y - R * .7f, juce::Colour(0xff5c5c5c), c.x + R * .6f, c.y + R, true);
    metal.addColour(.5, juce::Colour(0xffa4a4a4));
    g.setGradientFill(metal); g.fillPath(flange);
    g.setColour(juce::Colours::black.withAlpha(.55f)); g.strokePath(flange, juce::PathStrokeType(1));
    g.setColour(juce::Colours::white.withAlpha(.25f)); g.drawEllipse(c.x - R * .95f, c.y - R * .95f, R * 1.9f, R * 1.9f, .8f);
    // Hub with three keys.
    float h = R * .3f;
    juce::ColourGradient hub(juce::Colour(0xffcfcfcf), c.x - h * .4f, c.y - h * .5f, juce::Colour(0xff3c3c3c), c.x + h, c.y + h, true);
    g.setGradientFill(hub); g.fillEllipse(c.x - h, c.y - h, h * 2, h * 2);
    g.setColour(juce::Colours::black.withAlpha(.7f)); g.drawEllipse(c.x - h, c.y - h, h * 2, h * 2, 1);
    for (int i = 0; i < 3; ++i) {
        float a = angle + (float) i * MathConstants<float>::twoPi / 3;
        juce::Path key; key.addRoundedRectangle(-1.6f, -h * .95f, 3.2f, h * .5f, 1);
        g.setColour(juce::Colour(0xff222222)); g.fillPath(key, juce::AffineTransform::rotation(a).translated(c.x, c.y));
    }
    g.setColour(juce::Colour(0xff151515)); g.fillEllipse(c.x - h * .28f, c.y - h * .28f, h * .56f, h * .56f);
}
void roller(juce::Graphics& g, juce::Point<float> c, float r) {
    g.setColour(juce::Colours::black.withAlpha(.6f)); g.fillEllipse(c.x - r + 1, c.y - r + 2, r * 2, r * 2);
    juce::ColourGradient m(juce::Colour(0xffdedede), c.x - r * .4f, c.y - r * .5f, juce::Colour(0xff4a4a4a), c.x + r, c.y + r, true);
    g.setGradientFill(m); g.fillEllipse(c.x - r, c.y - r, r * 2, r * 2);
    g.setColour(juce::Colour(0xff202020)); g.fillEllipse(c.x - r * .35f, c.y - r * .35f, r * .7f, r * .7f);
}
}

void EchoDisplay::paint(juce::Graphics& g) {
    auto r = getLocalBounds().toFloat().reduced(.5f);
    // Smoked glass window in a chrome bezel.
    g.setColour(juce::Colours::black.withAlpha(.7f)); g.fillRoundedRectangle(r.translated(0, 2), 9);
    juce::ColourGradient bez(juce::Colour(0xff6a6a6e), 0, r.getY(), juce::Colour(0xff1c1c1f), 0, r.getBottom(), false);
    g.setGradientFill(bez); g.fillRoundedRectangle(r, 9);
    auto win = r.reduced(5);
    juce::ColourGradient bg(juce::Colour(0xff2b2622), win.getCentreX(), win.getY() + win.getHeight() * .35f, juce::Colour(0xff0b0a09), win.getX(), win.getBottom(), true);
    g.setGradientFill(bg); g.fillRoundedRectangle(win, 6);

    const float stripH = 30;
    auto deck = win.withTrimmedBottom(stripH);
    const float R = std::min(deck.getHeight() / 2 - 8, 58.f);
    const juce::Point<float> L(deck.getX() + 22 + R, deck.getCentreY() - 2), Rc(deck.getRight() - 22 - R, deck.getCentreY() - 2);
    const float d = proc.delaySeconds.load();
    const float spread = juce::jlimit(0.f, 1.f, d / 2.f);
    const float packL = .8f - .32f * spread, packR = .48f + .32f * spread;

    // Tape path: under each reel, round the guides and across the heads.
    const float baseY = deck.getBottom() - 12;
    const juce::Point<float> gL(L.x + R * .7f, baseY - 4), gR(Rc.x - R * .7f, baseY - 4);
    const float headW = 46, hx = deck.getCentreX();
    juce::Path tape;
    tape.startNewSubPath(L.x, L.y + R * packL);
    tape.quadraticTo(L.x + R * .45f, baseY + 2, gL.x, gL.y + 6);
    tape.lineTo(gR.x, gR.y + 6);
    tape.quadraticTo(Rc.x - R * .45f, baseY + 2, Rc.x, Rc.y + R * packR);
    g.setColour(juce::Colour(0xff6b4024)); g.strokePath(tape, juce::PathStrokeType(2.4f));
    g.setColour(juce::Colour(0xffa0663c).withAlpha(.5f)); g.strokePath(tape, juce::PathStrokeType(.8f));

    drawReel(g, L, R, reelAngle, packL);
    drawReel(g, Rc, R, reelAngle * (packL / packR), packR);
    roller(g, gL, 7); roller(g, gR, 7);
    roller(g, { hx - headW / 2 - 16, baseY - 2 }, 5.5f); roller(g, { hx + headW / 2 + 16, baseY - 2 }, 5.5f);
    // Head block: record + playback heads; the playback lamp flashes on each echo.
    auto block = juce::Rectangle<float>(hx - headW / 2, baseY - 16, headW, 20);
    juce::ColourGradient hb(juce::Colour(0xffcfcfcf), 0, block.getY(), juce::Colour(0xff555555), 0, block.getBottom(), false);
    g.setGradientFill(hb); g.fillRoundedRectangle(block, 3);
    g.setColour(juce::Colours::black.withAlpha(.7f)); g.drawRoundedRectangle(block, 3, 1);
    auto top = juce::Rectangle<float>(hx - 13, block.getY() - 9, 26, 10);
    g.setGradientFill(hb); g.fillRoundedRectangle(top, 2);
    g.setColour(juce::Colours::black.withAlpha(.6f)); g.drawRoundedRectangle(top, 2, 1);

    // Echo strip: where the repeats land, left lane above right lane.
    auto strip = win.removeFromBottom(stripH).reduced(10, 4);
    g.setColour(juce::Colours::black.withAlpha(.55f)); g.fillRoundedRectangle(strip, 3);
    const float span = 2.2f, mid = strip.getCentreY();
    auto px = [&](float t) { return strip.getX() + 16 + t / span * (strip.getWidth() - 24); };
    drawLabel(g, "L", { strip.getX() + 2, strip.getY(), 12, strip.getHeight() / 2 }, 8.5f, muted);
    drawLabel(g, "R", { strip.getX() + 2, mid, 12, strip.getHeight() / 2 }, 8.5f, muted);
    g.setColour(juce::Colours::white.withAlpha(.05f)); g.drawHorizontalLine((int) mid, strip.getX() + 16, strip.getRight() - 6);

    const float fb = std::min(1.f, proc.value("feedback") / 100), mix = proc.value("mix") / 100;
    const int mode = (int) proc.value("mode");
    const bool space = proc.value("space") > .5f;
    const float spaceAmt = proc.value("spaceAmount") / 100, width = proc.value("width") / 100, offset = proc.value("offset") / 1000;
    const float damp = proc.value("feedback") > 100 ? 1.f : std::pow(proc.value("highCut") / 20000, .12f);
    const double age = clock() - hitTime;
    const auto accent = look.accent;
    const float laneH = strip.getHeight() / 2 - 3;
    float flash = 0;
    auto bar = [&](float t, int lane, float a) {
        if (t > span || a < .01f) return;
        a = std::min(1.f, a);
        float bh = std::max(1.5f, laneH * a), x = px(t), hot = 0;
        double since = age - t;
        if (since >= 0 && since < .14) { hot = 1 - (float) (since / .14); flash = std::max(flash, hot * a); }
        auto rect = lane == 0 ? juce::Rectangle<float>(x - 2, mid - 1 - bh, 4, bh) : juce::Rectangle<float>(x - 2, mid + 1, 4, bh);
        if (hot > 0) { g.setColour(accent.withAlpha(.35f * hot)); g.fillRoundedRectangle(rect.expanded(3), 2); }
        g.setColour(accent.withAlpha(juce::jlimit(0.f, 1.f, .3f + .55f * a + hot * .15f)));
        g.fillRoundedRectangle(rect, 1);
    };
    float a = .55f + mix * .45f;
    for (int n = 1; n < 80 && d > 0; ++n) {
        float t = (float) n * d;
        if (t > span) break;
        if (space) { float p = std::sin((float) n * 1.9f + .6f) * spaceAmt; bar(t, 0, a * (1 - p) * .75f); bar(t, 1, a * (1 + p) * .75f); }
        else if (mode == swell::PingPong) bar(t, n % 2 == 1 ? 0 : 1, a);
        else if (mode == swell::Stereo) { float k = a * (.5f + width / 4); bar(t, 0, k); bar(t + offset, 1, k); }
        else { bar(t, 0, a); bar(t, 1, a); }
        a *= fb * damp;
        if (a < .01f) break;
    }
    if (age >= 0 && age < span) { g.setColour(accent.withAlpha(.35f)); g.drawVerticalLine((int) px((float) age), strip.getY() + 1, strip.getBottom() - 1); }
    // Playback lamp on the head block.
    auto lamp = juce::Rectangle<float>(hx - 3.5f, block.getCentreY() - 3.5f, 7, 7);
    g.setColour(accent.withAlpha(.15f + .5f * flash)); g.fillEllipse(lamp.expanded(4 * flash));
    g.setColour(accent.withMultipliedBrightness(.35f + .65f * std::max(flash, .25f))); g.fillEllipse(lamp);
    // Glass reflection.
    juce::Path glare; glare.addRoundedRectangle(r.reduced(5).withHeight(r.getHeight() * .42f), 6);
    juce::ColourGradient gl(juce::Colours::white.withAlpha(.07f), 0, r.getY(), juce::Colours::transparentWhite, 0, r.getY() + r.getHeight() * .42f, false);
    g.setGradientFill(gl); g.fillPath(glare);
}

//============================================================================== Icons
void IconButton::paint(juce::Graphics& g) {
    auto r = getLocalBounds().toFloat().reduced(2);
    float s = r.getWidth() / 24.f;
    juce::Path p;
    auto P = [&](float x, float y) { return juce::Point<float>(r.getX() + x * s, r.getY() + y * s); };
    switch (kind) {
        case Save:
            p.startNewSubPath(P(5, 4)); p.lineTo(P(16, 4)); p.lineTo(P(19, 7)); p.lineTo(P(19, 20)); p.lineTo(P(5, 20)); p.closeSubPath();
            p.startNewSubPath(P(8, 4)); p.lineTo(P(8, 9)); p.lineTo(P(15, 9)); p.lineTo(P(15, 4));
            p.startNewSubPath(P(8, 20)); p.lineTo(P(8, 14)); p.lineTo(P(16, 14)); p.lineTo(P(16, 20));
            break;
        case Folder:
            p.startNewSubPath(P(3, 7)); p.lineTo(P(9, 7)); p.lineTo(P(11, 9)); p.lineTo(P(21, 9)); p.lineTo(P(21, 19)); p.lineTo(P(3, 19)); p.closeSubPath();
            break;
        case Settings:
            p.addEllipse(r.getCentreX() - 3 * s, r.getCentreY() - 3 * s, 6 * s, 6 * s);
            for (int i = 0; i < 8; ++i) {
                float a = (float) i * juce::MathConstants<float>::pi / 4, c = std::cos(a), sn = std::sin(a);
                p.startNewSubPath(P(12 + 6 * c, 12 + 6 * sn)); p.lineTo(P(12 + 9.5f * c, 12 + 9.5f * sn));
            }
            break;
        case Help:
            p.addEllipse(P(3, 3).x, P(3, 3).y, 18 * s, 18 * s);
            p.startNewSubPath(P(9.5f, 9.5f)); p.cubicTo(P(9.5f, 6.5f), P(14.5f, 6.5f), P(14.5f, 9.5f)); p.cubicTo(P(14.5f, 11.5f), P(12, 11.5f), P(12, 14));
            p.addEllipse(P(11.3f, 16.6f).x, P(11.3f, 16.6f).y, 1.4f * s, 1.4f * s);
            break;
    }
    g.setColour(active ? look.accent : isMouseOver() ? ink : dim);
    g.strokePath(p, juce::PathStrokeType(1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

//============================================================================== Colour panel
namespace { const float panelSizes[4] { .75f, 1.f, 1.25f, 1.5f }; }
ColourPanel::ColourPanel(DelayLook& l) : look(l) {
    picker.setColour(juce::ColourSelector::backgroundColourId, juce::Colours::transparentBlack);
    picker.addChangeListener(this);
    addAndMakeVisible(picker);
}
ColourPanel::~ColourPanel() { picker.removeChangeListener(this); }
juce::Rectangle<float> ColourPanel::swatch(int i) const { return { 18.f + (float) i * 46, 42, 34, 34 }; }
juce::Rectangle<float> ColourPanel::tab(int i) const { return { 16.f + (float) i * 135, 110, 133, 28 }; }
juce::Rectangle<float> ColourPanel::sizeCell(int i) const { return { 16.f + (float) i * 68, 356, 64, 26 }; }
void ColourPanel::resized() { picker.setBounds(16, 146, getWidth() - 32, 170); }
void ColourPanel::setTheme(const Theme& t, float s) {
    theme = t; scale = s;
    picker.setCurrentColour(editing == 0 ? theme.metal1 : theme.accent, juce::dontSendNotification);
    repaint();
}
void ColourPanel::changeListenerCallback(juce::ChangeBroadcaster*) {
    auto c = picker.getCurrentColour().withAlpha(1.f);
    theme = editing == 0 ? Theme::custom(c, theme.accent) : Theme::custom(theme.metal1, c);
    if (onTheme) onTheme(theme);
    repaint();
}
void ColourPanel::paint(juce::Graphics& g) {
    auto r = getLocalBounds().toFloat();
    g.setColour(juce::Colours::black.withAlpha(.5f)); g.fillRoundedRectangle(r.translated(0, 6).reduced(2), 8);
    g.setColour(juce::Colour(0xff0d0d0e)); g.fillRoundedRectangle(r, 7);
    g.setColour(juce::Colours::white.withAlpha(.07f)); g.drawRoundedRectangle(r.reduced(.5f), 7, 1);
    drawLabel(g, juce::String::fromUTF8("COULEUR DE L\xe2\x80\x99INTERFACE"), { 18, 12, r.getWidth() - 36, 18 }, 11.5f, dim, juce::Justification::centredLeft, .2f);
    const auto& list = Theme::presets();
    for (int i = 0; i < 6; ++i) {
        auto s = swatch(i);
        bool on = list[(size_t) i].name == theme.name;
        g.setColour(on ? look.accent : juce::Colour(0xff333333)); g.drawEllipse(s, 2);
        auto in = s.reduced(4);
        juce::Path left; left.addPieSegment(in, juce::MathConstants<float>::pi * 1.75f, juce::MathConstants<float>::pi * 2.75f, 0);
        juce::Path right; right.addPieSegment(in, juce::MathConstants<float>::pi * .75f, juce::MathConstants<float>::pi * 1.75f, 0);
        g.setColour(list[(size_t) i].metal1.brighter(.15f)); g.fillPath(right);
        g.setColour(list[(size_t) i].accent); g.fillPath(left);
    }
    drawLabel(g, theme.name, { 18, 82, r.getWidth() - 36, 18 }, 13, look.accent, juce::Justification::centredLeft);
    for (int i = 0; i < 2; ++i) {
        auto t = tab(i);
        bool on = editing == i;
        g.setColour(on ? look.accent : offBtn); g.fillRoundedRectangle(t, 4);
        g.setColour(juce::Colours::black); g.drawRoundedRectangle(t, 4, 1);
        drawLabel(g, i == 0 ? juce::String::fromUTF8("FA\xc3\x87" "ADE") : juce::String::fromUTF8("LUMI\xc3\x88RES"), t, 11, on ? juce::Colour(0xff111111) : muted, juce::Justification::centred, .2f);
    }
    drawLabel(g, "TAILLE", { 18, 330, 120, 18 }, 11, dim, juce::Justification::centredLeft, .2f);
    for (int i = 0; i < 4; ++i) {
        auto c = sizeCell(i);
        bool on = std::abs(panelSizes[i] - scale) < .01f;
        g.setColour(on ? look.accent : offBtn); g.fillRoundedRectangle(c, 4);
        g.setColour(juce::Colours::black); g.drawRoundedRectangle(c, 4, 1);
        drawLabel(g, juce::String(juce::roundToInt(panelSizes[i] * 100)) + " %", c, 11.5f, on ? juce::Colour(0xff111111) : muted);
    }
}
void ColourPanel::mouseDown(const juce::MouseEvent& e) {
    const auto& list = Theme::presets();
    for (int i = 0; i < 6; ++i)
        if (swatch(i).contains(e.position)) { theme = list[(size_t) i]; setTheme(theme, scale); if (onTheme) onTheme(theme); return; }
    for (int i = 0; i < 2; ++i)
        if (tab(i).contains(e.position)) { editing = i; setTheme(theme, scale); return; }
    for (int i = 0; i < 4; ++i)
        if (sizeCell(i).contains(e.position)) { scale = panelSizes[i]; repaint(); if (onScale) onScale(scale); return; }
}

//============================================================================== Panel
Panel::Panel(DelayProcessor& p) : proc(p) {
    setLookAndFeel(&look);
    logo = juce::ImageCache::getFromMemory(BinaryData::swellmark_png, BinaryData::swellmark_pngSize);
    juce::PropertiesFile::Options o;
    o.applicationName = "Swell Delay"; o.filenameSuffix = ".settings"; o.folderName = "Swell Audio"; o.osxLibrarySubFolder = "Application Support";
    props = std::make_unique<juce::PropertiesFile>(o);

    auto knob = [&](const char* id, juce::String name, std::function<juce::String(float)> f, bool small) {
        auto k = std::make_unique<Knob>(p, look, id, std::move(name), std::move(f), small);
        addAndMakeVisible(*k);
        return k;
    };
    divisionKnob = knob("division", "Time", [](float v) { return juce::String(swell::divisions[(size_t) juce::jlimit(0, 17, juce::roundToInt(v))].name); }, false);
    msKnob = knob("time", "Time", ms, false);
    addChildComponent(*msKnob);
    feedback = knob("feedback", "Feedback", [](float v) { return v > 100.5f ? juce::String("Infini") : pct(v); }, false);
    mix = knob("mix", "Mix", pct, false);
    spaceAmount = knob("spaceAmount", "Amount", pct, true);
    advKnobs.push_back(knob("lowCut", "Low Cut", hz, true));
    advKnobs.push_back(knob("highCut", "High Cut", hz, true));
    advKnobs.push_back(knob("wow", "Wow", pct, true));
    advKnobs.push_back(knob("flutter", "Flutter", pct, true));
    advKnobs.push_back(knob("saturation", "Saturation", pct, true));
    advKnobs.push_back(knob("duck", "Amount", [](float v) { return v < .5f ? juce::String("Off") : pct(v); }, true));
    advKnobs.push_back(knob("duckRelease", "Release", ms, true));
    advKnobs.push_back(knob("width", "Width", pct, true));
    advKnobs.push_back(knob("offset", "Offset R", [](float v) { int i = juce::roundToInt(v); return (i > 0 ? "+" : "") + juce::String(i) + " ms"; }, true));

    syncSeg = std::make_unique<Segmented>(p, look, "sync", juce::StringArray { "SYNC", "MS" }, std::vector<float> { 1, 0 }, 0.f);
    modeSeg = std::make_unique<Segmented>(p, look, "mode", juce::StringArray { "MONO", juce::String::fromUTF8("ST\xc3\x89R\xc3\x89O"), "PING-PONG" }, std::vector<float> { 0, 1, 2 }, 8.f);
    spaceButton = std::make_unique<Segmented>(p, look, "space", juce::StringArray { "SPACE" }, std::vector<float> { 1 }, 0.f, true, 12.5f);
    for (auto* c : std::initializer_list<juce::Component*> { syncSeg.get(), modeSeg.get(), spaceButton.get(), &categoryBox, &presetBox, &echo, &save, &folder, &settings, &help }) addAndMakeVisible(c);
    addChildComponent(colours);

    categoryBox.onPrev = [this] { stepCategory(-1); }; categoryBox.onNext = [this] { stepCategory(1); }; categoryBox.onMenu = [this] { categoryMenu(); };
    presetBox.onPrev = [this] { stepPreset(-1); }; presetBox.onNext = [this] { stepPreset(1); }; presetBox.onMenu = [this] { presetMenu(); };
    save.onClick = [this] { savePreset(); }; folder.onClick = [this] { folderMenu(); };
    settings.onClick = [this] { showColours(!colours.isVisible()); };
    help.onClick = [] {
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::NoIcon, "Swell Delay",
            juce::String::fromUTF8("Vintage Delay - version 0.2\nSwell Audio, by @sevyondatracks\n\n"
                                   "SYNC : le delay suit le tempo du morceau. MS : temps libre.\n"
                                   "SPACE : chaque r\xc3\xa9p\xc3\xa9tition part \xc3\xa0 un endroit diff\xc3\xa9rent.\n"
                                   "Roue crant\xc3\xa9" "e : couleur et taille de l'interface.\n\n"
                                   "Double-clic sur un bouton : valeur par d\xc3\xa9" "faut.\nShift + glisser : r\xc3\xa9glage fin.\n"
                                   "Les presets ne touchent jamais au Mix."));
    };
    colours.onTheme = [this](const Theme& t) { applyTheme(t, true); };
    colours.onScale = [this](float s) { scale = s; props->setValue("scale", (double) s); props->saveIfNeeded(); if (onScale) onScale(s); };

    renderWood();
    Theme t = Theme::presets()[0];
    if (props->containsKey("face")) {
        auto name = props->getValue("theme");
        bool found = false;
        for (auto& pre : Theme::presets()) if (pre.name == name) { t = pre; found = true; }
        if (!found) t = Theme::custom(juce::Colour::fromString(props->getValue("face")), juce::Colour::fromString(props->getValue("accent")));
    }
    applyTheme(t, false);
    scale = (float) props->getDoubleValue("scale", 1.0);
    refreshNames();
    setSize(W, H);
    timerCallback();
    startTimerHz(30);
}
Panel::~Panel() { setLookAndFeel(nullptr); }

void Panel::applyTheme(const Theme& t, bool persist) {
    theme = t;
    look.setAccent(t.accent);
    renderMetal();
    colours.setTheme(t, scale);
    if (persist) {
        props->setValue("theme", t.name); props->setValue("face", t.metal1.toString()); props->setValue("accent", t.accent.toString());
        props->saveIfNeeded();
    }
    repaint();
    for (auto* c : getChildren()) c->repaint();
}
void Panel::setScaleShown(float s) { scale = s; colours.setTheme(theme, s); }
void Panel::showColours(bool show) {
    colours.setTheme(theme, scale);
    colours.setVisible(show);
    if (show) colours.toFront(false);
    settings.active = show; settings.repaint();
}
void Panel::mouseDown(const juce::MouseEvent&) { if (colours.isVisible()) showColours(false); }

void Panel::renderWood() {
    auto makeWood = [](int seed) {
        juce::Image img(juce::Image::ARGB, 34, H, true);
        juce::Graphics g(img);
        juce::ColourGradient base(juce::Colour(0xff7a4524), 0, 0, juce::Colour(0xff683820), 0, (float) H, false);
        base.addColour(.3, juce::Colour(0xff5e3219)); base.addColour(.55, juce::Colour(0xff6f3d1f)); base.addColour(.8, juce::Colour(0xff4f2913));
        g.setGradientFill(base); g.fillAll();
        juce::Random r(seed);
        for (int i = 0; i < 70; ++i) {
            float x = r.nextFloat() * 34;
            juce::Path p; p.startNewSubPath(x, 0);
            for (int y = 16; y <= H + 16; y += 16) { x += (r.nextFloat() - .5f) * 1.3f; p.lineTo(x, (float) y); }
            bool light = r.nextFloat() < .25f;
            g.setColour((light ? juce::Colour(0xffffc890) : juce::Colour(0xff1e0c04)).withAlpha(light ? .05f + r.nextFloat() * .06f : .08f + r.nextFloat() * .22f));
            g.strokePath(p, juce::PathStrokeType(.4f + r.nextFloat() * 1.6f));
        }
        for (int k = 0; k < 2; ++k) {
            float cy = (float) H * (.25f + .45f * (float) k) + r.nextFloat() * 40;
            juce::ColourGradient knot(juce::Colour(0xff1e0c04).withAlpha(.35f), 16, cy, juce::Colours::transparentBlack, 34, cy, true);
            g.setGradientFill(knot); g.fillEllipse(-4, cy - 40, 42, 80);
        }
        juce::ColourGradient edge(juce::Colours::black.withAlpha(.45f), 0, 0, juce::Colours::black.withAlpha(.5f), 34, 0, false);
        edge.addColour(.18, juce::Colours::transparentBlack); edge.addColour(.5, juce::Colours::white.withAlpha(.06f)); edge.addColour(.82, juce::Colours::transparentBlack);
        g.setGradientFill(edge); g.fillAll();
        return img;
    };
    woodL = makeWood(7); woodR = makeWood(19);
}

void Panel::renderMetal() {
    metal = vui::brushedFace(1000, H, theme.metal1, theme.metal2, theme.metal3);
}

namespace { constexpr float advY = 494, advH = 136, stripY = 448; const int weights[4] { 2, 3, 2, 2 }; }

void Panel::resized() {
    const int x0 = (int) X0;
    categoryBox.setBounds(x0 + 40, 78, 400, 42);
    presetBox.setBounds(x0 + 40, 128, 400, 42);
    modeSeg->setBounds(x0 + 40, 182, 400, 32);
    echo.setBounds(x0 + 470, 72, 500, 150);
    divisionKnob->setBounds(x0 + 50, 230, 210, 192);
    msKnob->setBounds(x0 + 50, 230, 210, 192);
    syncSeg->setBounds(x0 + 75, 418, 160, 26);
    feedback->setBounds(x0 + 320, 230, 210, 192);
    mix->setBounds(x0 + 590, 230, 210, 192);
    spaceButton->setBounds(x0 + 838, 262, 120, 34);
    spaceAmount->setBounds(x0 + 846, 300, 104, 120);
    int x = x0 + 1000 - 32;
    for (auto* b : { &help, &settings, &folder, &save }) { x -= 30; b->setBounds(x, 18, 22, 22); }
    float unit = (940 - 42) / 9.f, gx = X0 + 30;
    int k = 0;
    for (int gi = 0; gi < 4; ++gi) {
        float gw = unit * (float) weights[gi], kw = (gw - 12) / (float) weights[gi];
        for (int j = 0; j < weights[gi]; ++j) advKnobs[(size_t) k++]->setBounds(juce::roundToInt(gx + 6 + kw * (float) j), (int) advY + 30, juce::roundToInt(kw), 104);
        gx += gw + 14;
    }
    colours.setBounds(x0 + 1000 - 22 - 300, 50, 300, 398);
}

void Panel::paint(juce::Graphics& g) {
    g.drawImageAt(woodL, 0, 0); g.drawImageAt(woodR, (int) X0 + 1000, 0);
    g.drawImageAt(metal, (int) X0, 0);
    auto face = juce::Rectangle<float>(X0, 0, 1000, (float) H);
    g.setColour(juce::Colours::black.withAlpha(.18f)); g.fillRect(face.withHeight(58));
    g.setColour(lineDark); g.drawHorizontalLine(58, X0, X0 + 1000);
    g.setColour(juce::Colours::white.withAlpha(.04f)); g.drawHorizontalLine(59, X0, X0 + 1000);
    vui::brand(g, logo, X0 + 24, 11);
    g.setColour(dim.withAlpha(.5f)); g.drawVerticalLine((int) X0 + 210, 14, 46);
    drawLabel(g, "VINTAGE DELAY", { X0 + 228, 12, 420, 34 }, 27, ink, juce::Justification::centredLeft, .12f, true);
    drawLabel(g, hint, { X0 + 240, 418, 260, 26 }, 11.5f, dim, juce::Justification::centredLeft, .04f);
    vui::groove(g, X0 + 20, X0 + 980, 224);

    g.setColour(juce::Colours::black.withAlpha(.25f)); g.fillRect(X0, stripY, 1000.f, 34.f);
    g.setColour(lineDark); g.drawHorizontalLine((int) stripY, X0, X0 + 1000);
    g.setColour(juce::Colours::white.withAlpha(.04f)); g.drawHorizontalLine((int) stripY + 1, X0, X0 + 1000);
    drawLabel(g, juce::String::fromUTF8("R\xc3\x89GLAGES AVANC\xc3\x89S"), { X0 + 40, stripY, 300, 34 }, 12.5f, ink, juce::Justification::centredLeft, .2f, true);
    g.setColour(juce::Colours::black.withAlpha(.28f)); g.fillRect(X0, stripY + 34, 1000.f, (float) H - stripY - 34);
    static const char* heads[4] { "FILTRES", "BANDE", "DUCKING", "ST\xc3\x89R\xc3\x89O" };
    float unit = (940 - 42) / 9.f, gx = X0 + 30;
    for (int gi = 0; gi < 4; ++gi) {
        auto r = juce::Rectangle<float>(gx, advY, unit * (float) weights[gi], advH);
        vui::plate(g, r, juce::String::fromUTF8(heads[gi]));
        gx += r.getWidth() + 14;
    }
    g.setColour(juce::Colours::black); g.drawRect(face, 1);
    for (auto pt : { juce::Point<float>(X0 + 12, 12), { X0 + 988, 12 }, { X0 + 12, (float) H - 12 }, { X0 + 988, (float) H - 12 } }) vui::screw(g, pt.x, pt.y);
}

void Panel::timerCallback() {
    bool sync = proc.value("sync") > .5f;
    divisionKnob->setVisible(sync); msKnob->setVisible(!sync);
    juce::String h = sync ? "= " + juce::String(juce::roundToInt(proc.delaySeconds.load() * 1000)) + juce::String::fromUTF8(" ms \xc3\xa0 ") + juce::String(juce::roundToInt(proc.bpm.load())) + " BPM"
                          : juce::String("libre, sans tempo");
    if (h != hint) { hint = h; repaint((int) X0 + 240, 418, 260, 26); }
    echo.tick();
    refreshNames();
}

void Panel::refreshNames() {
    int pr = (int) proc.value("program");
    juce::String cat = utf8(categories[(size_t) categoryOf(pr)]);
    juce::String name = proc.userPresetName.isNotEmpty() ? proc.userPresetName
                                                          : juce::String(positionInCategory(pr) + 1).paddedLeft('0', 2) + " " + utf8(presets[(size_t) pr].name);
    if (proc.userPresetName.isNotEmpty()) cat = "Mes presets";
    if (cat != categoryBox.label) { categoryBox.label = cat; categoryBox.repaint(); }
    if (name != presetBox.label) { presetBox.label = name; presetBox.repaint(); }
}

void Panel::stepCategory(int d) {
    int c = (categoryOf((int) proc.value("program")) + d + (int) categories.size()) % (int) categories.size();
    proc.loadPreset(presetIndex(c, 0)); refreshNames();
}
void Panel::stepPreset(int d) {
    int pr = (int) proc.value("program"), c = categoryOf(pr), n = presetsIn(c);
    proc.loadPreset(presetIndex(c, (positionInCategory(pr) + d + n) % n)); refreshNames();
}
void Panel::categoryMenu() {
    juce::PopupMenu m; m.setLookAndFeel(&look);
    int current = categoryOf((int) proc.value("program"));
    for (int c = 0; c < (int) categories.size(); ++c) m.addItem(c + 1, utf8(categories[(size_t) c]), true, c == current && proc.userPresetName.isEmpty());
    m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&categoryBox), [this](int r) { if (r > 0) { proc.loadPreset(presetIndex(r - 1, 0)); refreshNames(); } });
}
void Panel::presetMenu() {
    juce::PopupMenu m; m.setLookAndFeel(&look);
    int current = (int) proc.value("program");
    for (int c = 0; c < (int) categories.size(); ++c) {
        juce::PopupMenu sub;
        for (int n = 0; n < presetsIn(c); ++n) { int i = presetIndex(c, n); sub.addItem(i + 1, juce::String(n + 1).paddedLeft('0', 2) + " " + utf8(presets[(size_t) i].name), true, i == current && proc.userPresetName.isEmpty()); }
        m.addSubMenu(utf8(categories[(size_t) c]), sub, true, nullptr, c == categoryOf(current));
    }
    m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&presetBox), [this](int r) { if (r > 0) { proc.loadPreset(r - 1); refreshNames(); } });
}
void Panel::savePreset() {
    auto dir = DelayProcessor::userPresetFolder(); dir.createDirectory();
    chooser = std::make_unique<juce::FileChooser>("Enregistrer le preset", dir.getChildFile("Mon preset.swelldelay"), "*.swelldelay");
    chooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting, [this](const juce::FileChooser& fc) {
        auto f = fc.getResult();
        if (f == juce::File()) return;
        proc.saveUserPreset(f.withFileExtension("swelldelay")); refreshNames();
    });
}
void Panel::folderMenu() {
    auto dir = DelayProcessor::userPresetFolder();
    auto files = dir.findChildFiles(juce::File::findFiles, false, "*.swelldelay");
    files.sort();
    juce::PopupMenu m; m.setLookAndFeel(&look);
    m.addSectionHeader("Mes presets");
    if (files.isEmpty()) m.addItem(-1, juce::String::fromUTF8("(aucun preset enregistr\xc3\xa9)"), false);
    for (int i = 0; i < files.size(); ++i) m.addItem(i + 1, files[i].getFileNameWithoutExtension(), true, files[i].getFileNameWithoutExtension() == proc.userPresetName);
    m.addSeparator();
    m.addItem(1000, "Charger un fichier...");
    m.addItem(1001, "Ouvrir le dossier des presets");
    m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&folder), [this, files, dir](int r) {
        if (r > 0 && r <= files.size()) { proc.loadUserPreset(files[r - 1]); refreshNames(); }
        else if (r == 1000) {
            chooser = std::make_unique<juce::FileChooser>("Charger un preset", dir, "*.swelldelay");
            chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this](const juce::FileChooser& fc) {
                if (fc.getResult().existsAsFile()) { proc.loadUserPreset(fc.getResult()); refreshNames(); }
            });
        } else if (r == 1001) { dir.createDirectory(); dir.startAsProcess(); }
    });
}

//============================================================================== Editor
DelayEditor::DelayEditor(DelayProcessor& p) : AudioProcessorEditor(p), panel(p) {
    addAndMakeVisible(panel);
    panel.onScale = [this](float s) { scale = s; setSize(juce::roundToInt(Panel::W * s), juce::roundToInt(Panel::H * s)); };
    juce::PropertiesFile::Options o;
    o.applicationName = "Swell Delay"; o.filenameSuffix = ".settings"; o.folderName = "Swell Audio"; o.osxLibrarySubFolder = "Application Support";
    juce::PropertiesFile props(o);
    scale = juce::jlimit(.75f, 1.5f, (float) props.getDoubleValue("scale", 1.0));
    panel.setScaleShown(scale);
    setSize(juce::roundToInt(Panel::W * scale), juce::roundToInt(Panel::H * scale));
}
void DelayEditor::resized() {
    panel.setBounds(0, 0, Panel::W, Panel::H);
    panel.setTransform(juce::AffineTransform::scale(scale));
}
