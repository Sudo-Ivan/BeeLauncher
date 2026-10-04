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

#include "MigratePage.h"
#include "ui_MigratePage.h"

#include <QMap>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include "migrate/MigrateInstanceTask.h"
#include "ui/dialogs/NewInstanceDialog.h"

MigratePage::MigratePage(NewInstanceDialog* dialog, QWidget* parent) : QWidget(parent), ui(new Ui::MigratePage), dialog(dialog)
{
    ui->setupUi(this);
}

MigratePage::~MigratePage()
{
    delete ui;
}

void MigratePage::retranslate()
{
    ui->retranslateUi(this);
}

void MigratePage::openedImpl()
{
    rescan();
}

void MigratePage::rescan()
{
    m_profiles = LauncherMigration::detectProfiles();

    ui->profileTree->clear();
    QMap<QString, QTreeWidgetItem*> groups;
    for (int i = 0; i < m_profiles.size(); ++i) {
        const auto& profile = m_profiles.at(i);

        auto* group = groups.value(profile.launcherName, nullptr);
        if (!group) {
            group = new QTreeWidgetItem(ui->profileTree);
            group->setText(0, profile.launcherName);
            group->setFlags(Qt::ItemIsEnabled);
            groups.insert(profile.launcherName, group);
        }

        auto* item = new QTreeWidgetItem(group);
        item->setText(0, profile.name);
        item->setText(1, profile.mcVersion.isEmpty() ? tr("unknown") : profile.mcVersion);
        item->setText(2, profile.loaderUid.isEmpty()
                            ? tr("none")
                            : (profile.loaderVersion.isEmpty() ? profile.loaderUid.section('.', -1)
                                                               : tr("%1 %2").arg(profile.loaderUid.section('.', -1), profile.loaderVersion)));
        item->setData(0, Qt::UserRole, i);
        group->addChild(item);
    }
    ui->profileTree->expandAll();

    ui->emptyLabel->setVisible(m_profiles.isEmpty());
    ui->profileTree->setVisible(!m_profiles.isEmpty());
    on_profileTree_currentItemChanged(ui->profileTree->currentItem(), nullptr);
}

const DetectedProfile* MigratePage::selectedProfile() const
{
    auto* item = ui->profileTree->currentItem();
    if (!item)
        return nullptr;
    bool ok = false;
    int index = item->data(0, Qt::UserRole).toInt(&ok);
    if (!ok || index < 0 || index >= m_profiles.size())
        return nullptr;
    return &m_profiles.at(index);
}

void MigratePage::on_profileTree_currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)
{
    const auto* profile = selectedProfile();
    if (!profile) {
        dialog->setSuggestedPack();
        return;
    }
    dialog->setSuggestedPack(profile->name, new MigrateInstanceTask(*profile));
}

void MigratePage::on_refreshButton_clicked()
{
    rescan();
}
