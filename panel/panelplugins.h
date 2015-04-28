#if !defined(panel_panelplugins_h)
#define panel_panelplugins_h

#include <QAbstractListModel>
#include <memory>

namespace LxQt
{
    class PluginInfo;
    struct PluginData;
}

class LxQtPanel;
class Plugin;

class PanelPlugins : public QAbstractListModel
{
    Q_OBJECT
public:
    PanelPlugins(LxQtPanel * panel
            , QString const & namesKey
            , QStringList const & desktopDirs
            , QObject * parent = nullptr);
    ~PanelPlugins();

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
    void pluginAdded(LxQt::PluginData const & plugin);
    void pluginRemoved(LxQt::PluginData const & plugin);

public slots:
    void addPlugin(const LxQt::PluginInfo &desktopFile);
    void removePlugin();

private:
    void loadPlugins(QString const & namesKey, QStringList const & desktopDirs);
    QPointer<Plugin> loadPlugin(LxQt::PluginInfo const & desktopFile, QString const & settingsGroup);
    QString findNewPluginSettingsGroup(const QString &pluginType) const;

private:
    typedef QList<QPair <QString/*name*/, QPointer<Plugin> > > container_t;
    container_t mPlugins;
    LxQtPanel * mPanel;
};

#endif //panel_panelplugins_h
