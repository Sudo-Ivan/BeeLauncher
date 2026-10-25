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

#include "SavesRestoreDialog.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QLocale>
#include <QPushButton>
#include <QSplitter>
#include <QTreeWidget>
#include <QVBoxLayout>

#include "saves/SavesClient.h"

SavesRestoreDialog::SavesRestoreDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle(tr("Restore from Cloud"));
    resize(720, 420);

    m_client = new SavesClient(this);

    auto* layout = new QVBoxLayout(this);

    auto* splitter = new QSplitter(this);
    m_slots = new QListWidget(splitter);
    m_snapshots = new QTreeWidget(splitter);
    m_snapshots->setHeaderLabels({ tr("Date"), tr("Label"), tr("Device"), tr("Files"), tr("Size") });
    m_snapshots->setRootIsDecorated(false);
    splitter->addWidget(m_slots);
    splitter->addWidget(m_snapshots);
    splitter->setStretchFactor(1, 1);
    layout->addWidget(splitter);

    m_status = new QLabel(this);
    m_status->setWordWrap(true);
    layout->addWidget(m_status);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    m_restoreButton = buttons->addButton(tr("Restore"), QDialogButtonBox::AcceptRole);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);

    connect(m_slots, &QListWidget::currentItemChanged, this, &SavesRestoreDialog::onSlotChanged);
    connect(m_snapshots, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem*, QTreeWidgetItem*) { onSnapshotChanged(); });

    m_status->setText(tr("Loading worlds..."));
    m_client->listSlots([this](const QList<SavesClient::SlotInfo>& slotList, const QString& error) {
        if (!error.isEmpty()) {
            m_status->setText(error);
            return;
        }
        m_status->clear();
        if (slotList.isEmpty()) {
            m_status->setText(tr("No cloud worlds found."));
            return;
        }
        for (const SavesClient::SlotInfo& slot : slotList) {
            auto* item = new QListWidgetItem(slot.name, m_slots);
            item->setData(Qt::UserRole, slot.name);
        }
        m_slots->setCurrentRow(0);
    });
    updateButtons();
}

void SavesRestoreDialog::onSlotChanged(QListWidgetItem* current, QListWidgetItem*)
{
    m_snapshots->clear();
    m_slot.clear();
    m_snapshotId.clear();
    if (!current) {
        updateButtons();
        return;
    }
    const QString slot = current->data(Qt::UserRole).toString();
    m_status->setText(tr("Loading snapshots..."));
    m_client->listSnapshots(slot, [this, slot](const QList<SavesClient::SnapshotInfo>& snapshots, const QString& error) {
        m_status->clear();
        if (!error.isEmpty()) {
            m_status->setText(error);
            return;
        }
        for (const SavesClient::SnapshotInfo& snap : snapshots) {
            auto* item = new QTreeWidgetItem(m_snapshots);
            item->setText(0, snap.committedAt.toLocalTime().toString(QLocale().dateTimeFormat(QLocale::ShortFormat)));
            item->setText(1, snap.label);
            item->setText(2, snap.device);
            item->setText(3, QString::number(snap.fileCount));
            item->setText(4, QLocale().formattedDataSize(snap.totalBytes));
            item->setData(0, Qt::UserRole, snap.id);
            item->setData(0, Qt::UserRole + 1, slot);
        }
        if (m_snapshots->topLevelItemCount() > 0)
            m_snapshots->setCurrentItem(m_snapshots->topLevelItem(0));
        else
            m_status->setText(tr("No snapshots for this world."));
        onSnapshotChanged();
    });
}

void SavesRestoreDialog::onSnapshotChanged()
{
    auto* item = m_snapshots->currentItem();
    if (item) {
        m_snapshotId = item->data(0, Qt::UserRole).toString();
        m_slot = item->data(0, Qt::UserRole + 1).toString();
    } else {
        m_snapshotId.clear();
        m_slot.clear();
    }
    updateButtons();
}

void SavesRestoreDialog::updateButtons()
{
    m_restoreButton->setEnabled(!m_snapshotId.isEmpty());
}
