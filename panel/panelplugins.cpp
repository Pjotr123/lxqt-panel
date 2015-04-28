#include "panelplugins.h"
#include "plugin.h"
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
    if (plugin.isNull())
    {
        mPlugins.append({name, plugin});
        emit pluginAdded(LxQt::PluginData(plugin->desktopFile().id(), plugin, plugin->popupMenu()));
    }
}

void PanelPlugins::removePlugin()
{
    Plugin * p = qobject_cast<Plugin*>(sender());
    auto  plugin = std::find_if(mPlugins.begin(), mPlugins.end(), [p] (container_t::const_reference obj) { return p == obj.second; });
    if (mPlugins.end() != plugin)
    {
        mPlugins.erase(plugin);
        emit pluginRemoved(LxQt::PluginData(p->desktopFile().id(), p, nullptr/*don't want any menu*/));
        p->deleteLater();
    }
}

void PanelPlugins::movePlugin(Plugin const * plugin, QString const & nameAfter)
{
    //merge list of plugins (try to preserve original position)
    auto moved = std::find_if(mPlugins.begin(), mPlugins.end(), [plugin] (container_t::const_reference obj) { return plugin == obj.second.data(); });
    auto new_pos = std::find_if(mPlugins.begin(), mPlugins.end(), [nameAfter] (container_t::const_reference obj) { return nameAfter == obj.first; });

    auto pos = mPlugins.insert(new_pos, *moved);
    for (auto i = mPlugins.begin(), i_e = mPlugins.end(); i_e != i; ++i)
    {
        if (pos != i && i->first == pos->first)
        {
            mPlugins.erase(i);
            break;
        }
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


