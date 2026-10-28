#pragma once

#include <QMap>
#include <QObject>
#include <QPointer>
#include <QRect>
#include <QSize>
#include <QString>

#include <QtWaylandCompositor/QWaylandCompositorExtensionTemplate>
#include <QtWaylandCompositor/QWaylandQuickExtension>
#include <QtWaylandCompositor/QWaylandQuickShellIntegration>
#include <QtWaylandCompositor/QWaylandQuickShellSurfaceItem>
#include <QtWaylandCompositor/QWaylandResource>
#include <QtWaylandCompositor/QWaylandShellSurface>
#include <QtWaylandCompositor/QWaylandSurface>
#include <QtWaylandCompositor/QWaylandSurfaceRole>

#include "qwayland-server-xwayland-shell-v1.h"

class QWaylandCompositor;
class XWaylandHost;
class XWaylandShell;
class XWaylandSurface;

class XWaylandIntegration : public QWaylandQuickShellIntegration
{
public:
    explicit XWaylandIntegration(QWaylandQuickShellSurfaceItem *item);

private:
    QWaylandQuickShellSurfaceItem *m_item = nullptr;
};

class XWaylandSurface : public QWaylandShellSurfaceTemplate<XWaylandSurface>,
                        public QtWaylandServer::xwayland_surface_v1
{
    Q_OBJECT
    Q_PROPERTY(QString title READ title NOTIFY titleChanged)
    Q_PROPERTY(QString appId READ appId NOTIFY appIdChanged)
    Q_PROPERTY(bool mapped READ mapped NOTIFY frameChanged)
    Q_PROPERTY(bool decorated READ decorated CONSTANT)
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(QRect frame READ frame NOTIFY frameChanged)

public:
    XWaylandSurface(XWaylandShell *shell, QWaylandSurface *surface, const QWaylandResource &resource);
    ~XWaylandSurface() override;

    using QtWaylandServer::xwayland_surface_v1::interface;
    using QtWaylandServer::xwayland_surface_v1::interfaceName;

    static QWaylandSurfaceRole *role();
    static XWaylandSurface *fromResource(::wl_resource *resource);

    QWaylandQuickShellIntegration *createIntegration(QWaylandQuickShellSurfaceItem *item) override;

    QWaylandSurface *surface() const { return m_surface.data(); }
    QString title() const { return m_title; }
    QString appId() const { return m_appId; }
    bool mapped() const { return m_mapped; }
    bool decorated() const { return m_decorated; }
    bool active() const { return m_active; }
    void setActive(bool value);
    QRect frame() const { return m_frame; }

    Q_INVOKABLE void sendClose();
    Q_INVOKABLE void sendConfigure(const QSize &size, const QVariantList &states);
    Q_INVOKABLE void sendMaximized(const QSize &size);
    Q_INVOKABLE void sendUnmaximized();
    Q_INVOKABLE void sendResizing(const QSize &size);
    Q_INVOKABLE QSize sizeForResize(const QSize &size, const QPointF &delta, uint32_t edges) const;

    void wmSetWindow(quint32 window);
    void wmSetDecorated(bool value);
    void wmSetFrame(const QRect &frame);
    void wmMapped(const QRect &frame, bool decorated);
    void wmSetTitle(const QString &title);
    void wmSetAppId(const QString &appId);
    void wmUnmapped();
    void wmDestroyed();
    quint32 windowId() const { return m_window; }
    quint64 serial() const { return m_serial; }

signals:
    void titleChanged();
    void appIdChanged();
    void frameChanged();
    void activeChanged();
    void windowClosed();

protected:
    void xwayland_surface_v1_set_serial(Resource *resource, uint32_t serial_lo,
                                        uint32_t serial_hi) override;
    void xwayland_surface_v1_destroy(Resource *resource) override;
    void xwayland_surface_v1_destroy_resource(Resource *resource) override;

private:
    static QWaylandSurfaceRole s_role_impl;

    QPointer<XWaylandShell> m_shell;
    QPointer<QWaylandSurface> m_surface;
    QString m_title;
    QString m_appId;
    QRect m_frame;
    quint32 m_window = 0;
    quint64 m_serial = 0;
    bool m_mapped = false;
    bool m_decorated = true;
    bool m_active = false;
};

class XWaylandShell : public QWaylandCompositorExtensionTemplate<XWaylandShell>,
                      public QtWaylandServer::xwayland_shell_v1
{
    Q_OBJECT
public:
    explicit XWaylandShell(QObject *parent = nullptr);
    ~XWaylandShell() override;
    void initialize() override;

    using QtWaylandServer::xwayland_shell_v1::interface;
    using QtWaylandServer::xwayland_shell_v1::interfaceName;

    Q_INVOKABLE void closeWindow(quint32 window);
    Q_INVOKABLE void focusWindow(quint32 window);
    Q_INVOKABLE void configureWindow(quint32 window, const QRect &frame);
    Q_INVOKABLE bool available() const;

    XWaylandHost *host() const { return m_host; }

    void bindWindow(quint32 window, XWaylandSurface *surface);
    void unbindWindow(quint32 window, XWaylandSurface *surface);
    void bindSerial(quint64 serial, XWaylandSurface *surface);
    XWaylandSurface *surfaceForWindow(quint32 window) const;

signals:
    void xwaylandSurfaceCreated(XWaylandSurface *surface);

protected:
    void xwayland_shell_v1_get_xwayland_surface(Resource *resource, uint32_t id,
                                                ::wl_resource *surface) override;

private:
    struct WinState
    {
        QRect frame;
        bool mapped = false;
        bool decorated = true;
        bool valid = false;
        QString title;
        QString appId;
    };

    void startHost();
    void applyState(XWaylandSurface *surf, quint32 window);
    void handleSerial(quint32 window, quint64 serial);
    void handleMapped(quint32 window, const QRect &frame, bool overrideRedirect);
    void handleConfigured(quint32 window, const QRect &frame);
    void handleUnmapped(quint32 window);
    void handleDestroyed(quint32 window);
    void handleTitle(quint32 window, const QString &title);
    void handleAppId(quint32 window, const QString &appId);
    void handleFocused(quint32 window);

    QPointer<QWaylandCompositor> m_compositor;
    XWaylandHost *m_host = nullptr;
    QString m_socket;
    QMap<quint32, XWaylandSurface *> m_byWindow;
    QMap<quint64, XWaylandSurface *> m_bySerial;
    QMap<quint64, quint32> m_windowBySerial;
    QMap<quint32, WinState> m_state;
    QList<XWaylandSurface *> m_surfaces;
};

Q_COMPOSITOR_DECLARE_QUICK_EXTENSION_CLASS(XWaylandShell)
