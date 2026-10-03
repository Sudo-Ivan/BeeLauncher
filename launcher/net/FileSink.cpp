// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
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
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include "FileSink.h"

#include <QFile>
#include <QFileInfo>

#include "FileSystem.h"

#if defined(LAUNCHER_APPLICATION)
#include "Application.h"
#endif

#include "net/Logging.h"

namespace Net {

QString FileSink::partFileName() const
{
    return m_filename + ".part";
}

Task::State FileSink::init(QNetworkRequest& request)
{
    auto result = initCache(request);
    if (result != Task::State::Running) {
        return result;
    }

    // create a new save file and open it for writing
    if (!FS::ensureFilePathExists(m_filename)) {
        qCCritical(taskNetLogC) << "Could not create folder for " + m_filename;
        m_fail_reason = "Could not create folder";
        return Task::State::Failed;
    }

    m_wroteAnyData = false;
    m_status_seen = false;
    m_skip_writes = false;
    m_output_file.reset();

    // if a previous attempt left a partial download behind, ask the server
    // to continue from where it stopped
    m_resume_offset = 0;
    m_resuming = false;
    QFile partFile(partFileName());
    if (partFile.exists() && partFile.size() > 0) {
        m_resume_offset = partFile.size();
        m_resuming = true;
        request.setRawHeader("Range", QString("bytes=%1-").arg(m_resume_offset).toUtf8());
        qCDebug(taskNetLogC) << "Resuming download of" << m_filename << "from byte" << m_resume_offset;
    }

    m_output_file.reset(new QFile(partFileName()));
    if (!m_output_file->open(m_resuming ? QIODevice::Append : QIODevice::WriteOnly)) {
        const auto error = QString("Could not open %1 for writing: %2").arg(partFileName()).arg(m_output_file->errorString());
        qCCritical(taskNetLogC) << error;
        m_fail_reason = error;
        return Task::State::Failed;
    }

#if defined(LAUNCHER_APPLICATION)
    if (auto app = APPLICATION_DYN) {
        app->addQSavePath(QFileInfo(m_filename).absoluteFilePath() + ".");
    }
#endif

    if (initAllValidators(request))
        return Task::State::Running;
    m_fail_reason = "Failed to initialize validators";
    return Task::State::Failed;
}

void FileSink::statusReceived(QNetworkReply& reply)
{
    if (m_status_seen) {
        return;
    }
    m_status_seen = true;

    int statusCode = reply.attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (m_resuming) {
        if (statusCode == 206) {
            // the server honored the Range request: feed the existing part file
            // through the validators so the final checksum covers the whole file
            QFile partFile(partFileName());
            if (partFile.open(QIODevice::ReadOnly)) {
                while (!partFile.atEnd()) {
                    auto data = partFile.read(1 << 20);
                    if (data.isEmpty()) {
                        break;
                    }
                    if (!writeAllValidators(data)) {
                        qCCritical(taskNetLogC) << "Failed to feed partial data to validators for" << m_filename;
                    }
                }
                m_wroteAnyData = true;
                return;
            }
        } else if (statusCode >= 300) {
            // error or redirect response: keep the partial data for the next
            // attempt and discard this response body
            m_resuming = false;
            m_skip_writes = true;
            return;
        }

        // the server did not accept the Range request: start over from scratch
        qCDebug(taskNetLogC) << "Server did not accept resume for" << m_filename << "status" << statusCode;
        m_resuming = false;
        m_resume_offset = 0;
        m_output_file->close();
        if (!m_output_file->open(QIODevice::WriteOnly)) {
            qCCritical(taskNetLogC) << "Could not truncate partial file for" << m_filename;
        }
    }

    // do not persist error pages or redirect bodies into the partial file
    m_skip_writes = statusCode >= 300;
}

Task::State FileSink::write(QByteArray& data)
{
    if (m_skip_writes) {
        return Task::State::Running;
    }
    if (!writeAllValidators(data) || m_output_file->write(data) != data.size()) {
        QString error = QString("Failed writing into %1: %2").arg(m_filename);
        if (m_output_file->error() == QFileDevice::NoError) {
            error = error.arg("Validators failed");
        } else {
            error = error.arg(m_output_file->errorString());
        }
        qCCritical(taskNetLogC) << error;
        m_fail_reason = error;
        m_output_file.reset();
        QFile::remove(partFileName());
        m_wroteAnyData = false;
        return Task::State::Failed;
    }

    m_wroteAnyData = true;
    return Task::State::Running;
}

Task::State FileSink::abort()
{
    // keep the partial file so a retry can resume from it
    if (m_output_file) {
        m_output_file->close();
    }
    failAllValidators();
#if defined(LAUNCHER_APPLICATION)
    if (auto app = APPLICATION_DYN) {
        app->removeQSavePath(QFileInfo(m_filename).absoluteFilePath() + ".");
    }
#endif
    return Task::State::Failed;
}

Task::State FileSink::finalize(QNetworkReply& reply)
{
    bool gotFile = false;
    QVariant statusCodeV = reply.attribute(QNetworkRequest::HttpStatusCodeAttribute);
    bool validStatus = false;
    int statusCode = statusCodeV.toInt(&validStatus);
    if (validStatus) {
        // this leaves out 304 Not Modified; 206 is a completed resume
        gotFile = statusCode == 200 || statusCode == 203 || statusCode == 206;
    }

    // if we wrote any data to the save file, we try to commit the data to the real file.
    // if it actually got a proper file, we write it even if it was empty
    if (gotFile || m_wroteAnyData) {
        // ask validators for data consistency
        // we only do this for actual downloads, not 'your data is still the same' cache hits
        if (!finalizeAllValidators(reply)) {
            m_fail_reason = "Failed to finalize validators";
            // the partial data is suspect, do not resume from it next time
            m_output_file.reset();
            QFile::remove(partFileName());
#if defined(LAUNCHER_APPLICATION)
            if (auto app = APPLICATION_DYN) {
                app->removeQSavePath(QFileInfo(m_filename).absoluteFilePath() + ".");
            }
#endif
            return Task::State::Failed;
        }

        // nothing went wrong...
        m_output_file->close();
        QFile::remove(m_filename);
        if (!m_output_file->rename(m_filename)) {
            const auto error =
                QString("Failed to commit changes to %1: %2").arg(m_filename).arg(m_output_file->errorString());
            qCCritical(taskNetLogC) << error;
            m_fail_reason = error;
            m_output_file.reset();
#if defined(LAUNCHER_APPLICATION)
            if (auto app = APPLICATION_DYN) {
                app->removeQSavePath(QFileInfo(m_filename).absoluteFilePath() + ".");
            }
#endif
            return Task::State::Failed;
        }
    }

    // then get rid of the save file
    m_output_file.reset();
#if defined(LAUNCHER_APPLICATION)
    if (auto app = APPLICATION_DYN) {
        app->removeQSavePath(QFileInfo(m_filename).absoluteFilePath() + ".");
    }
#endif

    return finalizeCache(reply);
}

Task::State FileSink::initCache(QNetworkRequest&)
{
    return Task::State::Running;
}

Task::State FileSink::finalizeCache(QNetworkReply&)
{
    return Task::State::Succeeded;
}

bool FileSink::hasLocalData()
{
    QFileInfo info(m_filename);
    return info.exists() && info.size() != 0;
}
}  // namespace Net
