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

#include <QList>
#include <QString>

namespace CrashAnalyzer {

struct Finding {
    QString title;
    QString details;
    QStringList evidence;
};

// Scan Minecraft log or crash report text for known failure patterns.
// The returned list is deduplicated by rule, most relevant first.
QList<Finding> analyzeText(const QString& text);

// Scan the instance game directory: the tail of logs/latest.log plus the
// newest crash report in crash-reports/, if one exists.
// gameRoot is the instance's .minecraft directory.
// maxBytes bounds how much of each file is read from the end.
QList<Finding> analyzeGameDir(const QString& gameRoot, qint64 maxBytes = 512 * 1024);

}  // namespace CrashAnalyzer
