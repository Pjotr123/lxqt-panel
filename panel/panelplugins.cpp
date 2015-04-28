#include "panelplugins.h"
#include "plugin.h"
#include <QScopedPointer>
#include <XdgIcon>
#include <QDebug>

PanelPlugins::~PanelPlugins()
{
    qDeleteAll(mPlugins);
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

    Plugin * plugin = mPlugins[index.row()];
    QVariant ret;
    switch (role)
    {
        case Qt::DisplayRole:
            ret = QStringLiteral("<b>%1</b> (%2)<br/>%3").arg(plugin->name()
                    , plugin->settingsGroup()
                    , plugin->desktopFile().value(QStringLiteral("Comment")).toString());
            break;
        case Qt::DecorationRole:
            return ret = plugin->desktopFile().icon(XdgIcon::fromTheme("preferences-plugin"));
            break;
    }
    return ret;
}

Qt::ItemFlags PanelPlugins::flags(const QModelIndex & /*index*/) const
{
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QStringList const & PanelPlugins::pluginNames() const
{
    return mConfigOrder;
}

void PanelPlugins::setPluginNames(QStringList && names)
{
    mConfigOrder = names;
}

Plugin * PanelPlugins::addPlugin(std::unique_ptr<Plugin> plugin)
{
    mPlugins.append(plugin.release());
    return mPlugins.back();
}

void PanelPlugins::addPluginName(QString const & name)
{
    mConfigOrder.append(name);
}

void PanelPlugins::removePlugin(Plugin * plugin)
{
    mConfigOrder.removeAll(plugin->settingsGroup());
    mPlugins.removeAll(plugin);
    plugin->deleteLater();
}

void PanelPlugins::movePlugin(Plugin const * plugin, QString const & nameBefore)
{
    //merge list of plugins (try to preserve original position)
    mConfigOrder.removeAll(plugin->settingsGroup());
    mConfigOrder.insert(mConfigOrder.indexOf(nameBefore)/*-1 if not found*/ + 1, plugin->settingsGroup());
}
