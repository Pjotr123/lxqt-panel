#if !defined(panel_panelplugins_h)
#define panel_panelplugins_h

#include <QAbstractListModel>
#include <memory>

class Plugin;

class PanelPlugins : public QAbstractListModel
{
    Q_OBJECT
public:
    using QAbstractListModel::QAbstractListModel;
    ~PanelPlugins();

    virtual int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
    virtual Qt::ItemFlags flags(const QModelIndex & index) const override;

    QStringList const & pluginNames() const;
    void setPluginNames(QStringList && names);

    //TODO: B move loading plugins into model!?!
    Plugin * addPlugin(std::unique_ptr<Plugin> plugin);
    void addPluginName(QString const & name);
    //TODO: E move loading plugins into model!?!
    void removePlugin(Plugin * plugin);
    /*!
     * \param plugin plugin that has been moved
     * \param nameBefore name of plugin that is right before of moved plugin
     */
    void movePlugin(Plugin const * plugin, QString const & nameBefore);

private:
    QList<Plugin*> mPlugins;
    QStringList mConfigOrder;
};

#endif //panel_panelplugins_h
