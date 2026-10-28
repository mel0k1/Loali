#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QVariant>

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

private:
    explicit LoaliUtil(QObject *parent = nullptr)
        : QObject(parent)
    {
    }
};
