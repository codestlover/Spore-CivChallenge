
#include <Spore/ModAPI.h>
#include "core/SdkCompat.hpp"
#include <Spore/Internal.h>
#include <Spore/UTFWin/StdDrawable.h>
#include <Spore/UTFWin/SporeStdDrawable.h>
#include <Spore/UTFWin/SporeStdDrawableImageInfo.h>
#include <Spore/UTFWin/OutlineFormat.h>

#include <EASTL/internal/thread_support.h>

#define ADDR(ns, fn) (UTFWin::ns##_addresses::fn + baseAddress)

const Math::Color Math::Color::WHITE(0xffffffff);

int DefaultRefCounted::AddRef() {
    return eastl::Internal::atomic_increment(&mnRefCount);
}

int DefaultRefCounted::Release() {
    int count = eastl::Internal::atomic_decrement(&mnRefCount);
    if (!count)
        delete this;
    return count;
}

int DefaultRefCounted::GetReferenceCount() {
    return mnRefCount;
}

namespace UTFWin {

void* UTFWinObject::operator new(size_t size) {
    return ::operator new(size);
}

void UTFWinObject::operator delete(void* object) {
    ::operator delete(object);
}

void OutlineFormat::SetStrength(int s) {
    strength = s;
    switch (s) {
    case 1:
        smoothness = 0.0004f;
        saturation = 0.25f;
        break;
    case 2:
        smoothness = 0.0003f;
        saturation = 0.5f;
        break;
    case 3:
        smoothness = 0.0002f;
        saturation = 0.75f;
        break;
    case 4:
        smoothness = 0.0001f;
        saturation = 1.0f;
        break;
    case 5:
        smoothness = 0.0f;
        saturation = 1.25f;
        break;
    case 6:
        smoothness = 0.0f;
        saturation = 1.5f;
        break;
    case 7:
        smoothness = 0.0f;
        saturation = 1.85f;
        break;
    case 8:
        smoothness = 0.0f;
        saturation = 2.35f;
        break;
    case 9:
        smoothness = 0.0f;
        saturation = 3.15f;
        break;
    }
}

void OutlineFormat::SetSize(int sz) {
    size = sz;
    sizeX = static_cast<float>(sz);
    sizeY = static_cast<float>(sz);
}

DefaultLayoutElement::DefaultLayoutElement() : mnRefCount(0) {}

int DefaultLayoutElement::AddRef() {
    return eastl::Internal::atomic_increment(&mnRefCount);
}

int DefaultLayoutElement::Release() {
    if (eastl::Internal::atomic_decrement(&mnRefCount) == 0) {
        delete this;
        return 0;
    }
    return mnRefCount;
}

void* IDrawable::Cast(uint32_t type) const {
    if (type == IDrawable::TYPE)
        return (IDrawable*)this;
    if (type == ILayoutElement::TYPE)
        return (ILayoutElement*)this;
    if (type == Object::TYPE)
        return (Object*)(IDrawable*)this;
    return nullptr;
}

int StdDrawable::AddRef() {
    return eastl::Internal::atomic_increment(&mnRefCount);
}

int StdDrawable::Release() {
    if (eastl::Internal::atomic_decrement(&mnRefCount) == 0) {
        delete this;
        return 0;
    }
    return mnRefCount;
}

void* StdDrawable::Cast(uint32_t type) const {
    if (type == IDrawable::TYPE)
        return (IDrawable*)this;
    if (type == ILayoutElement::TYPE)
        return (ILayoutElement*)this;
    if (type == IStdDrawable::TYPE)
        return (IStdDrawable*)this;
    if (type == Object::TYPE)
        return (Object*)(IDrawable*)this;
    return nullptr;
}

void StdDrawable::SetSerializer(Serializer& dst) {
    ((void(__thiscall*)(ILayoutElement*, Serializer&))ADDR(StdDrawable, SetSerializer))(this, dst);
}

uint32_t StdDrawable::GetProxyID() const {
    return ((uint32_t(__thiscall*)(const ILayoutElement*))ADDR(StdDrawable, GetProxyID))(this);
}

void StdDrawable::Paint(UIRenderer* r, const Math::Rectangle& a, const RenderParams& p) {
    ((void(__thiscall*)(IDrawable*, UIRenderer*, const Math::Rectangle&, const RenderParams&))ADDR(StdDrawable, Paint))(
        this, r, a, p);
}

bool StdDrawable::IsColliding(const Math::Rectangle& a, const Math::Point& pt, RenderParams& p) {
    return ((bool(__thiscall*)(IDrawable*, const Math::Rectangle&, const Math::Point&, RenderParams&))ADDR(
        StdDrawable, IsColliding))(this, a, pt, p);
}

bool StdDrawable::GetDimensions(Dimensions& d, int s, int i) {
    return ((bool(__thiscall*)(IDrawable*, Dimensions&, int, int))ADDR(StdDrawable, GetDimensions))(this, d, s, i);
}

bool StdDrawable::UseCollision(uint32_t t, bool& dst) {
    return ((bool(__thiscall*)(IDrawable*, uint32_t, bool&))ADDR(StdDrawable, UseCollision))(this, t, dst);
}

Image* StdDrawable::GetImage(StateIndices idx) const {
    return ((Image * (__thiscall*)(const IStdDrawable*, StateIndices)) ADDR(StdDrawable, GetImage))(this, idx);
}

void StdDrawable::SetImage(StateIndices idx, Image* img) {
    ((void(__thiscall*)(IStdDrawable*, StateIndices, Image*))ADDR(StdDrawable, SetImage))(this, idx, img);
}

Scaling StdDrawable::GetScaleType() const {
    return ((Scaling(__thiscall*)(const IStdDrawable*))ADDR(StdDrawable, GetScaleType))(this);
}

void StdDrawable::SetScaleType(Scaling v) {
    ((void(__thiscall*)(IStdDrawable*, Scaling))ADDR(StdDrawable, SetScaleType))(this, v);
}

const Math::Rectangle& StdDrawable::GetScaleArea() const {
    return ((const Math::Rectangle&(__thiscall*)(const IStdDrawable*))ADDR(StdDrawable, GetScaleArea))(this);
}

void StdDrawable::SetScaleArea(const Math::Rectangle& v) {
    ((void(__thiscall*)(IStdDrawable*, const Math::Rectangle&))ADDR(StdDrawable, SetScaleArea))(this, v);
}

const Vector2& StdDrawable::GetScaleFactor() const {
    return ((const Vector2&(__thiscall*)(const IStdDrawable*))ADDR(StdDrawable, GetScaleFactor))(this);
}

void StdDrawable::SetScaleFactor(Vector2& v) {
    ((void(__thiscall*)(IStdDrawable*, Vector2&))ADDR(StdDrawable, SetScaleFactor))(this, v);
}

Object* StdDrawable::GetHitMask() const {
    return ((Object * (__thiscall*)(const IStdDrawable*)) ADDR(StdDrawable, GetHitMask))(this);
}

void StdDrawable::SetHitMask(Object* m) {
    ((void(__thiscall*)(IStdDrawable*, Object*))ADDR(StdDrawable, SetHitMask))(this, m);
}

float StdDrawable::GetHitFactor() const {
    return ((float(__thiscall*)(const IStdDrawable*))ADDR(StdDrawable, GetHitFactor))(this);
}

void StdDrawable::SetHitFactor(float v) {
    ((void(__thiscall*)(IStdDrawable*, float))ADDR(StdDrawable, SetHitFactor))(this, v);
}

SporeStdDrawableImageInfo::SporeStdDrawableImageInfo()
    : mpBackgroundImage(nullptr), mpIconImage(nullptr), mBackgroundColor(Color::WHITE), mIconColor(Color::WHITE),
      mIconDrawMode(IconDrawModes::WindowSize), mStrokeMode(ShadowModes::Full), mHaloMode(ShadowModes::Full),
      mBackgroundScale(1.0f, 1.0f), mBackgroundOffset(), mIconScale(1.0f, 1.0f), mIconOffset(), mStrokeShadow(),
      mHaloShadow() {
    mStrokeShadow.SetStrength(2);
    mHaloShadow.SetStrength(2);
}

SporeStdDrawableImageInfo::SporeStdDrawableImageInfo(const SporeStdDrawableImageInfo& o)
    : mpBackgroundImage(o.mpBackgroundImage), mpIconImage(o.mpIconImage), mBackgroundColor(o.mBackgroundColor),
      mIconColor(o.mIconColor), mIconDrawMode(o.mIconDrawMode), mStrokeMode(o.mStrokeMode), mHaloMode(o.mHaloMode),
      mBackgroundScale(o.mBackgroundScale), mBackgroundOffset(o.mBackgroundOffset), mIconScale(o.mIconScale),
      mIconOffset(o.mIconOffset), mStrokeShadow(o.mStrokeShadow), mHaloShadow(o.mHaloShadow) {}

SporeStdDrawableImageInfo::~SporeStdDrawableImageInfo() {}

int SporeStdDrawableImageInfo::AddRef() {
    return DefaultRefCounted::AddRef();
}

int SporeStdDrawableImageInfo::Release() {
    return DefaultRefCounted::Release();
}

void* SporeStdDrawableImageInfo::Cast(uint32_t type) const {
    if (type == Object::TYPE)
        return (Object*)this;
    if (type == ILayoutElement::TYPE)
        return (ILayoutElement*)this;
    if (type == SporeStdDrawableImageInfo::TYPE || type == 0xAE9CB0FA)
        return (SporeStdDrawableImageInfo*)this;
    return nullptr;
}

void SporeStdDrawableImageInfo::SetSerializer(Serializer& dst) {
    ((void(__thiscall*)(ILayoutElement*, Serializer&))ADDR(SporeStdDrawableImageInfo, SetSerializer))(this, dst);
}

uint32_t SporeStdDrawableImageInfo::GetProxyID() const {
    return ((uint32_t(__thiscall*)(const ILayoutElement*))ADDR(SporeStdDrawableImageInfo, GetProxyID))(this);
}

SporeStdDrawable::SporeStdDrawable()
    : mCurrentInfo(), mImageInfos{}, field_134(false), field_138(), field_13C(), field_140(), field_144(Color::WHITE),
      field_148(), field_158(1.0f, 1.0f) {
    mCurrentInfo.AddRef();
}

SporeStdDrawable::~SporeStdDrawable() {
    for (int i = 0; i < 8; ++i) {
        if (mImageInfos[i]) {
            mImageInfos[i]->SetBackgroundImage(nullptr);
            mImageInfos[i]->SetIconImage(nullptr);
        }
    }
}

int SporeStdDrawable::AddRef() {
    return eastl::Internal::atomic_increment(&mnRefCount);
}

int SporeStdDrawable::Release() {
    if (eastl::Internal::atomic_decrement(&mnRefCount) == 0) {
        delete this;
        return 0;
    }
    return mnRefCount;
}

void* SporeStdDrawable::Cast(uint32_t type) const {
    if (type == SporeStdDrawable::TYPE)
        return (SporeStdDrawable*)this;
    return StdDrawable::Cast(type);
}

void SporeStdDrawable::SetSerializer(Serializer& dst) {
    ((void(__thiscall*)(ILayoutElement*, Serializer&))ADDR(SporeStdDrawable, SetSerializer))(this, dst);
}

uint32_t SporeStdDrawable::GetProxyID() const {
    return ((uint32_t(__thiscall*)(const ILayoutElement*))ADDR(SporeStdDrawable, GetProxyID))(this);
}

void SporeStdDrawable::Paint(UIRenderer* r, const Math::Rectangle& a, const RenderParams& p) {
    ((void(__thiscall*)(IDrawable*, UIRenderer*, const Math::Rectangle&, const RenderParams&))ADDR(
        SporeStdDrawable, Paint))(this, r, a, p);
}

bool SporeStdDrawable::IsColliding(const Math::Rectangle& a, const Math::Point& pt, RenderParams& p) {
    return ((bool(__thiscall*)(IDrawable*, const Math::Rectangle&, const Math::Point&, RenderParams&))ADDR(
        SporeStdDrawable, IsColliding))(this, a, pt, p);
}

bool SporeStdDrawable::GetDimensions(Dimensions& d, int s, int i) {
    return ((bool(__thiscall*)(IDrawable*, Dimensions&, int, int))ADDR(SporeStdDrawable, GetDimensions))(this, d, s, i);
}

bool SporeStdDrawable::UseCollision(uint32_t t, bool& dst) {
    return ((bool(__thiscall*)(IDrawable*, uint32_t, bool&))ADDR(SporeStdDrawable, UseCollision))(this, t, dst);
}

Vector2 SporeStdDrawable::GetScale() const {
    return ((Vector2(__thiscall*)(const IDrawable*))ADDR(SporeStdDrawable, GetScale))(this);
}

}

#undef ADDR

