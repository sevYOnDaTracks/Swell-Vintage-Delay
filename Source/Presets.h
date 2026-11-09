#pragma once
#include <array>
#include <cstring>

// One factory preset. Loading it writes every control except Mix (the level you set is kept).
// Names are UTF-8.
struct Preset {
    const char* category; const char* name;
    bool sync; int division; float timeMs;   // division index into swell::divisions (used when sync)
    float feedback;                          // %
    float lowCut, highCut;                   // Hz
    float wow, flutter, saturation;          // %
    float duck, duckRelease;                 // %, ms
    float width, offset;                     // %, ms
    int mode;                                // 0 mono, 1 stereo, 2 ping-pong
    bool space; float spaceAmount;           // %
};

inline constexpr std::array<const char*, 6> categories = { "Basiques", "Ping-Pong", "Bande", "Voix", "Spatial", "Cr\xc3\xa9" "atifs" };

// Division indices: 2 1/32, 5 1/16, 6 1/8T, 8 1/8, 9 1/4T, 10 1/8D, 11 1/4, 13 1/4D, 14 1/2, 17 1/1.
inline constexpr int presetCount = 40;
inline constexpr std::array<Preset, presetCount> presets = {{
    // category      name                  sync div  ms     fb    lowC  highC  wow fl  sat  duck rel  width off  mode space amt
    { "Basiques",  "Quarter Note",        true, 11, 500,   40,   150,  8000,  0,  0,  0,   0,   250, 100,  0,   1, false, 60 },
    { "Basiques",  "Eighth Note",         true, 8,  250,   35,   150,  8000,  0,  0,  0,   0,   250, 100,  0,   1, false, 60 },
    { "Basiques",  "Dotted Eighth",       true, 10, 375,   45,   150,  7500,  0,  0,  0,   0,   250, 100,  0,   1, false, 60 },
    { "Basiques",  "Triplet Quarter",     true, 9,  333,   40,   150,  7500,  0,  0,  0,   0,   250, 100,  0,   1, false, 60 },
    { "Basiques",  "Sixteenth Groove",    true, 5,  125,   30,   200,  7000,  0,  0,  0,   0,   250, 100,  0,   1, false, 60 },
    { "Basiques",  "Half Note",           true, 14, 1000,  45,   150,  6000,  0,  0,  0,   0,   250, 100,  0,   1, false, 60 },
    { "Basiques",  "Slapback",            false, 11, 110,  5,    120,  9000,  0,  0,  10,  0,   250, 50,   0,   0, false, 60 },
    { "Basiques",  "Doubler",             false, 11, 28,   0,    150,  12000, 0,  10, 0,   0,   250, 150,  9,   1, false, 60 },

    { "Ping-Pong", "Wide Pong 1/8",       true, 8,  250,   50,   150,  7000,  0,  0,  0,   0,   250, 100,  0,   2, false, 60 },
    { "Ping-Pong", "Triplet Bounce",      true, 6,  167,   55,   150,  7000,  0,  0,  0,   0,   250, 100,  0,   2, false, 60 },
    { "Ping-Pong", "Dotted Pong",         true, 10, 375,   50,   150,  5000,  0,  0,  0,   0,   250, 100,  0,   2, false, 60 },
    { "Ping-Pong", "Quarter Pong",        true, 11, 500,   45,   150,  6500,  0,  0,  0,   0,   250, 100,  0,   2, false, 60 },
    { "Ping-Pong", "Fast Pong 1/16",      true, 5,  125,   45,   200,  6000,  0,  0,  0,   0,   250, 100,  0,   2, false, 60 },
    { "Ping-Pong", "Dark Pong",           true, 10, 375,   60,   250,  2500,  10, 0,  15,  0,   250, 100,  0,   2, false, 60 },

    { "Bande",     "Warm Tape",           true, 11, 500,   50,   200,  4500,  35, 25, 45,  0,   250, 100,  0,   1, false, 60 },
    { "Bande",     "Dub Echo",            true, 10, 375,   80,   300,  2800,  30, 15, 60,  0,   250, 100,  0,   2, false, 60 },
    { "Bande",     "Old Cassette",        true, 8,  250,   45,   200,  3500,  70, 50, 50,  0,   250, 100,  0,   1, false, 60 },
    { "Bande",     "Space Tape",          true, 10, 375,   65,   200,  3800,  25, 20, 40,  0,   250, 100,  0,   2, true,  50 },
    { "Bande",     "Roller Echo",         false, 11, 300,  55,   250,  3200,  20, 35, 50,  0,   250, 100,  0,   0, false, 60 },
    { "Bande",     "Tape Slap",           false, 11, 140,  15,   150,  4500,  15, 25, 55,  0,   250, 80,   0,   0, false, 60 },
    { "Bande",     "Worn Tape",           true, 11, 500,   55,   300,  2600,  85, 70, 65,  0,   250, 100,  0,   1, false, 60 },

    { "Voix",      "Vocal Throw",         true, 11, 500,   35,   300,  7000,  0,  0,  0,   70,  300, 100,  0,   2, false, 60 },
    { "Voix",      "Rap Doubler",         false, 11, 45,   0,    200,  10000, 0,  10, 0,   0,   250, 140,  12,  1, false, 60 },
    { "Voix",      "Hook Space",          true, 13, 750,   55,   250,  5500,  0,  0,  0,   50,  300, 100,  0,   2, true,  50 },
    { "Voix",      "Ad-lib Echo",         true, 8,  250,   40,   400,  6000,  0,  0,  10,  60,  200, 100,  0,   2, false, 60 },
    { "Voix",      "Chorus Glue",         true, 5,  125,   20,   250,  7000,  10, 10, 0,   40,  250, 120,  15,  1, false, 60 },
    { "Voix",      "Radio Voice",         true, 10, 375,   45,   800,  3000,  0,  0,  50,  50,  250, 80,   0,   0, false, 60 },
    { "Voix",      "Long Tail Vocal",     true, 14, 1000,  65,   250,  5000,  10, 0,  0,   75,  500, 100,  0,   2, true,  40 },

    { "Spatial",   "Space Orbit",         true, 10, 375,   60,   150,  5000,  0,  0,  0,   0,   250, 100,  0,   2, true,  90 },
    { "Spatial",   "Wide Halo",           true, 11, 500,   50,   200,  6000,  0,  0,  0,   0,   250, 160,  0,   1, true,  70 },
    { "Spatial",   "Floating Eighths",    true, 8,  250,   55,   200,  5500,  10, 0,  0,   0,   250, 100,  0,   0, true,  80 },
    { "Spatial",   "Cosmic Dust",         true, 6,  167,   70,   200,  4000,  20, 0,  10,  0,   250, 100,  0,   2, true,  100 },
    { "Spatial",   "Around You",          true, 9,  333,   50,   150,  6000,  0,  0,  0,   0,   250, 120,  20,  1, true,  85 },
    { "Spatial",   "Deep Field",          true, 17, 2000,  60,   200,  3500,  10, 0,  20,  0,   250, 100,  0,   2, true,  75 },

    { "Cr\xc3\xa9" "atifs", "Infinite Freeze",   true, 11, 500,   105,  150,  4000,  0,  0,  30,  0,   250, 100,  0,   2, false, 60 },
    { "Cr\xc3\xa9" "atifs", "Lo-Fi Wobble",      true, 10, 375,   60,   400,  2500,  100, 80, 70, 0,   250, 100,  0,   1, false, 60 },
    { "Cr\xc3\xa9" "atifs", "Telephone",         true, 8,  250,   50,   900,  2500,  0,  0,  60,  0,   250, 100,  0,   0, false, 60 },
    { "Cr\xc3\xa9" "atifs", "Glitch 1/32",       true, 2,  63,    75,   150,  9000,  0,  0,  0,   0,   250, 100,  0,   0, false, 60 },
    { "Cr\xc3\xa9" "atifs", "Self Oscillation",  true, 10, 375,   102,  300,  3000,  10, 0,  60,  0,   250, 100,  0,   0, false, 60 },
    { "Cr\xc3\xa9" "atifs", "Metallic Comb",     false, 11, 12,   85,   150,  12000, 0,  0,  0,   0,   250, 100,  0,   0, false, 60 },
}};

inline int presetIndex(int category, int n) {
    int seen = 0;
    for (int i = 0; i < presetCount; ++i)
        if (std::strcmp(presets[(size_t) i].category, categories[(size_t) category]) == 0 && seen++ == n) return i;
    return -1;
}
inline int categoryOf(int preset) {
    for (int c = 0; c < (int) categories.size(); ++c)
        if (std::strcmp(presets[(size_t) preset].category, categories[(size_t) c]) == 0) return c;
    return 0;
}
inline int presetsIn(int category) { int n = 0; while (presetIndex(category, n) >= 0) ++n; return n; }
inline int positionInCategory(int preset) {
    int c = categoryOf(preset), n = 0;
    for (int i = 0; i < preset; ++i) if (categoryOf(i) == c) ++n;
    return n;
}
