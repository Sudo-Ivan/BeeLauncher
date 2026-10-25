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

#include <QTest>

#include <BuildConfig.h>
#include <ServiceUrls.h>

class ServiceUrlsTest : public QObject {
    Q_OBJECT

   private slots:
    void test_metaBase_data()
    {
        QTest::addColumn<QString>("overrideUrl");
        QTest::addColumn<bool>("mvaEnabled");
        QTest::addColumn<QString>("mvaUrl");
        QTest::addColumn<QString>("expected");

        QTest::newRow("default") << QString() << false << QString() << BuildConfig.META_URL;
        QTest::newRow("mva on, default url") << QString() << true << QString() << BuildConfig.MVA_URL + "/meta/v1/";
        QTest::newRow("mva on, custom url") << QString() << true << QStringLiteral("https://mva.example.org")
                                            << QStringLiteral("https://mva.example.org/meta/v1/");
        QTest::newRow("mva on, custom url with slash")
            << QString() << true << QStringLiteral("https://mva.example.org/") << QStringLiteral("https://mva.example.org/meta/v1/");
        QTest::newRow("explicit override beats mva")
            << QStringLiteral("https://meta.example.org/") << true << QString() << QStringLiteral("https://meta.example.org/");
        QTest::newRow("mva off ignores url") << QString() << false << QStringLiteral("https://mva.example.org") << BuildConfig.META_URL;
    }

    void test_metaBase()
    {
        QFETCH(QString, overrideUrl);
        QFETCH(bool, mvaEnabled);
        QFETCH(QString, mvaUrl);
        QFETCH(QString, expected);
        QCOMPARE(ServiceUrls::metaBase(overrideUrl, mvaEnabled, mvaUrl), expected);
    }

    void test_resourceBase_data()
    {
        QTest::addColumn<QString>("overrideUrl");
        QTest::addColumn<bool>("mvaEnabled");
        QTest::addColumn<QString>("mvaUrl");
        QTest::addColumn<QString>("expected");

        QTest::newRow("default") << QString() << false << QString() << BuildConfig.DEFAULT_RESOURCE_BASE;
        QTest::newRow("mva on, default url") << QString() << true << QString() << BuildConfig.MVA_URL + "/assets/objects/";
        QTest::newRow("mva on, custom url") << QString() << true << QStringLiteral("https://mva.example.org/")
                                            << QStringLiteral("https://mva.example.org/assets/objects/");
        QTest::newRow("explicit override beats mva")
            << QStringLiteral("https://assets.example.org/") << true << QString() << QStringLiteral("https://assets.example.org/");
        QTest::newRow("mva off ignores url") << QString() << false << QStringLiteral("https://mva.example.org")
                                             << BuildConfig.DEFAULT_RESOURCE_BASE;
    }

    void test_resourceBase()
    {
        QFETCH(QString, overrideUrl);
        QFETCH(bool, mvaEnabled);
        QFETCH(QString, mvaUrl);
        QFETCH(QString, expected);
        QCOMPARE(ServiceUrls::resourceBase(overrideUrl, mvaEnabled, mvaUrl), expected);
    }

    void test_savesBase()
    {
        QCOMPARE(ServiceUrls::savesBase(QString()), BuildConfig.SAVES_URL + QStringLiteral("/"));
        QCOMPARE(ServiceUrls::savesBase(QStringLiteral("https://saves.example.org")), QStringLiteral("https://saves.example.org/"));
        QCOMPARE(ServiceUrls::savesBase(QStringLiteral("http://127.0.0.1:8787/")), QStringLiteral("http://127.0.0.1:8787/"));
    }
};

QTEST_GUILESS_MAIN(ServiceUrlsTest)
#include "ServiceUrls_test.moc"
