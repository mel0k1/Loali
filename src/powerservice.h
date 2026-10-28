#pragma once

#include <QObject>
#include <QQmlEngine>

class PowerService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool canSuspend READ canSuspend NOTIFY powerActionsChanged)
    Q_PROPERTY(bool canPowerOff READ canPowerOff NOTIFY powerActionsChanged)
    Q_PROPERTY(bool canReboot READ canReboot NOTIFY powerActionsChanged)
    Q_PROPERTY(bool logindAvailable READ logindAvailable NOTIFY powerActionsChanged)

public:
    static PowerService *instance();
    static void registerType();
    static void start();

    bool canSuspend() const { return m_canSuspend; }
    bool canPowerOff() const { return m_canPowerOff; }
    bool canReboot() const { return m_canReboot; }
    bool logindAvailable() const { return m_logind; }

    Q_INVOKABLE int inhibit(const QString &application, const QString &reason);
    Q_INVOKABLE void uninhibit(int cookie);
    Q_INVOKABLE void simulateUserActivity();
    Q_INVOKABLE quint64 idleTime() const;
    Q_INVOKABLE bool suspend();
    Q_INVOKABLE bool powerOff();
    Q_INVOKABLE bool reboot();
    Q_INVOKABLE void lock();
    Q_INVOKABLE void logout();

signals:
    void powerActionsChanged();
    void inhibited(int cookie, const QString &application, const QString &reason);
    void uninhibited(int cookie);
    void activitySimulated();
    void lockRequested();
    void logoutRequested();
    void actionFailed(const QString &action, const QString &message);

private:
    explicit PowerService(QObject *parent = nullptr);
    void probeLogind();
    bool callLogind(const QString &method);

    static PowerService *s_instance;
    uint m_nextCookie = 0;
    QHash<uint, QPair<QString, QString>> m_inhibits;
    bool m_canSuspend = false;
    bool m_canPowerOff = false;
    bool m_canReboot = false;
    bool m_logind = false;
};
