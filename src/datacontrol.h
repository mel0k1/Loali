#pragma once

#include <QList>
#include <QObject>
#include <QStringList>
#include <QTimer>

#include <QtWaylandCompositor/QWaylandCompositorExtensionTemplate>
#include <QtWaylandCompositor/QWaylandQuickExtension>
#include <QtWaylandCompositor/QWaylandResource>
#include <QtWaylandCompositor/QWaylandSeat>

#include "qwayland-server-wlr-data-control-unstable-v1.h"

class DataControlDevice;
class DataControlSource;
class DataControlManager;

class DataControlOffer : public QObject, public QtWaylandServer::zwlr_data_control_offer_v1
{
    Q_OBJECT
public:
    DataControlOffer(struct ::wl_resource *offerRes, const QStringList &mimeTypes);
    ~DataControlOffer() override;

    using QtWaylandServer::zwlr_data_control_offer_v1::interface;
    using QtWaylandServer::zwlr_data_control_offer_v1::interfaceName;

    QStringList mimeTypes() const { return m_mimeTypes; }

protected:
    void zwlr_data_control_offer_v1_receive(Resource *resource, const QString &mime_type,
                                            int32_t fd) override;
    void zwlr_data_control_offer_v1_destroy(Resource *resource) override;
    void zwlr_data_control_offer_v1_destroy_resource(Resource *resource) override;

private:
    QStringList m_mimeTypes;
};

class DataControlSource : public QObject, public QtWaylandServer::zwlr_data_control_source_v1
{
    Q_OBJECT
public:
    DataControlSource(struct ::wl_client *client, uint32_t id, uint32_t version);
    ~DataControlSource() override;

    using QtWaylandServer::zwlr_data_control_source_v1::interface;
    using QtWaylandServer::zwlr_data_control_source_v1::interfaceName;

    QStringList mimeTypes() const { return m_mimeTypes; }
    void sendTo(const QString &mime, int fd);
    void cancel();

    static DataControlSource *fromResource(::wl_resource *resource);

signals:
    void destroyed(DataControlSource *source);

protected:
    void zwlr_data_control_source_v1_offer(Resource *resource, const QString &mime_type) override;
    void zwlr_data_control_source_v1_destroy(Resource *resource) override;
    void zwlr_data_control_source_v1_destroy_resource(Resource *resource) override;

private:
    QStringList m_mimeTypes;
};

class DataControlDevice : public QObject, public QtWaylandServer::zwlr_data_control_device_v1
{
    Q_OBJECT
public:
    struct SelectionPayload;

    DataControlDevice(struct ::wl_client *client, uint32_t id, uint32_t version);
    ~DataControlDevice() override;

    using QtWaylandServer::zwlr_data_control_device_v1::interface;
    using QtWaylandServer::zwlr_data_control_device_v1::interfaceName;

    void updateSelection();

protected:
    void zwlr_data_control_device_v1_set_selection(Resource *resource, ::wl_resource *source) override;
    void zwlr_data_control_device_v1_set_primary_selection(Resource *resource,
                                                           ::wl_resource *source) override;
    void zwlr_data_control_device_v1_destroy(Resource *resource) override;
    void zwlr_data_control_device_v1_destroy_resource(Resource *resource) override;

private:
    void sendSelection(const SelectionPayload &payload);
};

class DataControlManager : public QWaylandCompositorExtensionTemplate<DataControlManager>,
                           public QtWaylandServer::zwlr_data_control_manager_v1
{
    Q_OBJECT
public:
    explicit DataControlManager(QObject *parent = nullptr);
    ~DataControlManager() override;
    void initialize() override;

    using QtWaylandServer::zwlr_data_control_manager_v1::interface;
    using QtWaylandServer::zwlr_data_control_manager_v1::interfaceName;

    static DataControlManager *instance() { return s_instance; }

    void selectionChanged();

    static DataControlSource *currentSelection() { return s_selection; }
    static void setCurrentSelection(DataControlSource *source);

protected:
    void zwlr_data_control_manager_v1_get_data_device(Resource *resource, uint32_t id,
                                                      ::wl_resource *seat) override;
    void zwlr_data_control_manager_v1_create_data_source(Resource *resource, uint32_t id) override;
    void zwlr_data_control_manager_v1_destroy(Resource *resource) override;

private:
    static DataControlManager *s_instance;
    static DataControlSource *s_selection;
    QList<DataControlDevice *> m_devices;
    QTimer *m_refresh = nullptr;
    void *m_lastQtSource = nullptr;

    friend class DataControlDevice;
};

Q_COMPOSITOR_DECLARE_QUICK_EXTENSION_CLASS(DataControlManager)
