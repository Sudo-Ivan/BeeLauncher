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

#include <QAtomicInt>
#include <QFuture>
#include <QFutureWatcher>
#include <tasks/Task.h>

class MinecraftInstance;

// Verifies the files an instance depends on (libraries, client jar, assets)
// against their expected checksums. Missing or corrupted files are marked
// stale in the meta cache or deleted, so the regular update tasks that follow
// re-download only what is broken.
class RepairVerifyTask : public Task {
    Q_OBJECT
   public:
    RepairVerifyTask(MinecraftInstance* inst) : m_inst(inst) {}
    virtual ~RepairVerifyTask() = default;

    bool canAbort() const override { return true; }

    int checkedCount() const { return m_checked; }
    int brokenCount() const { return m_broken; }

   protected:
    void executeTask() override;
    bool abort() override;

   private:
    void doWork();
    bool fileMatchesSha1(const QString& path, const QString& expectedSha1);
    void verifyLibraries();
    void verifyAssets();

   private:
    MinecraftInstance* m_inst;
    QAtomicInt m_checked{ 0 };
    QAtomicInt m_broken{ 0 };
    QAtomicInt m_cancel{ 0 };

    QFuture<void> m_future;
    QFutureWatcher<void> m_watcher;
};
