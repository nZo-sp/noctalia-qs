#pragma once

#include <QHash>
#include <QList>
#include <QObject>
#include <QStringList>
#include <QtGui/qscreen.h>
#include <QtTypes>
#include <QtWaylandClient/QWaylandClientExtension>
#include <qtmetamacros.h>
#include <wayland-client-protocol.h>

#include "output.hpp"
#include "proto.hpp"

namespace qs::dwl {

class DwlIpcManager: public QWaylandClientExtension {
	Q_OBJECT;

public:
	explicit DwlIpcManager();

	[[nodiscard]] quint32 tagCount() const;
	[[nodiscard]] QStringList layouts() const;
	[[nodiscard]] QList<DwlIpcOutput*> outputs() const;

	DwlIpcOutput* bindOutput(struct wl_output* wlOutput, const QString& name);
	void removeOutput(struct wl_output* wlOutput);

	static DwlIpcManager* instance();

	[[nodiscard]] const wl_interface* extensionInterface() const override;
	void bind(struct ::wl_registry* registry, int id, int version) override;

signals:
	void tagCountChanged();
	void layoutsChanged();
	void outputAdded(DwlIpcOutput* output);
	void outputRemoved(DwlIpcOutput* output);

private slots:
	void onScreenAdded(QScreen* screen);
	void onScreenRemoved(QScreen* screen);

private:
	static void onTags(void* data, struct zdwl_ipc_manager_v2* /*unused*/, uint32_t amount);
	static void onLayout(void* data, struct zdwl_ipc_manager_v2* /*unused*/, const char* name);
	static ZdwlIpcManagerV2Listener listener;

	struct zdwl_ipc_manager_v2* mHandle = nullptr;
	quint32 mTagCount = 0;
	QStringList mLayouts;
	QList<DwlIpcOutput*> mOutputs;
	QHash<struct wl_output*, DwlIpcOutput*> mOutputMap;
};

} // namespace qs::dwl
