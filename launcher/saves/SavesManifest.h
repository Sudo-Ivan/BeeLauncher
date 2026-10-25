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

#include <QAtomicInt>
#include <QByteArray>
#include <QList>
#include <QString>
#include <QStringList>

#include <functional>

namespace Saves {

constexpr qint64 CHUNK_SIZE = 1 << 20;
constexpr qint64 INLINE_MAX_BYTES = 64 << 10;
constexpr const char* APP_ID = "minecraft-java";
constexpr const char* MANIFEST_ROOT = "world";

struct ChunkRef {
    QString hash;
    quint32 size = 0;
    qint64 offset = 0;
};

struct ManifestFile {
    QString path;
    QString localPath;
    qint64 size = 0;
    quint32 mode = 0644;
    qint64 mtimeNs = 0;
    QList<ChunkRef> chunks;
    QByteArray inlineData;
};

struct SkippedPath {
    QString path;
    QString reason;
};

struct ScanResult {
    QList<ManifestFile> files;
    QStringList dirs;
    QList<SkippedPath> skipped;
    qint64 totalBytes = 0;
    qint64 totalChunks = 0;
    QString error;
    bool ok() const { return error.isEmpty(); }
};

struct ParsedManifest {
    QString parent;
    QString device;
    QString label;
    QStringList dirs;
    QList<ManifestFile> files;
};

/// Portable path rules shared with the saves server. path is a slash
/// separated manifest path such as "world/level.dat".
bool validatePath(const QString& path, QString* error = nullptr);

/// Slot name rules: non-empty, at most 128 bytes of UTF-8, no slashes,
/// no control characters.
bool validateSlotName(const QString& name, QString* error = nullptr);

/// The slot name used for a world folder: the name itself when valid,
/// otherwise sanitized with a content hash suffix.
QString slotNameFor(const QString& worldFolderName);

/// Basename ignore rules: *.tmp, *.bak, Thumbs.db, .DS_Store, session.lock.
bool isIgnored(const QString& baseName);

/// Recursively scan a world directory. cancel stops early when nonzero.
/// progress reports hashed bytes against the total bytes to hash.
ScanResult scanWorld(const QString& worldDir, QAtomicInt* cancel = nullptr, const std::function<void(qint64, qint64)>& progress = {});

/// Canonical JSON encoding of the manifest, Go struct field order.
QByteArray encodeManifest(const QList<ManifestFile>& files,
                          const QStringList& dirs,
                          const QString& parent,
                          const QString& device,
                          const QString& label);

/// Strict decode of a manifest document. Fails on unknown versions,
/// encrypted manifests, and invalid paths.
bool decodeManifest(const QByteArray& data, ParsedManifest* out, QString* error = nullptr);

}  // namespace Saves
