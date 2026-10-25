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

#include <QCryptographicHash>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTest>

#include <FileSystem.h>
#include <saves/SavesManifest.h>

class SavesManifestTest : public QObject {
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
    void test_validatePath_data()
    {
        QTest::addColumn<QString>("path");
        QTest::addColumn<bool>("valid");

        QTest::newRow("simple") << "world/level.dat" << true;
        QTest::newRow("empty") << "" << false;
        QTest::newRow("leading slash") << "/world/level.dat" << false;
        QTest::newRow("trailing slash") << "world/" << false;
        QTest::newRow("double slash") << "world//level.dat" << false;
        QTest::newRow("dot") << "world/./level.dat" << false;
        QTest::newRow("dotdot") << "world/../level.dat" << false;
        QTest::newRow("backslash") << "world/re\\gion" << false;
        QTest::newRow("forbidden chars") << "world/a<b" << false;
        QTest::newRow("forbidden colon") << "world/a:b" << false;
        QTest::newRow("control char") << "world/a\u0007b" << false;
        QTest::newRow("trailing dot") << "world/level.dat." << false;
        QTest::newRow("trailing space") << "world/level.dat " << false;
        QTest::newRow("device name") << "world/CON" << false;
        QTest::newRow("device name ext") << "world/com1.txt" << false;
        QTest::newRow("device name deep") << "world/sub/LPT9" << false;
        QTest::newRow("not device name") << "world/CONSOLE" << true;
        QTest::newRow("non ascii") << QString::fromUtf8("world/\xc3\xa9.dat") << true;

        QString deep = "world";
        for (int i = 0; i < 63; ++i)
            deep += "/d";
        QTest::newRow("depth 64") << deep << true;
        QTest::newRow("depth 65") << (deep + "/x") << false;

        const QString long1024 = QStringLiteral("world/") + QString(255, QLatin1Char('a')) + QLatin1Char('/') +
                                 QString(255, QLatin1Char('b')) + QLatin1Char('/') + QString(255, QLatin1Char('c')) + QLatin1Char('/') +
                                 QString(250, QLatin1Char('d'));
        QCOMPARE(long1024.toUtf8().size(), 1024);
        QTest::newRow("1024 bytes") << long1024 << true;
        QTest::newRow("1025 bytes") << (long1024 + QLatin1Char('e')) << false;

        QString comp255 = QStringLiteral("world/") + QString(255, QLatin1Char('b'));
        QTest::newRow("component 255") << comp255 << true;
        QTest::newRow("component 256") << (comp255 + "c") << false;
    }

    void test_validatePath()
    {
        QFETCH(QString, path);
        QFETCH(bool, valid);
        QCOMPARE(Saves::validatePath(path), valid);
    }

    void test_slotNameFor()
    {
        QCOMPARE(Saves::slotNameFor(QStringLiteral("My World")), QStringLiteral("My World"));
        QCOMPARE(Saves::slotNameFor(QString::fromUtf8("caf\xc3\xa9")), QString::fromUtf8("caf\xc3\xa9"));

        const QString sanitized = Saves::slotNameFor(QStringLiteral("bad/name"));
        QVERIFY(sanitized.startsWith(QStringLiteral("bad_name-")));
        QCOMPARE(sanitized.size(), QStringLiteral("bad_name-").size() + 8);
        QVERIFY(Saves::validateSlotName(sanitized));

        const QString ctrl = Saves::slotNameFor(QStringLiteral("a\u0001b"));
        QVERIFY(ctrl.startsWith(QStringLiteral("a_b-")));
        QVERIFY(Saves::validateSlotName(ctrl));
    }

    void test_scanWorld()
    {
        QTemporaryDir world;
        QVERIFY(world.isValid());

        QByteArray big(2621440, '\0');
        for (qsizetype i = 0; i < big.size(); ++i)
            big[i] = static_cast<char>(i % 251);
        writeFile(world.filePath("big.bin"), big);
        writeFile(world.filePath("small.txt"), "tiny content");
        writeFile(world.filePath("zero.dat"), QByteArray());
        writeFile(world.filePath("session.lock"), "lock");
        writeFile(world.filePath("cache.tmp"), "tmp");
        writeFile(world.filePath("sub/nested.dat"), "nested");
        QVERIFY(QDir().mkpath(world.filePath("emptydir")));
        writeFile(world.filePath(QString::fromUtf8("caf\xc3\xa9.txt")), "nonascii");
#ifndef Q_OS_WIN
        writeFile(world.filePath("bad:name.txt"), "invalid name");
        QVERIFY(QFile::link(world.filePath("small.txt"), world.filePath("link.txt")));
#endif
        writeFile(world.filePath("UPPER.txt"), "one");
        writeFile(world.filePath("upper.txt"), "two");
        QFile upperProbe(world.filePath("UPPER.txt"));
        QVERIFY(upperProbe.open(QFile::ReadOnly));
        const bool caseSensitive = upperProbe.readAll() == "one";
        upperProbe.close();

        auto result = Saves::scanWorld(world.path());
        QVERIFY2(result.ok(), qPrintable(result.error));

        QStringList paths;
        for (const auto& f : result.files)
            paths.append(f.path);

        QCOMPARE(paths, QStringList({ "world/UPPER.txt", "world/big.bin", "world/caf\xc3\xa9.txt", "world/small.txt",
                                      "world/sub/nested.dat", "world/zero.dat" }));

        // case-fold duplicate: upper.txt was skipped, UPPER.txt kept
        QStringList skippedPaths;
        for (const auto& s : result.skipped)
            skippedPaths.append(s.path);
        QCOMPARE(skippedPaths.contains("upper.txt"), caseSensitive);
#ifndef Q_OS_WIN
        QVERIFY(skippedPaths.contains("bad:name.txt"));
        QVERIFY(skippedPaths.contains("link.txt"));
#endif

        QVERIFY(result.dirs.contains("world/emptydir"));
        QVERIFY(!result.dirs.contains("world/sub"));

        auto big2 = std::find_if(result.files.begin(), result.files.end(),
                                 [](const Saves::ManifestFile& f) { return f.path == QLatin1String("world/big.bin"); });
        QVERIFY(big2 != result.files.end());
        QCOMPARE(big2->chunks.size(), 3);
        QCOMPARE(big2->chunks.at(0).size, quint32(1 << 20));
        QCOMPARE(big2->chunks.at(1).size, quint32(1 << 20));
        QCOMPARE(big2->chunks.at(2).size, quint32(512 << 10));
        QCOMPARE(big2->chunks.at(0).offset, qint64(0));
        QCOMPARE(big2->chunks.at(1).offset, qint64(1 << 20));
        QCOMPARE(big2->chunks.at(0).hash,
                 QString::fromLatin1(QCryptographicHash::hash(big.left(1 << 20), QCryptographicHash::Sha256).toHex()));
        QCOMPARE(big2->chunks.at(2).hash,
                 QString::fromLatin1(QCryptographicHash::hash(big.right(512 << 10), QCryptographicHash::Sha256).toHex()));

        auto small = std::find_if(result.files.begin(), result.files.end(),
                                  [](const Saves::ManifestFile& f) { return f.path == QLatin1String("world/small.txt"); });
        QVERIFY(small != result.files.end());
        QCOMPARE(small->inlineData, QByteArray("tiny content"));
        QVERIFY(small->chunks.isEmpty());
        QCOMPARE(small->mode, quint32(0644));

        auto zero = std::find_if(result.files.begin(), result.files.end(),
                                 [](const Saves::ManifestFile& f) { return f.path == QLatin1String("world/zero.dat"); });
        QVERIFY(zero != result.files.end());
        QVERIFY(zero->chunks.isEmpty());
        QVERIFY(zero->inlineData.isEmpty());
    }

    void test_encodeManifestGolden()
    {
        const QByteArray chunk = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
        const QString hash = QString::fromLatin1(QCryptographicHash::hash(chunk.left(32), QCryptographicHash::Sha256).toHex());

        QList<Saves::ManifestFile> files;
        {
            Saves::ManifestFile a;
            a.path = "world/a.txt";
            a.size = 5;
            a.mtimeNs = 1700000000000000000LL;
            a.inlineData = "hello";
            files.append(a);
        }
        {
            Saves::ManifestFile b;
            b.path = QString::fromUtf8("world/b \xc3\xa9.bin");
            b.size = 2097152;
            b.mtimeNs = 1700000001000000000LL;
            b.chunks.append({ hash, 1048576, 0 });
            b.chunks.append({ hash, 1048576, 1048576 });
            files.append(b);
        }
        {
            Saves::ManifestFile z;
            z.path = "world/zero";
            z.size = 0;
            z.mtimeNs = 1700000002000000000LL;
            files.append(z);
        }

        const QByteArray out =
            Saves::encodeManifest(files, { "world/emptydir" }, QStringLiteral("snap-parent-1"),
                                  QString::fromUtf8("dev \"q\" \\ caf\xc3\xa9"), QString::fromUtf8("lb\x08l\tx\xe2\x80\xa8y"));

        const QByteArray golden = QByteArrayLiteral(
            "{\"version\":1,\"parent\":\"snap-parent-1\",\"device\":\"dev \\\"q\\\" \\\\ caf\u00e9\","
            "\"label\":\"lb\\bl\\tx\\u2028y\",\"dirs\":[\"world/emptydir\"],\"files\":["
            "{\"path\":\"world/a.txt\",\"size\":5,\"mode\":420,\"mtime_ns\":1700000000000000000,\"inline\":\"aGVsbG8=\"},"
            "{\"path\":\"world/b \u00e9.bin\",\"size\":2097152,\"mode\":420,\"mtime_ns\":1700000001000000000,"
            "\"chunks\":[{\"h\":\"3eb1bd439947eb762998e566ccc2e099c791118b2f40579cc4f7da2b5061b7f9\",\"n\":1048576},"
            "{\"h\":\"3eb1bd439947eb762998e566ccc2e099c791118b2f40579cc4f7da2b5061b7f9\",\"n\":1048576}]},"
            "{\"path\":\"world/zero\",\"size\":0,\"mode\":420,\"mtime_ns\":1700000002000000000}]}");
        QCOMPARE(out, golden);
    }

    void test_decodeManifest()
    {
        Saves::ParsedManifest m;
        QString error;

        QVERIFY(!Saves::decodeManifest("{}", &m, &error));
        QVERIFY(!Saves::decodeManifest("{\"version\":2,\"files\":[]}", &m, &error));
        QVERIFY(!Saves::decodeManifest("{\"version\":1,\"enc\":{\"v\":1},\"files\":[]}", &m, &error));
        QVERIFY2(error.contains("encrypt"), qPrintable(error));
        QVERIFY(!Saves::decodeManifest(
            "{\"version\":1,\"files\":[{\"path\":\"../x\",\"size\":1,\"mode\":420,\"mtime_ns\":0,\"inline\":\"eA==\"}]}", &m, &error));
        QVERIFY(!Saves::decodeManifest(
            "{\"version\":1,\"files\":[{\"path\":\"\",\"size\":1,\"mode\":420,\"mtime_ns\":0,\"inline\":\"eA==\"}]}", &m, &error));

        QList<Saves::ManifestFile> files;
        Saves::ManifestFile a;
        a.path = "world/a.txt";
        a.size = 5;
        a.mtimeNs = 123;
        a.inlineData = "hello";
        files.append(a);
        const QByteArray encoded = Saves::encodeManifest(files, { "world/empty" }, QString(), QString(), QString());
        QVERIFY(Saves::decodeManifest(encoded, &m, &error));
        QCOMPARE(m.files.size(), 1);
        QCOMPARE(m.files.at(0).path, QStringLiteral("world/a.txt"));
        QCOMPARE(m.files.at(0).inlineData, QByteArray("hello"));
        QCOMPARE(m.dirs, QStringList({ "world/empty" }));
    }
};

QTEST_GUILESS_MAIN(SavesManifestTest)
#include "SavesManifest_test.moc"
