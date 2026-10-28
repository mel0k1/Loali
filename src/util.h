#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QVariant>

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

    Q_INVOKABLE int pingClients(const QVariant &compositor, const QVariant &shell)
    {
        auto *comp = qvariant_cast<QWaylandCompositor *>(compositor);
        auto *xdg = qvariant_cast<QObject *>(shell);
        if (!comp || !xdg)
            return 0;
        int count = 0;
        const QList<QWaylandClient *> clients = comp->clients();
        for (QWaylandClient *client : clients) {
            if (!client)
                continue;
            const bool ok = QMetaObject::invokeMethod(xdg, "ping",
                                                      Q_ARG(QWaylandClient *, client));
            if (ok)
                ++count;
        }
        return count;
    }

private:
    explicit LoaliUtil(QObject *parent = nullptr)
        : QObject(parent)
    {
    }
};
