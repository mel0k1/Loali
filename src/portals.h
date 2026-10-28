#pragma once

#include <QObject>
#include <QString>

class PortalsAdaptor;

class Portals : public QObject
{
    Q_OBJECT

public:
    static Portals *instance() { return s_instance; }
    static bool create();
    static void registerTypes();

private:
    explicit Portals(QObject *parent = nullptr);

    static Portals *s_instance;
};
