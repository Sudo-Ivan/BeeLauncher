// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Bee Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Bee Launcher Contributors
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "BackupsPage.h"
#include "ui_BackupsPage.h"

#include <QFileInfo>
#include <QMessageBox>
#include <QTreeWidgetItem>

#include "Application.h"
#include "DesktopServices.h"
#include "FileSystem.h"
#include "StringUtils.h"
#include "MMCZip.h"
#include "archive/ExtractZipTask.h"
#include "minecraft/InstanceBackup.h"
#include "ui/dialogs/CustomMessageBox.h"
#include "ui/dialogs/ProgressDialog.h"

BackupsPage::BackupsPage(BaseInstance* inst, QWidget* parent) : QWidget(parent), ui(new Ui::BackupsPage), m_inst(inst)
{
    ui->setupUi(this);
    refreshList();
}

BackupsPage::~BackupsPage()
{
    delete ui;
}

void BackupsPage::retranslate()
{
    ui->retranslateUi(this);
}

void BackupsPage::openedImpl()
{
    auto settings = m_inst->settings();
    m_loadingSettings = true;
    ui->autoBackupGroup->setChecked(settings->get("AutoBackupEnabled").toBool());
    ui->intervalSpinBox->setValue(settings->get("AutoBackupIntervalHours").toInt());
    ui->keepSpinBox->setValue(settings->get("AutoBackupKeepCount").toInt());
    m_loadingSettings = false;
    refreshList();
}

void BackupsPage::refreshList()
{
    ui->backupsTree->clear();
    for (const auto& info : InstanceBackup::listBackups(m_inst)) {
        auto* item = new QTreeWidgetItem(ui->backupsTree);
        item->setText(0, info.fileName());
        item->setText(1, info.lastModified().toString(Qt::ISODate));
        item->setText(2, StringUtils::humanReadableFileSize(info.size(), true));
        item->setData(0, Qt::UserRole, info.absoluteFilePath());
    }
    ui->backupsTree->resizeColumnToContents(0);
}

QString BackupsPage::selectedBackupPath() const
{
    auto* item = ui->backupsTree->currentItem();
    if (!item)
        return {};
    return item->data(0, Qt::UserRole).toString();
}

void BackupsPage::on_createButton_clicked()
{
    if (m_inst->isRunning()) {
        CustomMessageBox::selectable(this, tr("Instance is running"),
                                     tr("Stop the instance before creating a backup."), QMessageBox::Warning)
            ->show();
        return;
    }
    auto task = makeShared<BackupInstanceTask>(m_inst, 0,
                                               ui->keepSpinBox->value(), true);
    ProgressDialog dialog(this);
    dialog.execWithTask(task.get());
    if (!task->createdPath().isEmpty())
        refreshList();
}

void BackupsPage::on_restoreButton_clicked()
{
    QString path = selectedBackupPath();
    if (path.isEmpty()) {
        CustomMessageBox::selectable(this, tr("No backup selected"), tr("Select a backup to restore."), QMessageBox::Warning)->show();
        return;
    }
    if (m_inst->isRunning()) {
        CustomMessageBox::selectable(this, tr("Instance is running"),
                                     tr("Stop the instance before restoring a backup."), QMessageBox::Warning)
            ->show();
        return;
    }

    auto response = CustomMessageBox::selectable(
                        this, tr("Restore backup?"),
                        tr("This replaces the current instance files with the contents of %1.\n\nExisting files will be overwritten.")
                            .arg(QFileInfo(path).fileName()),
                        QMessageBox::Question, QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
                        ->exec();
    if (response != QMessageBox::Yes)
        return;

    auto task = makeShared<MMCZip::ExtractZipTask>(path, QDir(m_inst->gameRoot()));
    ProgressDialog dialog(this);
    dialog.execWithTask(task.get());
}

void BackupsPage::on_deleteButton_clicked()
{
    QString path = selectedBackupPath();
    if (path.isEmpty())
        return;
    QFile::remove(path);
    refreshList();
}

void BackupsPage::on_openFolderButton_clicked()
{
    DesktopServices::openPath(InstanceBackup::backupsDirPath(m_inst), true);
}

void BackupsPage::on_refreshButton_clicked()
{
    refreshList();
}

void BackupsPage::on_autoBackupGroup_toggled(bool checked)
{
    if (!m_loadingSettings)
        m_inst->settings()->set("AutoBackupEnabled", checked);
}

void BackupsPage::on_intervalSpinBox_valueChanged(int value)
{
    if (!m_loadingSettings)
        m_inst->settings()->set("AutoBackupIntervalHours", value);
}

void BackupsPage::on_keepSpinBox_valueChanged(int value)
{
    if (!m_loadingSettings)
        m_inst->settings()->set("AutoBackupKeepCount", value);
}
