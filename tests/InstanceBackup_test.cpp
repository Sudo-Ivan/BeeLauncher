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

#include <QFileInfo>
#include <QTemporaryDir>
#include <QTest>

#include <FileSystem.h>
#include <MMCZip.h>
#include <minecraft/InstanceBackup.h>

class InstanceBackupTest : public QObject {
    Q_OBJECT

   private:
    static void writeFile(const QString& path, const QByteArray& content)
    {
        QVERIFY(FS::ensureFilePathExists(path));
        QFile f(path);
        QVERIFY(f.open(QFile::WriteOnly));
        QCOMPARE(f.write(content), qint64(content.size()));
        f.close();
    }

   private slots:
    void test_generateName_sanitizesAndDeduplicates()
    {
        QTemporaryDir backups;
        QVERIFY(backups.isValid());

        QString first = InstanceBackup::generateBackupFileName("My World: test?", backups.path());
        QVERIFY(first.startsWith("My World test"));
        QVERIFY(first.endsWith(".zip"));

        // create it, a second run must not collide
        writeFile(FS::PathCombine(backups.path(), first), "x");
        QString second = InstanceBackup::generateBackupFileName("My World: test?", backups.path());
        QCOMPARE_NE(first, second);
    }

    void test_listAndPrune()
    {
        QTemporaryDir backups;
        QVERIFY(backups.isValid());

        QStringList names = { "a-1.zip", "b-2.zip", "c-3.zip", "d-4.zip" };
        for (const auto& name : names) {
            writeFile(FS::PathCombine(backups.path(), name), "x");
        }

        auto listed = InstanceBackup::listBackups(backups.path());
        QCOMPARE(listed.size(), 4);

        QCOMPARE(InstanceBackup::pruneBackups(backups.path(), 2), 2);
        QCOMPARE(InstanceBackup::listBackups(backups.path()).size(), 2);
    }

    void test_createBackup_roundTrip()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QString gameRoot = FS::PathCombine(tmp.path(), "game");
        writeFile(FS::PathCombine(gameRoot, "options.txt"), "fov:90\n");
        writeFile(FS::PathCombine(gameRoot, "saves/world/level.dat"), "leveldata");
        writeFile(FS::PathCombine(gameRoot, "mods/mod.jar"), "jarbytes");

        QString zipPath = FS::PathCombine(tmp.path(), "backup.zip");
        QCOMPARE(InstanceBackup::createBackup(gameRoot, zipPath), QString());

        QString outDir = FS::PathCombine(tmp.path(), "out");
        auto extracted = MMCZip::extractDir(zipPath, outDir);
        QVERIFY(extracted.has_value());

        QCOMPARE(QFileInfo::exists(FS::PathCombine(outDir, "options.txt")), true);
        QCOMPARE(QFileInfo::exists(FS::PathCombine(outDir, "saves/world/level.dat")), true);
        QCOMPARE(QFileInfo::exists(FS::PathCombine(outDir, "mods/mod.jar")), true);

        QFile f(FS::PathCombine(outDir, "options.txt"));
        QVERIFY(f.open(QFile::ReadOnly));
        QCOMPARE(f.readAll(), QByteArray("fov:90\n"));
    }

    void test_createBackup_missingGameRoot()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QString zipPath = FS::PathCombine(tmp.path(), "backup.zip");
        auto error = InstanceBackup::createBackup(FS::PathCombine(tmp.path(), "nonexistent"), zipPath);
        QVERIFY(!error.isEmpty());
        QCOMPARE(QFileInfo::exists(zipPath), false);
    }
};

QTEST_GUILESS_MAIN(InstanceBackupTest)

#include "InstanceBackup_test.moc"
