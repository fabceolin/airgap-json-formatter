#ifndef JSONBRIDGE_H
#define JSONBRIDGE_H

#include <QObject>
#include <QString>
#include <QVariantMap>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/val.h>
#endif

class JsonBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool ready READ isReady NOTIFY readyChanged)

public:
    explicit JsonBridge(QObject *parent = nullptr);

    bool isReady() const;

    Q_INVOKABLE QVariantMap formatJson(const QString &input, const QString &indentType);
    Q_INVOKABLE QVariantMap minifyJson(const QString &input);
    Q_INVOKABLE QVariantMap validateJson(const QString &input);
    Q_INVOKABLE void copyToClipboard(const QString &text);
    Q_INVOKABLE QString readFromClipboard();

signals:
    void readyChanged();

private:
    bool m_ready = false;
    void checkReady();
};

#endif // JSONBRIDGE_H
