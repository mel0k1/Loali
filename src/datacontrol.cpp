#include "datacontrol.h"

#include <QTimer>
#include <QtWaylandCompositor/QWaylandCompositor>

#if __has_include(<QtWaylandCompositor/private/qwldatadevice_p.h>)
#define LOALI_QT_SELECTION_BRIDGE 1
#include <QGuiApplication>
#include <QList>
#include <QObject>
#include <QString>
#pragma push_macro("private")
#undef private
#define private public
#include <QtWaylandCompositor/private/qwldatadevice_p.h>
#include <QtWaylandCompositor/private/qwldatasource_p.h>
#include <QtWaylandCompositor/private/qwaylandseat_p.h>
#pragma pop_macro("private")
#endif

struct DataControlDevice::SelectionPayload
{
    QStringList mimes;
    std::function<void(const QString &, int)> recv;
};

DataControlManager *DataControlManager::s_instance = nullptr;
DataControlSource *DataControlManager::s_selection = nullptr;

namespace {

DataControlDevice::SelectionPayload currentPayload()
{
    if (DataControlManager::currentSelection())
        return {DataControlManager::currentSelection()->mimeTypes(),
                [](const QString &mime, int fd) {
                    DataControlManager::currentSelection()->sendTo(mime, fd);
                }};
#ifdef LOALI_QT_SELECTION_BRIDGE
    auto *compositor = qobject_cast<QWaylandCompositor *>(DataControlManager::instance()
                                                              ? DataControlManager::instance()
                                                                  ->extensionContainer()
                                                              : nullptr);
    if (compositor && compositor->defaultSeat()) {
        auto *dev = QWaylandSeatPrivate::get(compositor->defaultSeat())->dataDevice();
        auto *src = dev ? dev->m_selectionSource : nullptr;
        if (src)
            return {src->m_mimeTypes,
                    [src](const QString &mime, int fd) { src->send_send(mime, fd); }};
    }
#endif
    return {};
}

}

DataControlSource::DataControlSource(struct ::wl_client *client, uint32_t id, uint32_t version)
{
    init(client, id, int(version));
}

DataControlSource::~DataControlSource() = default;

DataControlSource *DataControlSource::fromResource(::wl_resource *resource)
{
    auto *res = Resource::fromResource(resource);
    return res ? static_cast<DataControlSource *>(res->object()) : nullptr;
}

void DataControlSource::sendTo(const QString &mime, int fd)
{
    send_send(mime, fd);
}

void DataControlSource::cancel()
{
    send_cancelled();
}

void DataControlSource::zwlr_data_control_source_v1_offer(Resource *, const QString &mime_type)
{
    if (!m_mimeTypes.contains(mime_type))
        m_mimeTypes.append(mime_type);
}

void DataControlSource::zwlr_data_control_source_v1_destroy(Resource *resource)
{
    wl_resource_destroy(resource->handle);
}

void DataControlSource::zwlr_data_control_source_v1_destroy_resource(Resource *resource)
{
    Q_UNUSED(resource)
    emit destroyed(this);
    deleteLater();
}

DataControlDevice::DataControlDevice(struct ::wl_client *client, uint32_t id, uint32_t version)
{
    init(client, id, int(version));
    if (DataControlManager::instance()) {
        DataControlManager::instance()->m_devices.append(this);
        updateSelection();
    }
}
DataControlDevice::~DataControlDevice() = default;

void DataControlDevice::sendSelection(const SelectionPayload &payload)
{
    if (payload.mimes.isEmpty()) {
        send_selection(nullptr);
        return;
    }
    auto *res = resource();
    if (!res)
        return;
    struct ::wl_resource *offerRes = wl_resource_create(
        res->client(), &zwlr_data_control_offer_v1_interface,
        uint32_t(qMin(res->version(), 2)), 0);
    if (!offerRes)
        return;
    auto *offer = new DataControlOffer(offerRes, payload.mimes);
    send_data_offer(offerRes);
    for (const QString &mime : payload.mimes)
        offer->send_offer(mime);
    send_selection(offerRes);
}

void DataControlDevice::updateSelection()
{
    sendSelection(currentPayload());
}

void DataControlDevice::zwlr_data_control_device_v1_set_selection(Resource *,
                                                                  ::wl_resource *source)
{
    auto *src = source ? DataControlSource::fromResource(source) : nullptr;
    DataControlManager::setCurrentSelection(src);
    if (DataControlManager::instance())
        DataControlManager::instance()->selectionChanged();
}

void DataControlDevice::zwlr_data_control_device_v1_set_primary_selection(Resource *,
                                                                          ::wl_resource *source)
{
    Q_UNUSED(source)
}

void DataControlDevice::zwlr_data_control_device_v1_destroy(Resource *resource)
{
    wl_resource_destroy(resource->handle);
}

void DataControlDevice::zwlr_data_control_device_v1_destroy_resource(Resource *resource)
{
    Q_UNUSED(resource)
    deleteLater();
}

DataControlOffer::DataControlOffer(struct ::wl_resource *offerRes, const QStringList &mimeTypes)
    : m_mimeTypes(mimeTypes)
{
    init(offerRes);
}

DataControlOffer::~DataControlOffer() = default;

void DataControlOffer::zwlr_data_control_offer_v1_receive(Resource *, const QString &mime_type,
                                                          int32_t fd)
{
    const auto payload = currentPayload();
    if (payload.recv && payload.mimes.contains(mime_type))
        payload.recv(mime_type, fd);
    else
        ::close(fd);
}

void DataControlOffer::zwlr_data_control_offer_v1_destroy(Resource *resource)
{
    wl_resource_destroy(resource->handle);
}

void DataControlOffer::zwlr_data_control_offer_v1_destroy_resource(Resource *resource)
{
    Q_UNUSED(resource)
    deleteLater();
}

DataControlManager::DataControlManager(QObject *parent)
    : QWaylandCompositorExtensionTemplate<DataControlManager>()
{
    Q_UNUSED(parent)
    s_instance = this;
}

DataControlManager::~DataControlManager()
{
    if (s_instance == this)
        s_instance = nullptr;
}

void DataControlManager::initialize()
{
    QWaylandCompositorExtensionTemplate<DataControlManager>::initialize();
    auto *compositor = qobject_cast<QWaylandCompositor *>(extensionContainer());
    if (!compositor)
        return;
    init(compositor->display(), 2);
#ifdef LOALI_QT_SELECTION_BRIDGE
    m_refresh = new QTimer(this);
    m_refresh->setInterval(1000);
    connect(m_refresh, &QTimer::timeout, this, [this] {
        auto *compositor = qobject_cast<QWaylandCompositor *>(extensionContainer());
        if (!compositor || !compositor->defaultSeat())
            return;
        auto *dev = QWaylandSeatPrivate::get(compositor->defaultSeat())->dataDevice();
        void *src = dev ? reinterpret_cast<void *>(dev->m_selectionSource) : nullptr;
        if (src != m_lastQtSource) {
            m_lastQtSource = src;
            if (!DataControlManager::currentSelection())
                selectionChanged();
        }
    });
    m_refresh->start();
#endif
    qInfo() << "DataControlManager: wlr_data_control_v1 ready";
}

void DataControlManager::setCurrentSelection(DataControlSource *source)
{
    s_selection = source;
}

void DataControlManager::selectionChanged()
{
    for (DataControlDevice *dev : std::as_const(m_devices))
        dev->updateSelection();
}

void DataControlManager::zwlr_data_control_manager_v1_get_data_device(Resource *resource,
                                                                      uint32_t id,
                                                                      ::wl_resource *seat)
{
    Q_UNUSED(seat)
    new DataControlDevice(resource->client(), id, wl_resource_get_version(resource->handle));
}

void DataControlManager::zwlr_data_control_manager_v1_create_data_source(Resource *resource,
                                                                         uint32_t id)
{
    new DataControlSource(resource->client(), id, wl_resource_get_version(resource->handle));
}

void DataControlManager::zwlr_data_control_manager_v1_destroy(Resource *resource)
{
    wl_resource_destroy(resource->handle);
}

#include "datacontrol.moc"
