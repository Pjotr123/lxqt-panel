#ifndef PANELPLUGINSMODEL_H
#define PANELPLUGINSMODEL_H

#include <QAbstractListModel>
#include <memory>

namespace LxQt
{
    class PluginInfo;
    struct PluginData;
}

class LxQtPanel;
class Plugin;

class PanelPluginsModel : public QAbstractListModel
{
    Q_OBJECT
public:
    PanelPluginsModel(LxQtPanel * panel,
                      QString const & namesKey,
                      QStringList const & desktopDirs,
                      QObject * parent = nullptr);
    ~PanelPluginsModel();

    virtual int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
    virtual Qt::ItemFlags flags(const QModelIndex & index) const override;

    QStringList pluginNames() const;
    QList<Plugin *> plugins() const;

    /*!
     * \param plugin plugin that has been moved
     * \param nameAfter name of plugin that is right after moved plugin
     */
    void movePlugin(Plugin const * plugin, QString const & nameAfter);

signals:
    void pluginAdded(Plugin * plugin);
    void pluginRemoved(Plugin * plugin);
    /*!
     * Emiting only move-up for simplification of using (and problematic layout/list move)
     */
    void pluginMovedUp(Plugin * plugin);

public slots:
    void addPlugin(const LxQt::PluginInfo &desktopFile);
    void removePlugin();

    // slots for configuration dialog
    void onActivatedIndex(QModelIndex const & index);
    void onMovePluginUp();
    void onMovePluginDown();
    void onConfigurePlugin();
    void onRemovePlugin();

private:
    typedef QList<QPair <QString/*name*/, QPointer<Plugin> > > pluginslist_t;

private:
    void loadPlugins(QString const & namesKey, QStringList const & desktopDirs);
    QPointer<Plugin> loadPlugin(LxQt::PluginInfo const & desktopFile, QString const & settingsGroup);
    QString findNewPluginSettingsGroup(const QString &pluginType) const;
    bool isActiveIndexValid() const;
    void removePlugin(pluginslist_t::iterator plugin);

private:
    pluginslist_t mPlugins;
    LxQtPanel * mPanel;
    QPersistentModelIndex mActive;
};

#endif // PANELPLUGINSMODEL_H
