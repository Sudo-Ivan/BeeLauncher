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

#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>

#include <migrate/DetectedProfile.h>

class MigrateTest : public QObject {
    Q_OBJECT

   private:
    static void parse(const QString& versionId, QString& mc, QString& uid, QString& ver)
    {
        mc.clear();
        uid.clear();
        ver.clear();
        LauncherMigration::parseVersionId(versionId, mc, uid, ver);
    }

   private slots:
    void test_loaderUidMapping()
    {
        QCOMPARE(LauncherMigration::componentUidForLoader("forge"), QStringLiteral("net.minecraftforge"));
        QCOMPARE(LauncherMigration::componentUidForLoader("Forge"), QStringLiteral("net.minecraftforge"));
        QCOMPARE(LauncherMigration::componentUidForLoader("fabric"), QStringLiteral("net.fabricmc.fabric-loader"));
        QCOMPARE(LauncherMigration::componentUidForLoader("fabric-loader"), QStringLiteral("net.fabricmc.fabric-loader"));
        QCOMPARE(LauncherMigration::componentUidForLoader("quilt"), QStringLiteral("org.quiltmc.quilt-loader"));
        QCOMPARE(LauncherMigration::componentUidForLoader("neoforge"), QStringLiteral("net.neoforged"));
        QCOMPARE(LauncherMigration::componentUidForLoader("liteloader"), QStringLiteral("com.mumfrey.liteloader"));
        QVERIFY(LauncherMigration::componentUidForLoader("vanilla").isEmpty());
        QVERIFY(LauncherMigration::componentUidForLoader("").isEmpty());
    }

    void test_parseVersionId_vanilla()
    {
        QString mc, uid, ver;
        parse("1.20.1", mc, uid, ver);
        QCOMPARE(mc, QStringLiteral("1.20.1"));
        QVERIFY(uid.isEmpty());
        QVERIFY(ver.isEmpty());
    }

    void test_parseVersionId_snapshot()
    {
        QString mc, uid, ver;
        parse("24w14a", mc, uid, ver);
        QCOMPARE(mc, QStringLiteral("24w14a"));
        QVERIFY(uid.isEmpty());
    }

    void test_parseVersionId_fabric()
    {
        QString mc, uid, ver;
        parse("fabric-loader-0.15.0-1.20.1", mc, uid, ver);
        QCOMPARE(mc, QStringLiteral("1.20.1"));
        QCOMPARE(uid, QStringLiteral("net.fabricmc.fabric-loader"));
        QCOMPARE(ver, QStringLiteral("0.15.0"));
    }

    void test_parseVersionId_forge()
    {
        QString mc, uid, ver;
        parse("1.20.1-forge-47.2.0", mc, uid, ver);
        QCOMPARE(mc, QStringLiteral("1.20.1"));
        QCOMPARE(uid, QStringLiteral("net.minecraftforge"));
        QCOMPARE(ver, QStringLiteral("47.2.0"));
    }

    void test_parseVersionId_neoforge()
    {
        QString mc, uid, ver;
        parse("neoforge-21.0.0", mc, uid, ver);
        QCOMPARE(uid, QStringLiteral("net.neoforged"));
        QCOMPARE(ver, QStringLiteral("21.0.0"));
        QVERIFY(mc.isEmpty());
    }

    void test_parseMmcPack()
    {
        auto json = QJsonDocument::fromJson(R"({
            "components": [
                { "uid": "net.minecraft", "version": "1.20.1" },
                { "uid": "net.minecraftforge", "version": "47.2.0" },
                { "uid": "org.lwjgl3", "version": "3.3.1" }
            ]
        })")
                        .object();
        QString mc, uid, ver;
        LauncherMigration::parseMmcPack(json, mc, uid, ver);
        QCOMPARE(mc, QStringLiteral("1.20.1"));
        QCOMPARE(uid, QStringLiteral("net.minecraftforge"));
        QCOMPARE(ver, QStringLiteral("47.2.0"));
    }

    void test_parseMmcPack_noLoader()
    {
        auto json = QJsonDocument::fromJson(R"({
            "components": [
                { "uid": "net.minecraft", "version": "1.16.5" }
            ]
        })")
                        .object();
        QString mc, uid, ver;
        LauncherMigration::parseMmcPack(json, mc, uid, ver);
        QCOMPARE(mc, QStringLiteral("1.16.5"));
        QVERIFY(uid.isEmpty());
    }

    void test_detectDoesNotCrash()
    {
        // Scans whatever is on this machine, mostly nothing on CI. Just verify it completes.
        auto profiles = LauncherMigration::detectProfiles();
        for (const auto& profile : profiles) {
            QVERIFY(!profile.name.isEmpty());
            QVERIFY(!profile.launcherName.isEmpty());
            QVERIFY(!profile.contentDir.isEmpty());
        }
    }
};

QTEST_GUILESS_MAIN(MigrateTest)

#include "Migrate_test.moc"
