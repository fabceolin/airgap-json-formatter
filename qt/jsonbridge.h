#ifndef JSONBRIDGE_H
#define JSONBRIDGE_H

#include <QObject>
#include <QString>
#include <QVariantMap>
#include "qjsontreemodel.h"
#include "qxmltreemodel.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/val.h>
#endif

class JsonBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool ready READ isReady NOTIFY readyChanged)
    Q_PROPERTY(QJsonTreeModel* treeModel READ treeModel CONSTANT)
    Q_PROPERTY(QXmlTreeModel* xmlTreeModel READ xmlTreeModel CONSTANT)
    Q_PROPERTY(bool busy READ isBusy NOTIFY busyChanged)

public:
    explicit JsonBridge(QObject *parent = nullptr);

    bool isReady() const;
    QJsonTreeModel* treeModel() const;
    QXmlTreeModel* xmlTreeModel() const;
    bool isBusy() const;

    // Async operations (fire-and-forget, results via signals)
    Q_INVOKABLE void formatJson(const QString &input, const QString &indentType);
    Q_INVOKABLE void minifyJson(const QString &input);
    Q_INVOKABLE void validateJson(const QString &input);

    // Synchronous operations
    Q_INVOKABLE QString highlightJson(const QString &input);
    Q_INVOKABLE bool loadTreeModel(const QString &json);
    Q_INVOKABLE bool loadXmlTreeModel(const QString &xml);

    // Async clipboard operations (results via signals)
    Q_INVOKABLE void copyToClipboard(const QString &text);
    Q_INVOKABLE void readFromClipboard();

    // Async history methods (results via signals)
    Q_INVOKABLE void saveToHistory(const QString &json);
    Q_INVOKABLE void loadHistory();
    Q_INVOKABLE void getHistoryEntry(const QString &id);
    Q_INVOKABLE void deleteHistoryEntry(const QString &id);
    Q_INVOKABLE void clearHistory();
    Q_INVOKABLE bool isHistoryAvailable();

    // Mermaid diagram rendering (Story 10.2)
    Q_INVOKABLE void renderMermaid(const QString &code, const QString &theme = "dark");

    // Markdown rendering (Story 10.3)
    Q_INVOKABLE void requestRenderMarkdown(const QString &input);
    Q_INVOKABLE void requestRenderMarkdownWithMermaid(const QString &input, const QString &theme = "dark");

    // Async XML operations (Story 8.3)
    Q_INVOKABLE void formatXml(const QString &input, const QString &indentType);
    Q_INVOKABLE void minifyXml(const QString &input);

    // Synchronous XML operations (Story 8.3)
    Q_INVOKABLE QString highlightXml(const QString &input);

    // Format auto-detection (Story 8.4)
    Q_INVOKABLE QString detectFormat(const QString &input);

signals:
    // Format operations
    void formatCompleted(const QVariantMap &result);
    void minifyCompleted(const QVariantMap &result);
    void validateCompleted(const QVariantMap &result);

    // History operations
    void historySaved(bool success, const QString &id);
    void historyLoaded(const QVariantList &entries);
    void historyEntryLoaded(const QString &content);
    void historyEntryDeleted(bool success);
    void historyCleared(bool success);

    // Clipboard operations
    void copyCompleted(bool success);
    void clipboardRead(const QString &content);

    // Mermaid rendering (Story 10.2)
    void renderMermaidCompleted(const QVariantMap &result);

    // Markdown rendering (Story 10.3)
    void markdownRendered(const QString &html);
    void markdownRenderError(const QString &error);
    void markdownWithMermaidRendered(const QString &html, const QStringList &warnings);
    void markdownWithMermaidError(const QString &error);

    // XML operations (Story 8.3)
    void formatXmlCompleted(const QVariantMap &result);
    void minifyXmlCompleted(const QVariantMap &result);

    // Format auto-detection (Story 8.4)
    void formatDetected(const QString &format);

    // State signals
    void readyChanged();
    void busyChanged(bool busy);

private:
    bool m_ready = false;
    QJsonTreeModel* m_treeModel;
    QXmlTreeModel* m_xmlTreeModel;
    void checkReady();
    void connectAsyncSerialiserSignals();
};

#endif // JSONBRIDGE_H
