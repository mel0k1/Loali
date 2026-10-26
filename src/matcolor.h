#pragma once

#include <QColor>
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <qqml.h>

class MatColor : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QVariantMap dark READ dark NOTIFY paletteChanged)
    Q_PROPERTY(QVariantMap light READ light NOTIFY paletteChanged)
    Q_PROPERTY(QVariantList suggested READ suggested NOTIFY paletteChanged)
    Q_PROPERTY(QString source READ source NOTIFY paletteChanged)

public:
    static MatColor *create(QQmlEngine *engine, QJSEngine *jsEngine);
    static MatColor *instance() { return s_instance; }

    QVariantMap dark() const { return m_dark; }
    QVariantMap light() const { return m_light; }
    QVariantList suggested() const { return m_suggested; }
    QString source() const { return m_source; }

    Q_INVOKABLE void generateFromImage(const QString &source);
    Q_INVOKABLE void generateFromColor(const QColor &color);

signals:
    void paletteChanged();

private:
    explicit MatColor(QObject *parent = nullptr);

    void apply(uint32_t argb);

    static MatColor *s_instance;
    QVariantMap m_dark;
    QVariantMap m_light;
    QVariantList m_suggested;
    QString m_source;
};
