#include "panelplugins.h"
#include "plugin.h"
#include "ilxqtpanelplugin.h"
#include "lxqtpanel.h"
#include <QPointer>
#include <XdgIcon>
#include <LXQt/Settings>

#include <QDebug>

PanelPlugins::PanelPlugins(LxQtPanel * panel
        , QString const & namesKey
        , QStringList const & desktopDirs
        , QObject * parent/* = nullptr*/
        )
    : QAbstractListModel{parent}
    , mPanel{panel}
{
    loadPlugins(namesKey, desktopDirs);
}

PanelPlugins::~PanelPlugins()
{
    qDeleteAll(plugins());
}

int PanelPlugins::rowCount(const QModelIndex & parent/* = QModelIndex()*/) const
{
    return QModelIndex() == parent ? mPlugins.size() : 0;
}


QVariant PanelPlugins::data(const QModelIndex & index, int role/* = Qt::DisplayRole*/) const
{
    Q_ASSERT(QModelIndex() == index.parent()
            && 0 == index.column()
            && mPlugins.size() > index.row()
            );

    container_t::const_reference plugin = mPlugins[index.row()];
    QVariant ret;
    switch (role)
    {
        case Qt::DisplayRole:
            if (plugin.second.isNull())
                ret = QStringLiteral("<b>unknown</b> (%1)<br/>we weren't able to load this plugin").arg(plugin.first);
            else
                ret = QStringLiteral("<b>%1</b> (%2)<br/>%3").arg(plugin.second->name()
                        , plugin.first
                        , plugin.second->desktopFile().value(QStringLiteral("Comment")).toString());
            break;
        case Qt::DecorationRole:
            if (plugin.second.isNull())
                ret = XdgIcon::fromTheme("preferences-plugin");
            else
                ret = plugin.second->desktopFile().icon(XdgIcon::fromTheme("preferences-plugin"));
            break;
    }
    return ret;
}

Qt::ItemFlags PanelPlugins::flags(const QModelIndex & index) const
{
    Qt::ItemFlags f = Qt::ItemIsSelectable | Qt::ItemNeverHasChildren;
    if (index.row() < mPlugins.size() && !mPlugins[index.row()].second.isNull())
        f |= Qt::ItemIsEnabled;
    return f;
}

QStringList PanelPlugins::pluginNames() const
{
    QStringList names;
    for (auto const & p : mPlugins)
        names.append(p.first);
    return std::move(names);
}

QList<Plugin *> PanelPlugins::plugins() const
{
    QList<Plugin *> plugins;
    for (auto const & p : mPlugins)
        if (!p.second.isNull())
            plugins.append(p.second.data());
    return std::move(plugins);
}

void PanelPlugins::addPlugin(const LxQt::PluginInfo &desktopFile)
{
    QString name = findNewPluginSettingsGroup(desktopFile.id());
    QPointer<Plugin> plugin = loadPlugin(desktopFile, name);
    if (!plugin.isNull())
    {
        beginInsertRows(QModelIndex(), mPlugins.size(), mPlugins.size());
        mPlugins.append({name, plugin});
        endInsertRows();
        emit pluginAdded(plugin.data());
    }
}

void PanelPlugins::removePlugin()
{
    Plugin * p = qobject_cast<Plugin*>(sender());
    auto plugin = std::find_if(mPlugins.begin(), mPlugins.end(), [p] (container_t::const_reference obj) { return p == obj.second; });
    if (mPlugins.end() != plugin)
    {
        const int row = plugin - mPlugins.begin();
        beginRemoveRows(QModelIndex(), row, row);
        mPlugins.erase(plugin);
        endRemoveRows();
        mActive = mPlugins.isEmpty() ? QModelIndex() : createIndex(mPlugins.size() > row ? row : row - 1, 0);
        emit pluginRemoved(plugin->second.data());
        p->deleteLater();
    }
}

void PanelPlugins::movePlugin(Plugin const * plugin, QString const & nameAfter)
{
    //merge list of plugins (try to preserve original position)
    auto moved = std::find_if(mPlugins.begin(), mPlugins.end(), [plugin] (container_t::const_reference obj) { return plugin == obj.second.data(); });
    auto new_pos = std::find_if(mPlugins.begin(), mPlugins.end(), [nameAfter] (container_t::const_reference obj) { return nameAfter == obj.first; });

    const int from = moved - mPlugins.begin();
    const int to = new_pos == mPlugins.end() ? mPlugins.size() - 1 : new_pos - mPlugins.begin();
    int higher, lower;
    if (to > from)
    {
        higher = to;
        lower = from;
    } else
    {
        higher = from;
        lower = to;
    }

    if (higher != lower)
    {
        beginMoveRows(QModelIndex(), higher, higher, QModelIndex(), lower);
        mPlugins.swap(higher, lower);
        endMoveRows();
    }
}

void PanelPlugins::loadPlugins(QString const & namesKey, QStringList const & desktopDirs)
{
    QStringList plugin_names = mPanel->settings()->value(namesKey).toStringList();

#ifdef DEBUG_PLUGIN_LOADTIME
    QElapsedTimer timer;
    timer.start();
    qint64 lastTime = 0;
#endif
    for (auto const & name : plugin_names)
    {
        container_t::iterator i = mPlugins.insert(mPlugins.end(), {name, nullptr});
        QString type = mPanel->settings()->value(name + "/type").toString();
        if (type.isEmpty())
        {
            qWarning() << QString("Section \"%1\" not found in %2.").arg(name, mPanel->settings()->fileName());
            continue;
        }

        LxQt::PluginInfoList list = LxQt::PluginInfo::search(desktopDirs, "LxQtPanel/Plugin", QString("%1.desktop").arg(type));
        if( !list.count())
        {
            qWarning() << QString("Plugin \"%1\" not found.").arg(type);
            continue;
        }

        i->second = loadPlugin(list.first(), name);
#ifdef DEBUG_PLUGIN_LOADTIME
        qDebug() << "load plugin" << type << "takes" << (timer.elapsed() - lastTime) << "ms";
        lastTime = timer.elapsed();
#endif
    }
}

QPointer<Plugin> PanelPlugins::loadPlugin(LxQt::PluginInfo const & desktopFile, QString const & settingsGroup)
{
    std::unique_ptr<Plugin> plugin{new Plugin{desktopFile, mPanel->settings()->fileName(), settingsGroup, mPanel}};
    if (plugin->isLoaded())
    {
        connect(plugin.get(), &Plugin::remove, this, &PanelPlugins::removePlugin);
        return plugin.release();
    }

    return nullptr;
}

QString PanelPlugins::findNewPluginSettingsGroup(const QString &pluginType) const
{
    QStringList groups = mPanel->settings()->childGroups();
    groups.sort();

    // Generate new section name
    for (int i = 2; true; ++i)
        if (!groups.contains(QStringLiteral("%1%2").arg(pluginType).arg(i)))
            return QStringLiteral("%1%2").arg(pluginType).arg(i);
}

void PanelPlugins::onActivatedIndex(QModelIndex const & index)
{
    mActive = index;
}

bool PanelPlugins::isActiveIndexValid() const
{
    return mActive.isValid() && QModelIndex() == mActive.parent()
        && 0 == mActive.column() && mPlugins.size() > mActive.row();
}

void PanelPlugins::onMovePluginUp()
{
    if (!isActiveIndexValid())
        return;

    const int row = mActive.row();
    if (0 >= row)
        return; //can't move up

    beginMoveRows(QModelIndex(), row, row, QModelIndex(), row - 1);
    mPlugins.swap(row - 1, row);
    endMoveRows();
    container_t::const_reference moved_plugin = mPlugins[row - 1];
    container_t::const_reference prev_plugin = mPlugins[row];

    //emit signal for layout only in case both plugins are loaded/displayed
    if (!moved_plugin.second.isNull() && !prev_plugin.second.isNull())
        emit pluginMovedUp(moved_plugin.second.data());
}

void PanelPlugins::onMovePluginDown()
{
    if (!isActiveIndexValid())
        return;

    const int row = mActive.row();
    if (mPlugins.size() <= row + 1)
        return; //can't move down

    beginMoveRows(QModelIndex(), row, row, QModelIndex(), row + 2);
    mPlugins.swap(row, row + 1);
    endMoveRows();
    container_t::const_reference moved_plugin = mPlugins[row + 1];
    container_t::const_reference next_plugin = mPlugins[row];

    //emit signal for layout only in case both plugins are loaded/displayed
    if (!moved_plugin.second.isNull() && !next_plugin.second.isNull())
        emit pluginMovedUp(next_plugin.second.data());
}

void PanelPlugins::onConfigurePlugin()
{
    if (!isActiveIndexValid())
        return;

    Plugin * const plugin = mPlugins[mActive.row()].second.data();
    if (nullptr != plugin && (ILxQtPanelPlugin::HaveConfigDialog & plugin->iPlugin()->flags()))
        plugin->showConfigureDialog();
}

void PanelPlugins::onRemovePlugin()
{
    if (!isActiveIndexValid())
        return;

    Plugin * const plugin = mPlugins[mActive.row()].second.data();
    if (nullptr != plugin)
        plugin->requestRemove();
}
