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

#include <QDirIterator>
#include <QElapsedTimer>
#include <QNetworkAccessManager>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include <FileSystem.h>
#include <saves/SavesClient.h>
#include <saves/SavesManifest.h>
#include <saves/SavesPushTask.h>
#include <saves/SavesRestoreTask.h>

class SavesClientTest : public QObject {
    Q_OBJECT

   private:
    QNetworkAccessManager m_nam;
    QString m_url;
    QString m_token;
    QString m_slot;

    static void writeFile(const QString& path, const QByteArray& content)
    {
        QVERIFY(FS::ensureFilePathExists(path));
        QFile f(path);
        QVERIFY(f.open(QFile::WriteOnly));
        QCOMPARE(f.write(content), qint64(content.size()));
        f.close();
    }

    static bool runTask(Task& task, int timeoutMs = 120000)
    {
        task.start();
        QElapsedTimer timer;
        timer.start();
        while (!task.isFinished() && timer.elapsed() < timeoutMs)
            QTest::qWait(10);
        return task.isFinished();
    }

    static void setMtime(const QString& path, qint64 msecs)
    {
        QFile f(path);
        QVERIFY(f.open(QFile::ReadWrite));
        QVERIFY(f.setFileTime(QDateTime::fromMSecsSinceEpoch(msecs), QFileDevice::FileModificationTime));
        f.close();
    }

    static void compareTrees(const QString& expected, const QString& actual)
    {
        const QDir expectedDir(expected);
        const QDir actualDir(actual);
        const QFileInfoList entries =
            expectedDir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden, QDir::NoSort);
        for (const QFileInfo& e : entries) {
            if (e.isSymbolicLink() || Saves::isIgnored(e.fileName()))
                continue;
            const QString rel = expectedDir.relativeFilePath(e.absoluteFilePath());
            const QString other = actualDir.absoluteFilePath(rel);
            if (e.isDir()) {
                QVERIFY2(QFileInfo(other).isDir(), qPrintable(rel));
                compareTrees(e.absoluteFilePath(), other);
                continue;
            }
            QVERIFY2(QFileInfo(other).isFile(), qPrintable(rel));
            QFile a(e.absoluteFilePath());
            QFile b(other);
            QVERIFY(a.open(QFile::ReadOnly));
            QVERIFY(b.open(QFile::ReadOnly));
            QCOMPARE(a.readAll(), b.readAll());
            QVERIFY2(qAbs(QFileInfo(other).lastModified().toMSecsSinceEpoch() - e.lastModified().toMSecsSinceEpoch()) <= 1,
                     qPrintable(rel));
        }
    }

   private slots:
    void initTestCase()
    {
        m_url = QString::fromUtf8(qgetenv("SAVES_TEST_URL"));
        m_token = QString::fromUtf8(qgetenv("SAVES_TEST_TOKEN"));
        if (m_url.isEmpty() || m_token.isEmpty())
            QSKIP("SAVES_TEST_URL and SAVES_TEST_TOKEN are required for the saves integration test");
        m_slot = QStringLiteral("it-world-%1").arg(QDateTime::currentMSecsSinceEpoch());
    }

    void test_guards()
    {
        SavesClient noToken(&m_nam, m_url, QString());
        bool called = false;
        QString err;
        noToken.me([&](const SavesClient::MeInfo&, const QString& error) {
            called = true;
            err = error;
        });
        QTRY_VERIFY(called);
        QVERIFY2(err.contains("No saves token set"), qPrintable(err));

        SavesClient plainHttp(&m_nam, QStringLiteral("http://example.com:8787/"), QStringLiteral("tok"));
        called = false;
        err.clear();
        plainHttp.me([&](const SavesClient::MeInfo&, const QString& error) {
            called = true;
            err = error;
        });
        QTRY_VERIFY(called);
        QVERIFY2(err.contains("Refusing to send the token over plain http"), qPrintable(err));

        SavesClient badToken(&m_nam, m_url, QStringLiteral("definitely-wrong-token"));
        called = false;
        err.clear();
        badToken.me([&](const SavesClient::MeInfo&, const QString& error) {
            called = true;
            err = error;
        });
        QTRY_VERIFY(called);
        QCOMPARE(err, QStringLiteral("Invalid or missing token"));
    }

    void test_pushRestoreRoundtrip()
    {
        SavesClient client(&m_nam, m_url, m_token);

        SavesClient::MeInfo me;
        {
            bool called = false;
            client.me([&](const SavesClient::MeInfo& info, const QString& error) {
                called = true;
                me = info;
                QVERIFY2(error.isEmpty(), qPrintable(error));
            });
            QTRY_VERIFY(called);
            QVERIFY(!me.tier.isEmpty());
        }

        QTemporaryDir world;
        QVERIFY(world.isValid());

        QByteArray big(2621440, '\0');
        for (qsizetype i = 0; i < big.size(); ++i)
            big[i] = static_cast<char>(i % 251);
        writeFile(world.filePath("big.bin"), big);
        writeFile(world.filePath("level.dat"), "level data");
        writeFile(world.filePath("region/r.0.0.mca"), "region bytes");
        writeFile(world.filePath("zero.dat"), QByteArray());
        QVERIFY(QDir().mkpath(world.filePath("emptydir")));
        QVERIFY(QFile::link(world.filePath("level.dat"), world.filePath("link.txt")));

        const qint64 fixedMtime = 1700000000000LL;
        setMtime(world.filePath("level.dat"), fixedMtime);
        setMtime(world.filePath("big.bin"), fixedMtime);
        setMtime(world.filePath("region/r.0.0.mca"), fixedMtime);
        setMtime(world.filePath("zero.dat"), fixedMtime);

        SavesPushTask push(world.path(), m_slot, QStringLiteral("integration test"), &client);
        QVERIFY2(runTask(push), "push task did not finish");
        QVERIFY2(push.getState() == Task::State::Succeeded, qPrintable(push.failReason()));
        QVERIFY(!push.snapshotId().isEmpty());
        QCOMPARE(push.chunksUploaded() + push.chunksDeduped(), qint64(3));

        SavesClient::SlotInfo slotInfo;
        {
            bool called = false;
            client.getSlot(m_slot, [&](const SavesClient::SlotInfo& info, const QString& error) {
                called = true;
                QVERIFY2(error.isEmpty(), qPrintable(error));
                slotInfo = info;
            });
            QTRY_VERIFY(called);
        }
        QCOMPARE(slotInfo.headSnapshotId, push.snapshotId());

        QList<SavesClient::SnapshotInfo> snapshots;
        {
            bool called = false;
            client.listSnapshots(m_slot, [&](const QList<SavesClient::SnapshotInfo>& list, const QString& error) {
                called = true;
                QVERIFY2(error.isEmpty(), qPrintable(error));
                snapshots = list;
            });
            QTRY_VERIFY(called);
        }
        QCOMPARE(snapshots.size(), 1);
        QCOMPARE(snapshots.first().id, push.snapshotId());
        QCOMPARE(snapshots.first().label, QStringLiteral("integration test"));

        QTemporaryDir savesDir;
        QVERIFY(savesDir.isValid());
        SavesRestoreTask restore(savesDir.path(), m_slot, push.snapshotId(), &client);
        QVERIFY2(runTask(restore), "restore task did not finish");
        QVERIFY2(restore.getState() == Task::State::Succeeded, qPrintable(restore.failReason()));
        QCOMPARE(restore.restoredDirName(), m_slot);

        compareTrees(world.path(), FS::PathCombine(savesDir.path(), m_slot));

        // change one byte in the second 1 MiB chunk, push again, expect a single chunk upload
        {
            QFile f(world.filePath("big.bin"));
            QVERIFY(f.open(QFile::ReadWrite));
            QVERIFY(f.seek((1 << 20) + 7));
            const QByteArray marker = QByteArray::number(QDateTime::currentMSecsSinceEpoch());
            QVERIFY(f.write(marker) == marker.size());
        }
        SavesPushTask push2(world.path(), m_slot, QStringLiteral("second"), &client);
        QVERIFY2(runTask(push2), "second push task did not finish");
        QVERIFY2(push2.getState() == Task::State::Succeeded, qPrintable(push2.failReason()));
        QCOMPARE(push2.chunksUploaded(), qint64(1));

        QTemporaryDir savesDir2;
        QVERIFY(savesDir2.isValid());
        SavesRestoreTask restore2(savesDir2.path(), m_slot, push2.snapshotId(), &client);
        QVERIFY2(runTask(restore2), "second restore task did not finish");
        QVERIFY2(restore2.getState() == Task::State::Succeeded, qPrintable(restore2.failReason()));
        QCOMPARE(restore2.restoredDirName(), m_slot);
        compareTrees(world.path(), FS::PathCombine(savesDir2.path(), m_slot));
    }
};

QTEST_GUILESS_MAIN(SavesClientTest)
#include "SavesClient_test.moc"
