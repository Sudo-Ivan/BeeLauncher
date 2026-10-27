#include "ShellInstanceBridge.h"
#include "ShellResourceModel.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>
#include <QVariantMap>

#include "../../../Application.h"
#include "../../../BaseInstance.h"
#include "../../../DesktopServices.h"
#include "../../../FileSystem.h"
#include "../../../InstanceList.h"
#include "../../../minecraft/MinecraftInstance.h"
#include "../../../minecraft/WorldList.h"
#include "../../../minecraft/mod/DataPackFolderModel.h"
#include "../../../minecraft/mod/ModFolderModel.h"
#include "../../../minecraft/mod/ResourcePackFolderModel.h"
#include "../../../minecraft/mod/ShaderPackFolderModel.h"
#include "../../../minecraft/mod/TexturePackFolderModel.h"
#include "../../../settings/SettingsObject.h"
#include "../../MainWindow.h"

namespace {

// Same trick as ShellInstanceModel: the version label lives in mmc-pack.json
// and is cheap to read straight off disk.
QString packVersionText(BaseInstance* inst)
{
    const QString path = QDir(inst->instanceRoot()).filePath(QStringLiteral("mmc-pack.json"));
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    QString mc, loader;
    static const QList<QPair<QString, QString>> loaders = { { "net.fabricmc.fabric-loader", "Fabric" },
                                                            { "org.quiltmc.quilt-loader", "Quilt" },
                                                            { "net.neoforged", "NeoForge" },
                                                            { "net.minecraftforge", "Forge" },
                                                            { "com.mumfrey.liteloader", "LiteLoader" } };
    for (const auto& value : QJsonDocument::fromJson(file.readAll()).object().value("components").toArray()) {
        const auto component = value.toObject();
        const QString uid = component.value("uid").toString();
        if (uid == "net.minecraft")
            mc = component.value("version").toString();
        for (const auto& [loaderUid, label] : loaders)
            if (uid == loaderUid && loader.isEmpty())
                loader = label;
    }
    return loader.isEmpty() ? mc : (mc.isEmpty() ? loader : mc + " " + loader);
}

}  // namespace

ShellInstanceBridge::ShellInstanceBridge(QObject* parent) : QObject(parent)
{
    m_mods = new ShellResourceModel(this);
    m_coreMods = new ShellResourceModel(this);
    m_resourcePacks = new ShellResourceModel(this);
    m_texturePacks = new ShellResourceModel(this);
    m_shaderPacks = new ShellResourceModel(this);
    m_dataPacks = new ShellResourceModel(this);
    m_worlds = new ShellResourceModel(this);
}

void ShellInstanceBridge::attachBackend()
{
    m_backend = true;
    if (!m_id.isEmpty())
        resolve();
}

void ShellInstanceBridge::setInstanceId(const QString& id)
{
    if (m_id == id)
        return;
    m_id = id;
    resolve();
    emit instanceChanged();
}

void ShellInstanceBridge::resolve()
{
    if (m_inst)
        disconnect(m_inst, nullptr, this, nullptr);
    if (m_settings)
        disconnect(m_settings, nullptr, this, nullptr);

    m_inst = nullptr;
    m_mc = nullptr;
    m_settings = nullptr;
    if (m_backend && !m_id.isEmpty())
        m_inst = APPLICATION->instances()->getInstanceById(m_id);
    if (m_inst)
        m_mc = qobject_cast<MinecraftInstance*>(m_inst.data());

    if (m_inst) {
        connect(m_inst, &BaseInstance::runningStatusChanged, this, &ShellInstanceBridge::instanceChanged);
        connect(m_inst, &BaseInstance::propertiesChanged, this, &ShellInstanceBridge::instanceChanged);
        m_settings = m_inst->settings();
        connect(m_settings, &SettingsObject::SettingChanged, this, [this](const Setting&, QVariant) { emit settingsChanged(); });
        connect(m_settings, &SettingsObject::settingReset, this, [this](const Setting&) { emit settingsChanged(); });
        connect(m_inst, &QObject::destroyed, this, &ShellInstanceBridge::instanceChanged);
    }

    // Attach the resource models. For non-Minecraft instances these all
    // become null and the QML side shows its empty states.
    m_mods->setSource(m_mc ? m_mc->loaderModList() : nullptr);
    m_coreMods->setSource(m_mc ? m_mc->coreModList() : nullptr);
    m_resourcePacks->setSource(m_mc ? m_mc->resourcePackList() : nullptr);
    m_texturePacks->setSource(m_mc ? m_mc->texturePackList() : nullptr);
    m_shaderPacks->setSource(m_mc ? m_mc->shaderPackList() : nullptr);
    m_dataPacks->setSource(m_mc ? m_mc->dataPackList() : nullptr);
    m_worlds->setSource(m_mc ? m_mc->worldList() : nullptr);
}

QString ShellInstanceBridge::name() const
{
    return m_inst ? m_inst->name() : QString();
}

QString ShellInstanceBridge::versionText() const
{
    if (!m_inst)
        return {};
    QString text = m_inst->getVersionString();
    if (text.isEmpty())
        text = packVersionText(m_inst);
    return text;
}

QString ShellInstanceBridge::iconSource() const
{
    return m_inst ? QStringLiteral("image://beeicons/") + m_inst->iconKey() : QString();
}

bool ShellInstanceBridge::running() const
{
    return m_inst && m_inst->isRunning();
}

QString ShellInstanceBridge::typeDisplay() const
{
    return m_inst ? m_inst->typeName() : QString();
}

QString ShellInstanceBridge::instanceRoot() const
{
    return m_inst ? m_inst->instanceRoot() : QString();
}

QString ShellInstanceBridge::gameRoot() const
{
    return m_inst ? m_inst->gameRoot() : QString();
}

QString ShellInstanceBridge::notes() const
{
    return m_inst ? m_inst->notes() : QString();
}

void ShellInstanceBridge::setNotes(const QString& text)
{
    if (m_inst && m_inst->notes() != text)
        m_inst->setNotes(text);
}

void ShellInstanceBridge::launch()
{
    if (m_backend && m_inst)
        APPLICATION->launch(m_inst);
}

void ShellInstanceBridge::kill()
{
    if (!m_backend || !m_inst) {
        emit actionFailed(tr("Stop is unavailable right now"));
        return;
    }
    auto* window = APPLICATION->legacyWindow();
    if (!window) {
        emit actionFailed(tr("Stop is unavailable right now"));
        return;
    }
    QMetaObject::invokeMethod(window, "setSelectedInstanceById", Q_ARG(QString, m_id));
    if (!QMetaObject::invokeMethod(window, "on_actionKillInstance_triggered"))
        emit actionFailed(tr("Stop is unavailable right now"));
}

void ShellInstanceBridge::openFolder()
{
    if (m_inst)
        DesktopServices::openPath(m_inst->instanceRoot(), true);
}

void ShellInstanceBridge::openGameFolder()
{
    if (m_inst)
        DesktopServices::openPath(m_inst->gameRoot(), true);
}

void ShellInstanceBridge::openClassic()
{
    if (m_backend && m_inst)
        APPLICATION->showInstanceWindow(m_inst);
}

void ShellInstanceBridge::openClassicPage(const QString& page)
{
    if (m_backend && m_inst)
        APPLICATION->showInstanceWindow(m_inst, page);
}

void ShellInstanceBridge::openFile(const QString& path)
{
    if (!path.isEmpty())
        DesktopServices::openPath(path);
}

QVariantList ShellInstanceBridge::screenshots()
{
    QVariantList out;
    if (!m_inst)
        return out;
    const QDir dir(FS::PathCombine(m_inst->gameRoot(), QStringLiteral("screenshots")));
    const auto entries = dir.entryInfoList({ "*.png", "*.jpg", "*.jpeg" }, QDir::Files | QDir::Readable, QDir::Time);
    for (const auto& entry : entries)
        out.append(QVariantMap{ { "path", entry.absoluteFilePath() },
                                { "name", entry.fileName() },
                                { "mtime", entry.lastModified().toMSecsSinceEpoch() } });
    return out;
}

QString ShellInstanceBridge::screenshotUrl(const QString& path)
{
    return QStringLiteral("file://") + path;
}

void ShellInstanceBridge::openScreenshotsFolder()
{
    if (m_inst)
        DesktopServices::openPath(FS::PathCombine(m_inst->gameRoot(), QStringLiteral("screenshots")), true);
}

QVariantList ShellInstanceBridge::logFiles()
{
    QVariantList out;
    if (!m_inst)
        return out;
    // Same enumeration as OtherLogsPage: relative paths under gameRoot.
    const QString base = m_inst->gameRoot();
    const QDir baseDir(base);
    QStringList searchPaths = m_inst->getLogFileSearchPaths();
    if (searchPaths.isEmpty())
        searchPaths = { FS::PathCombine(base, "crash-reports"), FS::PathCombine(base, "logs"), base };
    for (const QString& searchPath : searchPaths) {
        QDir searchDir(searchPath);
        if (searchDir.isRelative())
            searchDir = QDir(baseDir.absoluteFilePath(searchPath));
        QStringList filters{ "*.log", "*.log.gz" };
        if (searchDir.absolutePath() != baseDir.absolutePath())
            filters.append("*.txt");
        const auto entries = searchDir.entryInfoList(filters, QDir::Files | QDir::Readable, QDir::Time);
        for (const auto& entry : entries)
            out.append(QVariantMap{ { "name", baseDir.relativeFilePath(entry.absoluteFilePath()) },
                                    { "path", entry.absoluteFilePath() },
                                    { "sizeBytes", entry.size() },
                                    { "mtime", entry.lastModified().toMSecsSinceEpoch() } });
    }
    return out;
}

QString ShellInstanceBridge::readLogTail(const QString& path, int maxLines)
{
    if (path.isEmpty())
        return {};
    if (path.endsWith(QStringLiteral(".gz")))
        return tr("This log is compressed. Open it in an external viewer to read it.");
    maxLines = qBound(1, maxLines, 5000);
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return tr("Could not open %1").arg(path);
    // Read at most the last 4 MiB so huge logs stay cheap.
    constexpr qint64 kMaxTail = 4 * 1024 * 1024;
    if (file.size() > kMaxTail) {
        file.seek(file.size() - kMaxTail);
        file.readLine();  // drop the partial first line
    }
    const QString text = QString::fromUtf8(file.readAll());
    const QStringList lines = text.split(QLatin1Char('\n'));
    if (lines.size() <= maxLines)
        return text;
    return lines.mid(lines.size() - maxLines).join(QLatin1Char('\n'));
}

void ShellInstanceBridge::openLog(const QString& path)
{
    if (!path.isEmpty())
        DesktopServices::openPath(path);
}

QVariant ShellInstanceBridge::instValue(const QString& key)
{
    if (!m_inst)
        return {};
    return m_inst->settings()->get(key);
}

QVariant ShellInstanceBridge::normalizeFor(const QVariant& current, const QVariant& value)
{
    // QML hands us JS numbers and strings; coerce to the stored type so
    // passthrough settings keep their declared types.
    QVariant v = value;
    if (current.isValid() && v.typeId() != current.typeId() && v.canConvert(current.metaType()))
        v.convert(current.metaType());
    return v;
}

bool ShellInstanceBridge::setInstValue(const QString& key, const QVariant& value)
{
    if (!m_inst)
        return false;
    auto* settings = m_inst->settings();
    return settings->set(key, normalizeFor(settings->get(key), value));
}

bool ShellInstanceBridge::instGate(const QString& gateKey)
{
    return m_inst && m_inst->settings()->get(gateKey).toBool();
}

void ShellInstanceBridge::setInstGate(const QString& gateKey, bool on)
{
    if (m_inst)
        m_inst->settings()->set(gateKey, on);
}

void ShellInstanceBridge::resetInstValue(const QString& key)
{
    if (m_inst)
        m_inst->settings()->reset(key);
}
