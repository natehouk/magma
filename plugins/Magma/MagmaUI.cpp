// Magma — custom vector UI (DPF/NanoVG). Original design. License: MIT.
#include "DistrhoUI.hpp"
#include "MagmaPlugin.hpp"
#include "UserPresets.hpp"
#include "ComponentPresets.hpp"
#include "ResizeHandle.hpp"
#include "Help.hpp"
#include "Fft.hpp"
#include <chrono>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#if defined(_WIN32)
extern "C" __declspec(dllimport) void* __stdcall ShellExecuteA(void*, const char*, const char*, const char*, const char*, int);
#endif

START_NAMESPACE_DISTRHO

static void openURL(const char* url) {
#if defined(_WIN32)
    ShellExecuteA(nullptr, "open", url, nullptr, nullptr, 1);
#elif defined(__APPLE__)
    const std::string c = std::string("open '") + url + "' &"; if (std::system(c.c_str())) {}
#else
    const std::string c = std::string("xdg-open '") + url + "' >/dev/null 2>&1 &"; if (std::system(c.c_str())) {}
#endif
}
static bool gTipsOn = true;
static double nowSec() { return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }

using namespace magma;
using DGL_NAMESPACE::ResizeHandle;

static constexpr float W = 1200.f, H = 800.f;

#define C_BG        Color(0x12, 0x0f, 0x0e)
#define C_PANEL     Color(0x1c, 0x18, 0x16)
#define C_PANEL_HI  Color(0x2a, 0x24, 0x21)
#define C_LINE      Color(0x36, 0x2f, 0x2b)
#define C_TEXT      Color(0xee, 0xe6, 0xe0)
#define C_DIM       Color(0x96, 0x8a, 0x82)
#define C_FAINT     Color(0x56, 0x4c, 0x46)
#define C_ACC       Color(0xff, 0x7a, 0x3c)       // molten
#define C_DISPLAY   Color(0x0c, 0x0a, 0x09)
#define C_FAV       Color(0xff, 0xc8, 0x5a)       // favourite star

static Color withAlpha(const Color& c, float a) { Color r(c); r.alpha = a; return r; }
static Color bandColor(int b) {
    static const Color c[kMaxBands] = { Color(0xff, 0x6a, 0x3d), Color(0xff, 0xb1, 0x3d), Color(0xe8, 0xd8, 0x5a), Color(0x7e, 0xd6, 0xa5), Color(0x6b, 0xb6, 0xff), Color(0xc0, 0x8b, 0xff) };
    return c[b % kMaxBands];
}
static Color srcColor(int s) {
    if (s >= kSrcXlfo1 && s <= kSrcXlfo4) return Color(0xff, 0x8a, 0x4c);
    if (s == kSrcEg1 || s == kSrcEg2) return Color(0xf2, 0xcf, 0x5c);
    if (s == kSrcEf1 || s == kSrcEf2) return Color(0x7e, 0xd6, 0xa5);
    if (s >= kSrcXy1X && s <= kSrcXy2Y) return Color(0x7c, 0xb8, 0xff);
    if (s >= kSrcVelocity) return Color(0xd0, 0x94, 0xff);
    return C_DIM;
}

// ---------------------------------------------------------------- layout
static constexpr float DX = 16, DY = 60, DW = 1168, DH = 232;           // band display
static constexpr float BX = 16, BY = 302, BW = 780, BH = 238;          // band panel
static constexpr float GX = 804, GY = 302, GW = 380, GH = 238;         // global panel
static constexpr float MX = 16, MY = 550, MW = 580, MH = 236;          // modulation sources
static constexpr float SX = 604, SY = 550, SW = 580, SH = 236;         // modulation slots
static constexpr float SEG_H = 22, CHIP_W = 66, CHIP_H = 18;
static constexpr float TAB_Y = MY + 18, TAB_W = 50, TAB_H = 20;
static constexpr int kNumTabs = 11;                                    // XLFO 1-4, EG 1-2, EF 1-2, XY 1-2, MIDI
static constexpr float SLOT_ROWH = 22, SLOT_COLW = 282, SLOT_Y0 = SY + 34; static constexpr int SLOT_ROWS = 8;
static constexpr float STEP_X = MX + 16, STEP_Y = MY + 104, STEP_W = 380, STEP_H = 112;
static constexpr float COMP_W = 110, COMP_H = 20, COMP_X = MX + MW - 14 - COMP_W, COMP_Y = MY + 78;
static constexpr float XY_X = MX + 20, XY_Y = MY + 44, XY_W = 300, XY_H = 180;

enum WType { Knob, Toggle, Segment, Stepper };
enum Group { GGlobal, GBand, GXlfo, GEg, GEf };
struct Ctl { WType type; int group, field; float x, y, r; const char* label; };   // Segment/Stepper: r = width

static std::vector<Ctl> buildControls() {
    std::vector<Ctl> c;
    // band panel
    c.push_back({ Knob, GBand, kBDrive, BX + 82, BY + 128, 42, "DRIVE" });
    c.push_back({ Knob, GBand, kBDynamics, BX + 212, BY + 96, 22, "DYNAMICS" });
    c.push_back({ Knob, GBand, kBFbAmt, BX + 292, BY + 96, 22, "FEEDBACK" });
    c.push_back({ Knob, GBand, kBFbFreq, BX + 372, BY + 96, 22, "FB FREQ" });
    c.push_back({ Knob, GBand, kBMix, BX + 452, BY + 96, 22, "MIX" });
    c.push_back({ Knob, GBand, kBLevel, BX + 212, BY + 180, 22, "LEVEL" });
    c.push_back({ Knob, GBand, kBPan, BX + 292, BY + 180, 22, "PAN" });
    c.push_back({ Knob, GBand, kBBass, BX + 562, BY + 96, 22, "BASS" });
    c.push_back({ Knob, GBand, kBMid, BX + 642, BY + 96, 22, "MID" });
    c.push_back({ Knob, GBand, kBTreble, BX + 722, BY + 96, 22, "TREBLE" });
    c.push_back({ Knob, GBand, kBPresence, BX + 642, BY + 180, 22, "PRESENCE" });
    c.push_back({ Toggle, GBand, kBMute, BX + 560, BY + 24, 0, "MUTE" });
    c.push_back({ Toggle, GBand, kBSolo, BX + 640, BY + 24, 0, "SOLO" });
    // global panel
    c.push_back({ Segment, GGlobal, kChannelMode, GX + 100, GY + 46, 168, "" });
    c.push_back({ Segment, GGlobal, kQuality, GX + 286, GY + 46, 168, "" });
    c.push_back({ Segment, GGlobal, kXSlope, GX + 100, GY + 106, 168, "" });
    c.push_back({ Segment, GGlobal, kXPhase, GX + 286, GY + 106, 168, "" });
    c.push_back({ Knob, GGlobal, kMix, GX + 74, GY + 180, 26, "MIX" });
    c.push_back({ Knob, GGlobal, kOutGain, GX + 186, GY + 180, 26, "OUTPUT" });
    c.push_back({ Toggle, GGlobal, kAutoMute, GX + 274, GY + 180, 0, "AUTO MUTE" });
    // XLFO editor
    c.push_back({ Segment, GXlfo, kLMode, MX + 82, MY + 62, 130, "" });
    c.push_back({ Segment, GXlfo, kLSync, MX + 296, MY + 62, 250, "" });
    c.push_back({ Segment, GXlfo, kLWave, MX + 206, MY + 88, 380, "" });
    c.push_back({ Knob, GXlfo, kLRate, MX + 452, MY + 140, 20, "RATE" });
    c.push_back({ Stepper, GXlfo, kLDiv, MX + 452, MY + 140, 74, "" });
    c.push_back({ Knob, GXlfo, kLBalance, MX + 530, MY + 140, 20, "BALANCE" });
    c.push_back({ Knob, GXlfo, kLGlide, MX + 530, MY + 140, 20, "GLIDE" });
    c.push_back({ Stepper, GXlfo, kLSteps, MX + 452, MY + 204, 74, "" });
    // EG editor
    c.push_back({ Segment, GEg, kETrigger, MX + 116, MY + 62, 170, "" });
    c.push_back({ Knob, GEg, kEAttack, MX + 60, MY + 150, 22, "ATTACK" });
    c.push_back({ Knob, GEg, kEDecay, MX + 140, MY + 150, 22, "DECAY" });
    c.push_back({ Knob, GEg, kESustain, MX + 220, MY + 150, 22, "SUSTAIN" });
    c.push_back({ Knob, GEg, kERelease, MX + 300, MY + 150, 22, "RELEASE" });
    c.push_back({ Knob, GEg, kEThreshold, MX + 400, MY + 150, 22, "THRESHOLD" });
    // EF editor
    c.push_back({ Segment, GEf, kFSource, MX + 126, MY + 62, 190, "" });
    c.push_back({ Knob, GEf, kFAttack, MX + 60, MY + 150, 22, "ATTACK" });
    c.push_back({ Knob, GEf, kFRelease, MX + 140, MY + 150, 22, "RELEASE" });
    c.push_back({ Knob, GEf, kFGain, MX + 220, MY + 150, 22, "GAIN" });
    return c;
}
static const std::vector<Ctl>& controls() { static std::vector<Ctl> c = buildControls(); return c; }

static const char* const* segLabels(int group, int field, int& n) {
    static const char* const chm[] = { "STEREO", "MID/SIDE" };
    static const char* const q[] = { "NORMAL", "GOOD", "SUPERB" };
    static const char* const sl[] = { "6 DB", "12 DB", "24 DB", "48 DB" };
    static const char* const ph[] = { "NATURAL", "LINEAR" };
    static const char* const lm[] = { "WAVE", "STEPS" };
    static const char* const sy[] = { "FREE", "SYNC", "DOTTED", "TRIPLET" };
    static const char* const wv[] = { "SIN", "TRI", "SAW+", "SAW-", "SQR", "RND", "S&H" };
    static const char* const tr[] = { "MIDI", "AUDIO" };
    static const char* const efs[] = { "INPUT", "SIDECHAIN" };
    n = 0;
    if (group == GGlobal) { if (field == kChannelMode) { n = 2; return chm; } if (field == kQuality) { n = 3; return q; } if (field == kXSlope) { n = 4; return sl; } if (field == kXPhase) { n = 2; return ph; } }
    if (group == GXlfo) { if (field == kLMode) { n = 2; return lm; } if (field == kLSync) { n = 4; return sy; } if (field == kLWave) { n = 7; return wv; } }
    if (group == GEg && field == kETrigger) { n = 2; return tr; }
    if (group == GEf && field == kFSource) { n = 2; return efs; }
    return nullptr;
}

// Segments whose order on screen differs from the parameter value. QUALITY spans two parameters
// (Quality and the 2.0 Superb switch); the slope segment shows 6 dB/oct first although its value is 3.
static int segCurrent(const Ctl& c, const float* val, uint32_t p) {
    if (c.group == GGlobal && c.field == kQuality) return val[kSuperb] > 0.5f ? 2 : (val[kQuality] > 0.5f ? 1 : 0);
    if (c.group == GGlobal && c.field == kXSlope) { const int v = (int)std::round(val[kXSlope]); return v == kSlope6 ? 0 : v + 1; }
    return (int)std::round(val[p]);
}
static constexpr float PS_X = 400.f, PS_Y = 18.f, PS_W = 220.f, PS_H = 28.f, PS_ARROW = 28.f;
static constexpr float A_X = 628.f, AB_W = 30.f, COPY_X = 692.f, COPY_W = 46.f;
static constexpr float UNDO_X = 750.f, REDO_X = 802.f, UR_W = 48.f, LEARN_X = 862.f, LEARN_W = 66.f, MIDI_X = 932.f, MIDI_W = 48.f, HELP_X = 986.f, HELP_W = 48.f;
static constexpr float PM_COLW = 129.f, PM_ROW = 22.f, PM_HEAD = 22.f, PM_PAD = 8.f;
static constexpr int ANA_N = 4096, ANA_PTS = 280;

class MagmaUI : public UI {
public:
    MagmaUI() : UI((uint)W, (uint)H), resizeHandle(this) {
        loadSharedResources();
        const Preset& d = presets()[0];
        for (uint32_t i = 0; i < kParamCount; ++i) { val[i] = d.v[i]; defv[i] = d.v[i]; }
        for (int& m : paramCC) m = -1;
        const double sf = getScaleFactor();
        if (sf != 1.0) setSize((uint)(W * sf), (uint)(H * sf));
        setGeometryConstraints((uint)(W * 0.6 * sf), (uint)(H * 0.6 * sf), true);
        if (isResizable()) resizeHandle.hide();
        userPresets = listUserPresets();
        std::copy(val, val + kParamCount, before);
        fft.init(ANA_N); for (int i = 0; i < ANA_PTS; ++i) specIn[i] = specOut[i] = -120;
        std::fill(liveOff, liveOff + kNumDests, 0.f); std::fill(liveSrc, liveSrc + kNumSources, 0.f);
    }

protected:
    float uiScale() const { return (float)getWidth() / W; }
    int nb() const { return (int)clampf(std::round(val[kNumBands]), 1, kMaxBands); }

    // ---- values -------------------------------------------------------------
    void setValue(uint32_t p, float v) {
        const ParamDef& d = paramDef(p); v = clampf(v, d.min, d.max);
        if (d.hints & kHintInteger) v = std::round(v);
        if (v == val[p]) return;
        val[p] = v; setParameterValue(p, v); repaint();
    }
    void commit(uint32_t p, float v) { editParameter(p, true); setValue(p, v); editParameter(p, false); }
    void parameterChanged(uint32_t i, float v) override {
        if (i >= kParamCount) return;
        if (i == kOutLearn) {
            if (v != lastLearnEcho && v > 0) {
                const int code = (int)v % 1000000 - 1, prm = code / 128, cc = code % 128;
                for (int& m : paramCC) if (m == cc) m = -1;
                if (prm >= 0 && prm < (int)kParamCount) paramCC[prm] = cc;
                learnWaiting = -1;
            }
            lastLearnEcho = v; repaint(); return;
        }
        if (val[i] != v) { val[i] = v; if (!gesture) before[i] = v; repaint(); }
        if (selBand >= nb()) selBand = nb() - 1;
    }
    void stateChanged(const char* key, const char* value) override {
        if (std::strcmp(key, "midimap") != 0) return;
        for (int& m : paramCC) m = -1;
        const char* q = value;
        while (q && *q) { int cc = -1, prm = -1; if (std::sscanf(q, "%d:%d", &cc, &prm) == 2 && prm >= 0 && prm < (int)kParamCount) paramCC[prm] = cc;
            const char* s = std::strchr(q, ';'); if (!s) break; q = s + 1; }
        repaint();
    }

    // ---- undo / redo / A-B --------------------------------------------------------
    typedef std::vector<float> Snap;
    Snap snapshot() const { Snap s(val, val + kParamCount); s.push_back((float)abSlot); return s; }
    bool differs(const float* a, const float* b) const { for (uint32_t i = 0; i < kParamCount; ++i) if (isSetting(i) && a[i] != b[i]) return true; return false; }
    void beginGesture() { gesture = true; std::copy(val, val + kParamCount, before); }
    void endGesture() {
        gesture = false;
        if (differs(before, val)) { Snap u(before, before + kParamCount); u.push_back((float)abSlot); undoStack.push_back(u); if (undoStack.size() > 200) undoStack.erase(undoStack.begin()); redoStack.clear(); }
        std::copy(val, val + kParamCount, before); repaint();
    }
    void applyAll(const float* s) { for (uint32_t i = 0; i < kParamCount; ++i) if (isSetting(i) && val[i] != s[i]) commit(i, s[i]); if (selBand >= nb()) selBand = nb() - 1; }
    void restoreSnap(const Snap& s) {
        if (s.size() > kParamCount && (int)s[kParamCount] != abSlot) { abStore[abSlot] = snapshot(); abSlot = (int)s[kParamCount]; }
        applyAll(s.data()); std::copy(val, val + kParamCount, before);
    }
    void undo() { if (undoStack.empty()) return; redoStack.push_back(snapshot()); restoreSnap(undoStack.back()); undoStack.pop_back(); flash("Undo"); }
    void redo() { if (redoStack.empty()) return; undoStack.push_back(snapshot()); restoreSnap(redoStack.back()); redoStack.pop_back(); flash("Redo"); }
    void switchAB(int to) {
        if (to == abSlot) return;
        abStore[abSlot] = snapshot(); if (abStore[to].empty()) abStore[to] = snapshot();
        beginGesture(); applyAll(abStore[to].data()); endGesture();
        abSlot = to; flash(to == 0 ? "Setting A" : "Setting B");
    }
    void copyAB() { abStore[1 - abSlot] = snapshot(); flash(abSlot == 0 ? "Copied A to B" : "Copied B to A"); }

    // ---- parameter of a control (depends on the selected band / source tab) ------
    uint32_t effParam(const Ctl& c) const {
        switch (c.group) {
        case GBand: return bp(selBand, c.field);
        case GXlfo: return lp_(std::min(tab, kNumXlfo - 1), c.field);
        case GEg: return ep(clampTab(tab - 4, kNumEg), c.field);
        case GEf: return fp(clampTab(tab - 6, kNumEf), c.field);
        default: return (uint32_t)c.field;
        }
    }
    static int clampTab(int t, int n) { return t < 0 ? 0 : (t >= n ? n - 1 : t); }
    bool visible(const Ctl& c) const {
        if (c.group == GXlfo) {
            if (tab > 3) return false;
            const int l = tab; const bool steps = val[lp_(l, kLMode)] > 0.5f, sync = val[lp_(l, kLSync)] > 0.5f;
            if (c.field == kLWave || c.field == kLBalance) return !steps;
            if (c.field == kLGlide || c.field == kLSteps) return steps;
            if (c.field == kLRate) return !sync;
            if (c.field == kLDiv) return sync;
            return true;
        }
        if (c.group == GEg) return tab == 4 || tab == 5;
        if (c.group == GEf) return tab == 6 || tab == 7;
        if (c.group == GBand && c.field == kBFbFreq) return true;
        return true;
    }
    bool dimmed(uint32_t p) const {
        if (p >= kBandFirst && p < kXlfoFirst && (p - kBandFirst) % kBandFields == kBFbFreq) return val[p - kBFbFreq + kBFbAmt] < 0.001f;
        if (p >= kEgFirst && p < kEfFirst && (p - kEgFirst) % kEgFields == kEThreshold) return val[p - kEThreshold + kETrigger] < 0.5f;
        return false;
    }

    // ---- formatting ---------------------------------------------------------
    void fmtValue(uint32_t p, float v, char* b, size_t n) const {
        int ne = 0;
        if (const char* const* l = enumLabels(p, ne)) { snprintf(b, n, "%s", l[std::max(0, std::min(ne - 1, (int)std::round(v)))]); return; }
        const ParamDef& d = paramDef(p);
        if (d.hints & kHintBoolean) { snprintf(b, n, "%s", v > 0.5f ? "On" : "Off"); return; }
        if (std::strcmp(d.unit, "Hz") == 0) { if (v >= 1000) snprintf(b, n, "%.2f kHz", v / 1000); else snprintf(b, n, v < 10 ? "%.2f Hz" : "%.0f Hz", v); return; }
        if (std::strcmp(d.unit, "dB") == 0) { snprintf(b, n, "%+.1f dB", v); return; }
        if (std::strcmp(d.unit, "s") == 0) { if (v < 1) snprintf(b, n, "%.0f ms", v * 1000); else snprintf(b, n, "%.2f s", v); return; }
        if (p == kNumBands || (p >= kXlfoFirst && p < kEgFirst && (p - kXlfoFirst) % kXlfoFields == kLSteps)) { snprintf(b, n, "%.0f", v); return; }
        if (isSlotAmount(p) || (d.hints & kHintBipolar)) { snprintf(b, n, "%+.0f%%", v * 100); return; }
        snprintf(b, n, "%.0f%%", v * 100);
    }
    void fmt(uint32_t p, char* b, size_t n) const { fmtValue(p, val[p], b, n); }
    bool parse(uint32_t p, const std::string& in, float& out) const {
        std::string s; for (char ch : in) if (!std::isspace((unsigned char)ch)) s += (char)std::tolower((unsigned char)ch);
        if (s.empty()) return false;
        char* end = nullptr; double x = std::strtod(s.c_str(), &end);
        if (end == s.c_str()) return false;
        const std::string suf(end); const ParamDef& d = paramDef(p);
        if (suf == "k" || suf == "khz") x *= 1000;
        if (std::strcmp(d.unit, "s") == 0) { out = (float)(suf == "s" ? x : x / 1000.0); return true; }
        if (std::strcmp(d.unit, "Hz") == 0 || std::strcmp(d.unit, "dB") == 0) { out = (float)x; return true; }
        out = (float)(x / 100.0); return true;
    }

    // ---- drawing ------------------------------------------------------------
    void onNanoDisplay() override {
        const float s = uiScale();
        save(); scale(s, s);
        beginPath(); rect(0, 0, W, H); fillColor(C_BG); fill();
        fontFace(NANOVG_DEJAVU_SANS_TTF);
        drawHeader();
        drawDisplay();
        drawBandPanel();
        drawGlobalPanel();
        drawSources();
        drawSlots();
        const std::vector<Ctl>& cs = controls();
        for (int i = 0; i < (int)cs.size(); ++i) if (visible(cs[i])) drawCtl(cs[i], i == active || i == hover);
        if (dragSrc > 0) drawChip(dragSrc, dragPX - CHIP_W / 2, dragPY - CHIP_H / 2, true, true);
        if (editing >= 0) drawEditBox();
        drawStatus();
        if (kpan.open) drawKnobPanel();
        if (popup.open) drawPopup();
        if (menuOpen) drawPresetMenu();
        if (saving) drawSaveBox();
        if (helpOpen) drawHelp();
        else if (tipShown && gTipsOn && dragSrc <= 0) drawTooltip();
        restore();
    }
    void panel(float x, float y, float w, float h) { beginPath(); roundedRect(x, y, w, h, 6); fillColor(C_PANEL); fill(); strokeColor(C_LINE); strokeWidth(1); stroke(); }
    void caption(float x, float y, const char* t, int align = ALIGN_LEFT) { fontSize(7.5f); textLetterSpacing(1.8f); textAlign(align | ALIGN_MIDDLE); fillColor(C_FAINT); text(x, y, t, nullptr); }
    void drawHeaderButton(float x, float w, const char* label, bool on, bool hot, bool disabled) {
        beginPath(); roundedRect(x, PS_Y, w, PS_H, 5); fillColor(on ? withAlpha(C_ACC, 0.2f) : C_PANEL); fill();
        strokeColor(on ? C_ACC : (hot && !disabled ? C_DIM : C_LINE)); strokeWidth(1); stroke();
        fontSize(9); textLetterSpacing(1.6f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(disabled ? C_FAINT : (on ? C_TEXT : C_DIM));
        text(x + w / 2, PS_Y + PS_H / 2 + 0.5f, label, nullptr);
    }
    void drawHeader() {
        // logo: a cooling crust with a glowing crack
        beginPath(); roundedRect(16, 17, 30, 30, 4); fillColor(Color(0x2e, 0x26, 0x22)); fill();
        beginPath(); moveTo(19, 30); lineTo(26, 27); lineTo(29, 33); lineTo(35, 24); lineTo(40, 29); lineTo(44, 26);
        strokeColor(C_ACC); strokeWidth(2.2f); lineJoin(ROUND); stroke();
        beginPath(); moveTo(26, 27); lineTo(24, 40); moveTo(35, 24); lineTo(37, 41); strokeColor(withAlpha(Color(0xff, 0xb1, 0x3d), 0.8f)); strokeWidth(1.2f); stroke();
        fontSize(22); textLetterSpacing(9); fillColor(C_TEXT); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); text(56, 33, "MAGMA", nullptr);
        fontSize(9); textLetterSpacing(2.2f); fillColor(C_DIM); text(196, 28, "MULTIBAND DISTORTION", nullptr);
        fontSize(7); textLetterSpacing(1.1f); fillColor(C_FAINT); text(196, 40, "EPSILON AUDIO 2.0", nullptr);
        drawPresetBar();
        for (int k = 0; k < 2; ++k) {
            const float x = A_X + k * AB_W; const bool on = abSlot == k, hot = hoverHeader == 6 + k;
            beginPath(); roundedRect(x, PS_Y, AB_W - 2, PS_H, 5); fillColor(on ? withAlpha(C_ACC, 0.25f) : C_PANEL); fill();
            strokeColor(on ? C_ACC : (hot ? C_DIM : C_LINE)); strokeWidth(1); stroke();
            fontSize(10); textLetterSpacing(0); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(on ? C_TEXT : C_DIM); text(x + (AB_W - 2) / 2, PS_Y + PS_H / 2 + 0.5f, k ? "B" : "A", nullptr);
        }
        drawHeaderButton(COPY_X, COPY_W, "COPY", false, hoverHeader == 8, false);
        drawHeaderButton(UNDO_X, UR_W, "UNDO", false, hoverHeader == 3, undoStack.empty());
        drawHeaderButton(REDO_X, UR_W, "REDO", false, hoverHeader == 4, redoStack.empty());
        drawHeaderButton(LEARN_X, LEARN_W, learnMode ? "LEARNING" : "LEARN", learnMode, hoverHeader == 1, false);
        drawHeaderButton(MIDI_X, MIDI_W, "MIDI", val[kMidiOn] > 0.5f, hoverHeader == 2, false);
        drawHeaderButton(HELP_X, HELP_W, "HELP", helpOpen, hoverHeader == 5, false);
        char b[24]; snprintf(b, sizeof b, "%.1f ms", latencyMs);
        fontSize(7); textLetterSpacing(1.4f); textAlign(ALIGN_RIGHT | ALIGN_MIDDLE); fillColor(C_FAINT); text(W - 16, 27, "LATENCY", nullptr);
        fontSize(9.5f); textLetterSpacing(0.4f); fillColor(C_DIM); text(W - 16, 40, b, nullptr);
    }

    // ---- band display ------------------------------------------------------------
    static float xOfHz(float hz) { return DX + 10 + (DW - 20) * std::log(clampf(hz, 20.f, 20000.f) / 20.f) / std::log(1000.f); }
    static float hzOfX(float x) { return 20.f * std::pow(1000.f, (x - DX - 10) / (DW - 20)); }
    static float yOfSpec(float db) { return DY + 14 + (DH - 34) * clampf(-db / 84.f, 0.f, 1.f); }
    float xoverHz(int i) const { return val[kX1 + i]; }
    float bandLo(int b) const { return b == 0 ? 20.f : xoverHz(b - 1); }
    float bandHi(int b) const { return b == nb() - 1 ? 20000.f : xoverHz(b); }
    float driveY(int b) const { return DY + DH - 26 - val[bp(b, kBDrive)] * (DH - 74); }
    void drawDisplay() {
        beginPath(); roundedRect(DX, DY, DW, DH, 6); fillColor(C_DISPLAY); fill(); strokeColor(C_LINE); strokeWidth(1); stroke();
        const float grid[] = { 50, 100, 200, 500, 1000, 2000, 5000, 10000 }; const char* gl[] = { "50", "100", "200", "500", "1k", "2k", "5k", "10k" };
        beginPath(); for (float g : grid) { const float x = xOfHz(g); moveTo(x, DY + 8); lineTo(x, DY + DH - 18); }
        strokeColor(Color(255, 255, 255, 8 / 255.f)); strokeWidth(1); stroke();
        fontSize(8); textLetterSpacing(1); textAlign(ALIGN_CENTER | ALIGN_BOTTOM); fillColor(C_FAINT);
        for (int i = 0; i < 8; ++i) text(xOfHz(grid[i]), DY + DH - 4, gl[i], nullptr);
        bool anySolo = false; for (int b = 0; b < nb(); ++b) anySolo |= val[bp(b, kBSolo)] > 0.5f;
        // band regions
        for (int b = 0; b < nb(); ++b) {
            const float x0 = xOfHz(bandLo(b)), x1 = xOfHz(bandHi(b)), col_a = b == selBand ? 0.13f : (b == hoverBand ? 0.08f : 0.045f);
            const bool off = val[bp(b, kBMute)] > 0.5f || (anySolo && val[bp(b, kBSolo)] < 0.5f);
            beginPath(); rect(x0, DY + 1, x1 - x0, DH - 19); fillColor(withAlpha(bandColor(b), off ? 0.02f : col_a)); fill();
            if (b == selBand) { beginPath(); rect(x0, DY + 1, x1 - x0, 2.5f); fillColor(bandColor(b)); fill(); }
        }
        // spectrum (input faint, output bright)
        scissor(DX + 1, DY + 1, DW - 2, DH - 19);
        auto specPath = [&](const float* sp) { beginPath(); moveTo(DX + 10, DY + DH - 18);
            for (int i = 0; i < ANA_PTS; ++i) lineTo(DX + 10 + (DW - 20) * i / (ANA_PTS - 1.f), yOfSpec(sp[i]));
            lineTo(DX + DW - 10, DY + DH - 18); closePath(); };
        specPath(specIn); fillColor(Color(0x60, 0x58, 0x52, 0.25f)); fill();
        specPath(specOut); fillPaint(linearGradient(0, DY, 0, DY + DH, withAlpha(C_ACC, 0.32f), withAlpha(C_ACC, 0.04f))); fill();
        beginPath(); for (int i = 0; i < ANA_PTS; ++i) { const float x = DX + 10 + (DW - 20) * i / (ANA_PTS - 1.f), y = yOfSpec(specOut[i]); if (!i) moveTo(x, y); else lineTo(x, y); }
        strokeColor(withAlpha(C_ACC, 0.7f)); strokeWidth(1.2f); stroke();
        resetScissor();
        // drive lines and labels
        for (int b = 0; b < nb(); ++b) {
            const float x0 = xOfHz(bandLo(b)), x1 = xOfHz(bandHi(b)), w = x1 - x0, y = driveY(b);
            const bool off = val[bp(b, kBMute)] > 0.5f || (anySolo && val[bp(b, kBSolo)] < 0.5f);
            const Color col = bandColor(b);
            const float pad = std::min(10.f, w * 0.15f);
            beginPath(); moveTo(x0 + pad, y); lineTo(x1 - pad, y); strokeColor(withAlpha(col, off ? 0.25f : (b == dragDriveBand || b == hoverBand ? 1.f : 0.75f))); strokeWidth(b == selBand ? 3.f : 2.f); stroke();
            // live drive (modulation)
            const int dd = kDstBandFirst + b * kNumBandDests;
            if (std::fabs(liveOff[dd]) > 0.002f) { const float yl = DY + DH - 26 - clampf(val[bp(b, kBDrive)] + liveOff[dd], 0.f, 1.f) * (DH - 74);
                beginPath(); circle((x0 + x1) / 2, yl, 3.5f); fillColor(col); fill(); }
            // level glow
            const float lv = clampf((gainToDb(liveLevel[b]) + 48.f) / 48.f, 0.f, 1.f);
            if (lv > 0.01f) { beginPath(); rect(x0 + 2, DY + DH - 22, (w - 4) * lv, 2); fillColor(withAlpha(col, 0.7f)); fill(); }
            if (w > 46) {
                char t[40]; snprintf(t, sizeof t, "%d", b + 1);
                fontSize(10); textLetterSpacing(0.5f); textAlign(ALIGN_LEFT | ALIGN_TOP); fillColor(off ? C_FAINT : col); text(x0 + 7, DY + 9, t, nullptr);
                if (w > 110) { std::string nm = styleName((int)std::round(val[bp(b, kBStyle)])); for (char& ch : nm) ch = (char)std::toupper((unsigned char)ch);
                    fontSize(8); textLetterSpacing(1.4f); fillColor(off ? C_FAINT : C_DIM); text(x0 + 20, DY + 10.5f, nm.c_str(), nullptr); }
                if (val[bp(b, kBMute)] > 0.5f) { fontSize(7.5f); fillColor(C_FAINT); textAlign(ALIGN_RIGHT | ALIGN_TOP); text(x1 - 6, DY + 10.5f, "MUTE", nullptr); }
                else if (val[bp(b, kBSolo)] > 0.5f) { fontSize(7.5f); fillColor(col); textAlign(ALIGN_RIGHT | ALIGN_TOP); text(x1 - 6, DY + 10.5f, "SOLO", nullptr); }
            }
        }
        // crossovers
        for (int i = 0; i < nb() - 1; ++i) {
            const float x = xOfHz(xoverHz(i)); const bool hot = hoverDiv == i || dragDiv == i;
            beginPath(); moveTo(x, DY + 4); lineTo(x, DY + DH - 18); strokeColor(withAlpha(C_TEXT, hot ? 0.8f : 0.3f)); strokeWidth(hot ? 2.f : 1.f); stroke();
            const float off = liveOff[kDstXFirst + i];
            if (std::fabs(off) > 0.002f) { const float xl = xOfHz(valueOf(kX1 + i, normOf(kX1 + i, xoverHz(i)) + off)); beginPath(); moveTo(xl, DY + 30); lineTo(xl, DY + DH - 30); strokeColor(withAlpha(C_ACC, 0.5f)); strokeWidth(1); stroke(); }
            beginPath(); roundedRect(x - 5, DY + DH / 2 - 12, 10, 24, 3); fillColor(hot ? C_TEXT : C_PANEL_HI); fill(); strokeColor(withAlpha(C_TEXT, 0.5f)); stroke();
            if (hot) { char b[24]; fmt(kX1 + i, b, sizeof b); fontSize(9); textLetterSpacing(0.4f); textAlign(ALIGN_CENTER | ALIGN_BOTTOM); fillColor(C_TEXT); text(x, DY + DH / 2 - 16, b, nullptr); }
        }
        if (nb() < kMaxBands && hoverBand >= 0 && dragDiv < 0 && dragDriveBand < 0) {
            fontSize(8); textLetterSpacing(1); textAlign(ALIGN_RIGHT | ALIGN_BOTTOM); fillColor(C_FAINT); text(DX + DW - 10, DY + DH - 20, "DOUBLE-CLICK TO SPLIT A BAND", nullptr);
        }
    }

    // ---- band panel ------------------------------------------------------------
    void drawBandPanel() {
        panel(BX, BY, BW, BH);
        const int b = selBand; const Color col = bandColor(b);
        beginPath(); circle(BX + 22, BY + 24, 6); fillColor(col); fill();
        char t[24]; snprintf(t, sizeof t, "BAND %d", b + 1);
        fontSize(11); textLetterSpacing(2.2f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(C_TEXT); text(BX + 36, BY + 24.5f, t, nullptr);
        char r[48]; snprintf(r, sizeof r, "%.0f HZ - %.0f HZ", bandLo(b), bandHi(b));
        if (bandHi(b) >= 1000) { if (bandLo(b) >= 1000) snprintf(r, sizeof r, "%.1f - %.1f KHZ", bandLo(b) / 1000, bandHi(b) / 1000); else snprintf(r, sizeof r, "%.0f HZ - %.1f KHZ", bandLo(b), bandHi(b) / 1000); }
        fontSize(7.5f); textLetterSpacing(1.2f); fillColor(C_FAINT); text(BX + 36, BY + 38, r, nullptr);
        // style button
        const float sx = STYLE_X, sy = BY + 12, sw = STYLE_W, sh = 26; const bool hot = hoverStyle;
        beginPath(); roundedRect(sx, sy, sw, sh, 5); fillColor(hot ? C_PANEL_HI : Color(0x24, 0x1f, 0x1c)); fill(); strokeColor(withAlpha(col, 0.8f)); strokeWidth(1); stroke();
        const int st = (int)std::round(val[bp(b, kBStyle)]);
        std::string cat = categoryName(styleCategory(st)); for (char& ch : cat) ch = (char)std::toupper((unsigned char)ch);
        fontSize(7.5f); textLetterSpacing(1.4f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(C_FAINT); text(sx + 10, sy + sh / 2 + 0.5f, cat.c_str(), nullptr);
        fontSize(11.5f); textLetterSpacing(0.6f); fillColor(C_TEXT); textAlign(ALIGN_RIGHT | ALIGN_MIDDLE); text(sx + sw - 24, sy + sh / 2 + 0.5f, styleName(st), nullptr);
        beginPath(); moveTo(sx + sw - 15, sy + 11); lineTo(sx + sw - 8, sy + 11); lineTo(sx + sw - 11.5f, sy + 16); closePath(); fillColor(C_DIM); fill();
        // remove band
        if (nb() > 1) { const bool h = hoverRemove; beginPath(); roundedRect(REM_X, BY + 12, 64, 26, 5); fillColor(h ? C_PANEL_HI : C_PANEL); fill(); strokeColor(h ? C_DIM : C_LINE); stroke();
            fontSize(8.5f); textLetterSpacing(1.4f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(C_DIM); text(REM_X + 32, BY + 25.5f, "REMOVE", nullptr); }
        caption(BX + 562 + 80 - 80, BY + 58, "", ALIGN_CENTER);
        fontSize(7.5f); textLetterSpacing(1.8f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(C_FAINT); text(BX + 642, BY + 58, "TONE  (AFTER DISTORTION)", nullptr);
        beginPath(); moveTo(BX + 512, BY + 66); lineTo(BX + 512, BY + BH - 14); strokeColor(C_LINE); stroke();
    }
    static constexpr float STYLE_X = BX + 196, STYLE_W = 250, REM_X = BX + 696;

    void drawGlobalPanel() {
        panel(GX, GY, GW, GH);
        caption(GX + 100, GY + 24, "CHANNELS", ALIGN_CENTER); caption(GX + 286, GY + 24, "QUALITY", ALIGN_CENTER);
        caption(GX + 100, GY + 84, "CROSSOVER SLOPE", ALIGN_CENTER); caption(GX + 286, GY + 84, "CROSSOVER PHASE", ALIGN_CENTER);
    }

    // ---- modulation sources ----------------------------------------------------
    static const char* tabName(int t) { static const char* const n[kNumTabs] = { "LFO 1", "LFO 2", "LFO 3", "LFO 4", "EG 1", "EG 2", "EF 1", "EF 2", "XY 1", "XY 2", "MIDI" }; return n[t]; }
    static int tabSource(int t) { static const int s[kNumTabs] = { kSrcXlfo1, kSrcXlfo2, kSrcXlfo3, kSrcXlfo4, kSrcEg1, kSrcEg2, kSrcEf1, kSrcEf2, kSrcXy1X, kSrcXy2X, kSrcVelocity }; return s[t]; }
    struct Chip { int src; float x, y; };
    std::vector<Chip> chips() const {
        std::vector<Chip> c;
        if (tab <= 7) c.push_back({ tabSource(tab), MX + MW - CHIP_W - 14, MY + 52 });
        else if (tab <= 9) { const int x = tab - 8; c.push_back({ kSrcXy1X + 2 * x, MX + MW - CHIP_W - 14, MY + 52 }); c.push_back({ kSrcXy1Y + 2 * x, MX + MW - CHIP_W - 14, MY + 78 }); }
        else for (int i = 0; i < 5; ++i) c.push_back({ kSrcVelocity + i, MX + 24, MY + 52 + i * 34 });
        return c;
    }
    void drawSources() {
        panel(MX, MY, MW, MH);
        for (int t = 0; t < kNumTabs; ++t) {
            const float x = MX + 10 + t * (TAB_W + 1.6f); const bool on = t == tab, hot = t == hoverTab;
            const int s = tabSource(t);
            beginPath(); roundedRect(x, TAB_Y - TAB_H / 2, TAB_W, TAB_H, 4); fillColor(on ? withAlpha(srcColor(s), 0.25f) : (hot ? C_PANEL_HI : Color(0x22, 0x1d, 0x1a))); fill();
            if (on) { strokeColor(srcColor(s)); strokeWidth(1); stroke(); }
            fontSize(8); textLetterSpacing(1); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(on ? C_TEXT : C_DIM); text(x + TAB_W / 2, TAB_Y + 0.5f, tabName(t), nullptr);
            if (sourceUsed(s) || (t >= 8 && t <= 9 && sourceUsed(s + 1)) || (t == 10 && anyMidiUsed())) { beginPath(); circle(x + TAB_W - 5, TAB_Y - TAB_H / 2 + 4, 2); fillColor(srcColor(s)); fill(); }
        }
        if (tab <= 3) drawXlfo(tab);
        else if (tab <= 5) drawEg(tab - 4);
        else if (tab <= 7) drawEf(tab - 6);
        else if (tab <= 9) drawXy(tab - 8);
        else drawMidi();
        for (const Chip& c : chips()) drawChip(c.src, c.x, c.y, hoverChip == c.src, false);
        if (tab <= 7) {
            const bool hot = hoverComp || (popup.open && popup.kind == 3);
            beginPath(); roundedRect(COMP_X, COMP_Y, COMP_W, COMP_H, 4); fillColor(hot ? C_PANEL_HI : Color(0x22, 0x1d, 0x1a)); fill(); strokeColor(hot ? C_DIM : C_LINE); strokeWidth(1); stroke();
            fontSize(8); textLetterSpacing(1.2f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(hot ? C_TEXT : C_DIM);
            text(COMP_X + 10, COMP_Y + COMP_H / 2 + 0.5f, tab <= 3 ? "LFO PRESETS" : (tab <= 5 ? "EG PRESETS" : "EF PRESETS"), nullptr);
            beginPath(); const float ax = COMP_X + COMP_W - 12, ay = COMP_Y + COMP_H / 2; moveTo(ax - 3.5f, ay - 2); lineTo(ax + 3.5f, ay - 2); lineTo(ax, ay + 2.5f); closePath(); fillColor(C_DIM); fill();
        }
    }
    // ---- per-source presets ----------------------------------------------------------
    int compKind() const { return tab <= 3 ? kCompXlfo : (tab <= 5 ? kCompEg : kCompEf); }
    int compIndex() const { return tab <= 3 ? tab : (tab <= 5 ? tab - 4 : tab - 6); }
    bool compHit(float x, float y) const { return tab <= 7 && inRect(x, y, COMP_X, COMP_Y, COMP_W, COMP_H); }
    struct CompItem { int type, idx; std::string name; };       // type 0 factory, 1 user file, 2 save
    std::vector<CompItem> compItems; int compPendingDel = -1; bool savingComp = false; int savingCompKind = 0, savingCompIdx = 0; bool hoverComp = false;
    void openCompMenu() {
        compItems.clear(); const int k = compKind();
        for (int i : compFactoryFor(k)) compItems.push_back({ 0, i, compFactory()[i].name });
        const std::vector<std::string> u = listCompPresets(k);
        for (int i = 0; i < (int)u.size(); ++i) compItems.push_back({ 1, i, u[i] });
        compItems.push_back({ 2, 0, "Save current..." });
        compPendingDel = -1; popup.open = true; popup.kind = 3; popup.slot = -1; popup.x = COMP_X + COMP_W - 186; popup.y = COMP_Y + COMP_H + 4;
        const float ph = compItems.size() * 20.f + 12; if (popup.y + ph > H - 8) popup.y = COMP_Y - ph - 4;
        popup.hover = -1; repaint();
    }
    void applyComp(const float* fields) {
        const int k = compKind(), ix = compIndex();
        beginGesture(); for (int f = 0; f < compFieldCount(k); ++f) commit(compParam(k, ix, f), fields[f]); endGesture();
    }
    void pickComp(int i) {
        const CompItem it = compItems[i]; const int k = compKind();
        float fv[kXlfoFields];
        if (it.type == 0) { compFactoryValues(it.idx, fv); applyComp(fv); flash(std::string(compKindName(k)) + " preset: " + it.name); }
        else if (it.type == 1) {
            if (compPendingDel == i) { deleteCompPreset(k, it.name); openCompMenu(); return; }
            if (loadCompPreset(k, it.name, fv)) { applyComp(fv); flash(std::string(compKindName(k)) + " preset: " + it.name); }
        } else { savingComp = true; savingCompKind = k; savingCompIdx = compIndex(); saving = true; saveError = false; saveName.clear(); }
        popup.open = false; repaint();
    }
    bool sourceUsed(int s) const { for (int k = 0; k < kNumSlots; ++k) if ((int)std::round(val[sp(k, kSlotSrc)]) == s && (int)std::round(val[sp(k, kSlotDst)]) > 0) return true; return false; }
    bool anyMidiUsed() const { for (int s = kSrcVelocity; s < kNumSources; ++s) if (sourceUsed(s)) return true; return false; }
    void drawChip(int src, float x, float y, bool hot, bool dragging) {
        const Color col = srcColor(src);
        beginPath(); roundedRect(x, y, CHIP_W, CHIP_H, CHIP_H / 2); fillColor(withAlpha(col, dragging ? 0.9f : (hot ? 0.35f : 0.16f))); fill(); strokeColor(col); strokeWidth(1); stroke();
        // live value bar inside the chip
        const float v = liveSrc[src]; const bool bip = (src >= kSrcXlfo1 && src <= kSrcXlfo4) || src == kSrcKey || src == kSrcBend;
        beginPath(); circle(x + 9, y + CHIP_H / 2, 3); fillColor(dragging ? C_BG : col); fill();
        if (!dragging) { const float bx = x + 17, bw = CHIP_W - 24, by = y + CHIP_H - 4;
            beginPath(); rect(bx, by, bw, 1.5f); fillColor(withAlpha(col, 0.25f)); fill();
            const float a = bip ? bx + bw / 2 : bx, b2 = bip ? bx + bw / 2 + v * bw / 2 : bx + clampf(v, 0.f, 1.f) * bw;
            beginPath(); rect(std::min(a, b2), by, std::fabs(b2 - a) + 1, 1.5f); fillColor(col); fill(); }
        fontSize(8); textLetterSpacing(1.1f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(dragging ? C_BG : C_TEXT); text(x + CHIP_W / 2 + 5, y + CHIP_H / 2 - 0.5f, srcShort(src), nullptr);
    }
    void drawXlfo(int l) {
        caption(MX + 82, MY + 44, "MODE", ALIGN_CENTER); caption(MX + 296, MY + 44, "TEMPO SYNC", ALIGN_CENTER);
        const bool steps = val[lp_(l, kLMode)] > 0.5f;
        beginPath(); roundedRect(STEP_X, STEP_Y, STEP_W, STEP_H, 4); fillColor(C_DISPLAY); fill(); strokeColor(C_LINE); stroke();
        const float mid = STEP_Y + STEP_H / 2; const Color col = srcColor(kSrcXlfo1);
        beginPath(); moveTo(STEP_X, mid); lineTo(STEP_X + STEP_W, mid); strokeColor(Color(255, 255, 255, 10 / 255.f)); stroke();
        if (steps) {
            const int n = (int)clampf(std::round(val[lp_(l, kLSteps)]), 1, kNumSteps); const float sw = STEP_W / kNumSteps;
            for (int s = 0; s < kNumSteps; ++s) {
                const float v = val[lp_(l, kLStep1 + s)], x = STEP_X + s * sw, y = mid - v * (STEP_H / 2 - 6);
                const bool on = s < n;
                beginPath(); rect(x + 2, std::min(y, mid), sw - 4, std::max(1.5f, std::fabs(y - mid))); fillColor(withAlpha(col, on ? 0.75f : 0.15f)); fill();
            }
            const float ph = (float)livePhase[l]; beginPath(); const float px = STEP_X + ph * n * sw; moveTo(px, STEP_Y + 2); lineTo(px, STEP_Y + STEP_H - 2); strokeColor(withAlpha(C_TEXT, 0.6f)); stroke();
        } else {
            beginPath();
            const int w = (int)std::round(val[lp_(l, kLWave)]); const float bal = val[lp_(l, kLBalance)];
            Xlfo tmp; tmp.rndA = -0.6f; tmp.rndB = 0.5f; tmp.held = 0.4f;
            for (int i = 0; i <= 120; ++i) { tmp.phase = i / 120.0; const float v = (w == 5 || w == 6) ? std::sin(i * 0.31f) * 0.6f * ((i / 20) % 2 ? 1 : -0.7f) : tmp.wave(w, bal);
                const float x = STEP_X + 4 + (STEP_W - 8) * i / 120.f, y = mid - v * (STEP_H / 2 - 8); if (!i) moveTo(x, y); else lineTo(x, y); }
            strokeColor(col); strokeWidth(1.6f); stroke();
            beginPath(); circle(STEP_X + 4 + (STEP_W - 8) * (float)livePhase[l], mid - liveSrc[kSrcXlfo1 + l] * (STEP_H / 2 - 8), 3.5f); fillColor(C_TEXT); fill();
        }
    }
    void drawEg(int e) {
        caption(MX + 116, MY + 44, "TRIGGER", ALIGN_CENTER);
        const float v = liveSrc[kSrcEg1 + e]; const Color col = srcColor(kSrcEg1);
        beginPath(); roundedRect(MX + 250, MY + 54, 200, 14, 3); fillColor(C_DISPLAY); fill(); beginPath(); rect(MX + 252, MY + 56, 196 * clampf(v, 0.f, 1.f), 10); fillColor(col); fill();
    }
    void drawEf(int e) {
        caption(MX + 126, MY + 44, "LISTENS TO", ALIGN_CENTER);
        const float v = liveSrc[kSrcEf1 + e]; const Color col = srcColor(kSrcEf1);
        beginPath(); roundedRect(MX + 260, MY + 54, 190, 14, 3); fillColor(C_DISPLAY); fill(); beginPath(); rect(MX + 262, MY + 56, 186 * clampf(v, 0.f, 1.f), 10); fillColor(col); fill();
    }
    void drawXy(int x) {
        beginPath(); roundedRect(XY_X, XY_Y, XY_W, XY_H, 5); fillColor(C_DISPLAY); fill(); strokeColor(C_LINE); stroke();
        beginPath(); moveTo(XY_X + XY_W / 2, XY_Y + 6); lineTo(XY_X + XY_W / 2, XY_Y + XY_H - 6); moveTo(XY_X + 6, XY_Y + XY_H / 2); lineTo(XY_X + XY_W - 6, XY_Y + XY_H / 2);
        strokeColor(Color(255, 255, 255, 10 / 255.f)); stroke();
        const float px = XY_X + 8 + (XY_W - 16) * val[xyp(x, 0)], py = XY_Y + XY_H - 8 - (XY_H - 16) * val[xyp(x, 1)];
        const Color col = srcColor(kSrcXy1X);
        beginPath(); moveTo(px, XY_Y + 4); lineTo(px, XY_Y + XY_H - 4); moveTo(XY_X + 4, py); lineTo(XY_X + XY_W - 4, py); strokeColor(withAlpha(col, 0.3f)); stroke();
        beginPath(); circle(px, py, dragXy ? 10 : 8); fillColor(col); fill();
        char b[32]; snprintf(b, sizeof b, "X %.0f%%   Y %.0f%%", val[xyp(x, 0)] * 100, val[xyp(x, 1)] * 100);
        fontSize(9); textLetterSpacing(0.6f); textAlign(ALIGN_LEFT | ALIGN_TOP); fillColor(C_DIM); text(XY_X + XY_W + 16, XY_Y + 100, b, nullptr);
    }
    void drawMidi() {
        static const char* const tip[5] = { "last note velocity", "last note, around middle C", "mod wheel (CC1)", "pitch bend wheel", "channel or poly aftertouch" };
        for (int i = 0; i < 5; ++i) { fontSize(9); textLetterSpacing(0.4f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(C_FAINT); text(MX + 24 + CHIP_W + 16, MY + 52 + i * 34 + CHIP_H / 2, tip[i], nullptr); }
    }

    // ---- modulation slots --------------------------------------------------------
    bool slotUsed(int s) const { return (int)std::round(val[sp(s, kSlotSrc)]) > 0 || (int)std::round(val[sp(s, kSlotDst)]) > 0; }
    std::vector<int> usedSlots() const { std::vector<int> v; for (int s = 0; s < kNumSlots; ++s) if (slotUsed(s)) v.push_back(s); return v; }
    struct SlotRow { int slot; float x, y; };
    std::vector<SlotRow> slotRows() const {
        std::vector<SlotRow> r; const std::vector<int> u = usedSlots();
        for (int i = slotScroll * 2; i < (int)u.size() && (int)r.size() < SLOT_ROWS * 2; ++i) { const int k = i - slotScroll * 2; r.push_back({ u[i], SX + 10 + (k / SLOT_ROWS) * SLOT_COLW, SLOT_Y0 + (k % SLOT_ROWS) * SLOT_ROWH }); }
        return r;
    }
    void drawSlots() {
        panel(SX, SY, SW, SH);
        const std::vector<int> u = usedSlots();
        char b[48]; snprintf(b, sizeof b, "MODULATION  %d / %d", (int)u.size(), kNumSlots);
        fontSize(8); textLetterSpacing(1.8f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(C_FAINT); text(SX + 14, SY + 18, b, nullptr);
        if (u.empty()) { fontSize(10.5f); textLetterSpacing(0.3f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(C_FAINT);
            text(SX + SW / 2, SY + SH / 2 - 6, "Drag a source chip from the left onto any knob", nullptr);
            fontSize(9); text(SX + SW / 2, SY + SH / 2 + 12, "Each connection appears here; up to 50", nullptr); return; }
        for (const SlotRow& r : slotRows()) {
            const int s = r.slot, src = (int)std::round(val[sp(s, kSlotSrc)]), dst = (int)std::round(val[sp(s, kSlotDst)]);
            const bool byp = val[sp(s, kSlotBypass)] > 0.5f, sel = s == selSlot;
            beginPath(); roundedRect(r.x, r.y + 1, SLOT_COLW - 8, SLOT_ROWH - 2, 4); fillColor(sel ? withAlpha(C_ACC, 0.12f) : C_PANEL_HI); fill();
            beginPath(); circle(r.x + 10, r.y + SLOT_ROWH / 2, 3); fillColor(byp ? C_FAINT : srcColor(src)); fill();
            fontSize(8.5f); textLetterSpacing(0.6f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
            fillColor(hoverSlotPart == s * 10 + 1 ? C_TEXT : (byp ? C_FAINT : srcColor(src))); text(r.x + 18, r.y + SLOT_ROWH / 2 + 0.5f, srcShort(src), nullptr);
            fillColor(C_FAINT); text(r.x + 62, r.y + SLOT_ROWH / 2 + 0.5f, ">", nullptr);
            std::string nm = destName(dst); if (nm.size() > 18) nm = nm.substr(0, 17) + ".";
            fillColor(hoverSlotPart == s * 10 + 2 ? C_TEXT : (byp ? C_FAINT : C_DIM)); text(r.x + 72, r.y + SLOT_ROWH / 2 + 0.5f, nm.c_str(), nullptr);
            // amount mini knob
            const float kx = r.x + 200, ky = r.y + SLOT_ROWH / 2; const uint32_t ap = sp(s, kSlotAmt);
            drawKnobBody(kx, ky, 7.5f, toNorm(ap, val[ap]), true, hoverSlotPart == s * 10 + 3 || activeSlotAmt == s, byp, ap);
            char a[16]; fmt(ap, a, sizeof a); fontSize(8); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(byp ? C_FAINT : C_DIM); text(kx + 11, ky + 0.5f, a, nullptr);
            // bypass and delete
            beginPath(); circle(r.x + SLOT_COLW - 30, ky, 4.5f); fillColor(byp ? C_PANEL : withAlpha(C_ACC, 0.8f)); fill(); strokeColor(byp ? C_FAINT : C_ACC); strokeWidth(1); stroke();
            const float dx = r.x + SLOT_COLW - 16;
            beginPath(); moveTo(dx - 3.5f, ky - 3.5f); lineTo(dx + 3.5f, ky + 3.5f); moveTo(dx + 3.5f, ky - 3.5f); lineTo(dx - 3.5f, ky + 3.5f);
            strokeColor(hoverSlotPart == s * 10 + 5 ? C_TEXT : C_FAINT); strokeWidth(1.3f); stroke();
        }
        const int pages = ((int)u.size() + 1) / 2 - SLOT_ROWS;
        if (pages > 0) { fontSize(8); textLetterSpacing(1); textAlign(ALIGN_RIGHT | ALIGN_MIDDLE); fillColor(C_FAINT); text(SX + SW - 14, SY + 18, "SCROLL FOR MORE", nullptr); }
    }
    // slot row part under the pointer: slot*10 + part (1 source, 2 destination, 3 amount, 4 bypass, 5 delete), or -1
    int slotPartHit(float x, float y) const {
        for (const SlotRow& r : slotRows()) {
            if (y < r.y || y >= r.y + SLOT_ROWH || x < r.x || x >= r.x + SLOT_COLW - 8) continue;
            const float rx = x - r.x;
            if (rx < 60) return r.slot * 10 + 1;
            if (rx < 189) return r.slot * 10 + 2;
            if (rx < SLOT_COLW - 38) return r.slot * 10 + 3;
            if (rx < SLOT_COLW - 22) return r.slot * 10 + 4;
            return r.slot * 10 + 5;
        }
        return -1;
    }

    // ---- floating slot panel: every slot that modulates one knob ---------------------
    struct KnobPanel { bool open = false; int dest = 0; float x = 0, y = 0; } kpan; int hoverKp = -1;
    static constexpr float KP_W = 260, KP_HEAD = 26, KP_ROWH = 22;
    std::vector<int> slotsFor(int d) const { std::vector<int> v; for (int k = 0; k < kNumSlots; ++k) if ((int)std::round(val[sp(k, kSlotDst)]) == d && (int)std::round(val[sp(k, kSlotSrc)]) > 0) v.push_back(k); return v; }
    void kpLayout(float& x, float& y, float& h) const {
        h = KP_HEAD + std::max<size_t>(1, slotsFor(kpan.dest).size()) * KP_ROWH + 8;
        x = std::min(W - KP_W - 8, std::max(8.f, kpan.x)); y = std::min(H - h - 8, std::max(8.f, kpan.y));
    }
    bool kpInside(float px, float py) const { float x, y, h; kpLayout(x, y, h); return px >= x && px < x + KP_W && py >= y && py < y + h; }
    // slot*10 + part (1 source, 3 amount, 4 bypass, 5 delete), or -1
    int kpHit(float px, float py) const {
        float x, y, h; kpLayout(x, y, h); const std::vector<int> v = slotsFor(kpan.dest);
        for (int i = 0; i < (int)v.size(); ++i) { const float ry = y + KP_HEAD + i * KP_ROWH; if (py < ry || py >= ry + KP_ROWH || px < x || px >= x + KP_W) continue;
            const float rx = px - x; return v[i] * 10 + (rx < 120 ? 1 : (rx < KP_W - 44 ? 3 : (rx < KP_W - 24 ? 4 : 5))); }
        return -1;
    }
    void openKnobPanel(uint32_t p, float px, float py) {
        const int d = destForParam(p); if (d <= 0) return;
        if (slotsFor(d).empty()) { flash("Nothing modulates this control yet. Drag a source chip onto it."); return; }
        kpan.open = true; kpan.dest = d; kpan.x = px + 14; kpan.y = py - 10; hoverKp = -1; repaint();
    }
    void drawKnobPanel() {
        float x, y, h; kpLayout(x, y, h); const std::vector<int> v = slotsFor(kpan.dest);
        beginPath(); rect(x - 10, y - 4, KP_W + 20, h + 18); fillPaint(boxGradient(x, y + 2, KP_W, h, 6, 12, Color(0, 0, 0, 0.6f), Color(0, 0, 0, 0.f))); fill();
        beginPath(); roundedRect(x, y, KP_W, h, 6); fillColor(C_PANEL_HI); fill(); strokeColor(C_LINE); strokeWidth(1); stroke();
        std::string t = destName(kpan.dest); for (char& ch : t) ch = (char)std::toupper((unsigned char)ch);
        fontSize(8); textLetterSpacing(1.4f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(C_ACC); text(x + 12, y + 14, t.c_str(), nullptr);
        for (int i = 0; i < (int)v.size(); ++i) {
            const int sl = v[i], src = (int)std::round(val[sp(sl, kSlotSrc)]); const bool byp = val[sp(sl, kSlotBypass)] > 0.5f;
            const float ry = y + KP_HEAD + i * KP_ROWH, cy = ry + KP_ROWH / 2;
            if (hoverKp / 10 == sl) { beginPath(); roundedRect(x + 4, ry + 1, KP_W - 8, KP_ROWH - 2, 3); fillColor(withAlpha(C_ACC, 0.10f)); fill(); }
            beginPath(); circle(x + 14, cy, 3); fillColor(byp ? C_FAINT : srcColor(src)); fill();
            fontSize(9); textLetterSpacing(0.4f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(hoverKp == sl * 10 + 1 ? C_TEXT : (byp ? C_FAINT : srcColor(src))); text(x + 24, cy + 0.5f, sourceName(src), nullptr);
            const uint32_t ap = sp(sl, kSlotAmt); const float kx = x + 132;
            drawKnobBody(kx, cy, 7.5f, toNorm(ap, val[ap]), true, hoverKp == sl * 10 + 3 || activeSlotAmt == sl, byp, ap);
            char a[16]; fmt(ap, a, sizeof a); fontSize(8.5f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(byp ? C_FAINT : C_DIM); text(kx + 12, cy + 0.5f, a, nullptr);
            beginPath(); circle(x + KP_W - 34, cy, 4.5f); fillColor(byp ? C_PANEL : withAlpha(C_ACC, 0.8f)); fill(); strokeColor(byp ? C_FAINT : C_ACC); strokeWidth(1); stroke();
            const float dx = x + KP_W - 14;
            beginPath(); moveTo(dx - 3.5f, cy - 3.5f); lineTo(dx + 3.5f, cy + 3.5f); moveTo(dx + 3.5f, cy - 3.5f); lineTo(dx - 3.5f, cy + 3.5f);
            strokeColor(hoverKp == sl * 10 + 5 ? C_TEXT : C_FAINT); strokeWidth(1.3f); stroke();
        }
    }
    // the modulation ring around a modulated knob (clicking it opens the slot panel)
    bool ringHit(const Ctl& c, float x, float y) const {
        if (c.type != Knob || c.r <= 9) return false;
        const uint32_t p = effParam(c); const int d = destForParam(p); if (d <= 0 || slotsFor(d).empty()) return false;
        const float tw = c.r > 30 ? 5.f : (c.r > 18 ? 3.5f : (c.r > 10 ? 2.5f : 2.f)), rr = c.r + tw / 2 + 3;
        const float dist = std::sqrt((x - c.x) * (x - c.x) + (y - c.y) * (y - c.y));
        return std::fabs(dist - rr) <= 3.f;
    }

    // ---- knobs -------------------------------------------------------------------
    bool modInfo(uint32_t p, float& depth, Color& col, float& live) const {
        const int d = destForParam(p); if (d < 0) return false;
        depth = 0; float best = 0; int bs = 0;
        for (int s = 0; s < kNumSlots; ++s) {
            if (val[sp(s, kSlotBypass)] > 0.5f || (int)std::round(val[sp(s, kSlotDst)]) != d) continue;
            const int src = (int)std::round(val[sp(s, kSlotSrc)]); if (src <= 0) continue;
            const float a = std::fabs(val[sp(s, kSlotAmt)]); depth += a; if (a >= best) { best = a; bs = src; }
        }
        if (depth <= 0) return false;
        col = srcColor(bs); live = liveOff[d]; return true;
    }
    void drawKnobBody(float x, float y, float r, float n, bool bipolar, bool hot, bool dim, uint32_t p) {
        const float a0 = 0.75f * 3.14159265f, a1 = 2.25f * 3.14159265f, ang = a0 + (a1 - a0) * n;
        const float tw = r > 30 ? 5.f : (r > 18 ? 3.5f : (r > 10 ? 2.5f : 2.f));
        beginPath(); arc(x, y, r, a0, a1, NanoVG::CW); strokeColor(C_LINE); strokeWidth(tw); lineCap(ROUND); stroke();
        const float from = bipolar ? (a0 + a1) / 2 : a0;
        const Color arcCol = (p >= kBandFirst && p < kXlfoFirst) ? bandColor((p - kBandFirst) / kBandFields) : C_ACC;
        if (std::fabs(ang - from) > 0.001f) { beginPath(); arc(x, y, r, std::min(from, ang), std::max(from, ang), NanoVG::CW); strokeColor(dim ? C_FAINT : arcCol); stroke(); }
        float depth, live; Color mc;
        if (r > 9 && modInfo(p, depth, mc, live)) {
            const float span = std::max(0.08f, std::min(1.f, depth)) * (a1 - a0) * 0.5f;
            beginPath(); arc(x, y, r + tw / 2 + 3, std::max(a0, ang - span), std::min(a1, ang + span), NanoVG::CW); strokeColor(withAlpha(mc, 0.85f)); strokeWidth(2.f); lineCap(ROUND); stroke();
            const float la = a0 + (a1 - a0) * clampf(n + live, 0.f, 1.f);
            beginPath(); circle(x + std::cos(la) * (r + tw / 2 + 3), y + std::sin(la) * (r + tw / 2 + 3), 2.6f); fillColor(C_TEXT); fill();
        }
        const float cr = r - (r > 30 ? 10 : (r > 18 ? 7 : (r > 10 ? 5 : 3)));
        beginPath(); circle(x, y, cr);
        fillPaint(radialGradient(x - cr * 0.3f, y - cr * 0.4f, 0, cr * 1.4f, hot ? Color(0x44, 0x3a, 0x34) : C_PANEL_HI, Color(0x16, 0x13, 0x11)));
        fill(); strokeColor(Color(255, 255, 255, 18 / 255.f)); strokeWidth(1); stroke();
        beginPath(); moveTo(x + std::cos(ang) * cr * 0.3f, y + std::sin(ang) * cr * 0.3f); lineTo(x + std::cos(ang) * cr * 0.9f, y + std::sin(ang) * cr * 0.9f);
        strokeColor(dim ? C_DIM : C_TEXT); strokeWidth(r > 12 ? 2.f : 1.5f); lineCap(ROUND); stroke();
    }
    void drawCtl(const Ctl& c, bool hot) {
        const uint32_t p = effParam(c); const bool dim = dimmed(p); char buf[48];
        const bool dropTarget = dragSrc > 0 && hot && destForParam(p) >= 0;
        switch (c.type) {
        case Knob: {
            drawKnobBody(c.x, c.y, c.r, toNorm(p, val[p]), (paramDef(p).hints & kHintBipolar) != 0, hot, dim, p);
            if (dropTarget) { beginPath(); circle(c.x, c.y, c.r + 9); strokeColor(srcColor(dragSrc)); strokeWidth(2); stroke(); }
            fontSize(c.r > 30 ? 9.f : 8.f); textLetterSpacing(c.r > 30 ? 1.8f : 1.4f); textAlign(ALIGN_CENTER | ALIGN_TOP); fillColor(dim ? C_FAINT : C_DIM);
            text(c.x, c.y + c.r + 6, c.label, nullptr);
            fmt(p, buf, sizeof buf); fontSize(c.r > 30 ? 11.5f : 10.f); textLetterSpacing(0.4f); fillColor(dim ? C_FAINT : (hot ? C_ACC : Color(0xd6, 0xcc, 0xc4)));
            text(c.x, c.y + c.r + (c.r > 30 ? 19 : 17), buf, nullptr);
            break; }
        case Toggle: {
            const bool on = val[p] > 0.5f;
            const float w = 30, h = 16, x = c.x - w / 2, y = c.y - h / 2;
            beginPath(); roundedRect(x, y, w, h, h / 2); fillColor(on ? withAlpha(C_ACC, 0.25f) : C_PANEL_HI); fill(); strokeColor(on ? C_ACC : C_LINE); strokeWidth(1); stroke();
            beginPath(); circle(on ? x + w - h / 2 : x + h / 2, c.y, 5); fillColor(on ? C_ACC : C_DIM); fill();
            fontSize(8.5f); textLetterSpacing(1.5f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(on ? C_TEXT : C_DIM); text(c.x + w / 2 + 7, c.y + 0.5f, c.label, nullptr);
            break; }
        case Segment: {
            int n = 0; const char* const* names = segLabels(c.group, c.field, n);
            const int cur = segCurrent(c, val, p); const float sw = c.r / n, x0 = c.x - c.r / 2;
            for (int i = 0; i < n; ++i) {
                const float x = x0 + sw * i;
                beginPath(); roundedRect(x + 1.5f, c.y - SEG_H / 2, sw - 3, SEG_H, 4); fillColor(i == cur ? C_ACC : C_PANEL_HI); fill();
                fontSize(8.5f); textLetterSpacing(1.2f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(i == cur ? C_BG : C_DIM); text(x + sw / 2, c.y + 0.5f, names[i], nullptr);
            }
            break; }
        case Stepper: {
            const float x = c.x - c.r / 2, y = c.y - 11, w = c.r, h = 22;
            beginPath(); roundedRect(x, y, w, h, 4); fillColor(C_PANEL_HI); fill(); strokeColor(hot ? C_DIM : C_LINE); strokeWidth(1); stroke();
            for (int side = 0; side < 2; ++side) { const float ax = side == 0 ? x + 9 : x + w - 9, d = side == 0 ? -1.f : 1.f;
                beginPath(); moveTo(ax - 2.5f * d, c.y - 4); lineTo(ax + 2.5f * d, c.y); lineTo(ax - 2.5f * d, c.y + 4); closePath(); fillColor(C_DIM); fill(); }
            fmt(p, buf, sizeof buf); std::string up(buf); for (char& ch : up) ch = (char)std::toupper((unsigned char)ch);
            if (c.field == kLSteps) up += " STEPS";
            fontSize(8.5f); textLetterSpacing(0.8f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(C_ACC); text(c.x, c.y + 0.5f, up.c_str(), nullptr);
            break; }
        }
        if (learnMode && c.type != Stepper) {
            const int cc = paramCC[p]; const bool waiting = learnWaiting == (int)p;
            if (cc >= 0 || waiting) {
                const float by = c.type == Knob ? c.y - c.r - 12 : c.y - SEG_H / 2 - 8;
                snprintf(buf, sizeof buf, waiting ? "?" : "CC%d", cc);
                fontSize(8); textLetterSpacing(0.5f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
                const float bw = waiting ? 14 : 32;
                beginPath(); roundedRect(c.x - bw / 2, by - 6, bw, 12, 3); fillColor(waiting ? C_ACC : C_PANEL_HI); fill(); strokeColor(C_ACC); strokeWidth(1); stroke();
                fillColor(waiting ? C_BG : C_ACC); text(c.x, by + 0.5f, buf, nullptr);
            }
        }
    }

    void drawStatus() {
        std::string msg;
        if (statusFlash > 0) msg = statusText;
        else if (learnMode) msg = learnWaiting >= 0 ? std::string("Move a MIDI controller to assign: ") + paramDef(learnWaiting).name : "MIDI Learn: click a control, then move a controller. Right-click unassigns.";
        else if (dragSrc > 0) { msg = std::string("Drop ") + sourceName(dragSrc) + " onto a knob"; if (hover >= 0) { const int d = destForParam(effParam(controls()[hover])); if (d > 0) msg = std::string("Drop ") + sourceName(dragSrc) + " onto " + destName(d); } }
        if (!msg.empty()) { fontSize(10); textLetterSpacing(0.6f); textAlign(ALIGN_RIGHT | ALIGN_MIDDLE); fillColor(C_ACC); text(DX + DW - 14, DY + 16, msg.c_str(), nullptr); }
        if (learnMode && learnWaiting < 0) { fontSize(9); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(hoverClear ? C_TEXT : C_DIM); text(DX + 14, DY + DH - 30, "CLEAR ALL ASSIGNMENTS", nullptr); }
        if (statusFlash > 0) { --statusFlash; repaint(); }
    }
    void flash(const std::string& s) { statusText = s; statusFlash = 90; repaint(); }
    void drawEditBox() {
        const Ctl& c = controls()[editing];
        const float w = 90, h = 22, x = c.x - w / 2, y = c.y + c.r + 4;
        beginPath(); roundedRect(x, y, w, h, 4); fillColor(C_DISPLAY); fill(); strokeColor(C_ACC); strokeWidth(1); stroke();
        fontSize(12); textLetterSpacing(0.5f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(C_TEXT); text(c.x, y + h / 2 + 0.5f, (editBuf + "|").c_str(), nullptr);
    }

    // ---- popups: style, slot source, slot destination ---------------------------
    struct Popup { bool open = false; int kind = 0, slot = -1; float x = 0, y = 0; int hover = -1; } popup;   // kind 0 style, 1 source, 2 destination
    // row of a style inside its family column: the 2.0 styles are slotted in where they belong
    static int styleRow(int s) {
        static const int order[] = { 16, 0, 1, 2, 17, 3,  4, 5, 6, 18,  7, 19, 8, 9, 20,  10, 21, 11, 22,  12, 13, 23, 24, 14, 15, 25, 26, 27 };
        int k = 0, cat = styleCategory(s);
        for (int o : order) { if (o == s) return k; if (styleCategory(o) == cat) ++k; }
        return 0;
    }
    int popupCount() const { return popup.kind == 0 ? kNumStyles : (popup.kind == 1 ? kNumSources - 1 : (popup.kind == 3 ? (int)compItems.size() : kNumDests - 1)); }
    std::string popupLabel(int i) const {
        if (popup.kind == 3) { const CompItem& it = compItems[i]; return it.type == 1 ? (compPendingDel == i ? "Delete? click again" : "  " + it.name) : it.name; }
        return popup.kind == 0 ? styleName(i) : (popup.kind == 1 ? sourceName(i + 1) : destName(i + 1)); }
    void popupLayout(float& x, float& y, float& w, float& h, int& rows, float& colw) const {
        const int n = popupCount();
        if (popup.kind == 0) { rows = 9; colw = 132; w = 5 * colw + 12; h = rows * 22 + 12 + 20; }
        else if (popup.kind == 3) { rows = n; colw = 180; w = colw + 12; h = rows * 20 + 12; }
        else { rows = popup.kind == 1 ? 9 : 22; const int cols = (n + rows - 1) / rows; colw = popup.kind == 1 ? 110 : 150; w = cols * colw + 12; h = rows * 20 + 12; }
        x = std::min(W - w - 8, std::max(8.f, popup.x)); y = std::min(H - h - 8, std::max(8.f, popup.y));
    }
    void popupCell(int i, float x, float y, float colw, int rows, float& ix, float& iy) const {
        if (popup.kind == 0) { const int cat = styleCategory(i); const int k = styleRow(i); ix = x + 6 + cat * colw; iy = y + 6 + 20 + k * 22; return; }
        ix = x + 6 + (i / rows) * colw; iy = y + 6 + (i % rows) * 20;
    }
    void drawPopup() {
        float x, y, w, h, colw; int rows; popupLayout(x, y, w, h, rows, colw);
        beginPath(); rect(x - 10, y - 4, w + 20, h + 18); fillPaint(boxGradient(x, y + 2, w, h, 6, 12, Color(0, 0, 0, 0.6f), Color(0, 0, 0, 0.f))); fill();
        beginPath(); roundedRect(x, y, w, h, 6); fillColor(C_PANEL_HI); fill(); strokeColor(C_LINE); strokeWidth(1); stroke();
        int cur = -1;
        if (popup.kind == 0) cur = (int)std::round(val[bp(selBand, kBStyle)]);
        else if (popup.slot >= 0) cur = (int)std::round(val[sp(popup.slot, popup.kind == 1 ? kSlotSrc : kSlotDst)]) - 1;
        if (popup.kind == 0) for (int c = 0; c < 5; ++c) { std::string up = categoryName(c); for (char& ch : up) ch = (char)std::toupper((unsigned char)ch);
            fontSize(8); textLetterSpacing(1.6f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(C_ACC); text(x + 12 + c * colw, y + 16, up.c_str(), nullptr); }
        for (int i = 0; i < popupCount(); ++i) {
            float ix, iy; popupCell(i, x, y, colw, rows, ix, iy); const float rh = popup.kind == 0 ? 22 : 20;
            if (i == popup.hover) { beginPath(); roundedRect(ix, iy, colw - 4, rh, 3); fillColor(withAlpha(C_ACC, 0.22f)); fill(); }
            if (popup.kind == 1) { beginPath(); circle(ix + 7, iy + rh / 2, 3); fillColor(srcColor(i + 1)); fill(); }
            if (popup.kind == 3 && i > 0 && compItems[i].type != compItems[i - 1].type) { beginPath(); rect(ix + 4, iy - 0.5f, colw - 12, 1); fillColor(C_LINE); fill(); }
            const bool accent = popup.kind == 3 && (compItems[i].type == 2 || compPendingDel == i);
            fontSize(popup.kind == 0 ? 10.5f : 9.5f); textLetterSpacing(0.3f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(i == cur || accent ? C_ACC : (i == popup.hover ? C_TEXT : C_DIM));
            text(ix + (popup.kind == 1 ? 15 : 6), iy + rh / 2 + 0.5f, popupLabel(i).c_str(), nullptr);
        }
    }
    int popupHit(float px, float py) const {
        float x, y, w, h, colw; int rows; popupLayout(x, y, w, h, rows, colw);
        for (int i = 0; i < popupCount(); ++i) { float ix, iy; popupCell(i, x, y, colw, rows, ix, iy); const float rh = popup.kind == 0 ? 22 : 20;
            if (px >= ix && px < ix + colw - 4 && py >= iy && py < iy + rh) return i; }
        return -1;
    }
    bool popupInside(float px, float py) const { float x, y, w, h, colw; int rows; popupLayout(x, y, w, h, rows, colw); return px >= x && px < x + w && py >= y && py < y + h; }
    void popupPick(int i) {
        if (popup.kind == 3) { pickComp(i); return; }
        beginGesture();
        if (popup.kind == 0) commit(bp(selBand, kBStyle), (float)i);
        else if (popup.slot >= 0) commit(sp(popup.slot, popup.kind == 1 ? kSlotSrc : kSlotDst), (float)(i + 1));
        endGesture(); popup.open = false; repaint();
    }

    // ---- presets ------------------------------------------------------------------
    bool matchesValues(const float* v) const { for (uint32_t i = 0; i < kParamCount; ++i) if (isSetting(i) && std::fabs(val[i] - v[i]) > 1e-4f * std::max(1.f, std::fabs(v[i]))) return false; return true; }
    void applyValues(const float* v) { beginGesture(); applyAll(v); endGesture(); }
    void applyFactory(int idx) { idx = (idx % (int)kNumPresets + (int)kNumPresets) % (int)kNumPresets; applyValues(presets()[idx].v); curFactory = idx; curUser.clear(); selBand = 0; repaint(); }
    void applyUser(const std::string& name) { float v[kParamCount]; std::copy(defv, defv + kParamCount, v); if (!loadUserPreset(name, v)) return; applyValues(v); std::copy(v, v + kParamCount, userVals); curUser = name; curFactory = -1; selBand = 0; repaint(); }
    void programLoaded(uint32_t index) override { curFactory = (int)index; curUser.clear(); repaint(); }
    void step(int d) {
        const int nf = (int)kNumPresets, nu = (int)userPresets.size(), total = nf + nu;
        int cur = curFactory; if (!curUser.empty()) for (int i = 0; i < nu; ++i) if (userPresets[i] == curUser) cur = nf + i;
        if (cur < 0) cur = 0;
        const int next = ((cur + d) % total + total) % total;
        if (next < nf) applyFactory(next); else applyUser(userPresets[next - nf]);
    }
    void drawPresetBar() {
        const float x = PS_X, y = PS_Y, w = PS_W, h = PS_H, a = PS_ARROW;
        beginPath(); roundedRect(x, y, w, h, 5); fillColor(C_PANEL); fill(); strokeColor(menuOpen ? C_ACC : C_LINE); strokeWidth(1); stroke();
        beginPath(); moveTo(x + a, y + 5); lineTo(x + a, y + h - 5); moveTo(x + w - a, y + 5); lineTo(x + w - a, y + h - 5); strokeColor(C_LINE); stroke();
        for (int side = 0; side < 2; ++side) { const bool hot = presetHover == (side == 0 ? 0 : 2);
            const float cx = side == 0 ? x + a / 2 : x + w - a / 2, cy = y + h / 2, d = side == 0 ? -1.f : 1.f;
            beginPath(); moveTo(cx - 3 * d, cy - 5); lineTo(cx + 3 * d, cy); lineTo(cx - 3 * d, cy + 5); closePath(); fillColor(hot ? C_ACC : C_DIM); fill(); }
        std::string name; bool modified = false;
        if (!curUser.empty()) { name = curUser; modified = !matchesValues(userVals); }
        else if (curFactory >= 0) { name = presets()[curFactory].name; modified = !matchesValues(presets()[curFactory].v); }
        else name = "Custom";
        if (modified) name += " *";
        fontSize(11.5f); textLetterSpacing(0.6f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(presetHover == 1 || menuOpen ? C_TEXT : Color(0xd8, 0xd0, 0xca)); text(x + w / 2, y + h / 2 + 0.5f, name.c_str(), nullptr);
    }
    // preset browser: categories, user presets and favourites in columns; typing searches names and categories.
    // kind: 0 factory, 1 user, 2 save button, 3 search field, 4 favourite (idx into favs)
    struct MRect { float x, y, w, h; int kind; int idx; };
    static std::string lower(const std::string& s) { std::string r(s); for (char& c : r) c = (char)std::tolower((unsigned char)c); return r; }
    std::string favKey(int kind, int idx) const { return kind == 0 ? std::string("f:") + presets()[idx].name : (kind == 1 ? "u:" + userPresets[idx] : (idx < (int)favs.size() ? favs[idx] : std::string())); }
    bool isFav(const std::string& k) const { return std::find(favs.begin(), favs.end(), k) != favs.end(); }
    void toggleFav(const std::string& k) {
        if (k.empty()) return;
        auto it = std::find(favs.begin(), favs.end(), k); if (it != favs.end()) favs.erase(it); else favs.push_back(k);
        saveFavourites(favs); repaint();
    }
    static constexpr int kMenuCols = kNumCategories + 2, kMenuRows = 16;
    void menuLayout(float& mx, float& my, float& mw, float& mh, std::vector<MRect>& items) const {
        mw = PM_COLW * kMenuCols + 2 * PM_PAD; mx = std::min(W - mw - 8, std::max(8.f, PS_X + PS_W / 2 - mw / 2)); my = PS_Y + PS_H + 4;
        items.clear();
        const float top = my + PM_PAD + PM_ROW + 6 + PM_HEAD;
        int maxRows = 1;
        items.push_back({ mx + PM_PAD + PM_COLW, my + PM_PAD, 300, PM_ROW, 3, 0 });
        items.push_back({ mx + mw - PM_PAD - 150, my + PM_PAD, 150, PM_ROW, 2, 0 });
        if (!query.empty()) {
            const std::string q = lower(query); int k = 0;
            auto place = [&](int kind, int idx) { const int col = k / kMenuRows, row = k % kMenuRows; if (col >= kMenuCols) return; items.push_back({ mx + PM_PAD + col * PM_COLW, top + row * PM_ROW, PM_COLW - 4, PM_ROW, kind, idx }); maxRows = std::max(maxRows, row + 1); ++k; };
            for (uint32_t i = 0; i < kNumPresets; ++i) if (lower(presets()[i].name).find(q) != std::string::npos || lower(presets()[i].category).find(q) != std::string::npos) place(0, (int)i);
            for (int u = 0; u < (int)userPresets.size(); ++u) if (lower(userPresets[u]).find(q) != std::string::npos || std::string("user").find(q) != std::string::npos) place(1, u);
        } else {
            int rows[kNumCategories] = {};
            for (uint32_t i = 0; i < kNumPresets; ++i) {
                const char* cat = presets()[i].category;
                if (std::strcmp(cat, "Init") == 0) { items.push_back({ mx + PM_PAD, my + PM_PAD, PM_COLW - 4, PM_ROW, 0, (int)i }); continue; }
                for (int c = 0; c < kNumCategories; ++c)
                    if (std::strcmp(cat, kCategories[c]) == 0) { items.push_back({ mx + PM_PAD + c * PM_COLW, top + rows[c] * PM_ROW, PM_COLW - 4, PM_ROW, 0, (int)i }); maxRows = std::max(maxRows, ++rows[c]); }
            }
            for (int u = 0; u < (int)userPresets.size() && u < kMenuRows; ++u) items.push_back({ mx + PM_PAD + kNumCategories * PM_COLW, top + u * PM_ROW, PM_COLW - 4, PM_ROW, 1, u });
            maxRows = std::max(maxRows, std::min(kMenuRows, (int)userPresets.size()));
            for (int f = 0; f < (int)favs.size() && f < kMenuRows; ++f) items.push_back({ mx + PM_PAD + (kNumCategories + 1) * PM_COLW, top + f * PM_ROW, PM_COLW - 4, PM_ROW, 4, f });
            maxRows = std::max(maxRows, std::min(kMenuRows, (int)favs.size()));
        }
        mh = PM_PAD + PM_ROW + 6 + PM_HEAD + maxRows * PM_ROW + PM_PAD;
    }
    std::string itemLabel(const MRect& r) const {
        if (r.kind == 0) return presets()[r.idx].name;
        if (r.kind == 1) return userPresets[r.idx];
        if (r.kind == 4 && r.idx < (int)favs.size()) return favs[r.idx].substr(2);
        return "";
    }
    void drawPresetMenu() {
        float mx, my, mw, mh; std::vector<MRect> items; menuLayout(mx, my, mw, mh, items);
        beginPath(); rect(mx - 12, my - 4, mw + 24, mh + 20); fillPaint(boxGradient(mx, my + 2, mw, mh, 6, 14, Color(0, 0, 0, 0.6f), Color(0, 0, 0, 0.f))); fill();
        beginPath(); roundedRect(mx, my, mw, mh, 6); fillColor(C_PANEL_HI); fill(); strokeColor(C_LINE); strokeWidth(1); stroke();
        beginPath(); moveTo(mx + PM_PAD, my + PM_PAD + PM_ROW + 3); lineTo(mx + mw - PM_PAD, my + PM_PAD + PM_ROW + 3); strokeColor(C_LINE); stroke();
        fontSize(8.5f); textLetterSpacing(1.8f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
        const float hy = my + PM_PAD + PM_ROW + 6 + PM_HEAD / 2;
        if (query.empty()) {
            for (int c = 0; c < kMenuCols; ++c) {
                std::string up = c < kNumCategories ? kCategories[c] : (c == kNumCategories ? "User" : "Favourites"); for (char& ch : up) ch = (char)std::toupper((unsigned char)ch);
                fillColor(c < kNumCategories ? C_ACC : C_FAV); text(mx + PM_PAD + c * PM_COLW + 12, hy, up.c_str(), nullptr);
            }
            fontSize(10); textLetterSpacing(0.4f); fillColor(C_FAINT);
            if (userPresets.empty()) text(mx + PM_PAD + kNumCategories * PM_COLW + 12, hy + PM_HEAD / 2 + PM_ROW / 2, "(none saved yet)", nullptr);
            if (favs.empty()) text(mx + PM_PAD + (kNumCategories + 1) * PM_COLW + 12, hy + PM_HEAD / 2 + PM_ROW / 2, "(click a star)", nullptr);
        } else {
            int found = 0; for (const MRect& r : items) if (r.kind == 0 || r.kind == 1) ++found;
            char b[64]; snprintf(b, sizeof b, found ? "%d MATCHING PRESET%s" : "NO PRESET MATCHES", found, found == 1 ? "" : "S"); fillColor(C_ACC); text(mx + PM_PAD + 12, hy, b, nullptr);
        }
        for (int i = 0; i < (int)items.size(); ++i) {
            const MRect& r = items[i]; const bool hot = i == menuHover;
            if (r.kind == 2) {
                beginPath(); roundedRect(r.x, r.y + 1, r.w, r.h - 2, 4); fillColor(hot ? withAlpha(C_ACC, 0.25f) : C_PANEL); fill(); strokeColor(C_ACC); strokeWidth(1); stroke();
                fontSize(9.5f); textLetterSpacing(1.6f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(C_TEXT); text(r.x + r.w / 2, r.y + r.h / 2 + 0.5f, "SAVE CURRENT AS...", nullptr);
                continue;
            }
            if (r.kind == 3) {             // search field
                beginPath(); roundedRect(r.x, r.y + 1, r.w, r.h - 2, 4); fillColor(C_DISPLAY); fill(); strokeColor(query.empty() ? C_LINE : C_ACC); strokeWidth(1); stroke();
                beginPath(); circle(r.x + 12, r.y + r.h / 2, 4); moveTo(r.x + 15, r.y + r.h / 2 + 3); lineTo(r.x + 18, r.y + r.h / 2 + 6); strokeColor(C_DIM); strokeWidth(1.3f); stroke();
                fontSize(11); textLetterSpacing(0.3f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(query.empty() ? C_FAINT : C_TEXT);
                text(r.x + 26, r.y + r.h / 2 + 0.5f, query.empty() ? "Type to search names and categories" : (query + "|").c_str(), nullptr);
                continue;
            }
            const std::string key = favKey(r.kind, r.idx);
            const bool isCur = r.kind == 0 ? (curUser.empty() && r.idx == curFactory) : (r.kind == 1 ? curUser == userPresets[r.idx] : false);
            const bool del = r.kind == 1 && pendingDelete == r.idx;
            if (hot || del) { beginPath(); roundedRect(r.x, r.y, r.w, r.h, 4); fillColor(del ? Color(0xd0, 0x50, 0x40, 0.35f) : withAlpha(C_ACC, 0.18f)); fill(); }
            if (isCur) { beginPath(); circle(r.x + 6, r.y + r.h / 2, 2.5f); fillColor(C_ACC); fill(); }
            fontSize(11); textLetterSpacing(0.3f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(hot || isCur || del ? C_TEXT : C_DIM);
            const std::string label = del ? "Delete? click again" : itemLabel(r);
            text(r.x + 12, r.y + r.h / 2 + 0.5f, label.c_str(), nullptr);
            // the favourite star
            const bool fav = isFav(key);
            if (fav || hot) drawStar(r.x + r.w - 10, r.y + r.h / 2, 5.f, fav);
        }
    }
    void drawStar(float cx, float cy, float rr, bool filled) {
        beginPath();
        for (int k = 0; k < 10; ++k) { const float a = -3.14159265f / 2 + k * 3.14159265f / 5, q = k % 2 ? rr * 0.45f : rr; if (!k) moveTo(cx + q * std::cos(a), cy + q * std::sin(a)); else lineTo(cx + q * std::cos(a), cy + q * std::sin(a)); }
        closePath();
        if (filled) { fillColor(C_FAV); fill(); } else { strokeColor(C_DIM); strokeWidth(1); stroke(); }
    }
    int menuHit(float x, float y) const { float mx, my, mw, mh; std::vector<MRect> it; menuLayout(mx, my, mw, mh, it);
        for (int i = 0; i < (int)it.size(); ++i) if (x >= it[i].x && x < it[i].x + it[i].w && y >= it[i].y && y < it[i].y + it[i].h) return i;
        return -1; }
    MRect menuItem(int i) const { float mx, my, mw, mh; std::vector<MRect> it; menuLayout(mx, my, mw, mh, it); return it[i]; }
    void openMenu() { userPresets = listUserPresets(); favs = loadFavourites(); query.clear(); menuOpen = true; menuHover = -1; pendingDelete = -1; repaint(); }
    void applyFavourite(int idx) {
        if (idx >= (int)favs.size()) return;
        const std::string& k = favs[idx];
        if (k.compare(0, 2, "f:") == 0) { for (uint32_t i = 0; i < kNumPresets; ++i) if (k.substr(2) == presets()[i].name) { applyFactory((int)i); return; } }
        else applyUser(k.substr(2));
    }
    void drawSaveBox() {
        const float w = 360, h = 96, x = W / 2 - w / 2, y = 150;
        beginPath(); rect(0, 0, W, H); fillColor(Color(0, 0, 0, 0.45f)); fill();
        beginPath(); roundedRect(x, y, w, h, 8); fillColor(C_PANEL_HI); fill(); strokeColor(C_ACC); strokeWidth(1); stroke();
        fontSize(9.5f); textLetterSpacing(2.f); textAlign(ALIGN_LEFT | ALIGN_TOP); fillColor(C_DIM); text(x + 16, y + 14, savingComp ? (savingCompKind == kCompXlfo ? "SAVE LFO PRESET AS" : (savingCompKind == kCompEg ? "SAVE ENVELOPE PRESET AS" : "SAVE FOLLOWER PRESET AS")) : "SAVE PRESET AS", nullptr);
        beginPath(); roundedRect(x + 16, y + 36, w - 32, 26, 4); fillColor(C_DISPLAY); fill(); strokeColor(C_LINE); stroke();
        fontSize(13); textLetterSpacing(0.4f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(C_TEXT); text(x + 26, y + 49.5f, (saveName + "|").c_str(), nullptr);
        fontSize(8.5f); textLetterSpacing(1.2f); fillColor(C_FAINT); textAlign(ALIGN_RIGHT | ALIGN_TOP);
        text(x + w - 16, y + 72, saveError ? "COULD NOT SAVE - CHECK THE NAME" : "ENTER TO SAVE   ·   ESC TO CANCEL", nullptr);
    }

    // ---- band add / remove -----------------------------------------------------------
    // moves band settings and modulation targets so band 'from' and up shift by 'delta' (+1 insert, -1 delete)
    void shiftBands(int from, int delta) {
        const int n = nb();
        if (delta > 0) { for (int b = n - 1; b >= from; --b) for (int f = 0; f < kBandFields; ++f) commit(bp(b + 1, f), val[bp(b, f)]); }
        else { for (int b = from; b < n - 1; ++b) for (int f = 0; f < kBandFields; ++f) commit(bp(b, f), val[bp(b + 1, f)]); }
        for (int s = 0; s < kNumSlots; ++s) {
            const int d = (int)std::round(val[sp(s, kSlotDst)]);
            if (d >= kDstBandFirst && d < kDstXFirst) {
                const int b = (d - kDstBandFirst) / kNumBandDests, k = (d - kDstBandFirst) % kNumBandDests;
                if (delta < 0 && b == from - 1 + 0 && false) {}
                if (b >= from) {
                    const int nb2 = b + delta;
                    if (nb2 < 0 || nb2 >= kMaxBands) clearSlot(s); else commit(sp(s, kSlotDst), (float)(kDstBandFirst + nb2 * kNumBandDests + k));
                }
            }
        }
    }
    void addBandAt(float hz) {
        const int n = nb(); if (n >= kMaxBands) { flash("Six bands is the maximum"); return; }
        int b = 0; while (b < n - 1 && hz > xoverHz(b)) ++b;
        const float lo = bandLo(b), hi = bandHi(b);
        if (hz < lo * 1.3f || hz > hi / 1.3f) { flash("Too close to a crossover - split further inside the band"); return; }
        beginGesture();
        // crossovers: insert hz at position b
        std::vector<float> xs; for (int i = 0; i < n - 1; ++i) xs.push_back(xoverHz(i));
        xs.insert(xs.begin() + b, hz);
        shiftBands(b + 1, +1);
        for (int f = 0; f < kBandFields; ++f) commit(bp(b + 1, f), val[bp(b, f)]);         // the new band starts as a copy
        commit(bp(b + 1, kBSolo), 0);
        for (int i = 0; i < (int)xs.size(); ++i) commit(kX1 + i, xs[i]);
        commit(kNumBands, (float)(n + 1));
        endGesture(); selBand = b + 1; flash("Band split at " + fmtHz(hz));
    }
    void removeBand(int b) {
        const int n = nb(); if (n <= 1) return;
        beginGesture();
        std::vector<float> xs; for (int i = 0; i < n - 1; ++i) xs.push_back(xoverHz(i));
        xs.erase(xs.begin() + (b < n - 1 ? b : b - 1));
        for (int s = 0; s < kNumSlots; ++s) { const int d = (int)std::round(val[sp(s, kSlotDst)]); if (d >= kDstBandFirst && d < kDstXFirst && (d - kDstBandFirst) / kNumBandDests == b) clearSlot(s); }
        shiftBands(b + 1, -1);
        for (int i = 0; i < (int)xs.size(); ++i) commit(kX1 + i, xs[i]);
        commit(kNumBands, (float)(n - 1));
        endGesture(); selBand = std::max(0, std::min(b, n - 2)); flash("Band removed");
    }
    static std::string fmtHz(float hz) { char b[24]; if (hz >= 1000) snprintf(b, sizeof b, "%.2f kHz", hz / 1000); else snprintf(b, sizeof b, "%.0f Hz", hz); return b; }

    // ---- modulation assign ---------------------------------------------------------
    void assign(int src, int dst) {
        int slot = -1; for (int s = 0; s < kNumSlots; ++s) if (!slotUsed(s)) { slot = s; break; }
        if (slot < 0) { flash("All 50 modulation slots are in use - remove one first"); return; }
        beginGesture();
        commit(sp(slot, kSlotSrc), (float)src); commit(sp(slot, kSlotDst), (float)dst); commit(sp(slot, kSlotAmt), 0.3f); commit(sp(slot, kSlotBypass), 0);
        endGesture();
        selSlot = slot; const int pos = (int)usedSlots().size() - 1; if (pos / 2 >= slotScroll + SLOT_ROWS) slotScroll = pos / 2 - SLOT_ROWS + 1;
        flash(std::string(sourceName(src)) + " > " + destName(dst));
    }
    void clearSlot(int s) { commit(sp(s, kSlotSrc), 0); commit(sp(s, kSlotDst), 0); commit(sp(s, kSlotAmt), 0); commit(sp(s, kSlotBypass), 0); }

    // ---- hit testing ----------------------------------------------------------------
    static bool inRect(float x, float y, float rx, float ry, float rw, float rh) { return x >= rx && x < rx + rw && y >= ry && y < ry + rh; }
    int hit(float x, float y, int* seg = nullptr) const {
        const std::vector<Ctl>& cs = controls();
        for (int i = 0; i < (int)cs.size(); ++i) {
            const Ctl& c = cs[i]; if (!visible(c)) continue;
            switch (c.type) {
            case Knob: { const float rr = c.r + 6; if ((x - c.x) * (x - c.x) + (y - c.y) * (y - c.y) <= rr * rr) return i; break; }
            case Toggle: if (x >= c.x - 18 && x <= c.x + 70 && std::fabs(y - c.y) <= 11) return i; break;
            case Segment: { int n = 0; segLabels(c.group, c.field, n); const float x0 = c.x - c.r / 2;
                if (x >= x0 && x < x0 + c.r && std::fabs(y - c.y) <= SEG_H / 2 + 1) { if (seg) *seg = (int)((x - x0) / (c.r / n)); return i; } break; }
            case Stepper: if (std::fabs(x - c.x) <= c.r / 2 && std::fabs(y - c.y) <= 11) { if (seg) *seg = x < c.x ? -1 : 1; return i; } break;
            }
        }
        return -1;
    }
    int headerHit(float x, float y) const {
        if (y < PS_Y || y > PS_Y + PS_H) return 0;
        if (x >= LEARN_X && x <= LEARN_X + LEARN_W) return 1;
        if (x >= MIDI_X && x <= MIDI_X + MIDI_W) return 2;
        if (x >= UNDO_X && x <= UNDO_X + UR_W) return 3;
        if (x >= REDO_X && x <= REDO_X + UR_W) return 4;
        if (x >= HELP_X && x <= HELP_X + HELP_W) return 5;
        if (x >= A_X && x < A_X + AB_W) return 6;
        if (x >= A_X + AB_W && x < A_X + 2 * AB_W) return 7;
        if (x >= COPY_X && x <= COPY_X + COPY_W) return 8;
        return 0;
    }
    int presetBarHit(float x, float y) const { if (y < PS_Y || y > PS_Y + PS_H || x < PS_X || x > PS_X + PS_W) return -1; return x < PS_X + PS_ARROW ? 0 : (x > PS_X + PS_W - PS_ARROW ? 2 : 1); }
    bool inDisplay(float x, float y) const { return inRect(x, y, DX + 2, DY + 2, DW - 4, DH - 20); }
    int bandAt(float x) const { const float hz = hzOfX(x); for (int b = 0; b < nb(); ++b) if (hz < bandHi(b) || b == nb() - 1) return b; return nb() - 1; }
    int divHit(float x, float y) const { if (!inDisplay(x, y)) return -1; for (int i = 0; i < nb() - 1; ++i) if (std::fabs(x - xOfHz(xoverHz(i))) <= 6) return i; return -1; }
    bool driveHit(float x, float y) const { if (!inDisplay(x, y)) return false; const int b = bandAt(x); return std::fabs(y - driveY(b)) <= 7; }
    int tabHit(float x, float y) const { if (std::fabs(y - TAB_Y) > TAB_H / 2) return -1; for (int t = 0; t < kNumTabs; ++t) { const float tx = MX + 10 + t * (TAB_W + 1.6f); if (x >= tx && x < tx + TAB_W) return t; } return -1; }
    int chipHit(float x, float y) const { for (const Chip& c : chips()) if (inRect(x, y, c.x, c.y, CHIP_W, CHIP_H)) return c.src; return -1; }
    bool styleHit(float x, float y) const { return inRect(x, y, STYLE_X, BY + 12, STYLE_W, 26); }
    bool removeHit(float x, float y) const { return nb() > 1 && inRect(x, y, REM_X, BY + 12, 64, 26); }
    bool stepHit(float x, float y) const { return tab <= 3 && val[lp_(tab, kLMode)] > 0.5f && inRect(x, y, STEP_X, STEP_Y, STEP_W, STEP_H); }
    bool xyHit(float x, float y) const { return tab >= 8 && tab <= 9 && inRect(x, y, XY_X, XY_Y, XY_W, XY_H); }
    bool clearHit(float x, float y) const { return learnMode && learnWaiting < 0 && inRect(x, y, DX + 10, DY + DH - 40, 170, 20); }
    void setStepAt(float x, float y, bool reset) {
        const int l = tab, s = (int)clampf((x - STEP_X) / (STEP_W / kNumSteps), 0, kNumSteps - 1);
        const float v = reset ? 0.f : clampf((STEP_Y + STEP_H / 2 - y) / (STEP_H / 2 - 6), -1.f, 1.f);
        setValue(lp_(l, kLStep1 + s), std::round(v * 20.f) / 20.f);
    }
    void setXyAt(float x, float y) { const int k = tab - 8; setValue(xyp(k, 0), clampf((x - XY_X - 8) / (XY_W - 16), 0.f, 1.f)); setValue(xyp(k, 1), clampf((XY_Y + XY_H - 8 - y) / (XY_H - 16), 0.f, 1.f)); }

    void setLearn(bool on) { learnMode = on; learnWaiting = -1; if (!on) setState("cmd", "cancel"); repaint(); }
    void endEdit(bool apply) {
        if (editing < 0) return;
        const uint32_t p = effParam(controls()[editing]); float v;
        if (apply && parse(p, editBuf, v)) { beginGesture(); commit(p, v); endGesture(); }
        editing = -1; editBuf.clear(); repaint();
    }
    static float angleNorm(const Ctl& c, float x, float y) {
        float a = std::atan2(y - c.y, x - c.x); const float a0 = 0.75f * 3.14159265f;
        float rel = a - a0; while (rel < 0) rel += 2 * 3.14159265f;
        if (rel > 1.5f * 3.14159265f) rel = rel > 1.75f * 3.14159265f ? 0.f : 1.5f * 3.14159265f;
        return rel / (1.5f * 3.14159265f);
    }

    bool onKeyboard(const KeyboardEvent& ev) override {
        if (!ev.press) return false;
        std::string* buf = saving ? &saveName : (editing >= 0 ? &editBuf : nullptr);
        if (!buf && menuOpen) {                              // typing in the preset browser searches
            if (ev.key == kKeyEscape) { if (!query.empty()) query.clear(); else menuOpen = false; }
            else if (ev.key == kKeyBackspace || ev.key == kKeyDelete) { if (!query.empty()) query.pop_back(); }
            else if (ev.key >= 32 && ev.key < 127 && query.size() < 32) query += (char)ev.key;
            else return false;
            menuHover = -1; pendingDelete = -1; repaint(); return true;
        }
        if (!buf) {
            const bool cmd = (ev.mod & (kModifierControl | kModifierSuper)) != 0;
            if (cmd && (ev.key == 'z' || ev.key == 'Z')) { if (ev.mod & kModifierShift) redo(); else undo(); return true; }
            if (cmd && (ev.key == 'y' || ev.key == 'Y')) { redo(); return true; }
            if (ev.key == kKeyEscape) { if (helpOpen) helpOpen = false; else if (popup.open) popup.open = false; else if (kpan.open) kpan.open = false; else if (learnMode) setLearn(false); repaint(); return true; }
            return false;
        }
        if (ev.key == kKeyEnter) {
            if (saving && savingComp) {
                float fv[kXlfoFields]; for (int f = 0; f < compFieldCount(savingCompKind); ++f) fv[f] = val[compParam(savingCompKind, savingCompIdx, f)];
                if (saveCompPreset(savingCompKind, saveName, fv)) { saving = savingComp = false; saveError = false; flash("Saved " + sanitizeName(saveName)); } else saveError = true; }
            else if (saving) { if (saveUserPreset(saveName, val)) { userPresets = listUserPresets(); curUser = sanitizeName(saveName); curFactory = -1; std::copy(val, val + kParamCount, userVals); saving = false; saveError = false; } else saveError = true; }
            else endEdit(true);
            repaint(); return true;
        }
        if (ev.key == kKeyEscape) { if (saving) saving = savingComp = false; else endEdit(false); repaint(); return true; }
        if (ev.key == kKeyBackspace || ev.key == kKeyDelete) { if (!buf->empty()) buf->pop_back(); repaint(); return true; }
        if (ev.key >= 32 && ev.key < 127 && buf->size() < (saving ? 40u : 12u)) { *buf += (char)ev.key; repaint(); return true; }
        return true;
    }

    bool onMouse(const MouseEvent& ev) override {
        const float s = uiScale(), x = ev.pos.getX() / s, y = ev.pos.getY() / s;
        if (saving) return true;
        if (ev.press) { statusFlash = 0; tipShown = false; tipSuppressed = tipKey; }
        if (helpOpen) { if (ev.press && ev.button == 1) helpClick(x, y); return true; }
        if (ev.button == 2 && ev.press) {                  // right-click (DPF numbers buttons 1 left, 2 right, 3 middle)
            if (popup.open && popup.kind == 3) { const int m = popupHit(x, y); compPendingDel = (m >= 0 && compItems[m].type == 1) ? m : -1; repaint(); return true; }
            if (menuOpen) { const int m = menuHit(x, y); if (m >= 0 && menuItem(m).kind == 1) { pendingDelete = menuItem(m).idx; repaint(); } return true; }
            if (learnMode) { const int i = hit(x, y); if (i >= 0) { const uint32_t p = effParam(controls()[i]); char b[24]; snprintf(b, sizeof b, "unmap:%u", p); setState("cmd", b); paramCC[p] = -1; repaint(); } return true; }
            const int dv = divHit(x, y); if (dv >= 0) { removeBand(dv + 1 < nb() ? dv : dv); return true; }
            { const int i = hit(x, y); if (i >= 0 && controls()[i].type == Knob) { popup.open = false; openKnobPanel(effParam(controls()[i]), x, y); return true; } }
            return false;
        }
        if (ev.button != 1) return false;
        if (!ev.press) {
            if (dragSrc > 0) { const int i = hit(x, y); if (i >= 0) { const int d = destForParam(effParam(controls()[i])); if (d > 0) assign(dragSrc, d); else flash("That control can't be modulated"); }
                else { const int sp_ = slotPartHit(x, y); if (sp_ >= 0 && sp_ % 10 == 3) assign(dragSrc, kDstSlotFirst + sp_ / 10); }
                dragSrc = -1; repaint(); return true; }
            if (active >= 0) { editParameter(effParam(controls()[active]), false); active = -1; endGesture(); return true; }
            if (activeSlotAmt >= 0) { editParameter(sp(activeSlotAmt, kSlotAmt), false); activeSlotAmt = -1; endGesture(); return true; }
            if (dragDiv >= 0) { editParameter(kX1 + dragDiv, false); dragDiv = -1; endGesture(); return true; }
            if (dragDriveBand >= 0) { editParameter(bp(dragDriveBand, kBDrive), false); dragDriveBand = -1; endGesture(); return true; }
            if (dragStep) { dragStep = false; endGesture(); return true; }
            if (dragXy) { dragXy = false; endGesture(); return true; }
            return false;
        }
        if (editing >= 0) endEdit(true);
        if (popup.open) { const int i = popupHit(x, y); if (i >= 0) popupPick(i); else if (!popupInside(x, y)) popup.open = false; repaint(); return true; }
        if (kpan.open) {
            const int part = kpHit(x, y);
            if (part < 0) { if (!kpInside(x, y)) kpan.open = false; repaint(); return true; }
            const int sl = part / 10, k = part % 10;
            if (k == 1) { popup.open = true; popup.kind = 1; popup.slot = sl; popup.x = x - 40; popup.y = y + 12; popup.hover = -1; repaint(); return true; }
            if (k == 3) { const uint32_t ap = sp(sl, kSlotAmt);
                if (ev.mod & (kModifierControl | kModifierSuper)) { beginGesture(); commit(ap, 0.f); endGesture(); return true; }
                beginGesture(); activeSlotAmt = sl; dragY = y; dragNorm = toNorm(ap, val[ap]); editParameter(ap, true); repaint(); return true; }
            if (k == 4) { beginGesture(); commit(sp(sl, kSlotBypass), val[sp(sl, kSlotBypass)] > 0.5f ? 0.f : 1.f); endGesture(); return true; }
            beginGesture(); clearSlot(sl); endGesture(); if (selSlot == sl) selSlot = -1;
            if (slotsFor(kpan.dest).empty()) kpan.open = false;
            const int maxScroll = std::max(0, ((int)usedSlots().size() + 1) / 2 - SLOT_ROWS); slotScroll = std::min(slotScroll, maxScroll); repaint(); return true;
        }
        if (menuOpen) {
            const int m = menuHit(x, y);
            if (m >= 0) { const MRect r = menuItem(m);
                if (r.kind == 3) return true;                       // the search field: just type
                if ((r.kind == 0 || r.kind == 1 || r.kind == 4) && x >= r.x + r.w - 20) { toggleFav(favKey(r.kind, r.idx)); return true; }
                if (r.kind == 2) { menuOpen = false; saving = true; saveError = false; saveName = curUser; }
                else if (r.kind == 1 && pendingDelete == r.idx) { deleteUserPreset(userPresets[r.idx]); if (curUser == userPresets[r.idx]) curUser.clear(); userPresets = listUserPresets(); pendingDelete = -1; repaint(); return true; }
                else { menuOpen = false; if (r.kind == 0) applyFactory(r.idx); else if (r.kind == 1) applyUser(userPresets[r.idx]); else applyFavourite(r.idx); } }
            else menuOpen = false;
            pendingDelete = -1; menuHover = -1; repaint(); return true;
        }
        const int hh = headerHit(x, y);
        if (hh == 1) { setLearn(!learnMode); return true; }
        if (hh == 2) { beginGesture(); commit(kMidiOn, val[kMidiOn] > 0.5f ? 0.f : 1.f); endGesture(); return true; }
        if (hh == 3) { undo(); return true; } if (hh == 4) { redo(); return true; }
        if (hh == 5) { helpOpen = true; repaint(); return true; }
        if (hh == 6 || hh == 7) { switchAB(hh - 6); return true; } if (hh == 8) { copyAB(); return true; }
        const int pb = presetBarHit(x, y);
        if (pb == 0) { step(-1); return true; } if (pb == 2) { step(1); return true; }
        if (pb == 1) { openMenu(); return true; }
        if (clearHit(x, y)) { setState("cmd", "clear"); for (int& m : paramCC) m = -1; repaint(); return true; }
        const int tb = tabHit(x, y); if (tb >= 0) { tab = tb; repaint(); return true; }
        if (compHit(x, y) && !learnMode) { openCompMenu(); return true; }
        if (!learnMode) { const int ch = chipHit(x, y); if (ch > 0) { dragSrc = ch; dragPX = x; dragPY = y; repaint(); return true; } }
        if (styleHit(x, y) && !learnMode) { popup.open = true; popup.kind = 0; popup.slot = -1; popup.x = STYLE_X; popup.y = BY + 42; popup.hover = -1; repaint(); return true; }
        if (removeHit(x, y) && !learnMode) { removeBand(selBand); return true; }
        // slot rows
        const int part = slotPartHit(x, y);
        if (part >= 0 && !learnMode) {
            const int sl = part / 10, k = part % 10; selSlot = sl;
            if (k == 1 || k == 2) { popup.open = true; popup.kind = k; popup.slot = sl; popup.x = x - 40; popup.y = y + 12; popup.hover = -1; repaint(); return true; }
            if (k == 3) { const uint32_t ap = sp(sl, kSlotAmt);
                if (ev.mod & (kModifierControl | kModifierSuper)) { beginGesture(); commit(ap, 0.f); endGesture(); return true; }
                beginGesture(); activeSlotAmt = sl; dragY = y; dragNorm = toNorm(ap, val[ap]); editParameter(ap, true); repaint(); return true; }
            if (k == 4) { beginGesture(); commit(sp(sl, kSlotBypass), val[sp(sl, kSlotBypass)] > 0.5f ? 0.f : 1.f); endGesture(); return true; }
            if (k == 5) { beginGesture(); clearSlot(sl); endGesture(); selSlot = -1; const int maxScroll = std::max(0, ((int)usedSlots().size() + 1) / 2 - SLOT_ROWS); slotScroll = std::min(slotScroll, maxScroll); return true; }
        }
        int seg = 0; const int i = hit(x, y, &seg);
        if (i >= 0 && !learnMode && ringHit(controls()[i], x, y)) { openKnobPanel(effParam(controls()[i]), x, y); return true; }
        if (i >= 0) {
            const Ctl& c = controls()[i]; const uint32_t p = effParam(c);
            if (learnMode) { learnWaiting = (int)p; char b[24]; snprintf(b, sizeof b, "learn:%u", p); setState("cmd", b); repaint(); return true; }
            if (c.type == Toggle) { beginGesture(); commit(p, val[p] > 0.5f ? 0.f : 1.f); endGesture(); return true; }
            if (c.type == Segment) {
                beginGesture();
                if (c.group == GGlobal && c.field == kQuality) { commit(kQuality, seg > 0 ? 1.f : 0.f); commit(kSuperb, seg == 2 ? 1.f : 0.f); }
                else if (c.group == GGlobal && c.field == kXSlope) commit(kXSlope, seg == 0 ? (float)kSlope6 : (float)(seg - 1));
                else commit(p, (float)seg);
                endGesture(); return true; }
            if (c.type == Stepper) { const ParamDef& d = paramDef(p); float v = val[p] + seg; if (v > d.max) v = d.min; if (v < d.min) v = d.max; beginGesture(); commit(p, v); endGesture(); return true; }
            if (ev.mod & (kModifierControl | kModifierSuper)) { beginGesture(); commit(p, defv[p]); endGesture(); return true; }
            if (i == lastClick && ev.time - lastClickTime < 350) {
                lastClick = -1; editing = i; char b[40]; fmt(p, b, sizeof b); editBuf = b;
                const size_t spc = editBuf.find(' '); if (spc != std::string::npos) editBuf.resize(spc);
                if (!editBuf.empty() && editBuf.back() == '%') editBuf.pop_back();
                repaint(); return true;
            }
            lastClick = i; lastClickTime = ev.time;
            beginGesture(); active = i; dragY = y; dragNorm = toNorm(p, val[p]);
            const float dist = std::sqrt((x - c.x) * (x - c.x) + (y - c.y) * (y - c.y));
            circular = dist > c.r * 0.62f && c.r > 15;
            editParameter(p, true);
            if (circular) setValue(p, fromNorm(p, angleNorm(c, x, y)));
            repaint(); return true;
        }
        if (stepHit(x, y)) { beginGesture(); dragStep = true; setStepAt(x, y, (ev.mod & (kModifierControl | kModifierSuper)) != 0); return true; }
        if (xyHit(x, y)) { beginGesture(); dragXy = true; setXyAt(x, y); return true; }
        if (!learnMode && inDisplay(x, y)) {
            const int dv = divHit(x, y);
            if (dv >= 0) { beginGesture(); dragDiv = dv; editParameter(kX1 + dv, true); return true; }
            const int b = bandAt(x);
            if (ev.time - lastDispClick < 350 && std::fabs(x - lastDispX) < 6) { lastDispClick = 0; addBandAt(hzOfX(x)); return true; }
            lastDispClick = ev.time; lastDispX = x;
            selBand = b;
            beginGesture(); dragDriveBand = b; dragY = y; dragNorm = val[bp(b, kBDrive)]; editParameter(bp(b, kBDrive), true);
            repaint(); return true;
        }
        return false;
    }

    bool onMotion(const MotionEvent& ev) override {
        const float s = uiScale(), x = ev.pos.getX() / s, y = ev.pos.getY() / s;
        if (saving) return true;
        if (helpOpen) { const int h = helpHit(x, y); if (h != helpHover) { helpHover = h; repaint(); } return true; }
        if (dragSrc > 0) { dragPX = x; dragPY = y; hover = hit(x, y); repaint(); return true; }
        if (popup.open) { const int h = popupHit(x, y); if (h != popup.hover) { popup.hover = h; repaint(); } return true; }
        if (kpan.open && activeSlotAmt < 0) { const int h = kpHit(x, y); if (h != hoverKp) { hoverKp = h; repaint(); } return true; }
        const float fine = (ev.mod & kModifierShift) ? 0.2f : 1.f;
        if (active >= 0) {
            const Ctl& c = controls()[active]; const uint32_t p = effParam(c);
            if (circular) { setValue(p, fromNorm(p, angleNorm(c, x, y))); return true; }
            dragNorm = clampf(dragNorm + (dragY - y) / 240.f * fine, 0.f, 1.f); dragY = y; setValue(p, fromNorm(p, dragNorm)); return true;
        }
        if (activeSlotAmt >= 0) { const uint32_t ap = sp(activeSlotAmt, kSlotAmt); dragNorm = clampf(dragNorm + (dragY - y) / 400.f * fine, 0.f, 1.f); dragY = y; setValue(ap, fromNorm(ap, dragNorm)); return true; }
        if (dragDiv >= 0) {
            const float lo = dragDiv == 0 ? 20.f : xoverHz(dragDiv - 1), hi = dragDiv == nb() - 2 ? 20000.f : xoverHz(dragDiv + 1);
            setValue(kX1 + dragDiv, clampf(hzOfX(x), lo * 1.26f, hi / 1.26f)); return true;
        }
        if (dragDriveBand >= 0) { dragNorm = clampf(dragNorm + (dragY - y) / (DH - 74) * fine, 0.f, 1.f); dragY = y; setValue(bp(dragDriveBand, kBDrive), dragNorm); return true; }
        if (dragStep) { setStepAt(x, y, false); return true; }
        if (dragXy) { setXyAt(x, y); return true; }
        if (menuOpen) { const int m = menuHit(x, y); if (m != menuHover) { menuHover = m; repaint(); } return true; }
        const int hh = headerHit(x, y); if (hh != hoverHeader) { hoverHeader = hh; repaint(); }
        const int pb = presetBarHit(x, y); if (pb != presetHover) { presetHover = pb; repaint(); }
        const bool hc = clearHit(x, y); if (hc != hoverClear) { hoverClear = hc; repaint(); }
        const int h = hit(x, y); if (h != hover) { hover = h; repaint(); }
        const int dv = divHit(x, y); if (dv != hoverDiv) { hoverDiv = dv; repaint(); }
        const int hb = inDisplay(x, y) ? bandAt(x) : -1; if (hb != hoverBand) { hoverBand = hb; repaint(); }
        const int tb = tabHit(x, y); if (tb != hoverTab) { hoverTab = tb; repaint(); }
        const int ch = chipHit(x, y); if (ch != hoverChip) { hoverChip = ch; repaint(); }
        const bool hcp = compHit(x, y); if (hcp != hoverComp) { hoverComp = hcp; repaint(); }
        const bool hs = styleHit(x, y); if (hs != hoverStyle) { hoverStyle = hs; repaint(); }
        const bool hr = removeHit(x, y); if (hr != hoverRemove) { hoverRemove = hr; repaint(); }
        const int part = slotPartHit(x, y); if (part != hoverSlotPart) { hoverSlotPart = part; repaint(); }
        // tips
        int key = -1;
        if (h >= 0 && ringHit(controls()[h], x, y)) key = 4008; else if (h >= 0) key = h; else if (hh > 0) key = 1000 + hh; else if (pb >= 0) key = 2000 + pb;
        else if (ch > 0) key = 3000 + ch; else if (dv >= 0) key = 4000; else if (hs) key = 4001; else if (hr) key = 4002;
        else if (hcp) key = 4007;
        else if (stepHit(x, y)) key = 4003; else if (xyHit(x, y)) key = 4004; else if (tb >= 0) key = 4005; else if (part >= 0) key = 5000 + part; else if (hb >= 0) key = 4006;
        tipX = x; tipY = y;
        if (key != tipKey) { const bool was = tipShown; tipKey = key; tipSince = nowSec(); tipShown = false; if (key != tipSuppressed) tipSuppressed = -2; if (was) repaint(); }
        return false;
    }

    bool onScroll(const ScrollEvent& ev) override {
        const float s = uiScale(), sx = ev.pos.getX() / s, sy = ev.pos.getY() / s;
        if (menuOpen || saving || helpOpen || popup.open) return true;
        if (presetBarHit(sx, sy) >= 0) { step(ev.delta.getY() > 0 ? -1 : 1); return true; }
        if (inRect(sx, sy, SX, SY, SW, SH)) { const int maxScroll = std::max(0, ((int)usedSlots().size() + 1) / 2 - SLOT_ROWS);
            slotScroll = (int)clampf((float)(slotScroll + (ev.delta.getY() > 0 ? -1 : 1)), 0.f, (float)maxScroll); repaint(); return true; }
        const float st = (ev.mod & kModifierShift) ? 0.004f : 0.02f, d = ev.delta.getY() > 0 ? st : -st;
        const int i = hit(sx, sy);
        if (i >= 0 && controls()[i].type == Knob) { const uint32_t p = effParam(controls()[i]); beginGesture(); commit(p, fromNorm(p, toNorm(p, val[p]) + d)); endGesture(); return true; }
        if (inDisplay(sx, sy)) { const int b = bandAt(sx); beginGesture(); commit(bp(b, kBDrive), clampf(val[bp(b, kBDrive)] + d, 0.f, 1.f)); endGesture(); return true; }
        return false;
    }

    void uiIdle() override {
        const double t = nowSec();
        if (t - lastPoll > 0.033) {
            lastPoll = t;
            if (MagmaPlugin* const dsp = (MagmaPlugin*)getPluginInstancePointer()) {
                const Engine& e = dsp->engine;
                std::copy(e.modOff, e.modOff + kNumDests, liveOff); std::copy(e.srcVal, e.srcVal + kNumSources, liveSrc);
                for (int b = 0; b < kMaxBands; ++b) liveLevel[b] = e.bandLevel[b];
                for (int l = 0; l < kNumXlfo; ++l) livePhase[l] = phaseOf(e, l);
                latencyMs = e.latency() * 1000.f / std::max(1.f, e.sampleRate());
                const uint32_t w = e.anaWrite.load();
                if (w != lastAna) { lastAna = w; analyse(e, w); }
            }
            repaint();
        }
        if (gTipsOn && !tipShown && tipKey >= 0 && tipKey != tipSuppressed && active < 0 && dragSrc <= 0 && dragDiv < 0 && dragDriveBand < 0 && !menuOpen && !saving && !helpOpen && !popup.open && !kpan.open && editing < 0
            && t - tipSince > 0.6) { tipShown = true; repaint(); }
    }
    // the XLFO phase is not exposed; estimate it from the source value's history for the display
    double phaseOf(const Engine&, int l) { phaseEst[l] = std::fmod(phaseEst[l] + 0.033 * rateHz(l), 1.0); return phaseEst[l]; }
    double rateHz(int l) const {
        const int sync = (int)std::round(val[lp_(l, kLSync)]);
        if (sync == 0) return val[lp_(l, kLRate)];
        return 120.0 / 60.0 / 4.0 / (divBars((int)std::round(val[lp_(l, kLDiv)])) * syncFactor(sync));
    }
    void analyse(const Engine& e, uint32_t w) {
        const float sr = e.sampleRate();
        for (int which = 0; which < 2; ++which) {
            const float* src = which ? e.anaOut : e.anaIn;
            for (int i = 0; i < ANA_N; ++i) { const float hann = 0.5f - 0.5f * std::cos(2 * kPi * i / (ANA_N - 1)); buf[i] = cpx(src[(w - ANA_N + i + Engine::kAna) % Engine::kAna] * hann, 0); }
            fft.run(buf, false);
            float* sp_ = which ? specOut : specIn;
            for (int k = 0; k < ANA_PTS; ++k) {
                const float f0 = 20.f * std::pow(1000.f, k / (float)ANA_PTS), f1 = 20.f * std::pow(1000.f, (k + 1) / (float)ANA_PTS);
                int b0 = (int)(f0 / sr * ANA_N), b1 = std::max(b0 + 1, (int)(f1 / sr * ANA_N)); b1 = std::min(b1, ANA_N / 2);
                float m = 0; for (int b = b0; b < b1; ++b) m = std::max(m, std::norm(buf[b]));
                const float tilt = 4.5f * std::log2(std::max(f0, 20.f) / 1000.f);         // ~4.5 dB/oct tilt so a balanced mix looks flat
                const float db = 10.f * std::log10(m + 1e-20f) - 20.f * std::log10(ANA_N / 4.f) + tilt;
                sp_[k] = db > sp_[k] ? db : sp_[k] + (db - sp_[k]) * 0.25f;
            }
        }
    }

    // ---- tips --------------------------------------------------------------------
    bool tipContent(std::string& title, std::string& value, std::string& body) const {
        if (tipKey < 0) return false;
        char b[64];
        if (tipKey < 1000) { const uint32_t p = effParam(controls()[tipKey]); fmt(p, b, sizeof b); title = paramDef(p).name; value = b; body = paramTip(p);
            if (controls()[tipKey].type == Knob) body += "\n\nDrag up/down or grab the rim. Double-click to type, Cmd/Ctrl-click to reset. Drop a modulation chip here to modulate it; right-click to list the slots that modulate it.";
            if (paramCC[p] >= 0) { char cc[24]; snprintf(cc, sizeof cc, "   ·   CC%d", paramCC[p]); value += cc; }
            return true; }
        if (tipKey >= 5000) { const int sl = (tipKey - 5000) / 10, k = (tipKey - 5000) % 10; static const int f[6] = { 0, kSlotSrc, kSlotDst, kSlotAmt, kSlotBypass, kSlotSrc };
            title = "Slot " + std::to_string(sl + 1); value.clear(); body = k == 5 ? "Removes this modulation." : paramTip(sp(sl, f[k])); return true; }
        if (tipKey >= 3000 && tipKey < 4000) { title = sourceName(tipKey - 3000); value.clear(); body = targetTip(kTipChip); return true; }
        int t = -1;
        switch (tipKey) { case 4000: t = kTipDivider; break; case 4001: t = -2; break; case 4002: t = kTipRemoveBand; break; case 4003: t = kTipStepEdit; break;
                          case 4004: t = kTipXyPad; break; case 4005: t = kTipSourceTab; break; case 4006: t = kTipDisplay; break; case 4007: t = kTipCompPresets; break; case 4008: t = kTipModRing; break; default: break; }
        if (t == -2) { const int st = (int)std::round(val[bp(selBand, kBStyle)]); title = "Style"; value = styleName(st); body = std::string(styleTip(st)) + "\n\nClick to choose another style."; return true; }
        if (t < 0 && tipKey >= 2000) t = kTipPrev + (tipKey - 2000);
        if (t < 0) { const int hh = tipKey - 1000; t = hh == 1 ? kTipLearn : hh == 2 ? kTipMidi : hh == 3 ? kTipUndo : hh == 4 ? kTipRedo : hh == 5 ? kTipHelp : hh == 6 ? kTipA : hh == 7 ? kTipB : kTipCopy; }
        title = targetTitle(t); value.clear(); body = targetTip(t);
        if (t == kTipUndo || t == kTipRedo) { const size_t n = t == kTipUndo ? undoStack.size() : redoStack.size(); snprintf(b, sizeof b, "%d step%s", (int)n, n == 1 ? "" : "s"); value = b; }
        return true;
    }
    void drawTooltip() {
        std::string title, value, body; if (!tipContent(title, value, body)) return;
        const float w = 310, pad = 12;
        fontSize(11); textLetterSpacing(0.2f); textLineHeight(1.3f);
        float bb[4]; textBoxBounds(0, 0, w - 2 * pad, body.c_str(), nullptr, bb);
        const float h = pad + 16 + 8 + (bb[3] - bb[1]) + pad;
        float x = tipX + 16, y = tipY + 20;
        if (x + w > W - 8) x = tipX - w - 12;
        if (y + h > H - 8) y = tipY - h - 12;
        x = clampf(x, 8, W - w - 8); y = clampf(y, 8, H - h - 8);
        beginPath(); rect(x - 10, y - 4, w + 20, h + 18); fillPaint(boxGradient(x, y + 3, w, h, 6, 14, Color(0, 0, 0, 0.55f), Color(0, 0, 0, 0.f))); fill();
        beginPath(); roundedRect(x, y, w, h, 6); fillColor(Color(0x22, 0x1d, 0x1a)); fill(); strokeColor(C_LINE); strokeWidth(1); stroke();
        fontSize(10); textLetterSpacing(1.6f); textAlign(ALIGN_LEFT | ALIGN_TOP); fillColor(C_TEXT);
        std::string up = title; for (char& ch : up) ch = (char)std::toupper((unsigned char)ch); text(x + pad, y + pad + 1, up.c_str(), nullptr);
        if (!value.empty()) { fontSize(10.5f); textLetterSpacing(0.4f); textAlign(ALIGN_RIGHT | ALIGN_TOP); fillColor(C_ACC); text(x + w - pad, y + pad + 1, value.c_str(), nullptr); }
        fontSize(11); textLetterSpacing(0.2f); textLineHeight(1.3f); textAlign(ALIGN_LEFT | ALIGN_TOP); fillColor(C_DIM); textBox(x + pad, y + pad + 24, w - 2 * pad, body.c_str(), nullptr);
        textLineHeight(1.f);
    }

    // ---- help window ---------------------------------------------------------------
    static constexpr float HX = 220, HY = 110, HW = 760, HH = 540, HTAB_W = 170, HTAB_H = 34;
    int helpHit(float x, float y) const {
        if (x < HX || x > HX + HW || y < HY || y > HY + HH) return -2;
        for (int i = 0; i < kNumHelpPages; ++i) if (x >= HX + 12 && x < HX + 12 + HTAB_W && y >= HY + 56 + i * HTAB_H && y < HY + 56 + (i + 1) * HTAB_H) return i;
        const float by = HY + HH - 44;
        if (y >= by && y < by + 28) { if (x >= HX + 12 && x < HX + 12 + HTAB_W) return 100; if (x >= HX + HW - 330 && x < HX + HW - 140) return 101; if (x >= HX + HW - 124 && x < HX + HW - 16) return 102; }
        if (x >= HX + HW - 40 && y < HY + 40) return 102;
        return -1;
    }
    void helpClick(float x, float y) {
        const int h = helpHit(x, y);
        if (h >= 0 && h < kNumHelpPages) helpPage = h; else if (h == 100) gTipsOn = !gTipsOn; else if (h == 101) openURL(kManualURL); else if (h == 102 || h == -2) helpOpen = false;
        repaint();
    }
    void drawHelp() {
        beginPath(); rect(0, 0, W, H); fillColor(Color(0, 0, 0, 0.55f)); fill();
        beginPath(); roundedRect(HX, HY, HW, HH, 10); fillColor(C_PANEL); fill(); strokeColor(C_LINE); strokeWidth(1); stroke();
        fontSize(13); textLetterSpacing(4.f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(C_TEXT); text(HX + 22, HY + 28, "MAGMA HELP", nullptr);
        const float cx = HX + HW - 24, cy = HY + 26;
        beginPath(); moveTo(cx - 6, cy - 6); lineTo(cx + 6, cy + 6); moveTo(cx + 6, cy - 6); lineTo(cx - 6, cy + 6); strokeColor(helpHover == 102 ? C_TEXT : C_DIM); strokeWidth(1.6f); stroke();
        for (int i = 0; i < kNumHelpPages; ++i) {
            const float ty = HY + 56 + i * HTAB_H;
            if (i == helpPage || i == helpHover) { beginPath(); roundedRect(HX + 12, ty + 2, HTAB_W, HTAB_H - 4, 5); fillColor(i == helpPage ? withAlpha(C_ACC, 0.18f) : C_PANEL_HI); fill(); }
            if (i == helpPage) { beginPath(); roundedRect(HX + 12, ty + 8, 3, HTAB_H - 16, 1.5f); fillColor(C_ACC); fill(); }
            fontSize(11.5f); textLetterSpacing(0.6f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(i == helpPage ? C_TEXT : C_DIM); text(HX + 28, ty + HTAB_H / 2 + 0.5f, kHelpPages[i].title, nullptr);
        }
        beginPath(); moveTo(HX + HTAB_W + 26, HY + 56); lineTo(HX + HTAB_W + 26, HY + HH - 56); strokeColor(C_LINE); strokeWidth(1); stroke();
        const float bx = HX + HTAB_W + 46, bw = HW - HTAB_W - 70;
        fontSize(15); textLetterSpacing(0.8f); textAlign(ALIGN_LEFT | ALIGN_TOP); fillColor(C_ACC); text(bx, HY + 60, kHelpPages[helpPage].title, nullptr);
        fontSize(12); textLetterSpacing(0.2f); textLineHeight(1.38f); fillColor(Color(0xcc, 0xc2, 0xba)); textBox(bx, HY + 92, bw, kHelpPages[helpPage].body, nullptr); textLineHeight(1.f);
        const float by = HY + HH - 44;
        beginPath(); moveTo(HX + 12, by - 10); lineTo(HX + HW - 12, by - 10); strokeColor(C_LINE); stroke();
        const float tw = 34, th = 18, tx = HX + 16, tyy = by + 14 - th / 2;
        beginPath(); roundedRect(tx, tyy, tw, th, th / 2); fillColor(gTipsOn ? withAlpha(C_ACC, 0.25f) : C_PANEL_HI); fill(); strokeColor(gTipsOn ? C_ACC : C_LINE); stroke();
        beginPath(); circle(gTipsOn ? tx + tw - th / 2 : tx + th / 2, by + 14, 6); fillColor(gTipsOn ? C_ACC : C_DIM); fill();
        fontSize(10); textLetterSpacing(1.6f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(helpHover == 100 ? C_TEXT : C_DIM); text(tx + tw + 10, by + 14.5f, "HOVER TIPS", nullptr);
        auto btn = [&](float x0, float w0, const char* label, bool hot, bool primary) {
            beginPath(); roundedRect(x0, by, w0, 28, 5); fillColor(primary ? (hot ? withAlpha(C_ACC, 0.32f) : withAlpha(C_ACC, 0.16f)) : (hot ? C_PANEL_HI : C_PANEL)); fill();
            strokeColor(primary ? C_ACC : C_LINE); strokeWidth(1); stroke();
            fontSize(9.5f); textLetterSpacing(1.8f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(C_TEXT); text(x0 + w0 / 2, by + 14.5f, label, nullptr); };
        btn(HX + HW - 330, 190, "FULL MANUAL (WEB)", helpHover == 101, false);
        btn(HX + HW - 124, 108, "CLOSE", helpHover == 102, true);
    }

private:
    float val[kParamCount], defv[kParamCount], userVals[kParamCount] = {}, before[kParamCount];
    int paramCC[kParamCount];
    int active = -1, hover = -1, lastClick = -1, editing = -1;
    uint lastClickTime = 0, lastDispClick = 0; float lastDispX = 0;
    float dragY = 0, dragNorm = 0; bool circular = false, gesture = false;
    int selBand = 0, hoverBand = -1, hoverDiv = -1, dragDiv = -1, dragDriveBand = -1, tab = 0, hoverTab = -1;
    int dragSrc = -1, hoverChip = -1; float dragPX = 0, dragPY = 0;
    int selSlot = -1, slotScroll = 0, hoverSlotPart = -1, activeSlotAmt = -1;
    bool hoverStyle = false, hoverRemove = false, dragStep = false, dragXy = false;
    std::vector<Snap> undoStack, redoStack; Snap abStore[2]; int abSlot = 0;
    std::string editBuf, statusText; int statusFlash = 0;
    int curFactory = 0, presetHover = -1, menuHover = -1, pendingDelete = -1, hoverHeader = 0;
    std::string curUser, query; std::vector<std::string> userPresets, favs;
    bool menuOpen = false, saving = false, saveError = false; std::string saveName;
    bool learnMode = false, hoverClear = false; int learnWaiting = -1; float lastLearnEcho = 0;
    int tipKey = -1, tipSuppressed = -2; double tipSince = 0; bool tipShown = false; float tipX = 0, tipY = 0;
    bool helpOpen = false; int helpPage = 0, helpHover = -1;
    float liveOff[kNumDests], liveSrc[kNumSources], liveLevel[kMaxBands] = {}; double livePhase[kNumXlfo] = {}, phaseEst[kNumXlfo] = {};
    float latencyMs = 0; double lastPoll = 0; uint32_t lastAna = 0;
    Fft fft; cpx buf[ANA_N]; float specIn[ANA_PTS], specOut[ANA_PTS];
    ResizeHandle resizeHandle;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MagmaUI)
};

UI* createUI() { return new MagmaUI(); }

END_NAMESPACE_DISTRHO
