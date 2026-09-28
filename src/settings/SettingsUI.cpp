
#include <Windows.h>
#include <Spore/ModAPI.h>
#include "core/SdkCompat.hpp"
#include "settings/Settings.hpp"
#include "localization/Localization.hpp"
#include <Spore/UTFWin/IButton.h>
#include <Spore/UTFWin/IWinProc.h>
#include <Spore/UTFWin/IWindowManager.h>
#include <Spore/UTFWin/SporeStdDrawable.h>
#include <Spore/UTFWin/Image.h>

namespace CivSettings {
namespace {
using WindowManagerFn = UTFWin::IWindowManager* (*)();
using ButtonFactoryFn = UTFWin::IWindow*(__stdcall*)(void*, void*);
WindowManagerFn windowManager;
ButtonFactoryFn buttonFactory;
using ImageLoadFn = bool (*)(const ResourceKey&, eastl::intrusive_ptr<UTFWin::Image>&, bool, int, int);
using ImageBackgroundFn = bool (*)(UTFWin::IWindow*, UTFWin::Image*, int);
ImageLoadFn imageLoad;
ImageBackgroundFn imageBackground;
const uint8_t* clipDrawing;
bool clipping = false;

enum Asset : unsigned {
    TrackOff = 1,
    TrackOn,
    KnobOff,
    KnobOn,
    IconCiv,
    IconRaids,
    IconTurrets,
    IconSpice,
    IconSuper,
    RowIdle,
    RowHover,
    IconCompliments,
    IconGifts,
    ScrollTrack,
    ScrollThumb,
    ScrollThumbHover,
    ChevronDown,
    ChevronRight,
    SectionDivider,
    BadgeOff,
    BadgeOn
};

UTFWin::Image* images[BadgeOn + 1]{};
bool imageTried[BadgeOn + 1]{};

constexpr uint32_t StripID = 0x043de590;
constexpr uint32_t HostID = 0x046967c0;
constexpr uint32_t CreditsID = 0x0473ffb8;
constexpr uint32_t OnlineID = 0x0473ffa8;
constexpr uint32_t TabID = 0xcc170001;
constexpr uint32_t PageID = 0xcc170002;
constexpr uint32_t RowID = 0xcc170010;
constexpr uint32_t ToggleID = 0xcc170020;
constexpr uint32_t HeaderID = 0xcc170030;
constexpr uint32_t ScrollID = 0xcc170040;
constexpr uint32_t CatcherID = 0xcc170041;
constexpr uint32_t FontID = 0x00aebb69;
constexpr uint32_t CenteredFontID = 0x00aebb70;
constexpr uint32_t IdleCaption = 0xffd8e2ee;
constexpr uint32_t SelectedCaption = 0xff1b2833;
constexpr uint32_t MutedCaption = 0xffa9b7c7;
constexpr uint32_t SectionCaption = 0xffd6be70;
constexpr uint32_t SectionHoverCaption = 0xfff4e4a0;
constexpr uint32_t BadgeOnCaption = 0xffe9d57d;
constexpr float RowHeight = 40, RowGap = 10, DetailHeight = 74;
constexpr float ViewTop = 50, ViewBottom = 354, ViewHeight = ViewBottom - ViewTop;
constexpr float HintTop = 364, StatusTop = 392;
constexpr float HeaderHeight = 26, HeaderGap = 6, SectionGap = 12;
constexpr float FadeBand = 18, WheelStep = 50, ThumbMinimum = 28, RailCenter = 7, Faint = 0.3f;
constexpr unsigned SectionCount = 3, SectionSize = 2;
const Asset ruleIcons[RuleCount] = {IconRaids, IconTurrets, IconSpice, IconSuper, IconCompliments, IconGifts};
const CivText::Text titleTexts[RuleCount] = {CivText::TitleLandRaids,   CivText::TitleTurrets,
                                             CivText::TitleSpice,       CivText::TitleSuperweapons,
                                             CivText::TitleCompliments, CivText::TitleGifts};

const CivText::Text descriptionTexts[RuleCount][3] = {
    {CivText::DescLandRaids1, CivText::DescLandRaids2, CivText::DescLandRaids3},
    {CivText::DescTurrets1, CivText::DescTurrets2, CivText::DescTurrets3},
    {CivText::DescSpice1, CivText::DescSpice2, CivText::DescSpice3},
    {CivText::DescSuperweapons1, CivText::DescSuperweapons2, CivText::DescSuperweapons3},
    {CivText::DescCompliments1, CivText::DescCompliments2, CivText::DescCompliments3},
    {CivText::DescGifts1, CivText::DescGifts2, CivText::DescGifts3}};

const unsigned sectionRules[SectionCount][SectionSize] = {
    {LandRaids, Superweapons}, {Turrets, Spice}, {Compliments, Gifts}};
const CivText::Text sectionTexts[SectionCount] = {CivText::SectionWarfare, CivText::SectionCities,
                                                  CivText::SectionDiplomacy};
bool sectionOpen[SectionCount] = {true, true, true};

UTFWin::Image* Image(Asset asset) {
    if (!imageTried[asset]) {
        imageTried[asset] = true;
        eastl::intrusive_ptr<UTFWin::Image> image;
        if (imageLoad(ResourceKey(asset, 0x2f7d0004, 0xcc17f000), image, false, -1, -1) && image) {
            images[asset] = image.get();
            images[asset]->AddRef();
        }
    }
    return images[asset];
}

bool Background(UTFWin::IWindow* window, Asset asset) {
    auto* image = Image(asset);
    return image && imageBackground(window, image, -1);
}

float Clamp(float value, float low, float high) {
    return value < low ? low : value > high ? high : value;
}

float Ease(float value) {
    if (value < 0)
        return 0;
    if (value > 1)
        return 1;
    return value * value * (3 - 2 * value);
}

float Glide(float value) {
    float rest = 1 - Clamp(value, 0, 1);
    return 1 - rest * rest * rest;
}

float Pixel(float value) {
    return float(int(value < 0 ? value - 0.5f : value + 0.5f));
}

uint32_t Shade(float alpha, uint32_t rgb = 0xffffff) {
    return uint32_t(Clamp(alpha, 0, 1) * 255 + 0.5f) << 24 | (rgb & 0xffffff);
}

float Exposure(float top, float bottom, float low, float high) {
    if (high <= low || bottom <= low || top >= high)
        return 0;
    float over = (top < low ? low - top : 0) + (bottom > high ? bottom - high : 0);
    if (clipping)
        return 1 - 0.5f * Clamp(over / (bottom - top), 0, 1);
    float visible = Clamp(1 - over / FadeBand, 0, 1);
    return visible * visible;
}

void Present(UTFWin::IWindow* window, float alpha) {
    uint32_t color = Shade(alpha);
    window->SetShadeColor(Math::Color(color));
    window->SetVisible((color >> 24) != 0);
}

unsigned SectionOf(unsigned rule) {
    for (unsigned s = 0; s < SectionCount; ++s)
        for (unsigned k = 0; k < SectionSize; ++k)
            if (sectionRules[s][k] == rule)
                return s;
    return 0;
}

struct Tween {
    float from = 0, to = 0, span = 1;
    DWORD start = 0;
    bool glide = false;

    float At(DWORD now) const {
        float t = float(now - start) / span;
        return from + (to - from) * (glide ? Glide(t) : Ease(t));
    }

    void Go(float target, DWORD now, float duration) {
        if (target == to)
            return;
        from = At(now);
        to = target;
        start = now;
        span = duration;
    }

    void Set(float value) {
        from = to = value;
    }
};

struct Layout {
    float header[SectionCount], block[SectionCount], full[SectionCount], shown[SectionCount];
    float row[RuleCount], detail[RuleCount];
    float height;
};

void Arrange(Layout& layout, const float* folds, const float* details) {
    float y = 0;
    for (unsigned s = 0; s < SectionCount; ++s) {
        layout.header[s] = y;
        y += HeaderHeight;
        layout.block[s] = y;
        float block = HeaderGap;
        for (unsigned k = 0; k < SectionSize; ++k) {
            unsigned i = sectionRules[s][k];
            float detail = Pixel(details[i]);
            if (k)
                block += RowGap;
            layout.row[i] = block;
            layout.detail[i] = detail;
            block += RowHeight + detail + (detail < 4 ? detail : 4);
        }
        layout.full[s] = block;
        layout.shown[s] = Pixel(block * folds[s]);
        y += layout.shown[s] + SectionGap;
    }
    layout.height = y;
}

float Overflow(const Layout& layout) {
    float extra = layout.height - SectionGap - ViewHeight;
    return extra > 0 ? extra + (extra < SectionGap ? extra : SectionGap) : 0;
}

UTFWin::IWindow* Child(UTFWin::IWindowList_t::iterator it) {
    return static_cast<UTFWin::IWindow*>(&static_cast<UTFWin::Window_intrusive_list_node&>(*it));
}

UTFWin::IWindow* Direct(UTFWin::IWindow* parent, uint32_t id) {
    if (!parent)
        return nullptr;
    for (auto it = parent->GetChildrenBegin(); it != parent->GetChildrenEnd(); ++it) {
        auto* child = Child(it);
        if (child->GetControlID() == id)
            return child;
    }
    return nullptr;
}

UTFWin::IWindow* Find(UTFWin::IWindow* parent, uint32_t id, unsigned depth = 0) {
    if (!parent || depth > 24)
        return nullptr;
    if (parent->GetControlID() == id)
        return parent;
    for (auto it = parent->GetChildrenBegin(); it != parent->GetChildrenEnd(); ++it)
        if (auto* found = Find(Child(it), id, depth + 1))
            return found;
    return nullptr;
}

UTFWin::IButton* Button() {
    auto* w = buttonFactory(nullptr, nullptr);
    return w ? object_cast<UTFWin::IButton>(w) : nullptr;
}

UTFWin::IDrawable* Skin(UTFWin::IWindow* source, bool removeIcon) {
    if (!source)
        return nullptr;
    auto* native = object_cast<UTFWin::SporeStdDrawable>(source->GetDrawable());
    if (!native)
        return nullptr;
    auto* clone = new UTFWin::SporeStdDrawable();
    for (int i = 0; i < 8; ++i) {
        auto* info = native->GetImageInfo(i);
        if (!info)
            continue;
        auto* copy = new UTFWin::SporeStdDrawableImageInfo(*info);
        if (removeIcon)
            copy->SetIconImage(nullptr);
        clone->SetImageInfo(copy, i);
    }
    clone->SetScaleType(native->GetScaleType());
    clone->SetScaleArea(native->GetScaleArea());
    auto factor = native->GetScaleFactor();
    clone->SetScaleFactor(factor);
    clone->SetHitFactor(native->GetHitFactor());
    return clone;
}

UTFWin::IWindow* Panel(UTFWin::IWindow* parent, uint32_t id, const Math::Rectangle& area) {
    auto* button = Button();
    if (!button)
        return nullptr;
    auto* w = button->ToWindow();
    w->SetDrawable(nullptr);
    w->SetCaption(u"");
    w->SetControlID(id);
    w->SetCommandID(0);
    w->SetArea(area);
    w->SetFillColor(Math::Color(0x00ffffff));
    w->SetShadeColor(Math::Color(0xffffffff));
    w->SetVisible(true);
    w->SetEnabled(true);
    w->SetIgnoreMouse(true);
    parent->AddWindow(w);
    return w;
}

UTFWin::IWindow* Hotspot(UTFWin::IWindow* parent, uint32_t id, const Math::Rectangle& area) {
    auto* window = Panel(parent, id, area);
    if (!window)
        return nullptr;
    window->SetCommandID(id);
    window->SetIgnoreMouse(false);
    return window;
}

UTFWin::IButton* Label(UTFWin::IWindow* parent, const char16_t* text, const Math::Rectangle& area,
                       uint32_t color = IdleCaption, uint32_t font = FontID) {
    auto* button = Button();
    if (!button)
        return nullptr;
    auto* w = button->ToWindow();
    w->SetDrawable(nullptr);
    w->SetCaption(text);
    w->SetArea(area);
    w->SetTextFontID(font);
    w->SetFillColor(Math::Color(0));
    w->SetVisible(true);
    w->SetEnabled(false);
    w->SetIgnoreMouse(true);
    for (int i = 0; i < 8; ++i)
        button->SetCaptionColor(UTFWin::StateIndices(i), Math::Color(color));
    parent->AddWindow(w);
    return button;
}

UTFWin::IWindow* Glyph(UTFWin::IWindow* parent, const Math::Rectangle& area, Asset asset) {
    auto* window = Panel(parent, 0, area);
    if (window)
        Background(window, asset);
    return window;
}

UTFWin::IWindow* Mark(UTFWin::IWindow* parent, const Math::Rectangle& area, Asset asset, const char16_t* fallback) {
    auto* window = Panel(parent, 0, area);
    if (!window || Background(window, asset))
        return window;
    window->SetTextFontID(CenteredFontID);
    window->SetCaption(fallback);
    if (auto* button = object_cast<UTFWin::IButton>(window))
        for (int i = 0; i < 8; ++i)
            button->SetCaptionColor(UTFWin::StateIndices(i), Math::Color(0xffffffff));
    return window;
}

UTFWin::IWindow* Fill(UTFWin::IWindow* parent, const Math::Rectangle& area, Asset asset, uint32_t fallback) {
    auto* window = Panel(parent, 0, area);
    if (window && !Background(window, asset))
        window->SetFillColor(Math::Color(fallback));
    return window;
}

}

class SettingsUI final : public UTFWin::IWinProc {
    UTFWin::IWindow* strip = nullptr;
    UTFWin::IWindow* host = nullptr;
    UTFWin::IWindow* page = nullptr;
    UTFWin::IWindow* tab = nullptr;
    UTFWin::IWindow* previous = nullptr;
    UTFWin::IWindow* viewport = nullptr;
    UTFWin::IWindow* catcher = nullptr;
    UTFWin::IWindow* rail = nullptr;
    UTFWin::IWindow* track = nullptr;
    UTFWin::IWindow* thumb = nullptr;
    UTFWin::IWindow* rows[RuleCount]{};
    UTFWin::IWindow* switches[RuleCount]{};
    UTFWin::IButton* switchButtons[RuleCount]{};
    UTFWin::IWindow* knobs[RuleCount]{};
    UTFWin::IWindow* details[RuleCount]{};
    UTFWin::IButton* detailLabels[RuleCount][3]{};
    UTFWin::IWindow* groups[SectionCount]{};
    UTFWin::IWindow* headers[SectionCount]{};
    UTFWin::IWindow* plates[SectionCount]{};
    UTFWin::IButton* headerCaptions[SectionCount]{};
    UTFWin::IWindow* chevrons[SectionCount][2]{};
    UTFWin::IWindow* badges[SectionCount]{};
    UTFWin::IButton* badgeLabels[SectionCount]{};
    bool visualReady[RuleCount]{}, visualOn[RuleCount]{}, rowHovered[RuleCount]{};
    bool headerHovered[SectionCount]{};
    int badgeCount[SectionCount]{};
    float rowAlpha[RuleCount]{}, headerAlpha[SectionCount]{};
    float knobPosition[RuleCount]{}, knobFrom[RuleCount]{}, knobTarget[RuleCount]{};
    float detailHeight[RuleCount]{}, detailFrom[RuleCount]{};
    DWORD knobStarted[RuleCount]{};
    DWORD detailStarted = 0;
    int openDetail = -1;
    float pageWidth = 459;
    Tween folds[SectionCount], turns[SectionCount];
    Tween scroll{0, 0, 1, 0, true}, barShown, barHover;
    float maxScroll = 0, limit = 0, thumbTop = 0, thumbSize = ViewHeight, grab = 0, shownOffset = 0;
    bool dragging = false, thumbHot = false;
    UTFWin::IButton* tabCaption = nullptr;
    UTFWin::IButton* status = nullptr;
    bool selected = false;
    bool refreshing = false;
    bool captionSelected = false;
    bool haveAction[RuleCount * 2 + SectionCount]{};
    DWORD lastAction[RuleCount * 2 + SectionCount]{};

    void Forget() {
        strip = host = page = tab = previous = nullptr;
        viewport = catcher = rail = track = thumb = nullptr;
        tabCaption = status = nullptr;
        selected = false;
        refreshing = false;
        captionSelected = false;
        dragging = thumbHot = false;
        openDetail = -1;
        maxScroll = limit = thumbTop = grab = shownOffset = 0;
        thumbSize = ViewHeight;
        scroll.Set(0);
        barShown.Set(0);
        barHover.Set(0);
        for (unsigned i = 0; i < RuleCount; ++i) {
            rows[i] = switches[i] = nullptr;
            switchButtons[i] = nullptr;
            knobs[i] = details[i] = nullptr;
            visualReady[i] = visualOn[i] = rowHovered[i] = false;
            rowAlpha[i] = knobPosition[i] = knobFrom[i] = knobTarget[i] = detailHeight[i] = detailFrom[i] = 0;
            haveAction[i] = haveAction[i + RuleCount] = false;
            for (auto& label : detailLabels[i])
                label = nullptr;
        }
        for (unsigned s = 0; s < SectionCount; ++s) {
            groups[s] = headers[s] = plates[s] = badges[s] = chevrons[s][0] = chevrons[s][1] = nullptr;
            headerCaptions[s] = badgeLabels[s] = nullptr;
            headerHovered[s] = false;
            headerAlpha[s] = 0;
            badgeCount[s] = -1;
            haveAction[RuleCount * 2 + s] = false;
        }
    }

    void Refresh() {
        refreshing = true;
        for (unsigned i = 0; i < RuleCount; ++i) {
            if (!switches[i] || (visualReady[i] && visualOn[i] == rules[i]))
                continue;
            bool artwork = Background(switches[i], rules[i] ? TrackOn : TrackOff);
            if (knobs[i])
                artwork = Background(knobs[i], rules[i] ? KnobOn : KnobOff) && artwork;
            switches[i]->SetCaption(artwork    ? u""
                                    : rules[i] ? CivText::Get(CivText::SwitchOn)
                                               : CivText::Get(CivText::SwitchOff));
            knobFrom[i] = visualReady[i] ? knobPosition[i] : (rules[i] ? 1.0f : 0.0f);
            knobTarget[i] = rules[i] ? 1.0f : 0.0f;
            knobPosition[i] = knobFrom[i];
            knobStarted[i] = GetTickCount();
            visualReady[i] = true;
            visualOn[i] = rules[i];
        }
        for (unsigned s = 0; s < SectionCount; ++s) {
            int count = 0;
            for (unsigned k = 0; k < SectionSize; ++k)
                count += rules[sectionRules[s][k]] ? 1 : 0;
            if (!badges[s] || !badgeLabels[s] || count == badgeCount[s])
                continue;
            badgeCount[s] = count;
            char16_t text[24];
            badgeLabels[s]->ToWindow()->SetCaption(CivText::Format(CivText::SectionBadge, count, text, 24));
            auto color = Math::Color(count ? BadgeOnCaption : MutedCaption);
            for (int state = 0; state < 8; ++state)
                badgeLabels[s]->SetCaptionColor(UTFWin::StateIndices(state), color);
            if (!Background(badges[s], count ? BadgeOn : BadgeOff))
                badges[s]->SetFillColor(Math::Color(count ? 0xe6373015 : 0xe60d1621));
        }
        if (tabCaption && captionSelected != selected) {
            captionSelected = selected;
            auto color = Math::Color(selected ? SelectedCaption : IdleCaption);
            for (int i = 0; i < 8; ++i)
                tabCaption->SetCaptionColor(UTFWin::StateIndices(i), color);
        }
        refreshing = false;
    }

    void PlaceHeader(unsigned section, float y, DWORD now) {
        auto* header = headers[section];
        if (!header)
            return;
        header->SetArea(Math::Rectangle(22, y, pageWidth - 22, y + HeaderHeight));
        headerAlpha[section] = Exposure(y, y + HeaderHeight, 0, ViewHeight);
        Present(header, headerAlpha[section]);
        bool hover = selected && (header->GetState() & UTFWin::kStateHover);
        if (hover != headerHovered[section]) {
            headerHovered[section] = hover;
            if (plates[section])
                Present(plates[section], hover ? 0.35f : 0.0f);
            auto color = Math::Color(hover ? SectionHoverCaption : SectionCaption);
            if (headerCaptions[section])
                for (int state = 0; state < 8; ++state)
                    headerCaptions[section]->SetCaptionColor(UTFWin::StateIndices(state), color);
        }
        uint32_t tint = hover ? SectionHoverCaption : SectionCaption;
        float turn = turns[section].At(now);
        if (chevrons[section][0])
            chevrons[section][0]->SetShadeColor(Math::Color(Shade(Clamp((turn - 0.3f) / 0.7f, 0, 1), tint)));
        if (chevrons[section][1])
            chevrons[section][1]->SetShadeColor(Math::Color(Shade(Clamp((0.7f - turn) / 0.7f, 0, 1), tint)));
    }

    void PlaceRule(unsigned i, float y, float detail, float top, float low, float high, float fade) {
        if (rows[i]) {
            rows[i]->SetArea(Math::Rectangle(22, y, pageWidth - 22, y + RowHeight));
            rowAlpha[i] = Exposure(top + y, top + y + RowHeight, low, high) * fade;
            Present(rows[i], rowAlpha[i]);
            bool hover = selected && ((rows[i]->GetState() & UTFWin::kStateHover) ||
                                      (switches[i] && (switches[i]->GetState() & UTFWin::kStateHover)));
            if (hover != rowHovered[i]) {
                rowHovered[i] = hover;
                Background(rows[i], hover ? RowHover : RowIdle);
            }
        }
        if (!details[i])
            return;
        float natural = y + RowHeight + 4;
        float upper = natural > low - top ? natural : low - top;
        float lower = natural + detail < high - top ? natural + detail : high - top;
        bool open = detail > 0 && lower - upper >= 1;
        if (open)
            details[i]->SetArea(Math::Rectangle(22, upper, pageWidth - 22, lower));
        Present(details[i], open ? fade : 0);
        for (unsigned line = 0; line < 3; ++line) {
            auto* label = detailLabels[i][line] ? detailLabels[i][line]->ToWindow() : nullptr;
            if (!label)
                continue;
            float lineTop = 6 + line * 20 - (upper - natural);
            label->SetArea(Math::Rectangle(8, lineTop, pageWidth - 60, lineTop + 20));
            float boxTop = top + upper + lineTop, room = boxTop;
            if (ViewHeight - boxTop - 20 < room)
                room = ViewHeight - boxTop - 20;
            float ramp = clipping ? 1 : Clamp((room + 2) / FadeBand, 0, 1);
            bool inside = open && lineTop >= -2 && lineTop + 18 <= lower - upper;
            Present(label, inside ? ramp * ramp : 0);
        }
    }

    void PlaceScrollbar(float offset, DWORD now) {
        if (!rail)
            return;
        bool hover = selected && (rail->GetState() & UTFWin::kStateHover);
        if (dragging && !hover)
            dragging = false;
        barShown.Go(maxScroll > 0 ? 1.0f : 0.0f, now, 200);
        barHover.Go(hover ? 1.0f : 0.0f, now, 120);
        float emphasis = barHover.At(now);
        uint32_t color = Shade(barShown.At(now) * (0.8f + 0.2f * emphasis));
        rail->SetShadeColor(Math::Color(color));
        rail->SetVisible(dragging || (color >> 24) != 0);
        float half = 3 + emphasis;
        if (track)
            track->SetArea(Math::Rectangle(RailCenter - half, 0, RailCenter + half, ViewHeight));
        thumbSize = Clamp(Pixel(ViewHeight * ViewHeight / (ViewHeight + maxScroll)), ThumbMinimum, ViewHeight);
        thumbTop = maxScroll > 0 ? Pixel((ViewHeight - thumbSize) * offset / maxScroll) : 0;
        if (!thumb)
            return;
        thumb->SetArea(Math::Rectangle(RailCenter - half, thumbTop, RailCenter + half, thumbTop + thumbSize));
        if (hover != thumbHot) {
            thumbHot = hover;
            if (!Background(thumb, hover ? ScrollThumbHover : ScrollThumb))
                thumb->SetFillColor(Math::Color(hover ? 0xffd5bb5c : 0xffb29a49));
        }
    }

    void Animate() {
        DWORD now = GetTickCount();
        clipping = clipDrawing && *clipDrawing;
        float progress = Ease(float(now - detailStarted) / 180.0f);
        for (unsigned i = 0; i < RuleCount; ++i) {
            float k = Ease(float(now - knobStarted[i]) / 150.0f);
            knobPosition[i] = knobFrom[i] + (knobTarget[i] - knobFrom[i]) * k;
            if (knobs[i]) {
                float x = 2 + knobPosition[i] * 28;
                knobs[i]->SetArea(Math::Rectangle(x, 1, x + 24, 25));
            }
            float target = openDetail == int(i) ? DetailHeight : 0.0f;
            detailHeight[i] = detailFrom[i] + (target - detailFrom[i]) * progress;
        }
        float open[SectionCount];
        for (unsigned s = 0; s < SectionCount; ++s)
            open[s] = folds[s].At(now);
        Layout layout;
        Arrange(layout, open, detailHeight);
        maxScroll = Overflow(layout);
        float offset = Pixel(Clamp(scroll.At(now), 0, maxScroll));
        shownOffset = offset;
        for (unsigned s = 0; s < SectionCount; ++s) {
            PlaceHeader(s, layout.header[s] - offset, now);
            float top = layout.block[s] - offset, bottom = top + layout.shown[s];
            float low = Clamp(top, 0, ViewHeight), high = Clamp(bottom, 0, ViewHeight);
            if (groups[s]) {
                groups[s]->SetArea(Math::Rectangle(0, top, pageWidth, bottom));
                groups[s]->SetVisible(high > low);
            }
            float slide = layout.full[s] - layout.shown[s];
            for (unsigned k = 0; k < SectionSize; ++k) {
                unsigned i = sectionRules[s][k];
                PlaceRule(i, layout.row[i] - slide, layout.detail[i], top, low, high, open[s] * open[s]);
            }
        }
        PlaceScrollbar(offset, now);
    }

    void Settle(int section, int rule) {
        float open[SectionCount], heights[RuleCount];
        for (unsigned s = 0; s < SectionCount; ++s)
            open[s] = sectionOpen[s] ? 1.0f : 0.0f;
        for (unsigned i = 0; i < RuleCount; ++i)
            heights[i] = openDetail == int(i) ? DetailHeight : 0.0f;
        Layout layout;
        Arrange(layout, open, heights);
        limit = Overflow(layout);
        float target = scroll.to, top = 0, bottom = 0;
        if (section >= 0) {
            top = layout.header[section];
            bottom = layout.block[section] + layout.shown[section];
        }
        if (rule >= 0) {
            top = layout.block[SectionOf(unsigned(rule))] + layout.row[rule];
            bottom = top + RowHeight + (layout.detail[rule] > 0 ? layout.detail[rule] + 4 : 0);
        }
        if (bottom > top)
            bottom += SectionGap;
        if (bottom > top) {
            if (bottom - target > ViewHeight)
                target = bottom - ViewHeight;
            if (top < target)
                target = top;
        }
        scroll.Go(Clamp(target, 0, limit), GetTickCount(), 220);
    }

    void Describe(unsigned rule) {
        for (unsigned i = 0; i < RuleCount; ++i)
            detailFrom[i] = detailHeight[i];
        openDetail = openDetail == int(rule) ? -1 : int(rule);
        detailStarted = GetTickCount();
        Settle(-1, openDetail);
    }

    void Fold(unsigned section) {
        DWORD now = GetTickCount();
        sectionOpen[section] = !sectionOpen[section];
        float target = sectionOpen[section] ? 1.0f : 0.0f;
        folds[section].Go(target, now, 200);
        turns[section].Go(target, now, 150);
        if (!sectionOpen[section] && openDetail >= 0 && SectionOf(unsigned(openDetail)) == section)
            Describe(unsigned(openDetail));
        Settle(sectionOpen[section] ? int(section) : -1, -1);
    }

    void Wheel(int delta) {
        scroll.Go(Clamp(scroll.to - WheelStep * float(delta) / 120.0f, 0, limit), GetTickCount(), 160);
    }

    bool Drag(const UTFWin::Message& msg) {
        if (msg.IsType(UTFWin::kMsgMouseWheel)) {
            Wheel(msg.MouseWheel.wheelDelta);
            return true;
        }
        bool left = msg.Mouse.IsLeftButton();
        if (msg.IsType(UTFWin::kMsgMouseUp)) {
            if (!left)
                dragging = false;
            return false;
        }
        float y = msg.Mouse.mouseY;
        if (msg.IsType(UTFWin::kMsgMouseMove)) {
            if (dragging && !left)
                dragging = false;
            float travel = ViewHeight - thumbSize;
            if (dragging)
                scroll.Set(travel > 0 ? Clamp((y - grab) / travel * maxScroll, 0, maxScroll) : 0);
            return false;
        }
        if (!left || (msg.Mouse.mouseState & 0x30) || maxScroll <= 0)
            return false;
        if (y >= thumbTop && y < thumbTop + thumbSize) {
            dragging = true;
            grab = y - thumbTop;
            scroll.Set(shownOffset);
        } else {
            scroll.Go(Clamp(scroll.to + (y < thumbTop ? -ViewHeight : ViewHeight), 0, limit), GetTickCount(), 220);
        }
        return false;
    }

    bool Mouse(UTFWin::IWindow* window, const UTFWin::Message& msg) {
        if (!selected || !window)
            return false;
        if (window == rail)
            return Drag(msg);
        if (msg.IsType(UTFWin::kMsgMouseMove))
            return false;
        bool list = window == catcher;
        for (unsigned i = 0; i < RuleCount && !list; ++i)
            list = window == rows[i] || window == switches[i];
        for (unsigned s = 0; s < SectionCount && !list; ++s)
            list = window == headers[s];
        if (!list)
            return false;
        if (msg.IsType(UTFWin::kMsgMouseWheel)) {
            Wheel(msg.MouseWheel.wheelDelta);
            return true;
        }
        return window == catcher;
    }

    bool Accept(unsigned action) {
        DWORD now = GetTickCount();
        if (haveAction[action] && now - lastAction[action] < 180)
            return false;
        haveAction[action] = true;
        lastAction[action] = now;
        return true;
    }

    bool Belongs(UTFWin::IWindow* candidate) {
        if (!candidate || !host)
            return false;
        for (auto it = host->GetChildrenBegin(); it != host->GetChildrenEnd(); ++it)
            if (Child(it) == candidate)
                return true;
        return false;
    }

    void Reconcile(bool activate = false) {
        if (!page || !tab || !host)
            return;
        UTFWin::IWindow* other = nullptr;
        for (auto it = host->GetChildrenBegin(); it != host->GetChildrenEnd(); ++it) {
            auto* child = Child(it);
            if (child != page && child->IsVisible())
                other = child;
        }
        if (other)
            previous = other;
        if (activate) {
            for (auto it = host->GetChildrenBegin(); it != host->GetChildrenEnd(); ++it) {
                auto* child = Child(it);
                child->SetVisible(child == page);
            }
        } else if (selected && ((tab->GetState() & UTFWin::kBtnStateSelected) == 0 || other)) {
            selected = false;
        }
        page->SetVisible(selected);
        if (!selected && !other && Belongs(previous))
            previous->SetVisible(true);
        Refresh();
    }

    bool BuildHeader(unsigned section, float width) {
        auto* button = Button();
        if (!button)
            return false;
        auto* header = button->ToWindow();
        header->SetDrawable(nullptr);
        header->SetCaption(u"");
        header->SetControlID(HeaderID + section);
        header->SetCommandID(HeaderID + section);
        header->SetArea(Math::Rectangle(22, 0, width - 22, HeaderHeight));
        header->SetFillColor(Math::Color(0));
        header->SetVisible(true);
        header->SetEnabled(true);
        button->SetButtonType(UTFWin::ButtonTypes::Standard);
        viewport->AddWindow(header);
        header->AddWinProc(this);
        headers[section] = header;
        float middle = HeaderHeight / 2;
        plates[section] = Panel(header, 0, Math::Rectangle(0, 0, width - 44, HeaderHeight));
        if (!plates[section])
            return false;
        if (!Background(plates[section], RowHover))
            plates[section]->SetFillColor(Math::Color(0x33d6be70));
        Present(plates[section], 0);
        Math::Rectangle chevron(11, middle - 10, 31, middle + 10);
        chevrons[section][0] = Mark(header, chevron, ChevronDown, u"-");
        chevrons[section][1] = Mark(header, chevron, ChevronRight, u"+");
        float badge = CivText::BadgeWidth() + 12;
        badge = badge < 56 ? 56 : badge;
        float lead = width - 64 - badge;
        headerCaptions[section] = Label(header, CivText::Get(sectionTexts[section]),
                                        Math::Rectangle(44, 0, lead, HeaderHeight), SectionCaption);
        float widest = 0;
        for (unsigned s = 0; s < SectionCount; ++s)
            widest = CivText::CaptionWidth(s) > widest ? CivText::CaptionWidth(s) : widest;
        float divider = 44 + widest + 12;
        badges[section] = Panel(header, 0, Math::Rectangle(lead + 8, middle - 10, width - 56, middle + 10));
        if (!chevrons[section][0] || !chevrons[section][1] || !headerCaptions[section] || !badges[section] ||
            (divider + 12 < lead &&
             !Fill(header, Math::Rectangle(divider, middle, lead, middle + 1), SectionDivider, 0x9ed6be70)))
            return false;
        badgeLabels[section] =
            Label(badges[section], u"", Math::Rectangle(0, 0, badge, 20), MutedCaption, CenteredFontID);
        return badgeLabels[section] != nullptr;
    }

    bool BuildRule(unsigned i, UTFWin::IWindow* group, float width) {
        auto* rowButton = Button();
        auto* toggle = Button();
        if (!rowButton || !toggle)
            return false;
        auto* row = rowButton->ToWindow();
        row->SetControlID(RowID + i);
        row->SetCommandID(RowID + i);
        row->SetArea(Math::Rectangle(22, 0, width - 22, RowHeight));
        row->SetVisible(true);
        row->SetEnabled(true);
        rowButton->SetButtonType(UTFWin::ButtonTypes::Standard);
        Background(row, RowIdle);
        group->AddWindow(row);
        row->AddWinProc(this);
        rows[i] = row;
        float middle = RowHeight / 2;
        Glyph(row, Math::Rectangle(9, middle - 12, 33, middle + 12), ruleIcons[i]);
        Label(row, CivText::Get(titleTexts[i]), Math::Rectangle(44, 0, width - 120, RowHeight));
        auto* sw = toggle->ToWindow();
        sw->SetControlID(ToggleID + i);
        sw->SetCommandID(ToggleID + i);
        sw->SetArea(Math::Rectangle(width - 112, middle - 13, width - 56, middle + 13));
        sw->SetVisible(true);
        sw->SetEnabled(true);
        toggle->SetButtonType(UTFWin::ButtonTypes::Standard);
        toggle->SetButtonGroupID(0);
        sw->SetFillColor(Math::Color(0));
        for (int state = 0; state < 8; ++state)
            toggle->SetCaptionColor(UTFWin::StateIndices(state), Math::Color(IdleCaption));
        row->AddWindow(sw);
        sw->AddWinProc(this);
        switches[i] = sw;
        switchButtons[i] = toggle;
        knobs[i] = Glyph(sw, Math::Rectangle(2, 1, 26, 25), KnobOff);
        details[i] = Panel(group, 0, Math::Rectangle(22, RowHeight + 4, width - 22, RowHeight + 4));
        if (!knobs[i] || !details[i])
            return false;
        details[i]->SetFillColor(Math::Color(0x5008101c));
        details[i]->SetFlag(UTFWin::kWinFlagClip, true);
        details[i]->SetVisible(false);
        for (unsigned line = 0; line < 3; ++line)
            detailLabels[i][line] = Label(details[i], CivText::Get(descriptionTexts[i][line]),
                                          Math::Rectangle(8, 6 + line * 20, width - 60, 26 + line * 20), MutedCaption);
        return true;
    }

    bool BuildScrollbar(float width) {
        rail = Hotspot(page, ScrollID, Math::Rectangle(width - 18, ViewTop, width - 4, ViewBottom));
        if (!rail)
            return false;
        rail->AddWinProc(this);
        rail->SetShadeColor(Math::Color(Shade(0)));
        rail->SetVisible(false);
        track = Fill(rail, Math::Rectangle(RailCenter - 3, 0, RailCenter + 3, ViewHeight), ScrollTrack, 0x9e0d1621);
        thumb = Fill(rail, Math::Rectangle(RailCenter - 3, 0, RailCenter + 3, ViewHeight), ScrollThumb, 0xffb29a49);
        return track && thumb;
    }

    bool Build(UTFWin::IWindow* credits, UTFWin::IWindow* nativeTab) {
        auto* referencePage =
            host->GetChildrenBegin() != host->GetChildrenEnd() ? Child(host->GetChildrenBegin()) : nullptr;
        if (!referencePage)
            return false;
        auto area = referencePage->GetArea();
        page = Panel(host, PageID, area);
        if (!page)
            return false;
        page->SetVisible(false);
        const float width = area.GetWidth();
        pageWidth = width;
        uint32_t titleFont = FontID;
        for (auto it = referencePage->GetChildrenBegin(); it != referencePage->GetChildrenEnd(); ++it) {
            auto* child = Child(it);
            if (child->GetArea().y1 < 40 && child->GetCaption() && *child->GetCaption()) {
                titleFont = child->GetTextFontID();
                break;
            }
        }
        Glyph(page, Math::Rectangle(12, 2, 40, 30), IconCiv);
        Label(page, CivText::Get(CivText::PageTitle), Math::Rectangle(50, 0, width - 14, 34), 0xfff2f5f8, titleFont);
        viewport = Panel(page, 0, Math::Rectangle(0, ViewTop, width, ViewBottom));
        if (!viewport)
            return false;
        viewport->SetFlag(UTFWin::kWinFlagClip, true);
        catcher = Hotspot(viewport, CatcherID, Math::Rectangle(0, 0, width, ViewHeight));
        if (!catcher)
            return false;
        catcher->AddWinProc(this);
        for (unsigned s = 0; s < SectionCount; ++s) {
            groups[s] = Panel(viewport, 0, Math::Rectangle(0, 0, width, 0));
            if (!groups[s])
                return false;
            groups[s]->SetFlag(UTFWin::kWinFlagClip, true);
            for (unsigned k = 0; k < SectionSize; ++k)
                if (!BuildRule(sectionRules[s][k], groups[s], width))
                    return false;
            if (!BuildHeader(s, width))
                return false;
            folds[s].Set(sectionOpen[s] ? 1.0f : 0.0f);
            turns[s].Set(sectionOpen[s] ? 1.0f : 0.0f);
        }
        Label(page, CivText::Get(CivText::Hint), Math::Rectangle(22, HintTop, width - 69, HintTop + 22), MutedCaption);
        status = Label(page, CivText::Get(CivText::StatusSaved),
                       Math::Rectangle(22, StatusTop, width - 69, StatusTop + 26), MutedCaption);
        if (!BuildScrollbar(width))
            return false;
        auto* button = Button();
        if (!button)
            return false;
        tab = button->ToWindow();
        tab->SetControlID(TabID);
        tab->SetCommandID(PageID);
        tab->SetVisible(true);
        tab->SetEnabled(true);
        tab->SetTextFontID(credits->GetTextFontID());
        auto* source = object_cast<UTFWin::IButton>(nativeTab);
        button->SetButtonType(UTFWin::ButtonTypes::Radio);
        if (source) {
            button->SetButtonFlags(source->GetButtonFlags());
            button->SetButtonGroupID(source->GetButtonGroupID());
        }
        if (auto* skin = Skin(nativeTab, true))
            tab->SetDrawable(skin);
        strip->AddWindow(tab);
        tab->AddWinProc(this);
        auto ca = credits->GetArea();
        Glyph(tab, Math::Rectangle(10, (ca.GetHeight() - 26) / 2, 36, (ca.GetHeight() + 26) / 2), IconCiv);
        tabCaption =
            Label(tab, CivText::Get(CivText::TabCaption), Math::Rectangle(46, 0, ca.GetWidth() - 8, ca.GetHeight()),
                  IdleCaption, credits->GetTextFontID());
        Settle(-1, -1);
        Refresh();
        return true;
    }

public:
    int AddRef() override {
        return 2;
    }

    int Release() override {
        return 1;
    }

    void* Cast(uint32_t type) const override {
        if (type == UTFWin::IWinProc::TYPE || type == Object::TYPE)
            return const_cast<SettingsUI*>(this);
        return nullptr;
    }

    int GetPriority() const override {
        return 100;
    }

    int GetEventFlags() const override {
        return UTFWin::kEventFlagBasicInput | UTFWin::kEventFlagAdvanced;
    }

    bool HandleUIMessage(UTFWin::IWindow* window, const UTFWin::Message& msg) override {
        if (!page || refreshing)
            return false;
        if (msg.eventType >= UTFWin::kMsgMouseDown && msg.eventType <= UTFWin::kMsgMouseWheel)
            return Mouse(window, msg);
        bool activation = msg.IsType(UTFWin::kMsgButtonClick) || msg.IsType(UTFWin::kMsgComponentActivated);
        if (!msg.source || (!activation && !msg.IsType(UTFWin::kMsgButtonSelect)))
            return false;
        auto* manager = windowManager ? windowManager() : nullptr;
        auto* liveStrip = manager ? Find(manager->GetMainWindow(), StripID) : nullptr;
        if (liveStrip != strip || Direct(liveStrip, HostID) != host || Direct(host, PageID) != page)
            return false;
        if (msg.source == tab && msg.IsType(UTFWin::kMsgButtonSelect)) {
            selected = msg.ButtonSelect.isSelected;
            Reconcile(selected);
            return false;
        }
        if (!selected || !activation)
            return false;
        if (msg.source == rail || msg.source == catcher)
            return true;
        for (unsigned s = 0; s < SectionCount; ++s) {
            if (msg.source != headers[s])
                continue;
            if (Accept(RuleCount * 2 + s)) {
                if (headerAlpha[s] < Faint)
                    Settle(int(s), -1);
                else
                    Fold(s);
            }
            return false;
        }
        for (unsigned i = 0; i < RuleCount; ++i) {
            if (msg.source != switches[i] && msg.source != rows[i])
                continue;
            if (!sectionOpen[SectionOf(i)] || !Accept(i + (msg.source == rows[i] ? RuleCount : 0)))
                return false;
            if (rowAlpha[i] < Faint) {
                Settle(-1, int(i));
                return false;
            }
            if (msg.source == rows[i]) {
                Describe(i);
                return false;
            }
            bool saved = Set(i, !rules[i]);
            if (status)
                status->ToWindow()->SetCaption(CivText::Get(saved ? CivText::StatusSaved : CivText::StatusFailed));
            Refresh();
            return false;
        }
        return false;
    }

    void Pump() {
        if (!windowManager || !buttonFactory)
            return;
        auto* manager = windowManager();
        auto* main = manager ? manager->GetMainWindow() : nullptr;
        auto* currentStrip = Find(main, StripID);
        auto* currentHost = Direct(currentStrip, HostID);
        if (!currentHost) {
            Forget();
            return;
        }
        if (currentHost != host || Direct(currentHost, PageID) != page || Direct(currentStrip, TabID) != tab)
            Forget();
        strip = currentStrip;
        host = currentHost;
        auto* credits = Direct(strip, CreditsID);
        auto* online = Direct(strip, OnlineID);
        if (!credits || !online)
            return;
        if (!page) {
            if (!Build(credits, online)) {
                if (tab)
                    strip->DisposeWindowFamily(tab);
                if (page)
                    host->DisposeWindowFamily(page);
                Forget();
                return;
            }
        }
        auto a = credits->GetArea();
        float last = a.y1;
        for (auto it = strip->GetChildrenBegin(); it != strip->GetChildrenEnd(); ++it) {
            auto* child = Child(it);
            auto ca = child->GetArea();
            if (child != tab && child != host && ca.x1 == a.x1 && ca.GetWidth() == a.GetWidth() && ca.y1 > last)
                last = ca.y1;
        }
        tab->SetArea(Math::Rectangle(a.x1, last + 43, a.x2, last + 43 + a.GetHeight()));
        Reconcile();
        Animate();
    }
};

namespace {
SettingsUI ui;
}

void BindUI(uintptr_t base) {
    windowManager = reinterpret_cast<WindowManagerFn>(base + 0x67ca60 - 0x400000);
    buttonFactory = reinterpret_cast<ButtonFactoryFn>(base + 0x9670a0 - 0x400000);
    imageLoad = reinterpret_cast<ImageLoadFn>(base + 0x806320 - 0x400000);
    imageBackground = reinterpret_cast<ImageBackgroundFn>(base + 0x8069c0 - 0x400000);
    clipDrawing = reinterpret_cast<const uint8_t*>(base + 0x164e90c - 0x400000);
}

void UpdateUI() {
    ui.Pump();
}
}

