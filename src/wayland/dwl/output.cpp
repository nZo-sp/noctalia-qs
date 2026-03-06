#include "output.hpp"
#include <cstdint>
#include <utility>

#include <QList>
#include <QObject>
#include <QString>
#include <QtCore/qtmetamacros.h>
#include <QtTypes>

#include "proto.hpp"
#include "tag.hpp"

namespace qs::dwl {

ZdwlIpcOutputV2Listener DwlIpcOutput::listener = {
    .toggleVisibility = &DwlIpcOutput::onToggleVisibility,
    .active = &DwlIpcOutput::onActive,
    .tag = &DwlIpcOutput::onTag,
    .layout = &DwlIpcOutput::onLayout,
    .title = &DwlIpcOutput::onTitle,
    .appid = &DwlIpcOutput::onAppid,
    .layoutSymbol = &DwlIpcOutput::onLayoutSymbol,
    .frame = &DwlIpcOutput::onFrame,
    .fullscreen = &DwlIpcOutput::onFullscreen,
    .floating = &DwlIpcOutput::onFloating,
    // Mango
    .x = &DwlIpcOutput::onIgnoredInt,
    .y = &DwlIpcOutput::onIgnoredInt,
    .width = &DwlIpcOutput::onIgnoredInt,
    .height = &DwlIpcOutput::onIgnoredInt,
    .lastLayer = &DwlIpcOutput::onIgnoredStr,
    .kbLayout = &DwlIpcOutput::onKbLayout,
    .keymode = &DwlIpcOutput::onIgnoredStr,
    .scalefactor = &DwlIpcOutput::onIgnoredUint,
};

DwlIpcOutput::DwlIpcOutput(struct zdwl_ipc_output_v2* handle, QString name, QObject* parent)
    : QObject(parent)
    , mHandle(handle)
    , mOutputName(std::move(name)) {
	zdwlIpcOutputV2AddListener(this->mHandle, &listener, this);
}

DwlIpcOutput::~DwlIpcOutput() {
	if (this->mHandle) {
		zdwlIpcOutputV2Release(this->mHandle);
		this->mHandle = nullptr;
	}
}

bool DwlIpcOutput::active() const { return this->mActive; }
QString DwlIpcOutput::title() const { return this->mTitle; }
QString DwlIpcOutput::appId() const { return this->mAppId; }
quint32 DwlIpcOutput::layoutIndex() const { return this->mLayoutIndex; }
QString DwlIpcOutput::layoutSymbol() const { return this->mLayoutSymbol; }
bool DwlIpcOutput::fullscreen() const { return this->mFullscreen; }
bool DwlIpcOutput::floating() const { return this->mFloating; }
QList<DwlTag*> DwlIpcOutput::tags() const { return this->mTags; }
QString DwlIpcOutput::kbLayout() const { return this->mKbLayout; }
const QString& DwlIpcOutput::outputName() const { return this->mOutputName; }

void DwlIpcOutput::setTags(quint32 tagmask, quint32 toggleTagset) {
	zdwlIpcOutputV2SetTags(this->mHandle, tagmask, toggleTagset);
}
void DwlIpcOutput::setClientTags(quint32 andTags, quint32 xorTags) {
	zdwlIpcOutputV2SetClientTags(this->mHandle, andTags, xorTags);
}
void DwlIpcOutput::setLayout(quint32 index) { zdwlIpcOutputV2SetLayout(this->mHandle, index); }

void DwlIpcOutput::initTags(quint32 count) {
	for (DwlTag* t: this->mTags) t->deleteLater();
	this->mTags.clear();
	this->mTags.reserve(static_cast<qsizetype>(count));
	for (quint32 i = 0; i < count; ++i) this->mTags.append(new DwlTag(i, this));
	emit this->tagsChanged();
}

// Listener callbacks

void DwlIpcOutput::onToggleVisibility(void* data, struct zdwl_ipc_output_v2* /*unused*/) {
	auto* self = static_cast<DwlIpcOutput*>(data);
	emit self->toggleVisibility();
}

void DwlIpcOutput::onActive(void* data, struct zdwl_ipc_output_v2* /*unused*/, uint32_t active) {
	auto* self = static_cast<DwlIpcOutput*>(data);
	self->mPending.hasActive = true;
	self->mPending.active = active != 0;
}

void DwlIpcOutput::onTag(
    void* data,
    struct zdwl_ipc_output_v2* /*unused*/,
    uint32_t tag,
    uint32_t state,
    uint32_t clients,
    uint32_t focused
) {
	auto* self = static_cast<DwlIpcOutput*>(data);
	if (std::cmp_less(tag, self->mTags.size()))
		self->mTags[static_cast<qsizetype>(tag)]->updateState(state, clients, focused);
}

void DwlIpcOutput::onLayout(void* data, struct zdwl_ipc_output_v2* /*unused*/, uint32_t layout) {
	auto* self = static_cast<DwlIpcOutput*>(data);
	self->mPending.hasLayoutIndex = true;
	self->mPending.layoutIndex = layout;
}

void DwlIpcOutput::onTitle(void* data, struct zdwl_ipc_output_v2* /*unused*/, const char* title) {
	auto* self = static_cast<DwlIpcOutput*>(data);
	self->mPending.hasTitle = true;
	self->mPending.title = QString::fromUtf8(title);
}

void DwlIpcOutput::onAppid(void* data, struct zdwl_ipc_output_v2* /*unused*/, const char* appid) {
	auto* self = static_cast<DwlIpcOutput*>(data);
	self->mPending.hasAppId = true;
	self->mPending.appId = QString::fromUtf8(appid);
}

void DwlIpcOutput::onLayoutSymbol(
    void* data,
    struct zdwl_ipc_output_v2* /*unused*/,
    const char* layout
) {
	auto* self = static_cast<DwlIpcOutput*>(data);
	self->mPending.hasLayoutSymbol = true;
	self->mPending.layoutSymbol = QString::fromUtf8(layout);
}

void DwlIpcOutput::onFullscreen(
    void* data,
    struct zdwl_ipc_output_v2* /*unused*/,
    uint32_t isFullscreen
) {
	auto* self = static_cast<DwlIpcOutput*>(data);
	self->mPending.hasFullscreen = true;
	self->mPending.fullscreen = isFullscreen != 0;
}

void DwlIpcOutput::onFloating(
    void* data,
    struct zdwl_ipc_output_v2* /*unused*/,
    uint32_t isFloating
) {
	auto* self = static_cast<DwlIpcOutput*>(data);
	self->mPending.hasFloating = true;
	self->mPending.floating = isFloating != 0;
}

void DwlIpcOutput::onFrame(void* data, struct zdwl_ipc_output_v2* /*unused*/) {
	auto* self = static_cast<DwlIpcOutput*>(data);
	auto& p = self->mPending;

	if (p.hasActive && p.active != self->mActive) {
		self->mActive = p.active;
		emit self->activeChanged();
	}

	if (p.hasTitle && p.title != self->mTitle) {
		self->mTitle = p.title;
		emit self->titleChanged();
	}

	if (p.hasAppId && p.appId != self->mAppId) {
		self->mAppId = p.appId;
		emit self->appIdChanged();
	}

	if (p.hasLayoutIndex && p.layoutIndex != self->mLayoutIndex) {
		self->mLayoutIndex = p.layoutIndex;
		emit self->layoutIndexChanged();
	}

	if (p.hasLayoutSymbol && p.layoutSymbol != self->mLayoutSymbol) {
		self->mLayoutSymbol = p.layoutSymbol;
		emit self->layoutSymbolChanged();
	}

	if (p.hasFullscreen && p.fullscreen != self->mFullscreen) {
		self->mFullscreen = p.fullscreen;
		emit self->fullscreenChanged();
	}

	if (p.hasFloating && p.floating != self->mFloating) {
		self->mFloating = p.floating;
		emit self->floatingChanged();
	}

	if (p.hasKbLayout && p.kbLayout != self->mKbLayout) {
		self->mKbLayout = p.kbLayout;
		emit self->kbLayoutChanged();
	}

	p = {};
	emit self->frame();
}

void DwlIpcOutput::onKbLayout(
    void* data,
    struct zdwl_ipc_output_v2* /*unused*/,
    const char* kbLayout
) {
	auto* self = static_cast<DwlIpcOutput*>(data);
	self->mPending.hasKbLayout = true;
	self->mPending.kbLayout = QString::fromUtf8(kbLayout);
}

} // namespace qs::dwl
