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

#include "BaseInstance.h"
#include "ui/pages/BasePage.h"

namespace Ui {
class BackupsPage;
}

class BackupsPage : public QWidget, public BasePage {
    Q_OBJECT

   public:
    explicit BackupsPage(BaseInstance* inst, QWidget* parent = 0);
    virtual ~BackupsPage();

    virtual QString displayName() const override { return tr("Backups"); }
    virtual QIcon icon() const override { return QIcon::fromTheme("export"); }
    virtual QString id() const override { return "backups"; }
    virtual QString helpPage() const override { return "Backups"; }
    void retranslate() override;

    void openedImpl() override;

   private slots:
    void on_createButton_clicked();
    void on_restoreButton_clicked();
    void on_deleteButton_clicked();
    void on_openFolderButton_clicked();
    void on_refreshButton_clicked();
    void on_autoBackupGroup_toggled(bool checked);
    void on_intervalSpinBox_valueChanged(int value);
    void on_keepSpinBox_valueChanged(int value);

   private:
    void refreshList();
    QString selectedBackupPath() const;

   private:
    Ui::BackupsPage* ui;
    BaseInstance* m_inst;
    bool m_loadingSettings = false;
};
