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

#include "SavesManifest.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QSet>

#include <algorithm>

namespace Saves {

static void setErr(QString* error, const QString& message)
{
    if (error)
        *error = message;
}

static bool isWindowsDevice(const QString& component)
{
    QString base = component.section('.', 0, 0).toUpper();
    static const QSet<QString> devices = { "CON",  "PRN",  "AUX",  "NUL",  "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7",
                                           "COM8", "COM9", "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9" };
    return devices.contains(base);
}

static bool validateComponent(const QString& c, QString* error)
{
    if (c.isEmpty()) {
        setErr(error, QStringLiteral("empty component"));
        return false;
    }
    if (c == QStringLiteral(".") || c == QStringLiteral("..")) {
        setErr(error, QStringLiteral("dot component"));
        return false;
    }
    if (c.toUtf8().size() > 255) {
        setErr(error, QStringLiteral("component exceeds 255 bytes"));
        return false;
    }
    if (c.endsWith(QLatin1Char('.')) || c.endsWith(QLatin1Char(' '))) {
        setErr(error, QStringLiteral("component ends in dot or space"));
        return false;
    }
    static const QString forbidden = QStringLiteral("<>:\"|?*");
    for (const QChar ch : c) {
        const ushort u = ch.unicode();
        if (u < 0x20 || u == 0x7f || ch.isLowSurrogate() || ch.isHighSurrogate()) {
            setErr(error, QStringLiteral("control character"));
            return false;
        }
        if (u == '\\' || forbidden.contains(ch)) {
            setErr(error, QStringLiteral("forbidden character"));
            return false;
        }
    }
    if (isWindowsDevice(c)) {
        setErr(error, QStringLiteral("reserved device name"));
        return false;
    }
    return true;
}

bool validatePath(const QString& path, QString* error)
{
    if (path.isEmpty()) {
        setErr(error, QStringLiteral("empty"));
        return false;
    }
    const QByteArray utf8 = path.toUtf8();
    if (QString::fromUtf8(utf8) != path) {
        setErr(error, QStringLiteral("not valid UTF-8"));
        return false;
    }
    if (utf8.size() > 1024) {
        setErr(error, QStringLiteral("exceeds 1024 bytes"));
        return false;
    }
    if (path.startsWith(QLatin1Char('/')) || path.endsWith(QLatin1Char('/'))) {
        setErr(error, QStringLiteral("leading or trailing slash"));
        return false;
    }
    const QStringList parts = path.split(QLatin1Char('/'), Qt::KeepEmptyParts);
    if (parts.size() > 64) {
        setErr(error, QStringLiteral("depth exceeds 64"));
        return false;
    }
    for (const QString& c : parts) {
        if (!validateComponent(c, error))
            return false;
    }
    return true;
}

bool validateSlotName(const QString& name, QString* error)
{
    if (name.isEmpty()) {
        setErr(error, QStringLiteral("empty"));
        return false;
    }
    const QByteArray utf8 = name.toUtf8();
    if (utf8.size() > 128) {
        setErr(error, QStringLiteral("exceeds 128 bytes"));
        return false;
    }
    if (QString::fromUtf8(utf8) != name) {
        setErr(error, QStringLiteral("not valid UTF-8"));
        return false;
    }
    for (const QChar ch : name) {
        if (ch == QLatin1Char('/') || ch == QLatin1Char('\\')) {
            setErr(error, QStringLiteral("slash"));
            return false;
        }
        if (ch.category() == QChar::Other_Control || ch.isLowSurrogate() || ch.isHighSurrogate()) {
            setErr(error, QStringLiteral("control character"));
            return false;
        }
    }
    return true;
}

QString slotNameFor(const QString& worldFolderName)
{
    if (validateSlotName(worldFolderName))
        return worldFolderName;

    QString cleaned = worldFolderName;
    for (qsizetype i = 0; i < cleaned.size(); ++i) {
        const QChar ch = cleaned.at(i);
        if (ch == QLatin1Char('/') || ch == QLatin1Char('\\') || ch.category() == QChar::Other_Control || ch.isLowSurrogate() ||
            ch.isHighSurrogate()) {
            cleaned[i] = QLatin1Char('_');
        }
    }
    QByteArray utf8 = cleaned.toUtf8();
    const QString suffix =
        QString::fromLatin1(QCryptographicHash::hash(worldFolderName.toUtf8(), QCryptographicHash::Sha256).toHex().left(8));
    if (utf8.size() + 9 > 128)
        utf8.truncate(128 - 9);
    return QString::fromUtf8(utf8) + QLatin1Char('-') + suffix;
}

bool isIgnored(const QString& baseName)
{
    if (baseName == QStringLiteral("Thumbs.db") || baseName == QStringLiteral(".DS_Store") || baseName == QStringLiteral("session.lock"))
        return true;
    return baseName.endsWith(QStringLiteral(".tmp")) || baseName.endsWith(QStringLiteral(".bak"));
}

static bool canceled(QAtomicInt* cancel)
{
    return cancel && cancel->loadRelaxed();
}

ScanResult scanWorld(const QString& worldDir, QAtomicInt* cancel, const std::function<void(qint64, qint64)>& progress)
{
    ScanResult result;
    const QDir root(worldDir);
    if (!root.exists()) {
        result.error = QStringLiteral("world directory does not exist");
        return result;
    }

    QList<QFileInfo> found;
    QList<QPair<QString, QString>> skippedEntries;
    QSet<QString> seenFold;
    QStringList allDirs;

    std::function<bool(const QDir&, const QString&)> walk = [&](const QDir& dir, const QString& rel) -> bool {
        QFileInfoList entries =
            dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System, QDir::NoSort);
        std::sort(entries.begin(), entries.end(),
                  [](const QFileInfo& a, const QFileInfo& b) { return a.fileName().toUtf8() < b.fileName().toUtf8(); });
        for (const QFileInfo& e : entries) {
            if (canceled(cancel))
                return false;
            const QString childRel = rel.isEmpty() ? e.fileName() : rel + QLatin1Char('/') + e.fileName();
            if (e.isSymbolicLink() || (!e.isFile() && !e.isDir())) {
                skippedEntries.append({ childRel, QStringLiteral("unreadable or excluded") });
                continue;
            }
            if (isIgnored(e.fileName()))
                continue;
            const QString mpath = QStringLiteral("world/") + childRel;
            QString pathErr;
            if (!validatePath(mpath, &pathErr)) {
                skippedEntries.append({ childRel, pathErr });
                continue;
            }
            if (e.isDir()) {
                allDirs.append(mpath);
                if (!walk(QDir(e.absoluteFilePath()), childRel))
                    return false;
                continue;
            }
            const QString fold = mpath.toLower();
            if (seenFold.contains(fold)) {
                skippedEntries.append({ childRel, QStringLiteral("duplicate path under case folding") });
                continue;
            }
            seenFold.insert(fold);
            found.append(e);
        }
        return true;
    };

    if (!walk(root, QString())) {
        result.error = QStringLiteral("aborted");
        return result;
    }

    std::sort(found.begin(), found.end(), [&](const QFileInfo& a, const QFileInfo& b) {
        return root.relativeFilePath(a.absoluteFilePath()).toUtf8() < root.relativeFilePath(b.absoluteFilePath()).toUtf8();
    });
    std::sort(allDirs.begin(), allDirs.end(), [](const QString& a, const QString& b) { return a.toUtf8() < b.toUtf8(); });

    for (const auto& s : skippedEntries)
        result.skipped.append({ s.first, s.second });

    qint64 totalToHash = 0;
    for (const QFileInfo& e : found)
        totalToHash += e.size();
    qint64 hashed = 0;

    QByteArray buffer;
    buffer.resize(CHUNK_SIZE);

    for (const QFileInfo& e : found) {
        if (canceled(cancel)) {
            result.error = QStringLiteral("aborted");
            return result;
        }
        ManifestFile f;
        f.localPath = e.absoluteFilePath();
        f.path = QStringLiteral("world/") + root.relativeFilePath(f.localPath);
        f.size = e.size();
        f.mode = 0644;
        f.mtimeNs = e.lastModified().toMSecsSinceEpoch() * 1000000;
        result.totalBytes += f.size;

        if (f.size > 0 && f.size <= INLINE_MAX_BYTES) {
            QFile file(f.localPath);
            if (!file.open(QIODevice::ReadOnly)) {
                result.error = QStringLiteral("failed to open %1").arg(f.localPath);
                return result;
            }
            f.inlineData = file.readAll();
            if (f.inlineData.size() != f.size) {
                result.error = QStringLiteral("failed to read %1").arg(f.localPath);
                return result;
            }
        } else if (f.size > INLINE_MAX_BYTES) {
            QFile file(f.localPath);
            if (!file.open(QIODevice::ReadOnly)) {
                result.error = QStringLiteral("failed to open %1").arg(f.localPath);
                return result;
            }
            qint64 offset = 0;
            while (offset < f.size) {
                if (canceled(cancel)) {
                    result.error = QStringLiteral("aborted");
                    return result;
                }
                const qint64 want = std::min<qint64>(CHUNK_SIZE, f.size - offset);
                const qint64 got = file.read(buffer.data(), want);
                if (got != want) {
                    result.error = QStringLiteral("failed to read %1").arg(f.localPath);
                    return result;
                }
                ChunkRef ref;
                ref.hash = QString::fromLatin1(
                    QCryptographicHash::hash(QByteArrayView(buffer.constData(), want), QCryptographicHash::Sha256).toHex());
                ref.size = static_cast<quint32>(want);
                ref.offset = offset;
                f.chunks.append(ref);
                offset += want;
                hashed += want;
                if (progress)
                    progress(hashed, totalToHash);
            }
        } else {
            hashed += f.size;
            if (progress)
                progress(hashed, totalToHash);
        }
        result.totalChunks += f.chunks.size();
        result.files.append(f);
    }

    for (const QString& d : allDirs) {
        bool hasFileDescendant = false;
        const QString prefix = d + QLatin1Char('/');
        for (const ManifestFile& f : result.files) {
            if (f.path.startsWith(prefix)) {
                hasFileDescendant = true;
                break;
            }
        }
        if (!hasFileDescendant)
            result.dirs.append(d);
    }

    return result;
}

static void appendJsonString(QByteArray& out, const QString& value)
{
    out.append('"');
    const QByteArray utf8 = value.toUtf8();
    for (qsizetype i = 0; i < utf8.size(); ++i) {
        const uchar b = static_cast<uchar>(utf8.at(i));
        switch (b) {
            case '"':
                out.append("\\\"");
                break;
            case '\\':
                out.append("\\\\");
                break;
            case '\n':
                out.append("\\n");
                break;
            case '\r':
                out.append("\\r");
                break;
            case '\t':
                out.append("\\t");
                break;
            case '\b':
                out.append("\\b");
                break;
            case '\f':
                out.append("\\f");
                break;
            default:
                if (b < 0x20) {
                    out.append("\\u00");
                    out.append("0123456789abcdef"[b >> 4]);
                    out.append("0123456789abcdef"[b & 0xf]);
                } else if (b == 0xe2 && i + 2 < utf8.size() && static_cast<uchar>(utf8.at(i + 1)) == 0x80 &&
                           (static_cast<uchar>(utf8.at(i + 2)) == 0xa8 || static_cast<uchar>(utf8.at(i + 2)) == 0xa9)) {
                    out.append(static_cast<uchar>(utf8.at(i + 2)) == 0xa8 ? "\\u2028" : "\\u2029");
                    i += 2;
                } else {
                    out.append(static_cast<char>(b));
                }
        }
    }
    out.append('"');
}

QByteArray encodeManifest(const QList<ManifestFile>& files,
                          const QStringList& dirs,
                          const QString& parent,
                          const QString& device,
                          const QString& label)
{
    QByteArray out;
    out.append("{\"version\":1");
    if (!parent.isEmpty()) {
        out.append(",\"parent\":");
        appendJsonString(out, parent);
    }
    if (!device.isEmpty()) {
        out.append(",\"device\":");
        appendJsonString(out, device);
    }
    if (!label.isEmpty()) {
        out.append(",\"label\":");
        appendJsonString(out, label);
    }
    if (!dirs.isEmpty()) {
        out.append(",\"dirs\":[");
        for (qsizetype i = 0; i < dirs.size(); ++i) {
            if (i)
                out.append(',');
            appendJsonString(out, dirs.at(i));
        }
        out.append(']');
    }
    out.append(",\"files\":[");
    for (qsizetype i = 0; i < files.size(); ++i) {
        const ManifestFile& f = files.at(i);
        if (i)
            out.append(',');
        out.append("{\"path\":");
        appendJsonString(out, f.path);
        out.append(",\"size\":");
        out.append(QByteArray::number(f.size));
        out.append(",\"mode\":");
        out.append(QByteArray::number(f.mode));
        out.append(",\"mtime_ns\":");
        out.append(QByteArray::number(f.mtimeNs));
        if (!f.chunks.isEmpty()) {
            out.append(",\"chunks\":[");
            for (qsizetype j = 0; j < f.chunks.size(); ++j) {
                if (j)
                    out.append(',');
                out.append("{\"h\":");
                appendJsonString(out, f.chunks.at(j).hash);
                out.append(",\"n\":");
                out.append(QByteArray::number(f.chunks.at(j).size));
                out.append('}');
            }
            out.append(']');
        } else if (!f.inlineData.isEmpty()) {
            out.append(",\"inline\":");
            appendJsonString(out, QString::fromLatin1(f.inlineData.toBase64()));
        }
        out.append('}');
    }
    out.append("]}");
    return out;
}

bool decodeManifest(const QByteArray& data, ParsedManifest* out, QString* error)
{
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        setErr(error, QStringLiteral("manifest: invalid JSON"));
        return false;
    }
    static const QSet<QString> topKeys = { "version", "enc", "parent", "device", "label", "dirs", "files" };
    const QJsonObject obj = doc.object();
    for (const QString& key : obj.keys()) {
        if (!topKeys.contains(key)) {
            setErr(error, QStringLiteral("manifest: unknown field %1").arg(key));
            return false;
        }
    }
    if (obj.value(QStringLiteral("version")).toInt(-1) != 1) {
        setErr(error, QStringLiteral("manifest: unsupported version"));
        return false;
    }
    if (obj.contains(QStringLiteral("enc"))) {
        setErr(error, QStringLiteral("manifest: encrypted vault snapshots are not supported"));
        return false;
    }
    ParsedManifest m;
    m.parent = obj.value(QStringLiteral("parent")).toString();
    m.device = obj.value(QStringLiteral("device")).toString();
    m.label = obj.value(QStringLiteral("label")).toString();
    for (const QJsonValue& v : obj.value(QStringLiteral("dirs")).toArray()) {
        if (!v.isString()) {
            setErr(error, QStringLiteral("manifest: bad dirs entry"));
            return false;
        }
        const QString d = v.toString();
        if (!validatePath(d)) {
            setErr(error, QStringLiteral("manifest: invalid dir path %1").arg(d));
            return false;
        }
        m.dirs.append(d);
    }
    const QJsonValue filesValue = obj.value(QStringLiteral("files"));
    if (!filesValue.isArray()) {
        setErr(error, QStringLiteral("manifest: missing files"));
        return false;
    }
    static const QSet<QString> fileKeys = { "path", "size", "mode", "mtime_ns", "chunks", "inline" };
    static const QSet<QString> chunkKeys = { "h", "n" };
    for (const QJsonValue& v : filesValue.toArray()) {
        if (!v.isObject()) {
            setErr(error, QStringLiteral("manifest: bad file entry"));
            return false;
        }
        const QJsonObject fo = v.toObject();
        for (const QString& key : fo.keys()) {
            if (!fileKeys.contains(key)) {
                setErr(error, QStringLiteral("manifest: unknown file field %1").arg(key));
                return false;
            }
        }
        ManifestFile f;
        f.path = fo.value(QStringLiteral("path")).toString();
        if (!validatePath(f.path)) {
            setErr(error, QStringLiteral("manifest: invalid file path %1").arg(f.path));
            return false;
        }
        const double size = fo.value(QStringLiteral("size")).toDouble(-1);
        if (size < 0) {
            setErr(error, QStringLiteral("manifest: bad size for %1").arg(f.path));
            return false;
        }
        f.size = static_cast<qint64>(size);
        f.mode = static_cast<quint32>(fo.value(QStringLiteral("mode")).toDouble(0)) & 0777;
        f.mtimeNs = static_cast<qint64>(fo.value(QStringLiteral("mtime_ns")).toDouble(0));
        const QJsonValue chunksValue = fo.value(QStringLiteral("chunks"));
        if (chunksValue.isArray()) {
            for (const QJsonValue& cv : chunksValue.toArray()) {
                const QJsonObject co = cv.toObject();
                for (const QString& key : co.keys()) {
                    if (!chunkKeys.contains(key)) {
                        setErr(error, QStringLiteral("manifest: unknown chunk field %1").arg(key));
                        return false;
                    }
                }
                ChunkRef ref;
                ref.hash = co.value(QStringLiteral("h")).toString();
                ref.size = static_cast<quint32>(co.value(QStringLiteral("n")).toDouble(0));
                f.chunks.append(ref);
            }
        }
        const QJsonValue inlineValue = fo.value(QStringLiteral("inline"));
        if (inlineValue.isString())
            f.inlineData = QByteArray::fromBase64(inlineValue.toString().toLatin1());
        m.files.append(f);
    }
    if (out)
        *out = m;
    return true;
}

}  // namespace Saves
