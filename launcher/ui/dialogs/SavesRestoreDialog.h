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

#include <QDialog>

class QLabel;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;
class SavesClient;

/// Lists cloud worlds and their snapshots, and returns the selection
/// for a restore.
class SavesRestoreDialog : public QDialog {
    Q_OBJECT

   public:
    explicit SavesRestoreDialog(QWidget* parent = nullptr);

    QString selectedSlot() const { return m_slot; }
    QString selectedSnapshotId() const { return m_snapshotId; }

   private:
    void onSlotChanged(QListWidgetItem* current, QListWidgetItem* previous);
    void onSnapshotChanged();
    void updateButtons();

   private:
    SavesClient* m_client;
    QListWidget* m_slots;
    QTreeWidget* m_snapshots;
    QLabel* m_status;
    QPushButton* m_restoreButton;
    QString m_slot;
    QString m_snapshotId;
};
