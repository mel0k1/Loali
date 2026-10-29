#pragma once

#include <QDateTime>
#include <QHash>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QQmlEngine>
#include <QTimer>
#include <QVariant>

#include <wayland-server-core.h>

#include <QtWaylandCompositor/QWaylandClient>
#include <QtWaylandCompositor/QWaylandCompositor>
#include <QtWaylandCompositor/QWaylandSeat>
#include <QtWaylandCompositor/QWaylandSurface>

class LoaliUtil : public QObject
{
    Q_OBJECT
public:
    static QObject *create(QQmlEngine *qml, QJSEngine *js)
    {
        Q_UNUSED(js)
        static LoaliUtil *inst = nullptr;
        if (!inst)
            inst = new LoaliUtil();
        if (qml)
            qml->setObjectOwnership(inst, QQmlEngine::CppOwnership);
        return inst;
    }

    Q_INVOKABLE void focus(const QVariant &seat, const QVariant &surface)
    {
        auto *s = qvariant_cast<QWaylandSeat *>(seat);
        auto *f = qvariant_cast<QWaylandSurface *>(surface);
        if (s) {
            const bool ok = s->setKeyboardFocus(f);
            if (!ok)
                qWarning() << "loali: setKeyboardFocus failed for" << f;
        }
    }

    Q_INVOKABLE void watchClient(const QVariant &client)
    {
        auto *c = qvariant_cast<QWaylandClient *>(client);
        if (!c)
            return;
        connect(c, &QObject::destroyed, this, [this, c]() {
            m_watch.removeAll(QPointer<QWaylandClient>(c));
            for (auto it = m_pending.begin(); it != m_pending.end();) {
                if (it.value().client == c)
                    it = m_pending.erase(it);
                else
                    ++it;
            }
        });
        if (!m_pingTimer.isActive()) {
            connect(&m_pingTimer, &QTimer::timeout, this, &LoaliUtil::pingTick);
            connect(&m_sweepTimer, &QTimer::timeout, this, &LoaliUtil::sweepTick);
            m_pingTimer.start(PingIntervalMs);
            m_sweepTimer.start(SweepIntervalMs);
        }
        m_watch.append(c);
    }

    Q_INVOKABLE int pingClients(const QVariant &compositor, const QVariant &shell)
    {
        auto *comp = qvariant_cast<QWaylandCompositor *>(compositor);
        auto *xdg = qvariant_cast<QObject *>(shell);
        if (!comp || !xdg)
            return 0;
        m_compositor = comp;
        m_shell = xdg;
        if (!m_pongHooked) {
            m_pongHooked = true;
            QObject::connect(xdg, SIGNAL(pong(uint)), this, SLOT(pongArrived(uint)));
        }
        return sendPings();
    }

private slots:
    void pongArrived(uint serial)
    {
        m_pending.remove(serial);
    }

    void pingTick()
    {
        sendPings();
    }

    void sweepTick()
    {
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        for (auto it = m_pending.begin(); it != m_pending.end();) {
            if (!it.value().client) {
                it = m_pending.erase(it);
                continue;
            }
            if (now - it.value().sentAt < PongTimeoutMs) {
                ++it;
                continue;
            }
            QWaylandClient *c = it.value().client;
            ::wl_client *wl = c->client();
            it = m_pending.erase(it);
            m_watch.removeAll(QPointer<QWaylandClient>(c));
            if (wl) {
                qInfo() << "loali: killing unresponsive client" << wl << "(no pong for" << PongTimeoutMs << "ms)";
                wl_client_destroy(wl);
            }
        }
    }

private:
    struct PendingPing
    {
        QPointer<QWaylandClient> client;
        qint64 sentAt = 0;
    };

    int sendPings()
    {
        for (auto it = m_watch.begin(); it != m_watch.end();) {
            if (!(*it))
                it = m_watch.erase(it);
            else
                ++it;
        }
        if (!m_compositor || !m_shell)
            return 0;
        int count = 0;
        const QList<QWaylandClient *> clients = m_compositor->clients();
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        for (QWaylandClient *client : clients) {
            if (!client || !m_watch.contains(QPointer<QWaylandClient>(client)))
                continue;
            uint serial = 0;
            const bool ok = QMetaObject::invokeMethod(m_shell, "ping",
                                                      Q_RETURN_ARG(uint, serial),
                                                      Q_ARG(QWaylandClient *, client));
            qInfo() << "loali: watchdog ping ok" << ok << "serial" << serial << "client" << client;
            if (ok) {
                m_pending.insert(serial, { client, now });
                ++count;
            }
        }
        return count;
    }

    static constexpr int PingIntervalMs = 10000;
    static constexpr int PongTimeoutMs = 20000;
    static constexpr int SweepIntervalMs = 5000;

    QPointer<QWaylandCompositor> m_compositor;
    QPointer<QObject> m_shell;
    QList<QPointer<QWaylandClient>> m_watch;
    QHash<uint, PendingPing> m_pending;
    QTimer m_pingTimer;
    QTimer m_sweepTimer;
    bool m_pongHooked = false;
};
