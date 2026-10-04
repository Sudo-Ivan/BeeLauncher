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

#pragma once

#include <QWidget>

#include "migrate/DetectedProfile.h"
#include "ui/pages/BasePage.h"

namespace Ui {
class MigratePage;
}

class NewInstanceDialog;

// New-instance page listing game profiles detected in other launchers,
// for one-click migration.
class MigratePage : public QWidget, public BasePage {
    Q_OBJECT

   public:
    explicit MigratePage(NewInstanceDialog* dialog, QWidget* parent = 0);
    virtual ~MigratePage();

    virtual QString displayName() const override { return tr("Migrate"); }
    virtual QIcon icon() const override { return QIcon::fromTheme("copy"); }
    virtual QString id() const override { return "migrate"; }
    virtual QString helpPage() const override { return "Migrate"; }
    void retranslate() override;

    void openedImpl() override;

   private slots:
    void on_refreshButton_clicked();
    void on_profileTree_currentItemChanged(class QTreeWidgetItem* current, class QTreeWidgetItem* previous);

   private:
    void rescan();
    const DetectedProfile* selectedProfile() const;

   private:
    Ui::MigratePage* ui = nullptr;
    NewInstanceDialog* dialog = nullptr;
    QList<DetectedProfile> m_profiles;
};
