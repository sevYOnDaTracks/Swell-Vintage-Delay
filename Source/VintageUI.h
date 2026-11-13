#pragma once
// Swell vintage look, shared by Vintage Reverb and Vintage Delay:
// chrome caps on knurled skirts with a scale of ticks, warm amber read-outs, cream engraving.
#include <juce_gui_basics/juce_gui_basics.h>

namespace vui {
inline const juce::Colour cream { 0xffeadfc6 }, creamDim { 0xffa89f8c }, amber { 0xffffb547 }, amberDim { 0xff5a3a12 },
                          lcdBg { 0xff120c06 }, faceTop { 0xff26252a }, faceBottom { 0xff151417 };

inline juce::Font font(float size, bool bold = false, float kerning = 0) {
#if JUCE_WINDOWS
    const char* face = "Segoe UI";
#else
    const char* face = "Helvetica Neue";
#endif
    return juce::Font(juce::FontOptions(face, size, bold ? juce::Font::bold : juce::Font::plain)).withExtraKerningFactor(kerning);
}
inline juce::Font mono(float size) {
#if JUCE_WINDOWS
    return juce::Font(juce::FontOptions("Consolas", size, juce::Font::plain));
#else
    return juce::Font(juce::FontOptions("Menlo", size, juce::Font::plain));
#endif
}

// Text that seems lit from behind: a soft halo, then the glyphs.
inline void glowText(juce::Graphics& g, const juce::String& s, juce::Rectangle<float> r, const juce::Font& f, juce::Colour c, juce::Justification j = juce::Justification::centred) {
    g.setFont(f);
    g.setColour(c.withAlpha(.10f));
    for (auto d : { juce::Point<float>(-1.2f, 0), { 1.2f, 0 }, { 0, -1.2f }, { 0, 1.2f } }) g.drawText(s, r.translated(d.x, d.y), j, false);
    g.setColour(c);
    g.drawText(s, r, j, false);
}

// Recessed amber window (preset names, values).
inline void lcd(juce::Graphics& g, juce::Rectangle<float> r, float radius = 4) {
    g.setColour(juce::Colours::white.withAlpha(.06f)); g.drawRoundedRectangle(r.translated(0, 1), radius, 1);
    g.setColour(juce::Colours::black); g.fillRoundedRectangle(r, radius);
    auto in = r.reduced(2);
    juce::ColourGradient bg(juce::Colour(0xff2a1a08), in.getCentreX(), in.getCentreY(), lcdBg, in.getX(), in.getY(), true);
    g.setGradientFill(bg); g.fillRoundedRectangle(in, radius - 1);
    juce::ColourGradient shade(juce::Colours::black.withAlpha(.7f), 0, in.getY(), juce::Colours::transparentBlack, 0, in.getY() + 7, false);
    g.setGradientFill(shade); g.fillRoundedRectangle(in, radius - 1);
    g.setColour(juce::Colours::white.withAlpha(.035f)); g.fillRect(in.withHeight(in.getHeight() * .45f).reduced(3, 2));
}

// Small raised button cap (preset arrows, tabs).
inline void raised(juce::Graphics& g, juce::Rectangle<float> r, bool lit = false, float radius = 3) {
    g.setColour(juce::Colours::black.withAlpha(.7f)); g.fillRoundedRectangle(r.translated(0, 1.5f), radius);
    juce::ColourGradient grad(juce::Colour(lit ? 0xff3e3a33 : 0xff38373b), 0, r.getY(), juce::Colour(lit ? 0xff1e1b17 : 0xff19181b), 0, r.getBottom(), false);
    g.setGradientFill(grad); g.fillRoundedRectangle(r, radius);
    g.setColour(juce::Colours::white.withAlpha(.10f)); g.drawHorizontalLine((int) r.getY() + 1, r.getX() + 2, r.getRight() - 2);
    g.setColour(juce::Colours::black); g.drawRoundedRectangle(r, radius, 1);
}

inline void arrow(juce::Graphics& g, juce::Rectangle<float> r, bool right, juce::Colour c) {
    auto cc = r.getCentre();
    juce::Path p;
    float s = std::min(r.getWidth(), r.getHeight()) * .16f;
    if (right) { p.startNewSubPath(cc.x - s * .6f, cc.y - s); p.lineTo(cc.x + s * .6f, cc.y); p.lineTo(cc.x - s * .6f, cc.y + s); }
    else       { p.startNewSubPath(cc.x + s * .6f, cc.y - s); p.lineTo(cc.x - s * .6f, cc.y); p.lineTo(cc.x + s * .6f, cc.y + s); }
    g.setColour(c); g.strokePath(p, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

// Preset display: raised arrow buttons on both ends, glowing amber name.
inline void presetDisplay(juce::Graphics& g, juce::Rectangle<float> r, const juce::String& name, juce::Colour light = amber) {
    g.setColour(juce::Colours::black.withAlpha(.55f)); g.fillRoundedRectangle(r, 6);
    g.setColour(juce::Colours::white.withAlpha(.06f)); g.drawRoundedRectangle(r.translated(0, 1).reduced(.5f), 6, 1);
    auto in = r.reduced(4);
    float bw = std::min(34.f, in.getHeight() + 2);
    auto l = in.removeFromLeft(bw), rr = in.removeFromRight(bw);
    raised(g, l); raised(g, rr);
    arrow(g, l, false, cream); arrow(g, rr, true, cream);
    auto win = in.reduced(4, 0);
    lcd(g, win, 3);
    glowText(g, name, win.reduced(6, 0), mono(std::min(17.f, win.getHeight() * .56f)), light);
}

// Chrome knob. small: no knurled skirt shadow depth, fewer ticks.
inline void knob(juce::Graphics& g, juce::Rectangle<float> r, float pos, float start, float end, bool small, bool bipolar, juce::Colour light) {
    using juce::MathConstants;
    const float size = std::min(r.getWidth(), r.getHeight());
    const auto c = r.getCentre();
    const float outer = size / 2 - 1;
    const float tickIn = outer - (small ? 5.f : 8.f);
    const float arcR = tickIn - 3.f;
    const float skirtR = arcR - (small ? 3.f : 4.f);
    const float capR = skirtR * (small ? .76f : .74f);
    const float angle = start + pos * (end - start);
    auto at = [&](float a, float rad) { return juce::Point<float>(c.x + rad * std::sin(a), c.y - rad * std::cos(a)); };

    // Scale.
    const int ticks = small ? 21 : 31;
    for (int i = 0; i < ticks; ++i) {
        float t = (float) i / (float) (ticks - 1), a = start + t * (end - start);
        bool major = i % 5 == 0;
        float len = major ? (outer - tickIn) : (outer - tickIn) * .55f;
        bool passed = bipolar ? (std::abs(t - .5f) <= std::abs(pos - .5f) && ((t - .5f) * (pos - .5f) >= 0)) : t <= pos + 1e-4f;
        g.setColour((passed ? cream : creamDim.withAlpha(.45f)));
        g.drawLine(juce::Line<float>(at(a, outer - len), at(a, outer)), major ? 1.6f : 1.f);
    }
    // Value arc, thin and warm.
    float from = bipolar ? (start + end) / 2 : start;
    if (std::abs(angle - from) > .01f) {
        juce::Path arc; arc.addCentredArc(c.x, c.y, arcR, arcR, 0, std::min(from, angle), std::max(from, angle), true);
        g.setColour(light.withAlpha(.18f)); g.strokePath(arc, juce::PathStrokeType(small ? 4.f : 5.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour(light.withAlpha(.9f)); g.strokePath(arc, juce::PathStrokeType(small ? 1.6f : 2.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    // Drop shadow.
    juce::ColourGradient sh(juce::Colours::black.withAlpha(.75f), c.x, c.y + skirtR * .2f, juce::Colours::transparentBlack, c.x, c.y + skirtR * 1.35f, true);
    g.setGradientFill(sh); g.fillEllipse(c.x - skirtR * 1.3f, c.y - skirtR * 1.1f, skirtR * 2.6f, skirtR * 2.6f);
    // Knurled skirt: dark ring with ridges that turn with the knob.
    g.setColour(juce::Colour(0xff0d0d0e)); g.fillEllipse(c.x - skirtR, c.y - skirtR, skirtR * 2, skirtR * 2);
    const int ridges = small ? 32 : 48;
    for (int i = 0; i < ridges; ++i) {
        float a = angle + (float) i * MathConstants<float>::twoPi / (float) ridges;
        float lightness = .5f + .5f * std::cos(a + MathConstants<float>::pi * .75f);
        g.setColour(juce::Colour(0xff2c2c2f).interpolatedWith(juce::Colour(0xff5a5a5e), lightness * .6f));
        g.drawLine(juce::Line<float>(at(a, capR + 1), at(a, skirtR - 1)), small ? 1.2f : 1.5f);
    }
    g.setColour(juce::Colours::black); g.drawEllipse(c.x - skirtR, c.y - skirtR, skirtR * 2, skirtR * 2, 1);
    // Chrome cap: conical brushing as a fan of slices.
    g.setColour(juce::Colours::black.withAlpha(.8f)); g.fillEllipse(c.x - capR - 1, c.y - capR, capR * 2 + 2, capR * 2 + 2);
    const int slices = 72;
    for (int i = 0; i < slices; ++i) {
        float a0 = (float) i * MathConstants<float>::twoPi / (float) slices, a1 = a0 + MathConstants<float>::twoPi / (float) slices + .01f;
        float m = (float) i / (float) slices * MathConstants<float>::twoPi;
        float v = .55f + .28f * std::cos(2 * (m - MathConstants<float>::pi * .25f)) + .08f * std::cos(6 * m + 1.f);
        juce::Path p; p.addPieSegment(c.x - capR, c.y - capR, capR * 2, capR * 2, a0, a1, 0);
        g.setColour(juce::Colour::greyLevel(juce::jlimit(.15f, .95f, v)));
        g.fillPath(p);
    }
    juce::ColourGradient glare(juce::Colours::white.withAlpha(.22f), c.x - capR * .4f, c.y - capR * .6f, juce::Colours::transparentWhite, c.x + capR * .3f, c.y + capR * .5f, true);
    g.setGradientFill(glare); g.fillEllipse(c.x - capR, c.y - capR, capR * 2, capR * 2);
    juce::ColourGradient rim(juce::Colours::white.withAlpha(.6f), c.x, c.y - capR, juce::Colours::black.withAlpha(.6f), c.x, c.y + capR, false);
    g.setGradientFill(rim); g.drawEllipse(c.x - capR, c.y - capR, capR * 2, capR * 2, 1.2f);
    // Pointer: cream bar crossing from the cap onto the skirt.
    g.setColour(juce::Colours::black.withAlpha(.5f));
    g.drawLine(juce::Line<float>(at(angle, capR * .42f), at(angle, skirtR - 1)).withShortenedStart(0), small ? 3.2f : 4.4f);
    g.setColour(cream);
    g.drawLine(juce::Line<float>(at(angle, capR * .42f), at(angle, skirtR - 2)), small ? 2.f : 3.f);
}

// Dark brushed face with a faint warm vignette.
inline juce::Image brushedFace(int w, int h, juce::Colour top = faceTop, juce::Colour mid = juce::Colour(0xff1d1c20), juce::Colour bottom = faceBottom) {
    juce::Image img(juce::Image::RGB, w, h, true);
    juce::Graphics g(img);
    juce::ColourGradient grad(top, 0, 0, bottom, 0, (float) h, false);
    grad.addColour(.55, mid);
    g.setGradientFill(grad); g.fillAll();
    juce::Random r(3);
    for (int y = 0; y < h; ++y) {
        g.setColour((r.nextBool() ? juce::Colours::white : juce::Colours::black).withAlpha(r.nextFloat() * .04f));
        g.drawHorizontalLine(y, 0, (float) w);
    }
    for (int i = 0; i < 900; ++i) {
        float y = r.nextFloat() * (float) h, x = r.nextFloat() * (float) w, len = 30 + r.nextFloat() * 180;
        g.setColour(juce::Colours::white.withAlpha(r.nextFloat() * .025f));
        g.drawLine(x, y, x + len, y + (r.nextFloat() - .5f) * .6f, .6f);
    }
    juce::ColourGradient vig(juce::Colours::transparentBlack, (float) w / 2, (float) h * .4f, juce::Colours::black.withAlpha(.35f), 0, (float) h, true);
    g.setGradientFill(vig); g.fillAll();
    return img;
}

// Engraved groove separating sections.
inline void groove(juce::Graphics& g, float x0, float x1, float y) {
    g.setColour(juce::Colours::black.withAlpha(.9f)); g.drawHorizontalLine((int) y, x0, x1);
    g.setColour(juce::Colours::white.withAlpha(.06f)); g.drawHorizontalLine((int) y + 1, x0, x1);
}

// Inset section plate with a heading and an underline.
inline void plate(juce::Graphics& g, juce::Rectangle<float> r, const juce::String& head) {
    g.setColour(juce::Colours::black.withAlpha(.32f)); g.fillRoundedRectangle(r, 6);
    g.setColour(juce::Colours::white.withAlpha(.05f)); g.drawRoundedRectangle(r.translated(0, 1).reduced(.5f), 6, 1);
    g.setColour(juce::Colours::black); g.drawRoundedRectangle(r.reduced(.5f), 6, 1.2f);
    g.setColour(cream); g.setFont(font(r.getWidth() < 190 ? 11.f : 12.f, false, r.getWidth() < 190 ? .06f : .14f));
    g.drawText(head, r.withHeight(26).reduced(4, 0), juce::Justification::centred, false);
    groove(g, r.getX() + 10, r.getRight() - 10, r.getY() + 26);
}

inline void screw(juce::Graphics& g, float cx, float cy) {
    g.setColour(juce::Colours::black.withAlpha(.7f)); g.fillEllipse(cx - 6.5f, cy - 5.5f, 13, 13);
    juce::ColourGradient grad(juce::Colour(0xff9a9a9a), cx - 3, cy - 3, juce::Colour(0xff2e2e2e), cx + 5, cy + 5, true);
    g.setGradientFill(grad); g.fillEllipse(cx - 6, cy - 6, 12, 12);
    g.setColour(juce::Colour(0xff1a1a1a));
    juce::Path cross; cross.addRectangle(-4.f, -.9f, 8.f, 1.8f); cross.addRectangle(-.9f, -4.f, 1.8f, 8.f);
    g.fillPath(cross, juce::AffineTransform::rotation(.4f).translated(cx, cy));
}

// Lit LED segment of a meter, green to red.
inline juce::Colour ledColour(float t) {
    if (t < .62f) return juce::Colour(0xff5fe04a);
    if (t < .8f)  return juce::Colour(0xffd8e23a);
    if (t < .9f)  return juce::Colour(0xffffa62b);
    return juce::Colour(0xffff4a2b);
}
inline void led(juce::Graphics& g, juce::Rectangle<float> r, float t, bool on) {
    auto col = ledColour(t);
    if (on) {
        g.setColour(col.withAlpha(.22f)); g.fillRoundedRectangle(r.expanded(2), 3);
        g.setColour(col); g.fillRoundedRectangle(r, 1.5f);
        g.setColour(juce::Colours::white.withAlpha(.35f)); g.fillRect(r.withHeight(r.getHeight() * .35f).reduced(1, 0));
    } else {
        g.setColour(col.withMultipliedBrightness(.16f).withMultipliedSaturation(.6f)); g.fillRoundedRectangle(r, 1.5f);
        g.setColour(juce::Colours::black.withAlpha(.7f)); g.drawRoundedRectangle(r, 1.5f, 1);
    }
}

// Logo + wordmark as on the hardware nameplate.
inline void brand(juce::Graphics& g, const juce::Image& logo, float x, float y) {
    g.setColour(cream);
    g.drawImageWithin(logo, (int) x, (int) y, 40, 36, juce::RectanglePlacement::centred, true);
    g.setFont(font(26, true)); g.drawText("swell", juce::Rectangle<float>(x + 46, y - 4, 130, 30), juce::Justification::centredLeft, false);
    g.setColour(creamDim); g.setFont(font(9, false, .9f)); g.drawText("AUDIO", juce::Rectangle<float>(x + 48, y + 24, 130, 12), juce::Justification::centredLeft, false);
}
}
