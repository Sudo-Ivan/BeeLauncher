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

#include "InstanceCreationTask.h"
#include "migrate/DetectedProfile.h"

// Creates an instance from a profile detected in another launcher's data
// directory and copies its game content (mods, saves, configs, etc.) over.
class MigrateInstanceTask : public InstanceCreationTask {
    Q_OBJECT

   public:
    explicit MigrateInstanceTask(const DetectedProfile& profile) : m_profile(profile) {}
    virtual ~MigrateInstanceTask() = default;

   protected:
    std::unique_ptr<MinecraftInstance> createInstance() override;

   private:
    bool copyContent(const QString& destinationRoot);
    bool copyRecursively(const QString& sourceDir, const QString& destDir);

   private:
    DetectedProfile m_profile;
};
