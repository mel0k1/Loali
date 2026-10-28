#include "xwayland.h"

#include <QByteArray>
#include <QDebug>
#include <QMutex>
#include <QMetaObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QSocketNotifier>
#include <QHash>
#include <QStandardPaths>
#include <QThread>
#include <QTimer>
#include <QVariantList>
#include <QtWaylandCompositor/QWaylandCompositor>
#include <QtWaylandCompositor/QWaylandOutput>

#include <atomic>
#include <cerrno>
#include <cstdio>
#include <deque>
#include <functional>
#include <poll.h>
#include <cstring>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#include <xcb/composite.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>

class XWaylandHost : public QThread
{
    Q_OBJECT
public:
    explicit XWaylandHost(QObject *parent = nullptr)
        : QThread(parent)
    {
    }

    ~XWaylandHost() override
    {
        m_stop = true;
        wait(3000);
    }

    void begin(const QString &waylandSocket)
    {
        m_socket = waylandSocket;
        QThread::start();
    }

    void closeWindow(quint32 window)
    {
        job([this, window] { wmCloseWindow(window); });
    }

    void focusWindow(quint32 window)
    {
        job([this, window] { wmFocusWindow(window); });
    }

    void configureWindow(quint32 window, const QRect &frame)
    {
        job([this, window, frame] { wmConfigureWindow(window, frame); });
    }

signals:
    void windowMapped(quint32 window, const QRect &frame, bool overrideRedirect);
    void windowConfigured(quint32 window, const QRect &frame);
    void windowUnmapped(quint32 window);
    void windowDestroyed(quint32 window);
    void windowTitleChanged(quint32 window, const QString &title);
    void windowAppIdChanged(quint32 window, const QString &appId);
    void windowFocused(quint32 window);
    void windowSerialFound(quint32 window, quint64 serial);

protected:
    void run() override
    {
        if (!spawnServer())
            return;
        if (!connectWm())
            return;
        while (!m_stop) {
            runJobs();
            pumpEvents();
            if (m_xcb) {
                pollfd pfd;
                pfd.fd = xcb_get_file_descriptor(m_xcb);
                pfd.events = POLLIN;
                pfd.revents = 0;
                ::poll(&pfd, 1, 20);
            } else {
                QThread::msleep(20);
            }
        }
        if (m_xcb)
            xcb_disconnect(m_xcb);
    }

private:
    void job(std::function<void()> fn)
    {
        {
            QMutexLocker lock(&m_mutex);
            m_jobs.push_back(std::move(fn));
        }
    }

    void runJobs()
    {
        for (;;) {
            std::function<void()> fn;
            {
                QMutexLocker lock(&m_mutex);
                if (m_jobs.empty())
                    return;
                fn = std::move(m_jobs.front());
                m_jobs.pop_front();
            }
            fn();
        }
    }

    static bool socketInUse(int display, bool abstract)
    {
        int fd = ::socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
        if (fd < 0)
            return true;
        sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        socklen_t len = 0;
        if (abstract) {
            addr.sun_path[0] = 0;
            snprintf(addr.sun_path + 1, sizeof(addr.sun_path) - 1, "%d", display);
            len = offsetof(sockaddr_un, sun_path) + 1 + strlen(addr.sun_path + 1);
        } else {
            snprintf(addr.sun_path, sizeof(addr.sun_path), "/tmp/.X11-unix/X%d", display);
            len = offsetof(sockaddr_un, sun_path) + strlen(addr.sun_path);
        }
        bool used = ::connect(fd, reinterpret_cast<const sockaddr *>(&addr), len) == 0;
        ::close(fd);
        return used;
    }

    bool makeListener(int display, bool abstract)
    {
        int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd < 0)
            return false;
        sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        socklen_t len = 0;
        if (abstract) {
            addr.sun_path[0] = 0;
            snprintf(addr.sun_path + 1, sizeof(addr.sun_path) - 1, "%d", display);
            len = offsetof(sockaddr_un, sun_path) + 1 + strlen(addr.sun_path + 1);
        } else {
            snprintf(addr.sun_path, sizeof(addr.sun_path), "/tmp/.X11-unix/X%d", display);
            ::unlink(addr.sun_path);
            len = offsetof(sockaddr_un, sun_path) + strlen(addr.sun_path);
        }
        if (::bind(fd, reinterpret_cast<const sockaddr *>(&addr), len) < 0
            || ::listen(fd, 16) < 0) {
            ::close(fd);
            return false;
        }
        if (!abstract) {
            char path[64];
            snprintf(path, sizeof(path), "/tmp/.X11-unix/X%d", display);
            ::chmod(path, 0777);
        }
        if (abstract)
            m_fdAbstract = fd;
        else
            m_fdUnix = fd;
        return true;
    }

    bool claimDisplay()
    {
        for (int n = 1; n < 40; ++n) {
            if (socketInUse(n, true) || socketInUse(n, false))
                continue;
            if (!makeListener(n, false))
                continue;
            if (!makeListener(n, true)) {
                ::close(m_fdUnix);
                m_fdUnix = -1;
                continue;
            }
            m_display = n;
            return true;
        }
        qWarning() << "XWaylandHost: no free X display";
        return false;
    }

    bool spawnServer()
    {
        QString prog = QStandardPaths::findExecutable(QStringLiteral("Xwayland"));
        if (prog.isEmpty()) {
            qInfo() << "XWaylandHost: Xwayland binary not found, skipping";
            return false;
        }
        if (!claimDisplay())
            return false;

        auto *proc = new QProcess(nullptr);
        connect(proc, &QProcess::finished, proc, &QProcess::deleteLater);
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.insert(QStringLiteral("WAYLAND_DISPLAY"), m_socket);
        const QString runtime = qEnvironmentVariable("XDG_RUNTIME_DIR");
        if (!runtime.isEmpty())
            env.insert(QStringLiteral("XDG_RUNTIME_DIR"), runtime);
        if (qEnvironmentVariableIsSet("LOALI_XWAYLAND_DEBUG"))
            env.insert(QStringLiteral("WAYLAND_DEBUG"), QStringLiteral("1"));
        proc->setProcessEnvironment(env);
        proc->setProgram(prog);
        proc->setArguments({QStringLiteral(":%1").arg(m_display), QStringLiteral("-rootless"),
                            QStringLiteral("-listenfd"), QString::number(m_fdUnix),
                            QStringLiteral("-listenfd"), QString::number(m_fdAbstract),
                            QStringLiteral("-shm"), QStringLiteral("-noreset")});
        proc->setProcessChannelMode(QProcess::ForwardedErrorChannel);
        connect(proc, &QProcess::errorOccurred, this, [this](QProcess::ProcessError e) {
            qWarning() << "XWaylandHost: process error" << e;
        });
        proc->start();
        m_proc = proc;
        return true;
    }

    xcb_atom_t intern(const char *name)
    {
        auto cookie = xcb_intern_atom(m_xcb, false, uint16_t(strlen(name)), name);
        auto *reply = xcb_intern_atom_reply(m_xcb, cookie, nullptr);
        xcb_atom_t atom = reply ? reply->atom : XCB_ATOM_NONE;
        free(reply);
        return atom;
    }

    bool connectWm()
    {
        char name[16];
        snprintf(name, sizeof(name), ":%d", m_display);
        for (int attempt = 0; attempt < 60 && !m_stop; ++attempt) {
            m_xcb = xcb_connect(name, nullptr);
            if (m_xcb && !xcb_connection_has_error(m_xcb))
                break;
            if (m_xcb) {
                xcb_disconnect(m_xcb);
                m_xcb = nullptr;
            }
            QThread::msleep(100);
        }
        if (!m_xcb) {
            qWarning() << "XWaylandHost: cannot connect to X display" << name;
            return false;
        }
        auto *setup = xcb_get_setup(m_xcb);
        auto rootIter = xcb_setup_roots_iterator(setup);
        if (rootIter.rem == 0)
            return false;
        auto *screen = rootIter.data;
        m_root = screen->root;
        m_screen = QSize(int(screen->width_in_pixels), int(screen->height_in_pixels));

        m_wmProtocols = intern("WM_PROTOCOLS");
        m_wmDelete = intern("WM_DELETE_WINDOW");
        m_netWmName = intern("_NET_WM_NAME");
        m_wmName = intern("WM_NAME");
        m_utf8 = intern("UTF8_STRING");
        m_wmClass = intern("WM_CLASS");
        m_netActive = intern("_NET_ACTIVE_WINDOW");
        m_netSupported = intern("_NET_SUPPORTED");
        m_netCheck = intern("_NET_SUPPORTING_WM_CHECK");
        m_wlSerial = intern("WL_SURFACE_SERIAL");

        const uint32_t mask = XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY
                              | XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_PROPERTY_CHANGE
                              | XCB_EVENT_MASK_FOCUS_CHANGE;
        auto cookie = xcb_change_window_attributes_checked(m_xcb, m_root, XCB_CW_EVENT_MASK, &mask);
        if (auto *err = xcb_request_check(m_xcb, cookie)) {
            free(err);
            qWarning() << "XWaylandHost: another window manager is running";
            xcb_disconnect(m_xcb);
            m_xcb = nullptr;
            return false;
        }

        m_checkWin = xcb_generate_id(m_xcb);
        const uint32_t cmask = XCB_CW_OVERRIDE_REDIRECT | XCB_CW_EVENT_MASK;
        const uint32_t cvals[] = {1, 0};
        xcb_create_window(m_xcb, XCB_COPY_FROM_PARENT, m_checkWin, m_root, -1, -1, 1, 1, 0,
                          XCB_WINDOW_CLASS_INPUT_OUTPUT, XCB_COPY_FROM_PARENT, cmask, cvals);

        const char wmName[] = "Loali";
        xcb_change_property(m_xcb, XCB_PROP_MODE_REPLACE, m_checkWin, m_netCheck, XCB_ATOM_WINDOW,
                            32, 1, &m_checkWin);
        xcb_change_property(m_xcb, XCB_PROP_MODE_REPLACE, m_checkWin, m_netWmName, m_utf8, 8,
                            strlen(wmName), wmName);
        xcb_change_property(m_xcb, XCB_PROP_MODE_REPLACE, m_root, m_netCheck, XCB_ATOM_WINDOW, 32,
                            1, &m_checkWin);
        xcb_atom_t supported[] = {m_netActive, m_netSupported, m_netCheck, m_netWmName};
        xcb_change_property(m_xcb, XCB_PROP_MODE_REPLACE, m_root, m_netSupported, XCB_ATOM_ATOM, 32,
                            4, supported);
        xcb_composite_redirect_subwindows(m_xcb, m_root, XCB_COMPOSITE_REDIRECT_MANUAL);
        xcb_flush(m_xcb);
        qInfo() << "XWaylandHost: window manager ready on" << name << m_screen.width() << "x"
                << m_screen.height();
        return true;
    }

    void pumpEvents()
    {
        while (m_xcb) {
            auto *event = xcb_poll_for_event(m_xcb);
            if (!event)
                break;
            handleEvent(event);
            free(event);
        }
        if (m_xcb)
            xcb_flush(m_xcb);
    }

    QRect geometryOf(xcb_window_t window)
    {
        auto cookie = xcb_get_geometry(m_xcb, window);
        auto *reply = xcb_get_geometry_reply(m_xcb, cookie, nullptr);
        QRect rect;
        if (reply) {
            rect = QRect(0, 0, int(reply->width), int(reply->height));
            free(reply);
        }
        auto *trans = xcb_translate_coordinates_reply(
            m_xcb, xcb_translate_coordinates(m_xcb, window, m_root, 0, 0), nullptr);
        if (trans) {
            rect.moveTopLeft(QPoint(int(trans->dst_x), int(trans->dst_y)));
            free(trans);
        }
        return rect;
    }

    void placeWindow(xcb_window_t window)
    {
        QRect geom = geometryOf(window);
        if (geom.width() < 8 || geom.height() < 8) {
            geom.setWidth(640);
            geom.setHeight(480);
        }
        int k = m_cascade++ % 8;
        int x = 60 + k * 28;
        int y = 60 + k * 24;
        if (m_screen.width() > 0)
            x = qBound(0, x, m_screen.width() - qMin(geom.width(), m_screen.width()) - 4);
        if (m_screen.height() > 0)
            y = qBound(0, y, m_screen.height() - qMin(geom.height(), m_screen.height()) - 4);
        geom.moveTopLeft(QPoint(x, y));
        const uint32_t values[] = {uint32_t(x), uint32_t(y)};
        xcb_configure_window(m_xcb, window, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, values);
        m_frames.insert(window, geom);
        emit windowMapped(window, geom, false);
    }

    void readProps(xcb_window_t window)
    {
        if (m_netWmName != XCB_ATOM_NONE) {
            auto cookie = xcb_get_property(m_xcb, false, window, m_netWmName, XCB_ATOM_NONE, 0, 4096);
            if (auto *reply = xcb_get_property_reply(m_xcb, cookie, nullptr)) {
                if (reply->type == m_utf8 && reply->format == 8 && reply->value_len > 0)
                    emit windowTitleChanged(window, QString::fromUtf8(
                        static_cast<const char *>(xcb_get_property_value(reply)),
                        int(xcb_get_property_value_length(reply))));
                free(reply);
            }
        }
        if (m_wmName != XCB_ATOM_NONE) {
            auto cookie = xcb_get_property(m_xcb, false, window, m_wmName, XCB_ATOM_STRING, 0, 1024);
            if (auto *reply = xcb_get_property_reply(m_xcb, cookie, nullptr)) {
                if (reply->format == 8 && reply->value_len > 0)
                    emit windowTitleChanged(window, QString::fromLatin1(
                        static_cast<const char *>(xcb_get_property_value(reply)),
                        int(xcb_get_property_value_length(reply))));
                free(reply);
            }
        }
    if (m_wmClass != XCB_ATOM_NONE) {
            auto cookie = xcb_get_property(m_xcb, false, window, m_wmClass, XCB_ATOM_STRING, 0, 512);
            if (auto *reply = xcb_get_property_reply(m_xcb, cookie, nullptr)) {
                if (reply->format == 8 && reply->value_len > 1) {
                    const char *data = static_cast<const char *>(xcb_get_property_value(reply));
                    int len = int(xcb_get_property_value_length(reply));
                    int nul = -1;
                    for (int i = 0; i < len; ++i) {
                        if (data[i] == 0) {
                            nul = i;
                            break;
                        }
                    }
                    if (nul >= 0 && nul + 1 < len)
                        emit windowAppIdChanged(window, QString::fromLatin1(data + nul + 1));
                }
                free(reply);
            }
        }
    }

    bool supportsDelete(xcb_window_t window)
    {
        auto cookie = xcb_get_property(m_xcb, false, window, m_wmProtocols, XCB_ATOM_ATOM, 0, 128);
        bool ok = false;
        if (auto *reply = xcb_get_property_reply(m_xcb, cookie, nullptr)) {
            auto *atoms = static_cast<xcb_atom_t *>(xcb_get_property_value(reply));
            int count = int(reply->value_len) / int(sizeof(xcb_atom_t));
            for (int i = 0; i < count; ++i) {
                if (atoms[i] == m_wmDelete)
                    ok = true;
            }
            free(reply);
        }
        return ok;
    }

    void setActiveWindow(xcb_window_t window)
    {
        xcb_change_property(m_xcb, XCB_PROP_MODE_REPLACE, m_root, m_netActive, XCB_ATOM_WINDOW, 32,
                            1, &window);
    }

    void wmCloseWindow(xcb_window_t window)
    {
        if (!m_xcb)
            return;
        if (supportsDelete(window)) {
            xcb_client_message_event_t ev;
            memset(&ev, 0, sizeof(ev));
            ev.response_type = XCB_CLIENT_MESSAGE;
            ev.window = window;
            ev.type = m_wmProtocols;
            ev.format = 32;
            ev.data.data32[0] = m_wmDelete;
            ev.data.data32[1] = XCB_WINDOW_NONE;
            xcb_send_event(m_xcb, false, window, XCB_EVENT_MASK_NO_EVENT,
                           reinterpret_cast<const char *>(&ev));
        } else {
            xcb_destroy_window(m_xcb, window);
        }
        xcb_flush(m_xcb);
    }

    void wmFocusWindow(xcb_window_t window)
    {
        if (!m_xcb)
            return;
        const uint32_t stack = XCB_STACK_MODE_ABOVE;
        xcb_configure_window(m_xcb, window, XCB_CONFIG_WINDOW_STACK_MODE, &stack);
        xcb_set_input_focus(m_xcb, XCB_INPUT_FOCUS_POINTER_ROOT, window, XCB_CURRENT_TIME);
        setActiveWindow(window);
        xcb_flush(m_xcb);
    }

    void wmConfigureWindow(xcb_window_t window, const QRect &frame)
    {
        if (!m_xcb)
            return;
        const uint32_t values[] = {uint32_t(frame.x()), uint32_t(frame.y()),
                                   uint32_t(frame.width()), uint32_t(frame.height())};
        xcb_configure_window(m_xcb, window, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y
                                                | XCB_CONFIG_WINDOW_WIDTH
                                                | XCB_CONFIG_WINDOW_HEIGHT,
                             values);
        m_frames.insert(window, frame);
        emit windowConfigured(window, frame);
        xcb_flush(m_xcb);
    }

    void handleEvent(xcb_generic_event_t *event)
    {
        const int type = event->response_type & ~0x80;
        switch (type) {
        case XCB_MAP_REQUEST: {
            auto *ev = reinterpret_cast<xcb_map_request_event_t *>(event);
            placeWindow(ev->window);
            xcb_map_window(m_xcb, ev->window);
            break;
        }
        case XCB_CONFIGURE_REQUEST: {
            auto *ev = reinterpret_cast<xcb_configure_request_event_t *>(event);
            uint32_t values[7];
            int n = 0;
            uint16_t mask = 0;
            QRect frame = m_frames.value(ev->window, geometryOf(ev->window));
            if (ev->value_mask & XCB_CONFIG_WINDOW_X) {
                mask |= XCB_CONFIG_WINDOW_X;
                values[n++] = uint32_t(int16_t(ev->x));
                frame.moveLeft(int16_t(ev->x));
            }
            if (ev->value_mask & XCB_CONFIG_WINDOW_Y) {
                mask |= XCB_CONFIG_WINDOW_Y;
                values[n++] = uint32_t(int16_t(ev->y));
                frame.moveTop(int16_t(ev->y));
            }
            if (ev->value_mask & XCB_CONFIG_WINDOW_WIDTH) {
                mask |= XCB_CONFIG_WINDOW_WIDTH;
                values[n++] = ev->width;
                frame.setWidth(int(ev->width));
            }
            if (ev->value_mask & XCB_CONFIG_WINDOW_HEIGHT) {
                mask |= XCB_CONFIG_WINDOW_HEIGHT;
                values[n++] = ev->height;
                frame.setHeight(int(ev->height));
            }
            if (mask != 0)
                xcb_configure_window(m_xcb, ev->window, mask, values);
            xcb_configure_window(m_xcb, ev->window, XCB_CONFIG_WINDOW_STACK_MODE, &m_stackAbove);
            if (mask & (XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT)) {
                m_frames.insert(ev->window, frame);
                emit windowConfigured(ev->window, frame);
            }
            break;
        }
        case XCB_CONFIGURE_NOTIFY: {
            auto *ev = reinterpret_cast<xcb_configure_notify_event_t *>(event);
            if (ev->window == m_root) {
                m_screen = QSize(int(ev->width), int(ev->height));
            } else if (!ev->override_redirect) {
                QRect frame(int(ev->x), int(ev->y), int(ev->width), int(ev->height));
                m_frames.insert(ev->window, frame);
                emit windowConfigured(ev->window, frame);
            } else {
                QRect frame(int(ev->x), int(ev->y), int(ev->width), int(ev->height));
                m_frames.insert(ev->window, frame);
                emit windowMapped(ev->window, frame, true);
            }
            break;
        }
        case XCB_UNMAP_NOTIFY: {
            auto *ev = reinterpret_cast<xcb_unmap_notify_event_t *>(event);
            emit windowUnmapped(ev->window);
            break;
        }
        case XCB_DESTROY_NOTIFY: {
            auto *ev = reinterpret_cast<xcb_destroy_notify_event_t *>(event);
            m_frames.remove(ev->window);
            emit windowDestroyed(ev->window);
            break;
        }
        case XCB_CLIENT_MESSAGE: {
            auto *ev = reinterpret_cast<xcb_client_message_event_t *>(event);
            if (m_wlSerial != XCB_ATOM_NONE && ev->type == m_wlSerial)
                emit windowSerialFound(ev->window,
                                       quint64(ev->data.data32[0])
                                           | (quint64(ev->data.data32[1]) << 32));
            break;
        }
        case XCB_FOCUS_IN: {
            auto *ev = reinterpret_cast<xcb_focus_in_event_t *>(event);
            setActiveWindow(ev->event);
            emit windowFocused(ev->event);
            break;
        }
        default:
            break;
        }
    }

    QMutex m_mutex;
    std::deque<std::function<void()>> m_jobs;
    std::atomic<bool> m_stop{false};
    QString m_socket;
    QProcess *m_proc = nullptr;
    int m_display = -1;
    int m_fdUnix = -1;
    int m_fdAbstract = -1;
    xcb_connection_t *m_xcb = nullptr;
    xcb_window_t m_root = XCB_WINDOW_NONE;
    xcb_window_t m_checkWin = XCB_WINDOW_NONE;
    QSize m_screen;
    int m_cascade = 0;
    uint32_t m_stackAbove = XCB_STACK_MODE_ABOVE;
    xcb_atom_t m_wmProtocols = XCB_ATOM_NONE;
    xcb_atom_t m_wmDelete = XCB_ATOM_NONE;
    xcb_atom_t m_netWmName = XCB_ATOM_NONE;
    xcb_atom_t m_wmName = XCB_ATOM_NONE;
    xcb_atom_t m_utf8 = XCB_ATOM_NONE;
    xcb_atom_t m_wmClass = XCB_ATOM_NONE;
    xcb_atom_t m_netActive = XCB_ATOM_NONE;
    xcb_atom_t m_netSupported = XCB_ATOM_NONE;
    xcb_atom_t m_netCheck = XCB_ATOM_NONE;
    xcb_atom_t m_wlSerial = XCB_ATOM_NONE;
    QHash<quint32, QRect> m_frames;
};

#include "xwayland.moc"


XWaylandIntegration::XWaylandIntegration(QWaylandQuickShellSurfaceItem *item)
    : QWaylandQuickShellIntegration(item)
    , m_item(item)
{
    if (auto *surf = qobject_cast<XWaylandSurface *>(m_item->shellSurface()))
        m_item->setSurface(surf->surface());
}

QWaylandSurfaceRole XWaylandSurface::s_role_impl("xwayland_surface_v1");

QWaylandSurfaceRole *XWaylandSurface::role()
{
    return &s_role_impl;
}

XWaylandSurface::XWaylandSurface(XWaylandShell *shell, QWaylandSurface *surface,
                                 const QWaylandResource &resource)
    : m_shell(shell)
    , m_surface(surface)
{
    init(resource.resource());
    setExtensionContainer(surface);
    QWaylandCompositorExtension::initialize();
}

XWaylandSurface::~XWaylandSurface()
{
    if (m_shell && m_window != 0)
        m_shell->unbindWindow(m_window, this);
}

XWaylandSurface *XWaylandSurface::fromResource(::wl_resource *resource)
{
    auto *res = Resource::fromResource(resource);
    return res ? static_cast<XWaylandSurface *>(res->object()) : nullptr;
}

QWaylandQuickShellIntegration *XWaylandSurface::createIntegration(QWaylandQuickShellSurfaceItem *item)
{
    return new XWaylandIntegration(item);
}

void XWaylandSurface::setActive(bool value)
{
    if (m_active == value)
        return;
    m_active = value;
    emit activeChanged();
}

void XWaylandSurface::xwayland_surface_v1_set_serial(Resource *, uint32_t serial_lo, uint32_t serial_hi)
{
    m_serial = (uint64_t(serial_hi) << 32) | serial_lo;
    if (m_shell)
        m_shell->bindSerial(m_serial, this);
}

void XWaylandSurface::xwayland_surface_v1_destroy(Resource *resource)
{
    wl_resource_destroy(resource->handle);
}

void XWaylandSurface::xwayland_surface_v1_destroy_resource(Resource *resource)
{
    Q_UNUSED(resource)
    emit windowClosed();
    deleteLater();
}

void XWaylandSurface::wmSetWindow(quint32 window)
{
    m_window = window;
}

void XWaylandSurface::wmMapped(const QRect &frame, bool decorated)
{
    m_decorated = decorated;
    m_frame = frame;
    m_mapped = true;
    emit frameChanged();
}

void XWaylandSurface::wmSetDecorated(bool value)
{
    m_decorated = value;
}

void XWaylandSurface::wmSetFrame(const QRect &frame)
{
    m_frame = frame;
    emit frameChanged();
}

void XWaylandSurface::wmSetTitle(const QString &title)
{
    if (m_title == title)
        return;
    m_title = title;
    emit titleChanged();
}

void XWaylandSurface::wmSetAppId(const QString &appId)
{
    if (m_appId == appId)
        return;
    m_appId = appId;
    emit appIdChanged();
}

void XWaylandSurface::wmUnmapped()
{
    m_mapped = false;
    emit frameChanged();
}

void XWaylandSurface::wmDestroyed()
{
    m_mapped = false;
    emit windowClosed();
}

void XWaylandSurface::sendClose()
{
    if (m_shell && m_window != 0)
        m_shell->closeWindow(m_window);
}

void XWaylandSurface::sendConfigure(const QSize &size, const QVariantList &states)
{
    Q_UNUSED(size)
    Q_UNUSED(states)
}

void XWaylandSurface::sendMaximized(const QSize &size)
{
    Q_UNUSED(size)
}

void XWaylandSurface::sendUnmaximized()
{
}

void XWaylandSurface::sendResizing(const QSize &size)
{
    Q_UNUSED(size)
}

QSize XWaylandSurface::sizeForResize(const QSize &size, const QPointF &delta, uint32_t edges) const
{
    QSize result = size;
    QPointF d = delta;
    if (edges & Qt::LeftEdge)
        d.setX(-d.x());
    if (edges & Qt::TopEdge)
        d.setY(-d.y());
    result.setWidth(qMax(120, int(size.width() + d.x())));
    result.setHeight(qMax(80, int(size.height() + d.y())));
    return result;
}

XWaylandShell::XWaylandShell(QObject *parent)
    : QWaylandCompositorExtensionTemplate<XWaylandShell>()
{
    Q_UNUSED(parent)
}

XWaylandShell::~XWaylandShell()
{
    if (m_host) {
        m_host->deleteLater();
        m_host = nullptr;
    }
}

void XWaylandShell::initialize()
{
    QWaylandCompositorExtensionTemplate<XWaylandShell>::initialize();
    auto *compositor = qobject_cast<QWaylandCompositor *>(extensionContainer());
    if (!compositor)
        return;
    init(compositor->display(), 1);
    m_compositor = compositor;
    startHost();
    qInfo() << "XWaylandShell: xwayland_shell_v1 ready";
}

bool XWaylandShell::available() const
{
    return m_host != nullptr;
}

void XWaylandShell::startHost()
{
    if (qEnvironmentVariable("LOALI_XWAYLAND") == "0")
        return;
    auto *compositor = m_compositor.data();
    if (!compositor)
        return;
    QString socket = compositor->socketName();
    if (socket.isEmpty())
        socket = QStringLiteral("wayland-0");
    m_socket = socket;
    m_host = new XWaylandHost(this);
    connect(m_host, &XWaylandHost::windowMapped, this, &XWaylandShell::handleMapped);
    connect(m_host, &XWaylandHost::windowConfigured, this, &XWaylandShell::handleConfigured);
    connect(m_host, &XWaylandHost::windowUnmapped, this, &XWaylandShell::handleUnmapped);
    connect(m_host, &XWaylandHost::windowDestroyed, this, &XWaylandShell::handleDestroyed);
    connect(m_host, &XWaylandHost::windowTitleChanged, this, &XWaylandShell::handleTitle);
    connect(m_host, &XWaylandHost::windowAppIdChanged, this, &XWaylandShell::handleAppId);
    connect(m_host, &XWaylandHost::windowFocused, this, &XWaylandShell::handleFocused);
    connect(m_host, &XWaylandHost::windowSerialFound, this, &XWaylandShell::handleSerial);
    QTimer::singleShot(400, this, [this] {
        m_host->begin(m_socket);
    });
}

void XWaylandShell::bindWindow(quint32 window, XWaylandSurface *surface)
{
    m_byWindow.insert(window, surface);
    applyState(surface, window);
}

void XWaylandShell::unbindWindow(quint32 window, XWaylandSurface *surface)
{
    if (m_byWindow.value(window) == surface)
        m_byWindow.remove(window);
}

void XWaylandShell::bindSerial(quint64 serial, XWaylandSurface *surface)
{
    m_bySerial.insert(serial, surface);
    const auto winIt = m_windowBySerial.constFind(serial);
    if (winIt != m_windowBySerial.constEnd()) {
        surface->wmSetWindow(winIt.value());
        bindWindow(winIt.value(), surface);
    }
}

void XWaylandShell::applyState(XWaylandSurface *surf, quint32 window)
{
    const WinState st = m_state.value(window);
    if (!st.valid)
        return;
    if (!st.title.isEmpty())
        surf->wmSetTitle(st.title);
    if (!st.appId.isEmpty())
        surf->wmSetAppId(st.appId);
    if (st.mapped)
        surf->wmMapped(st.frame, st.decorated);
    else
        surf->wmUnmapped();
}

XWaylandSurface *XWaylandShell::surfaceForWindow(quint32 window) const
{
    return m_byWindow.value(window, nullptr);
}

void XWaylandShell::xwayland_shell_v1_get_xwayland_surface(Resource *resource, uint32_t id,
                                                           ::wl_resource *surface)
{
    QWaylandSurface *qs = QWaylandSurface::fromResource(surface);
    if (!qs)
        return;
    if (!qs->setRole(XWaylandSurface::role(), resource->handle, 1))
        return;
    QWaylandResource surfResource(wl_resource_create(resource->client(),
                                                     &::xwayland_surface_v1_interface,
                                                     wl_resource_get_version(resource->handle), id));
    if (!surfResource.resource())
        return;
    auto *surf = new XWaylandSurface(this, qs, surfResource);
    m_surfaces.append(surf);
    connect(surf, &QObject::destroyed, this, [this, surf]() { m_surfaces.removeAll(surf); });
    emit xwaylandSurfaceCreated(surf);
}

void XWaylandShell::handleSerial(quint32 window, quint64 serial)
{
    if (auto *surf = m_bySerial.value(serial, nullptr)) {
        surf->wmSetWindow(window);
        bindWindow(window, surf);
    }
}

void XWaylandShell::handleMapped(quint32 window, const QRect &frame, bool overrideRedirect)
{
    WinState &st = m_state[window];
    st.frame = frame;
    st.mapped = true;
    st.decorated = !overrideRedirect;
    st.valid = true;
    auto *surf = surfaceForWindow(window);
    if (surf)
        surf->wmMapped(frame, !overrideRedirect);
}

void XWaylandShell::handleConfigured(quint32 window, const QRect &frame)
{
    if (m_state.contains(window)) {
        m_state[window].frame = frame;
        m_state[window].valid = true;
    }
    if (auto *surf = surfaceForWindow(window))
        surf->wmSetFrame(frame);
}

void XWaylandShell::handleUnmapped(quint32 window)
{
    if (m_state.contains(window))
        m_state[window].mapped = false;
    if (auto *surf = surfaceForWindow(window))
        surf->wmUnmapped();
}

void XWaylandShell::handleDestroyed(quint32 window)
{
    if (auto *surf = surfaceForWindow(window)) {
        m_byWindow.remove(window);
        m_state.remove(window);
        surf->wmDestroyed();
    } else {
        m_state.remove(window);
    }
}

void XWaylandShell::handleTitle(quint32 window, const QString &title)
{
    if (m_state.contains(window)) {
        m_state[window].title = title;
        m_state[window].valid = true;
    }
    if (auto *surf = surfaceForWindow(window))
        surf->wmSetTitle(title);
}

void XWaylandShell::handleAppId(quint32 window, const QString &appId)
{
    if (m_state.contains(window)) {
        m_state[window].appId = appId;
        m_state[window].valid = true;
    }
    if (auto *surf = surfaceForWindow(window))
        surf->wmSetAppId(appId);
}

void XWaylandShell::handleFocused(quint32 window)
{
    for (XWaylandSurface *surf : std::as_const(m_surfaces))
        surf->setActive(surf->windowId() == window && window != 0);
}

void XWaylandShell::closeWindow(quint32 window)
{
    if (m_host)
        m_host->closeWindow(window);
}

void XWaylandShell::focusWindow(quint32 window)
{
    if (m_host)
        m_host->focusWindow(window);
}

void XWaylandShell::configureWindow(quint32 window, const QRect &frame)
{
    if (m_host)
        m_host->configureWindow(window, frame);
}
