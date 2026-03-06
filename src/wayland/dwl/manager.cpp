#include "manager.hpp"
#include <cstdint>

#include <QApplication>
#include <QByteArray>
#include <QGuiApplication>
#include <QList>
#include <QLoggingCategory>
#include <QMetaObject>
#include <QScreen>
#include <QString>
#include <Qt>
#include <QtCore/qtmetamacros.h>
#include <QtLogging>
#include <QtMinMax>
#include <QtTypes>
#include <QtWaylandClient/QWaylandClientExtension>
#include <qlist.h>
#include <qpa/qplatformnativeinterface.h>
#include <qstringlist.h>
#include <wayland-client-protocol.h>
#include <wayland-util.h>

#include "../../core/logcat.hpp"
#include "output.hpp"
#include "proto.hpp"

namespace {
QS_LOGGING_CATEGORY(logDwlIpc, "quickshell.dwl", QtWarningMsg);
}

namespace qs::dwl {

ZdwlIpcManagerV2Listener DwlIpcManager::listener = {
    .tags = &DwlIpcManager::onTags,
    .layout = &DwlIpcManager::onLayout,
};

DwlIpcManager::DwlIpcManager(): QWaylandClientExtension(2) {
	connect(this, &QWaylandClientExtension::activeChanged, this, [this]() {
		if (!this->isActive()) {
			qCWarning(logDwlIpc) << "DWL is not active";
			return;
		}

		const auto screens = QGuiApplication::screens();
		qCDebug(logDwlIpc) << "screen count:" << screens.size();

		for (QScreen* screen: screens) this->onScreenAdded(screen);

		connect(qApp, &QGuiApplication::screenAdded, this, &DwlIpcManager::onScreenAdded);
		connect(qApp, &QGuiApplication::screenRemoved, this, &DwlIpcManager::onScreenRemoved);
	});

	QMetaObject::invokeMethod(
	    this,
	    [this]() {
		    if (!this->isActive()) qCWarning(logDwlIpc) << "DWL is not available";
	    },
	    Qt::QueuedConnection
	);
}

DwlIpcManager* DwlIpcManager::instance() {
	static DwlIpcManager* instance = nullptr;
	if (!instance) instance = new DwlIpcManager();
	return instance;
}

const wl_interface* DwlIpcManager::extensionInterface() const {
	return &ZDWL_IPC_MANAGER_V2_INTERFACE;
}

void DwlIpcManager::bind(struct ::wl_registry* registry, int id, int version) {
	this->mHandle = static_cast<struct zdwl_ipc_manager_v2*>(
	    wl_registry_bind(registry, id, &ZDWL_IPC_MANAGER_V2_INTERFACE, qMin(version, 2))
	);

	zdwlIpcManagerV2AddListener(this->mHandle, &listener, this);
}

quint32 DwlIpcManager::tagCount() const { return this->mTagCount; }
// NOLINTNEXTLINE(misc-include-cleaner)
QStringList DwlIpcManager::layouts() const { return this->mLayouts; }
QList<DwlIpcOutput*> DwlIpcManager::outputs() const { return this->mOutputs; }

void DwlIpcManager::onScreenAdded(QScreen* screen) {
	auto* ni = qGuiApp->platformNativeInterface();
	if (!ni) return;

	auto* output = static_cast<struct wl_output*>(
	    ni->nativeResourceForScreen(QByteArrayLiteral("output"), screen)
	);

	if (!output) return;
	this->bindOutput(output, screen->name());
}

void DwlIpcManager::onScreenRemoved(QScreen* screen) {
	auto* ni = qGuiApp->platformNativeInterface();
	if (!ni) return;

	auto* output = static_cast<struct wl_output*>(
	    ni->nativeResourceForScreen(QByteArrayLiteral("output"), screen)
	);

	if (!output) return;
	this->removeOutput(output);
}

DwlIpcOutput* DwlIpcManager::bindOutput(struct wl_output* wlOutput, const QString& name) {
	if (auto* existing = this->mOutputMap.value(wlOutput, nullptr)) return existing;

	auto* raw = zdwlIpcManagerV2GetOutput(this->mHandle, wlOutput);
	if (!raw) {
		qCWarning(logDwlIpc) << "get_output returned null for" << name;
		return nullptr;
	}

	auto* output = new DwlIpcOutput(raw, name, this);
	output->initTags(this->mTagCount);
	this->mOutputs.append(output);
	this->mOutputMap.insert(wlOutput, output);
	emit this->outputAdded(output);
	return output;
}

void DwlIpcManager::removeOutput(struct wl_output* wlOutput) {
	auto* output = this->mOutputMap.take(wlOutput);
	if (!output) return;

	this->mOutputs.removeOne(output);
	emit this->outputRemoved(output);
	output->deleteLater();
}

void DwlIpcManager::onTags(void* data, struct zdwl_ipc_manager_v2* /*unused*/, uint32_t amount) {
	auto* self = static_cast<DwlIpcManager*>(data);
	if (amount == self->mTagCount) return;

	self->mTagCount = amount;
	for (DwlIpcOutput* o: self->mOutputs) o->initTags(amount);
	emit self->tagCountChanged();
}

void DwlIpcManager::onLayout(void* data, struct zdwl_ipc_manager_v2* /*unused*/, const char* name) {
	auto* self = static_cast<DwlIpcManager*>(data);
	self->mLayouts.append(QString::fromUtf8(name));
	emit self->layoutsChanged();
}

} // namespace qs::dwl
