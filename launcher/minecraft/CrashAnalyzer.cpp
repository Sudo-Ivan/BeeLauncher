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

#include "CrashAnalyzer.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QRegularExpression>
#include <QSet>

namespace CrashAnalyzer {

namespace {

struct Rule {
    QRegularExpression pattern;
    const char* title;
    const char* details;
};

using RE = QRegularExpression;
constexpr auto CI = QRegularExpression::CaseInsensitiveOption;

const QVector<Rule>& rules()
{
    static const QVector<Rule> s_rules = {
        { RE("OutOfMemoryError", CI), QT_TR_NOOP("Out of memory"),
          QT_TR_NOOP("Minecraft ran out of memory. Increase the maximum memory allocation in the instance settings "
                     "(Java tab) or reduce the number of mods.") },
        { RE("Could not reserve enough space for object heap|Error occurred during initialization of VM|Invalid maximum "
             "heap size",
             CI),
          QT_TR_NOOP("JVM failed to start"),
          QT_TR_NOOP("The Java VM could not allocate its heap. Check that the maximum memory is not set higher than your "
                     "available RAM and that you are using a 64-bit Java installation.") },
        { RE("UnsupportedClassVersionError|has been compiled by a more recent version of the Java Runtime",
             CI),
          QT_TR_NOOP("Wrong Java version"),
          QT_TR_NOOP("A mod or the game itself was built for a newer Java version than the one selected. Pick a newer "
                     "Java runtime in the instance settings or let the launcher download one automatically.") },
        { RE("UnsatisfiedLinkError|Can't load library|amd64.*x86|x86.*amd64|64-bit.*32-bit|32-bit.*64-bit",
             CI),
          QT_TR_NOOP("Wrong Java architecture or missing natives"),
          QT_TR_NOOP("The Java runtime or a native library does not match your CPU architecture, or native libraries "
                     "failed to load. Use a Java build for your system architecture and verify the instance libraries.") },
        { RE("Pixel format not accelerated|driver does not appear to support OpenGL|Failed to create GLFW window|"
             "GLFW error.*(initializ|window)|OpenGL.*(unsupported|too old)",
             CI),
          QT_TR_NOOP("Graphics driver problem"),
          QT_TR_NOOP("The graphics driver failed to provide a usable OpenGL context. Update your GPU drivers, and on "
                     "laptops make sure the game uses the dedicated GPU instead of the integrated one.") },
        { RE("EXCEPTION_ACCESS_VIOLATION|hs_err_pid|SIGSEGV|fatal error has been detected by the Java Runtime",
             CI),
          QT_TR_NOOP("Native crash"),
          QT_TR_NOOP("The crash happened in native code, most often a graphics driver, an overlay (Discord, MSI "
                     "Afterburner, etc.) or a mod with native libraries. Check the crash-reports folder and update "
                     "drivers or disable overlays.") },
        { RE("MixinApplyError|Mixin apply.*failed|InvalidMixinException|InjectionError|Critical injection failure|"
             "ApplyError",
             CI),
          QT_TR_NOOP("Mixin conflict between mods"),
          QT_TR_NOOP("A mod failed to patch the game, usually because two mods modify the same code in incompatible "
                     "ways. Update or remove recently added mods, and check the mod list for duplicates or known "
                     "conflicts.") },
        { RE("ModResolutionException|could not find required mod|requires .* of mod|Missing or unsupported mandatory "
             "dependencies|Incompatible mods? found|Incompatible mod set|Breaks version of mod|missing mandatory",
             CI),
          QT_TR_NOOP("Missing or incompatible mod dependencies"),
          QT_TR_NOOP("A mod requires another mod or version that is not installed. Read the mod list error lines to see "
                     "which dependency is missing or conflicting, then install or update it.") },
        { RE("Duplicate mods? found|duplicate mod|Found duplicate", CI),
          QT_TR_NOOP("Duplicate mods"),
          QT_TR_NOOP("The same mod is present more than once in the mods folder. Remove the duplicate files.") },
        { RE("NoSuchMethodError|NoSuchFieldError|NoClassDefFoundError|ClassNotFoundException|AbstractMethodError|"
             "IncompatibleClassChangeError"),
          QT_TR_NOOP("Incompatible mod version"),
          QT_TR_NOOP("A mod called code that does not exist in this game or library version. The mod was likely built "
                     "for a different Minecraft or loader version. Update the affected mod to a matching build.") },
        { RE("Caught exception from \\w+|Error loading mod|ModLoadingException|error during \\w+ (event|phase)|"
             "Failed to load mod|Constructing Mod.*Failed|error while loading",
             CI),
          QT_TR_NOOP("A mod failed to load"),
          QT_TR_NOOP("One of the mods crashed during startup. The log lines below the report usually name the "
                     "offending mod. Try updating or removing it.") },
        { RE("Ticking entity|Ticking block entity|Ticking world|Watching Server|hang watch|server thread.*hang", CI),
          QT_TR_NOOP("World or ticking crash"),
          QT_TR_NOOP("The game crashed while processing an entity, block entity or the world itself. The crash report "
                     "names the entity type and location. Removing the offending entity or mod, or restoring a world "
                     "backup, usually fixes it.") },
        { RE("Failed to synchronize registry data|Registry.*(mismatch|missing entries)|Fatally missing registry", CI),
          QT_TR_NOOP("Mod mismatch with server"),
          QT_TR_NOOP("Your mod list does not match the server's. Make sure the same mods and versions are installed "
                     "on both sides.") },
        { RE("zip END header not found|invalid CEN header|JarInputStream.*invalid|Failed to read.*jar|corrupt.*jar|"
             "invalid signature file|SecurityException.*signature",
             CI),
          QT_TR_NOOP("Corrupt file"),
          QT_TR_NOOP("A jar file is corrupted or only partially downloaded. Re-download the affected files or run the "
                     "instance repair from the right-click menu.") },
        { RE("Failed to (log ?in|authenticate)|Invalid session|Authentication.*(fail|error|unavailable)|"
             "authserver.*(unreachable|down)|session.*invalid",
             CI),
          QT_TR_NOOP("Authentication problem"),
          QT_TR_NOOP("The account session is invalid or the authentication servers could not be reached. Re-add the "
                     "account or try again later.") },
        { RE("Failed to bind to port|Address already in use|BindException", CI),
          QT_TR_NOOP("Port already in use"),
          QT_TR_NOOP("The game tried to open a network port that is already taken, usually by another running game "
                     "instance or the Open to LAN feature. Close the other instance or wait a moment.") },
    };
    return s_rules;
}

QList<Finding> runRules(const QString& text)
{
    QList<Finding> found;
    QHash<int, int> ruleToFinding;
    const QStringList lines = text.split('\n');

    for (const QString& line : lines) {
        const QString trimmed = line.trimmed();
        if (trimmed.isEmpty())
            continue;

        const auto& allRules = rules();
        for (int i = 0; i < allRules.size(); ++i) {
            const Rule& rule = allRules.at(i);
            if (!rule.pattern.match(trimmed).hasMatch())
                continue;

            int idx = ruleToFinding.value(i, -1);
            if (idx == -1) {
                Finding f;
                f.title = QObject::tr(rule.title);
                f.details = QObject::tr(rule.details);
                found.append(f);
                idx = found.size() - 1;
                ruleToFinding.insert(i, idx);
            }
            auto& finding = found[idx];
            if (finding.evidence.size() < 3 && !finding.evidence.contains(trimmed))
                finding.evidence.append(trimmed);
        }
    }
    return found;
}

QString readTail(const QString& path, qint64 maxBytes)
{
    QFile file(path);
    if (!file.open(QFile::ReadOnly))
        return {};
    if (file.size() > maxBytes)
        file.seek(file.size() - maxBytes);
    return QString::fromUtf8(file.readAll());
}

}  // namespace

QList<Finding> analyzeText(const QString& text)
{
    return runRules(text);
}

QList<Finding> analyzeGameDir(const QString& gameRoot, qint64 maxBytes)
{
    QStringList texts;

    QString latestLog = QDir(gameRoot).filePath("logs/latest.log");
    QString logText = readTail(latestLog, maxBytes);

    // The newest crash report is the most authoritative source, scan it first.
    // Skip it when it predates the current session log, so a stale report from
    // an older crash is not reported for a fresh failure.
    QDir crashDir(QDir(gameRoot).filePath("crash-reports"));
    const auto reports = crashDir.entryInfoList({ "crash-*.txt" }, QDir::Files, QDir::Time);
    if (!reports.isEmpty()) {
        const QFileInfo& report = reports.first();
        const QFileInfo logInfo(latestLog);
        bool fresh = logInfo.exists() ? report.lastModified().secsTo(logInfo.lastModified()) <= 60 : true;
        if (fresh) {
            QString reportText = readTail(report.absoluteFilePath(), maxBytes);
            if (!reportText.isEmpty())
                texts.append(reportText);
        }
    }

    if (!logText.isEmpty())
        texts.append(logText);

    QList<Finding> findings;
    QSet<QString> seenTitles;
    for (const QString& text : texts) {
        for (const Finding& f : runRules(text)) {
            if (seenTitles.contains(f.title))
                continue;
            seenTitles.insert(f.title);
            findings.append(f);
        }
    }
    return findings;
}

}  // namespace CrashAnalyzer
