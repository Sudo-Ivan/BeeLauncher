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

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QTest>

#include <QTemporaryDir>
#include <chrono>
#include <filesystem>
#include <minecraft/CrashAnalyzer.h>

class CrashAnalyzerTest : public QObject {
    Q_OBJECT

   private slots:
    void test_outOfMemory()
    {
        QString log = "[12:00:00] [main/INFO]: Starting\n"
                      "java.lang.OutOfMemoryError: Java heap space\n"
                      "\tat foo.Bar.baz(Bar.java:1)\n";
        auto findings = CrashAnalyzer::analyzeText(log);
        QVERIFY(!findings.isEmpty());
        QCOMPARE(findings.first().title, QStringLiteral("Out of memory"));
        QVERIFY(!findings.first().evidence.isEmpty());
    }

    void test_wrongJavaVersion()
    {
        QString log = "java.lang.UnsupportedClassVersionError: net/fabricmc/loader/impl/game/GameProvider has been compiled "
                      "by a more recent version of the Java Runtime\n";
        auto findings = CrashAnalyzer::analyzeText(log);
        QVERIFY(!findings.isEmpty());
        QCOMPARE(findings.first().title, QStringLiteral("Wrong Java version"));
    }

    void test_missingDeps_fabric()
    {
        QString log = "net.fabricmc.loader.impl.FormattedException: Mod resolution encountered an incompatible mod set!\n"
                      "\tCould not find required mod: sodium requires [fabric-api]\n";
        auto findings = CrashAnalyzer::analyzeText(log);
        QVERIFY(!findings.isEmpty());
        QCOMPARE(findings.first().title, QStringLiteral("Missing or incompatible mod dependencies"));
    }

    void test_missingDeps_forge()
    {
        QString log = "[main/FATAL]: Missing or unsupported mandatory dependencies:\n"
                      "\tMod ID: 'jei', Requested by: 'mymod', Expected range: '', Actual version: '[MISSING]'\n";
        auto findings = CrashAnalyzer::analyzeText(log);
        QVERIFY(!findings.isEmpty());
        QCOMPARE(findings.first().title, QStringLiteral("Missing or incompatible mod dependencies"));
    }

    void test_mixin()
    {
        QString log = "org.spongepowered.asm.mixin.injection.throwables.InjectionError: "
                      "Critical injection failure: callback\n";
        auto findings = CrashAnalyzer::analyzeText(log);
        QVERIFY(!findings.isEmpty());
        QCOMPARE(findings.first().title, QStringLiteral("Mixin conflict between mods"));
    }

    void test_gpuDriver()
    {
        QString log = "org.lwjgl.LWJGLException: Pixel format not accelerated\n";
        auto findings = CrashAnalyzer::analyzeText(log);
        QVERIFY(!findings.isEmpty());
        QCOMPARE(findings.first().title, QStringLiteral("Graphics driver problem"));
    }

    void test_nativeCrash()
    {
        QString log = "# A fatal error has been detected by the Java Runtime Environment:\n"
                      "#  EXCEPTION_ACCESS_VIOLATION (0xc0000005) at pc=0x00007ffb\n"
                      "# Problematic frame: atio6axx.dll\n";
        auto findings = CrashAnalyzer::analyzeText(log);
        QVERIFY(!findings.isEmpty());
        QCOMPARE(findings.first().title, QStringLiteral("Native crash"));
    }

    void test_dedupAndEvidence()
    {
        QString log = "java.lang.OutOfMemoryError: Java heap space\n"
                      "java.lang.OutOfMemoryError: GC overhead limit\n"
                      "java.lang.OutOfMemoryError: Metaspace\n";
        auto findings = CrashAnalyzer::analyzeText(log);
        QCOMPARE(findings.size(), 1);
        QCOMPARE(findings.first().evidence.size(), 3);
    }

    void test_cleanLogNoFindings()
    {
        QString log = "[12:00:00] [main/INFO]: Loading Minecraft 1.21.1 with Fabric Loader\n"
                      "[12:00:05] [Render thread/INFO]: Backend library: LWJGL version 3.3.3\n"
                      "[12:00:40] [Render thread/INFO]: Stopping!\n";
        auto findings = CrashAnalyzer::analyzeText(log);
        QVERIFY(findings.isEmpty());
    }

    void test_gameDir_prefersCrashReport()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QDir dir(tmp.path());
        QVERIFY(dir.mkpath("logs"));
        QVERIFY(dir.mkpath("crash-reports"));

        QFile latest(dir.filePath("logs/latest.log"));
        QVERIFY(latest.open(QFile::WriteOnly));
        latest.write("noise\njava.lang.OutOfMemoryError: Java heap space\n");
        latest.close();

        QFile report(dir.filePath("crash-reports/crash-2026-01-01_00.00.00-client.txt"));
        QVERIFY(report.open(QFile::WriteOnly));
        report.write("---- Minecraft Crash Report ----\nDescription: Ticking entity\n");
        report.close();

        auto findings = CrashAnalyzer::analyzeGameDir(tmp.path());
        QVERIFY(findings.size() >= 2);
        QStringList titles;
        for (const auto& f : findings)
            titles << f.title;
        QVERIFY(titles.contains("Out of memory"));
        QVERIFY(titles.contains("World or ticking crash"));
    }

    void test_gameDir_skipsStaleCrashReport()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QDir dir(tmp.path());
        QVERIFY(dir.mkpath("logs"));
        QVERIFY(dir.mkpath("crash-reports"));

        QString reportPath = dir.filePath("crash-reports/crash-2026-01-01_00.00.00-client.txt");
        QFile report(reportPath);
        QVERIFY(report.open(QFile::WriteOnly));
        report.write("---- Minecraft Crash Report ----\nDescription: Ticking entity\n");
        report.close();
        // move the mtime one hour into the past so it reads as an old session
        std::error_code ec;
        std::filesystem::last_write_time(reportPath.toStdString(),
                                         std::filesystem::file_time_type::clock::now() - std::chrono::hours(1), ec);
        QVERIFY(!ec);

        QFile latest(dir.filePath("logs/latest.log"));
        QVERIFY(latest.open(QFile::WriteOnly));
        latest.write("noise\njava.lang.OutOfMemoryError: Java heap space\n");
        latest.close();

        auto findings = CrashAnalyzer::analyzeGameDir(tmp.path());
        QStringList titles;
        for (const auto& f : findings)
            titles << f.title;
        QVERIFY(titles.contains("Out of memory"));
        QVERIFY(!titles.contains("World or ticking crash"));
    }

    void test_gameDir_missingLogs()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QVERIFY(CrashAnalyzer::analyzeGameDir(tmp.path()).isEmpty());
    }
};

QTEST_GUILESS_MAIN(CrashAnalyzerTest)

#include "CrashAnalyzer_test.moc"
