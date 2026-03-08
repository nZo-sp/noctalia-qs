#include "ext_workspace.hpp"

#include <qloggingcategory.h>
#include <qobject.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qwayland-ext-workspace-v1.h>
#include <wayland-ext-workspace-v1-client-protocol.h>

namespace qs::wayland::workspace {

Q_LOGGING_CATEGORY(logWorkspace, "quickshell.wm.wayland.workspace", QtWarningMsg);

WorkspaceManager::WorkspaceManager(): QWaylandClientExtensionTemplate(1) { this->initialize(); }

WorkspaceManager* WorkspaceManager::instance() {
	static auto* instance = new WorkspaceManager();
	return instance;
}

void WorkspaceManager::ext_workspace_manager_v1_workspace_group(
    ::ext_workspace_group_handle_v1* handle
) {
	auto* group = new WorkspaceGroup(handle);
	this->mGroups.insert(handle, group);
	emit this->groupCreated(group);
}

void WorkspaceManager::ext_workspace_manager_v1_workspace(::ext_workspace_handle_v1* handle) {
	auto* workspace = new Workspace(handle);
	this->mWorkspaces.insert(handle, workspace);
	emit this->workspaceCreated(workspace);
};

void WorkspaceManager::destroyWorkspace(Workspace* workspace) {
	this->mWorkspaces.remove(workspace->object());
	this->destroyedWorkspaces.append(workspace);
	emit this->workspaceDestroyed(workspace);
}

void WorkspaceManager::destroyGroup(WorkspaceGroup* group) {
	this->mGroups.remove(group->object());
	this->destroyedGroups.append(group);
	emit this->groupDestroyed(group);
}

void WorkspaceManager::ext_workspace_manager_v1_done() {
	emit this->serverCommit();

	for (auto* workspace: this->destroyedWorkspaces) delete workspace;
	for (auto* group: this->destroyedGroups) delete group;
	this->destroyedWorkspaces.clear();
	this->destroyedGroups.clear();
}

void WorkspaceManager::ext_workspace_manager_v1_finished() {
	qCWarning(logWorkspace) << "ext_workspace_manager_v1.finished() was received";
}

Workspace::~Workspace() {
	if (this->isInitialized()) this->destroy();
}

void Workspace::ext_workspace_handle_v1_id(const QString& id) { this->id = id; }

void Workspace::ext_workspace_handle_v1_name(const QString& name) { this->name = name; }

void Workspace::ext_workspace_handle_v1_coordinates(wl_array* coordinates) {
	this->coordinates.clear();

	auto* data = static_cast<qint32*>(coordinates->data);
	auto size = static_cast<qsizetype>(coordinates->size / sizeof(qint32));

	for (auto i = 0; i != size; ++i) {
		this->coordinates.append(data[i]); // NOLINT
	}
}

void Workspace::ext_workspace_handle_v1_state(quint32 state) {
	this->active = state & ext_workspace_handle_v1::state_active;
	this->urgent = state & ext_workspace_handle_v1::state_urgent;
	this->hidden = state & ext_workspace_handle_v1::state_hidden;
}

void Workspace::ext_workspace_handle_v1_capabilities(quint32 capabilities) {
	this->canActivate = capabilities & ext_workspace_handle_v1::workspace_capabilities_activate;
	this->canDeactivate = capabilities & ext_workspace_handle_v1::workspace_capabilities_deactivate;
	this->canRemove = capabilities & ext_workspace_handle_v1::workspace_capabilities_remove;
	this->canAssign = capabilities & ext_workspace_handle_v1::workspace_capabilities_assign;
}

void Workspace::ext_workspace_handle_v1_removed() {
	WorkspaceManager::instance()->destroyWorkspace(this);
	this->destroy();
}

void Workspace::enterGroup(WorkspaceGroup* group) { this->group = group; }

void Workspace::leaveGroup(WorkspaceGroup* group) {
	if (this->group == group) this->group = nullptr;
}

WorkspaceGroup::~WorkspaceGroup() {
	if (this->isInitialized()) this->destroy();
}

void WorkspaceGroup::ext_workspace_group_handle_v1_capabilities(uint32_t capabilities) {
	this->canCreateWorkspace =
	    capabilities & ext_workspace_group_handle_v1::group_capabilities_create_workspace;
}

void WorkspaceGroup::ext_workspace_group_handle_v1_output_enter(::wl_output* output) {
	this->screens.addOutput(output);
}

void WorkspaceGroup::ext_workspace_group_handle_v1_output_leave(::wl_output* output) {
	this->screens.removeOutput(output);
}

void WorkspaceGroup::ext_workspace_group_handle_v1_workspace_enter(::ext_workspace_handle_v1* handle
) {
	auto* workspace = WorkspaceManager::instance()->mWorkspaces.value(handle);
	if (workspace) workspace->enterGroup(this);
}

void WorkspaceGroup::ext_workspace_group_handle_v1_workspace_leave(::ext_workspace_handle_v1* handle
) {
	auto* workspace = WorkspaceManager::instance()->mWorkspaces.value(handle);
	if (workspace) workspace->leaveGroup(this);
}

void WorkspaceGroup::ext_workspace_group_handle_v1_removed() {
	WorkspaceManager::instance()->destroyGroup(this);
	this->destroy();
}

} // namespace qs::wayland::workspace
