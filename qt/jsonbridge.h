#ifndef JSONBRIDGE_H
#define JSONBRIDGE_H

#include <QObject>
#include <QString>
#include <QVariantMap>
#include "qjsontreemodel.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/val.h>
#endif

class JsonBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool ready READ isReady NOTIFY readyChanged)
    Q_PROPERTY(QJsonTreeModel* treeModel READ treeModel CONSTANT)

public:
    explicit JsonBridge(QObject *parent = nullptr);

    bool isReady() const;
    QJsonTreeModel* treeModel() const;

    Q_INVOKABLE QVariantMap formatJson(const QString &input, const QString &indentType);
    Q_INVOKABLE QVariantMap minifyJson(const QString &input);
    Q_INVOKABLE QVariantMap validateJson(const QString &input);
    Q_INVOKABLE QString highlightJson(const QString &input);
    Q_INVOKABLE void copyToClipboard(const QString &text);
    Q_INVOKABLE QString readFromClipboard();
    Q_INVOKABLE bool loadTreeModel(const QString &json);

    // History methods
    Q_INVOKABLE void saveToHistory(const QString &json);
    Q_INVOKABLE QVariantList loadHistory();
    Q_INVOKABLE QString getHistoryEntry(const QString &id);
    Q_INVOKABLE void deleteHistoryEntry(const QString &id);
    Q_INVOKABLE void clearHistory();
    Q_INVOKABLE bool isHistoryAvailable();

signals:
    void historySaved(bool success, const QString &id);
    void historyLoaded(const QVariantList &entries);
    void historyEntryLoaded(const QString &content);
    void historyEntryDeleted(bool success);
    void historyCleared(bool success);
    void readyChanged();

private:
    bool m_ready = false;
    QJsonTreeModel* m_treeModel;
    void checkReady();
};

#endif // JSONBRIDGE_H
