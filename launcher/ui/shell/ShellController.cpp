#include "ShellController.h"
#include "../../Application.h"
#include "../../InstanceList.h"
#include "../../BaseInstance.h"
#include "../../DesktopServices.h"

ShellController::ShellController(QObject* parent) : QObject(parent) {}

static BaseInstance* inst(const QString& id)
{
    return APPLICATION->instances()->getInstanceById(id);
}

void ShellController::launch(const QString& id)
{
    if (auto* i = inst(id))
        APPLICATION->launch(i);
}

void ShellController::kill(const QString&)
{
    emit toast(tr("Stop runs from the instance page"));
}

void ShellController::edit(const QString& id)
{
    if (auto* i = inst(id))
        APPLICATION->showInstanceWindow(i);
}

void ShellController::openFolder(const QString& id)
{
    if (auto* i = inst(id))
        DesktopServices::openPath(i->instanceRoot(), true);
}

void ShellController::changeGroup(const QString& id, const QString& group)
{
    if (auto* i = inst(id))
        APPLICATION->instances()->setInstanceGroup(i->id(), group);
}

void ShellController::addInstance()
{
    // stub: the add instance flow is a private MainWindow slot
    emit toast(tr("Add Instance opens in the classic interface for now"));
}

void ShellController::openSettings()
{
    // stub: private MainWindow slot
    emit toast(tr("Settings open in the classic interface for now"));
}

void ShellController::openAccounts()
{
    // stub: private MainWindow slot
    emit toast(tr("Account management opens in the classic interface for now"));
}

void ShellController::copyInstance(const QString&) { emit toast(tr("Copy is available in the classic interface for now")); }
void ShellController::deleteInstance(const QString&) { emit toast(tr("Delete is available in the classic interface for now")); }
void ShellController::createShortcut(const QString&) { emit toast(tr("Shortcuts are available in the classic interface for now")); }
void ShellController::backupNow(const QString&) { emit toast(tr("Backups are available in the classic interface for now")); }
void ShellController::repair(const QString&) { emit toast(tr("Repair is available in the classic interface for now")); }
