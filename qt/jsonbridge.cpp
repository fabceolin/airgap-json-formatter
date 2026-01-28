#include "jsonbridge.h"
#include "asyncserialiser.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QGuiApplication>
#include <QClipboard>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QUuid>
#include <QPromise>
#include <QRegularExpression>

#ifdef __EMSCRIPTEN__
#include <emscripten/val.h>
using namespace emscripten;
#endif

// Desktop-only: JSON formatting with indentation
#ifndef __EMSCRIPTEN__
static QString formatJsonNative(const QString &input, const QString &indentType) {
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(input.toUtf8(), &error);

    if (error.error != QJsonParseError::NoError) {
        return QString();
    }

    QJsonDocument::JsonFormat format = QJsonDocument::Indented;
    return QString::fromUtf8(doc.toJson(format));
}

static QString minifyJsonNative(const QString &input) {
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(input.toUtf8(), &error);

    if (error.error != QJsonParseError::NoError) {
        return QString();
    }

    return QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
}

static void countJsonStats(const QJsonValue &value, QVariantMap &stats, int depth) {
    int maxDepth = stats["max_depth"].toInt();
    if (depth > maxDepth) {
        stats["max_depth"] = depth;
    }

    if (value.isObject()) {
        stats["object_count"] = stats["object_count"].toInt() + 1;
        QJsonObject obj = value.toObject();
        stats["total_keys"] = stats["total_keys"].toInt() + obj.keys().size();
        for (const QString &key : obj.keys()) {
            countJsonStats(obj[key], stats, depth + 1);
        }
    } else if (value.isArray()) {
        stats["array_count"] = stats["array_count"].toInt() + 1;
        QJsonArray arr = value.toArray();
        for (const QJsonValue &v : arr) {
            countJsonStats(v, stats, depth + 1);
        }
    } else if (value.isString()) {
        stats["string_count"] = stats["string_count"].toInt() + 1;
    } else if (value.isDouble()) {
        stats["number_count"] = stats["number_count"].toInt() + 1;
    } else if (value.isBool()) {
        stats["boolean_count"] = stats["boolean_count"].toInt() + 1;
    } else if (value.isNull()) {
        stats["null_count"] = stats["null_count"].toInt() + 1;
    }
}

static QString getHistoryFilePath() {
    // Check if running in Docker/container (workspace directory exists)
    QDir workspaceDir("/workspace");
    if (workspaceDir.exists()) {
        // Use workspace directory for persistence in Docker
        return "/workspace/.history.json";
    }

    // Use standard app data location for native desktop
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(dataPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return dataPath + "/history.json";
}

static QJsonArray loadHistoryFromFile() {
    QFile file(getHistoryFilePath());
    if (!file.exists()) {
        return QJsonArray();
    }
    if (!file.open(QIODevice::ReadOnly)) {
        return QJsonArray();
    }
    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isArray()) {
        return doc.array();
    }
    return QJsonArray();
}

static bool saveHistoryToFile(const QJsonArray &history) {
    QFile file(getHistoryFilePath());
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    QJsonDocument doc(history);
    file.write(doc.toJson());
    file.close();
    return true;
}

static QString highlightJsonNative(const QString &input) {
    // Syntax highlighting for desktop - produces jq-like colored output
    // Wrap in <pre> to preserve whitespace and newlines
    QString result = "<pre style=\"margin:0; font-family:monospace; white-space:pre-wrap;\">";
    bool inString = false;
    bool inKey = false;
    bool escapeNext = false;

    for (int i = 0; i < input.length(); ++i) {
        QChar c = input[i];

        if (escapeNext) {
            // Handle escaped character - escape HTML entities if needed
            if (c == '<') result += "&lt;";
            else if (c == '>') result += "&gt;";
            else if (c == '&') result += "&amp;";
            else result += c;
            escapeNext = false;
            continue;
        }

        if (c == '\\' && inString) {
            result += c;
            escapeNext = true;
            continue;
        }

        if (c == '"') {
            if (!inString) {
                // Check if this is a key (followed eventually by :)
                int j = i + 1;
                while (j < input.length() && input[j] != '"') {
                    if (input[j] == '\\') j++;
                    j++;
                }
                j++; // skip closing quote
                while (j < input.length() && input[j].isSpace()) j++;
                inKey = (j < input.length() && input[j] == ':');

                if (inKey) {
                    result += "<span style=\"color:#8fa1b3;\">\"";  // Keys: light blue (jq style)
                } else {
                    result += "<span style=\"color:#a3be8c;\">\"";  // Strings: green (jq style)
                }
                inString = true;
            } else {
                result += "\"</span>";
                inString = false;
                inKey = false;
            }
            continue;
        }

        if (inString) {
            // Escape HTML entities
            if (c == '<') result += "&lt;";
            else if (c == '>') result += "&gt;";
            else if (c == '&') result += "&amp;";
            else result += c;
            continue;
        }

        // Numbers
        if (c.isDigit() || (c == '-' && i + 1 < input.length() && input[i+1].isDigit())) {
            result += "<span style=\"color:#d08770;\">";  // Numbers: orange (jq style)
            while (i < input.length() && (input[i].isDigit() || input[i] == '.' || input[i] == '-' || input[i] == 'e' || input[i] == 'E' || input[i] == '+')) {
                result += input[i];
                i++;
            }
            result += "</span>";
            i--;
            continue;
        }

        // true/false
        if (input.mid(i, 4) == "true") {
            result += "<span style=\"color:#b48ead;\">true</span>";  // Booleans: purple (jq style)
            i += 3;
            continue;
        }
        if (input.mid(i, 5) == "false") {
            result += "<span style=\"color:#b48ead;\">false</span>";  // Booleans: purple
            i += 4;
            continue;
        }
        // null
        if (input.mid(i, 4) == "null") {
            result += "<span style=\"color:#bf616a;\">null</span>";  // Null: red (jq style)
            i += 3;
            continue;
        }

        // Brackets and braces
        if (c == '{' || c == '}' || c == '[' || c == ']') {
            result += "<span style=\"color:#c0c5ce;\">" + QString(c) + "</span>";  // Punctuation: light gray
            continue;
        }

        // Colon and comma
        if (c == ':' || c == ',') {
            result += "<span style=\"color:#c0c5ce;\">" + QString(c) + "</span>";  // Punctuation: light gray
            continue;
        }

        // All other characters (whitespace, newlines) - pass through
        result += c;
    }

    result += "</pre>";
    return result;
}
#endif

JsonBridge::JsonBridge(QObject *parent)
    : QObject(parent)
    , m_treeModel(new QJsonTreeModel(this))
    , m_xmlTreeModel(new QXmlTreeModel(this))
{
    checkReady();
    connectAsyncSerialiserSignals();
}

void JsonBridge::connectAsyncSerialiserSignals()
{
    // Connect to AsyncSerialiser to emit busyChanged when queue changes
    connect(&AsyncSerialiser::instance(), &AsyncSerialiser::queueLengthChanged,
            this, [this]() {
        emit busyChanged(isBusy());
    });
    connect(&AsyncSerialiser::instance(), &AsyncSerialiser::taskStarted,
            this, [this](const QString&) {
        emit busyChanged(isBusy());
    });
    connect(&AsyncSerialiser::instance(), &AsyncSerialiser::taskCompleted,
            this, [this](const QString&, bool) {
        emit busyChanged(isBusy());
    });
}

bool JsonBridge::isBusy() const
{
    return AsyncSerialiser::instance().queueLength() > 0;
}

QJsonTreeModel* JsonBridge::treeModel() const
{
    return m_treeModel;
}

QXmlTreeModel* JsonBridge::xmlTreeModel() const
{
    return m_xmlTreeModel;
}

bool JsonBridge::loadTreeModel(const QString &json)
{
    return m_treeModel->loadJson(json);
}

bool JsonBridge::loadXmlTreeModel(const QString &xml)
{
    return m_xmlTreeModel->loadXml(xml);
}

void JsonBridge::checkReady()
{
#ifdef __EMSCRIPTEN__
    val window = val::global("window");
    val jsonBridge = window["JsonBridge"];
    if (!jsonBridge.isUndefined() && !jsonBridge.isNull()) {
        val isReadyFunc = jsonBridge["isReady"];
        if (!isReadyFunc.isUndefined()) {
            m_ready = jsonBridge.call<bool>("isReady");
        }
    }
#else
    // Desktop mode is always ready
    m_ready = true;
#endif
    emit readyChanged();
}

bool JsonBridge::isReady() const
{
    return m_ready;
}

void JsonBridge::formatJson(const QString &input, const QString &indentType)
{
    AsyncSerialiser::instance().enqueue("formatJson", [this, input, indentType]() {
        QPromise<QVariant> promise;
        auto future = promise.future();
        promise.start();

        QVariantMap result;
        result["success"] = false;

#ifdef __EMSCRIPTEN__
        try {
            val window = val::global("window");
            val jsonBridge = window["JsonBridge"];

            if (jsonBridge.isUndefined() || jsonBridge.isNull()) {
                result["error"] = "JsonBridge not available";
            } else {
                std::string inputStd = input.toStdString();
                std::string indentStd = indentType.toStdString();

                // Call returns JSON string to avoid embind property access issues
                std::string jsonResultStr = jsonBridge.call<std::string>("formatJson", inputStd, indentStd);

                // Parse the JSON response
                QJsonDocument doc = QJsonDocument::fromJson(QString::fromStdString(jsonResultStr).toUtf8());
                if (doc.isNull() || !doc.isObject()) {
                    result["error"] = "Failed to parse formatJson response";
                } else {
                    QJsonObject obj = doc.object();
                    bool success = obj["success"].toBool();
                    result["success"] = success;

                    if (success) {
                        result["result"] = obj["result"].toString();
                    } else {
                        result["error"] = obj["error"].toString();
                    }
                }
            }
        } catch (const std::exception &e) {
            result["error"] = QString("Exception: %1").arg(e.what());
        } catch (...) {
            result["error"] = "Unknown error in formatJson";
        }
#else
        // Desktop native implementation
        QString formatted = formatJsonNative(input, indentType);
        if (formatted.isEmpty()) {
            result["error"] = "Invalid JSON";
        } else {
            result["success"] = true;
            result["result"] = formatted;
        }
#endif

        // Emit signal on main thread
        QMetaObject::invokeMethod(this, [this, result]() {
            emit formatCompleted(result);
        }, Qt::QueuedConnection);

        promise.addResult(QVariant::fromValue(result));
        promise.finish();
        return future;
    });
}

void JsonBridge::minifyJson(const QString &input)
{
    AsyncSerialiser::instance().enqueue("minifyJson", [this, input]() {
        QPromise<QVariant> promise;
        auto future = promise.future();
        promise.start();

        QVariantMap result;
        result["success"] = false;

#ifdef __EMSCRIPTEN__
        try {
            val window = val::global("window");
            val jsonBridge = window["JsonBridge"];

            if (jsonBridge.isUndefined() || jsonBridge.isNull()) {
                result["error"] = "JsonBridge not available";
            } else {
                std::string inputStd = input.toStdString();

                // Call returns JSON string to avoid embind property access issues
                std::string jsonResultStr = jsonBridge.call<std::string>("minifyJson", inputStd);

                // Parse the JSON response
                QJsonDocument doc = QJsonDocument::fromJson(QString::fromStdString(jsonResultStr).toUtf8());
                if (doc.isNull() || !doc.isObject()) {
                    result["error"] = "Failed to parse minifyJson response";
                } else {
                    QJsonObject obj = doc.object();
                    bool success = obj["success"].toBool();
                    result["success"] = success;

                    if (success) {
                        result["result"] = obj["result"].toString();
                    } else {
                        result["error"] = obj["error"].toString();
                    }
                }
            }
        } catch (const std::exception &e) {
            result["error"] = QString("Exception: %1").arg(e.what());
        } catch (...) {
            result["error"] = "Unknown error in minifyJson";
        }
#else
        // Desktop native implementation
        QString minified = minifyJsonNative(input);
        if (minified.isEmpty()) {
            result["error"] = "Invalid JSON";
        } else {
            result["success"] = true;
            result["result"] = minified;
        }
#endif

        // Emit signal on main thread
        QMetaObject::invokeMethod(this, [this, result]() {
            emit minifyCompleted(result);
        }, Qt::QueuedConnection);

        promise.addResult(QVariant::fromValue(result));
        promise.finish();
        return future;
    });
}

void JsonBridge::validateJson(const QString &input)
{
    AsyncSerialiser::instance().enqueue("validateJson", [this, input]() {
        QPromise<QVariant> promise;
        auto future = promise.future();
        promise.start();

        QVariantMap result;
        result["isValid"] = false;

#ifdef __EMSCRIPTEN__
        try {
            val window = val::global("window");
            val jsonBridge = window["JsonBridge"];

            if (jsonBridge.isUndefined() || jsonBridge.isNull()) {
                QVariantMap error;
                error["message"] = "JsonBridge not available";
                error["line"] = 0;
                error["column"] = 0;
                result["error"] = error;
                result["stats"] = QVariantMap();
            } else {
                std::string inputStd = input.toStdString();

                // Call returns JSON string to avoid embind property access issues
                std::string jsonResultStr = jsonBridge.call<std::string>("validateJson", inputStd);

                // Parse the JSON response
                QJsonDocument doc = QJsonDocument::fromJson(QString::fromStdString(jsonResultStr).toUtf8());
                if (doc.isNull() || !doc.isObject()) {
                    QVariantMap error;
                    error["message"] = "Failed to parse validateJson response";
                    error["line"] = 0;
                    error["column"] = 0;
                    result["error"] = error;
                    result["stats"] = QVariantMap();
                } else {
                    QJsonObject obj = doc.object();
                    bool isValid = obj["isValid"].toBool();
                    result["isValid"] = isValid;

                    if (isValid) {
                        QVariantMap stats;
                        QJsonObject jsStats = obj["stats"].toObject();
                        stats["object_count"] = jsStats["objectCount"].toInt(0);
                        stats["array_count"] = jsStats["arrayCount"].toInt(0);
                        stats["string_count"] = jsStats["stringCount"].toInt(0);
                        stats["number_count"] = jsStats["numberCount"].toInt(0);
                        stats["boolean_count"] = jsStats["booleanCount"].toInt(0);
                        stats["null_count"] = jsStats["nullCount"].toInt(0);
                        stats["total_keys"] = jsStats["totalKeys"].toInt(0);
                        stats["max_depth"] = jsStats["maxDepth"].toInt(0);
                        result["stats"] = stats;
                    } else {
                        QJsonObject jsError = obj["error"].toObject();
                        QVariantMap error;
                        error["message"] = jsError["message"].toString("Unknown error");
                        error["line"] = jsError["line"].toInt(0);
                        error["column"] = jsError["column"].toInt(0);
                        result["error"] = error;
                        result["stats"] = QVariantMap();
                    }
                }
            }
        } catch (const std::exception &e) {
            QVariantMap error;
            error["message"] = QString("Exception: %1").arg(e.what());
            error["line"] = 0;
            error["column"] = 0;
            result["error"] = error;
            result["stats"] = QVariantMap();
        } catch (...) {
            QVariantMap error;
            error["message"] = "Unknown error in validateJson";
            error["line"] = 0;
            error["column"] = 0;
            result["error"] = error;
            result["stats"] = QVariantMap();
        }
#else
        // Desktop native implementation
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(input.toUtf8(), &parseError);

        if (parseError.error != QJsonParseError::NoError) {
            QVariantMap error;
            error["message"] = parseError.errorString();
            // Calculate line and column from offset
            int line = 1, column = 1;
            for (int i = 0; i < parseError.offset && i < input.length(); ++i) {
                if (input[i] == '\n') {
                    line++;
                    column = 1;
                } else {
                    column++;
                }
            }
            error["line"] = line;
            error["column"] = column;
            result["error"] = error;
            result["stats"] = QVariantMap();
        } else {
            result["isValid"] = true;
            QVariantMap stats;
            stats["object_count"] = 0;
            stats["array_count"] = 0;
            stats["string_count"] = 0;
            stats["number_count"] = 0;
            stats["boolean_count"] = 0;
            stats["null_count"] = 0;
            stats["total_keys"] = 0;
            stats["max_depth"] = 0;

            if (doc.isObject()) {
                countJsonStats(doc.object(), stats, 1);
            } else if (doc.isArray()) {
                countJsonStats(doc.array(), stats, 1);
            }
            result["stats"] = stats;
        }
#endif

        // Emit signal on main thread
        QMetaObject::invokeMethod(this, [this, result]() {
            emit validateCompleted(result);
        }, Qt::QueuedConnection);

        promise.addResult(QVariant::fromValue(result));
        promise.finish();
        return future;
    });
}

QString JsonBridge::highlightJson(const QString &input)
{
#ifdef __EMSCRIPTEN__
    try {
        val window = val::global("window");
        val jsonBridge = window["JsonBridge"];

        if (jsonBridge.isUndefined() || jsonBridge.isNull()) {
            // Return escaped HTML if bridge not available
            QString escaped = input;
            escaped.replace("&", "&amp;");
            escaped.replace("<", "&lt;");
            escaped.replace(">", "&gt;");
            return escaped;
        }

        std::string inputStd = input.toStdString();
        std::string result = jsonBridge.call<std::string>("highlightJson", inputStd);
        return QString::fromStdString(result);
    } catch (const std::exception &e) {
        qWarning() << "highlightJson error:" << e.what();
    } catch (...) {
        qWarning() << "Unknown error in highlightJson";
    }
    // Fallback: return escaped HTML
    QString escaped = input;
    escaped.replace("&", "&amp;");
    escaped.replace("<", "&lt;");
    escaped.replace(">", "&gt;");
    return escaped;
#else
    // Desktop native implementation
    return highlightJsonNative(input);
#endif
}

void JsonBridge::copyToClipboard(const QString &text)
{
    AsyncSerialiser::instance().enqueue("copyToClipboard", [this, text]() {
        QPromise<QVariant> promise;
        auto future = promise.future();
        promise.start();

        bool success = false;

#ifdef __EMSCRIPTEN__
        try {
            val window = val::global("window");
            val jsonBridge = window["JsonBridge"];

            if (!jsonBridge.isUndefined() && !jsonBridge.isNull()) {
                std::string textStd = text.toStdString();
                jsonBridge.call<void>("copyToClipboard", textStd);
                success = true;
            }
        } catch (...) {
            qWarning() << "Failed to copy to clipboard";
        }
#else
        // Desktop native implementation
        QClipboard *clipboard = QGuiApplication::clipboard();
        if (clipboard) {
            clipboard->setText(text);
            success = true;
        }
#endif

        // Emit signal on main thread
        QMetaObject::invokeMethod(this, [this, success]() {
            emit copyCompleted(success);
        }, Qt::QueuedConnection);

        promise.addResult(QVariant::fromValue(success));
        promise.finish();
        return future;
    });
}

void JsonBridge::readFromClipboard()
{
    AsyncSerialiser::instance().enqueue("readFromClipboard", [this]() {
        QPromise<QVariant> promise;
        auto future = promise.future();
        promise.start();

        QString content;

#ifdef __EMSCRIPTEN__
        try {
            val window = val::global("window");
            val jsonBridge = window["JsonBridge"];

            if (!jsonBridge.isUndefined() && !jsonBridge.isNull()) {
                // Note: This is a synchronous call to an async JS function
                // The JS side handles the promise internally
                val jsPromise = jsonBridge.call<val>("readFromClipboard");
                val result = jsPromise.await();
                if (!result.isUndefined() && !result.isNull()) {
                    std::string text = result.as<std::string>();
                    content = QString::fromStdString(text);
                }
            }
        } catch (const std::exception &e) {
            qWarning() << "Failed to read from clipboard:" << e.what();
        } catch (...) {
            qWarning() << "Failed to read from clipboard";
        }
#else
        // Desktop native implementation
        QClipboard *clipboard = QGuiApplication::clipboard();
        if (clipboard) {
            content = clipboard->text();
        }
#endif

        // Emit signal on main thread
        QMetaObject::invokeMethod(this, [this, content]() {
            emit clipboardRead(content);
        }, Qt::QueuedConnection);

        promise.addResult(QVariant::fromValue(content));
        promise.finish();
        return future;
    });
}

// History methods

void JsonBridge::saveToHistory(const QString &json)
{
    AsyncSerialiser::instance().enqueue("saveToHistory", [this, json]() {
        QPromise<QVariant> promise;
        auto future = promise.future();
        promise.start();

        bool success = false;
        QString id;

#ifdef __EMSCRIPTEN__
        try {
            val window = val::global("window");
            val jsonBridge = window["JsonBridge"];

            if (!jsonBridge.isUndefined() && !jsonBridge.isNull()) {
                std::string jsonStd = json.toStdString();
                val jsPromise = jsonBridge.call<val>("saveToHistory", jsonStd);
                val result = jsPromise.await();

                if (!result.isUndefined() && !result.isNull()) {
                    std::string resultStr = result.as<std::string>();
                    QJsonDocument doc = QJsonDocument::fromJson(QString::fromStdString(resultStr).toUtf8());
                    QJsonObject obj = doc.object();
                    success = obj["success"].toBool();
                    id = obj["id"].toString();
                }
            }
        } catch (const std::exception &e) {
            qWarning() << "Failed to save to history:" << e.what();
        } catch (...) {
            qWarning() << "Failed to save to history";
        }
#else
        // Desktop native implementation
        QJsonArray history = loadHistoryFromFile();

        id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        QString timestamp = QDateTime::currentDateTime().toString(Qt::ISODate);
        QString preview = json.left(100).simplified();
        if (json.length() > 100) preview += "...";

        QJsonObject entry;
        entry["id"] = id;
        entry["content"] = json;
        entry["timestamp"] = timestamp;
        entry["preview"] = preview;
        entry["size"] = json.size();

        // Add to beginning of array (most recent first)
        QJsonArray newHistory;
        newHistory.append(entry);
        for (int i = 0; i < history.size() && i < 49; ++i) { // Keep max 50 entries
            newHistory.append(history[i]);
        }

        success = saveHistoryToFile(newHistory);
#endif

        // Emit signal on main thread
        QMetaObject::invokeMethod(this, [this, success, id]() {
            emit historySaved(success, id);
        }, Qt::QueuedConnection);

        promise.addResult(QVariant::fromValue(success));
        promise.finish();
        return future;
    });
}

void JsonBridge::loadHistory()
{
    AsyncSerialiser::instance().enqueue("loadHistory", [this]() {
        QPromise<QVariant> promise;
        auto future = promise.future();
        promise.start();

        QVariantList entries;

#ifdef __EMSCRIPTEN__
        try {
            val window = val::global("window");
            val jsonBridge = window["JsonBridge"];

            if (!jsonBridge.isUndefined() && !jsonBridge.isNull()) {
                val jsPromise = jsonBridge.call<val>("loadHistory");
                val result = jsPromise.await();

                if (!result.isUndefined() && !result.isNull()) {
                    std::string resultStr = result.as<std::string>();
                    QJsonDocument doc = QJsonDocument::fromJson(QString::fromStdString(resultStr).toUtf8());
                    QJsonObject obj = doc.object();

                    if (obj["success"].toBool()) {
                        QJsonArray entriesArray = obj["entries"].toArray();
                        for (const QJsonValue &v : entriesArray) {
                            QJsonObject entry = v.toObject();
                            QVariantMap entryMap;
                            entryMap["id"] = entry["id"].toString();
                            entryMap["content"] = entry["content"].toString();
                            entryMap["timestamp"] = entry["timestamp"].toString();
                            entryMap["preview"] = entry["preview"].toString();
                            entryMap["size"] = entry["size"].toInt();
                            entries.append(entryMap);
                        }
                    }
                }
            }
        } catch (const std::exception &e) {
            qWarning() << "Failed to load history:" << e.what();
        } catch (...) {
            qWarning() << "Failed to load history";
        }
#else
        // Desktop native implementation
        QJsonArray history = loadHistoryFromFile();
        for (const QJsonValue &v : history) {
            QJsonObject entry = v.toObject();
            QVariantMap entryMap;
            entryMap["id"] = entry["id"].toString();
            entryMap["content"] = entry["content"].toString();
            entryMap["timestamp"] = entry["timestamp"].toString();
            entryMap["preview"] = entry["preview"].toString();
            entryMap["size"] = entry["size"].toInt();
            entries.append(entryMap);
        }
#endif

        // Emit signal on main thread
        QMetaObject::invokeMethod(this, [this, entries]() {
            emit historyLoaded(entries);
        }, Qt::QueuedConnection);

        promise.addResult(QVariant::fromValue(entries));
        promise.finish();
        return future;
    });
}

void JsonBridge::getHistoryEntry(const QString &id)
{
    AsyncSerialiser::instance().enqueue("getHistoryEntry", [this, id]() {
        QPromise<QVariant> promise;
        auto future = promise.future();
        promise.start();

        QString content;

#ifdef __EMSCRIPTEN__
        try {
            val window = val::global("window");
            val jsonBridge = window["JsonBridge"];

            if (!jsonBridge.isUndefined() && !jsonBridge.isNull()) {
                std::string idStd = id.toStdString();
                val jsPromise = jsonBridge.call<val>("getHistoryEntry", idStd);
                val result = jsPromise.await();

                if (!result.isUndefined() && !result.isNull()) {
                    std::string resultStr = result.as<std::string>();
                    QJsonDocument doc = QJsonDocument::fromJson(QString::fromStdString(resultStr).toUtf8());
                    QJsonObject obj = doc.object();

                    if (obj["success"].toBool()) {
                        QJsonObject entry = obj["entry"].toObject();
                        content = entry["content"].toString();
                    }
                }
            }
        } catch (const std::exception &e) {
            qWarning() << "Failed to get history entry:" << e.what();
        } catch (...) {
            qWarning() << "Failed to get history entry";
        }
#else
        // Desktop native implementation
        QJsonArray history = loadHistoryFromFile();
        for (const QJsonValue &v : history) {
            QJsonObject entry = v.toObject();
            if (entry["id"].toString() == id) {
                content = entry["content"].toString();
                break;
            }
        }
#endif

        // Emit signal on main thread
        QMetaObject::invokeMethod(this, [this, content]() {
            emit historyEntryLoaded(content);
        }, Qt::QueuedConnection);

        promise.addResult(QVariant::fromValue(content));
        promise.finish();
        return future;
    });
}

void JsonBridge::deleteHistoryEntry(const QString &id)
{
    AsyncSerialiser::instance().enqueue("deleteHistoryEntry", [this, id]() {
        QPromise<QVariant> promise;
        auto future = promise.future();
        promise.start();

        bool success = false;

#ifdef __EMSCRIPTEN__
        try {
            val window = val::global("window");
            val jsonBridge = window["JsonBridge"];

            if (!jsonBridge.isUndefined() && !jsonBridge.isNull()) {
                std::string idStd = id.toStdString();
                val jsPromise = jsonBridge.call<val>("deleteHistoryEntry", idStd);
                val result = jsPromise.await();

                if (!result.isUndefined() && !result.isNull()) {
                    std::string resultStr = result.as<std::string>();
                    QJsonDocument doc = QJsonDocument::fromJson(QString::fromStdString(resultStr).toUtf8());
                    QJsonObject obj = doc.object();
                    success = obj["success"].toBool();
                }
            }
        } catch (const std::exception &e) {
            qWarning() << "Failed to delete history entry:" << e.what();
        } catch (...) {
            qWarning() << "Failed to delete history entry";
        }
#else
        // Desktop native implementation
        QJsonArray history = loadHistoryFromFile();
        QJsonArray newHistory;
        bool found = false;
        for (const QJsonValue &v : history) {
            QJsonObject entry = v.toObject();
            if (entry["id"].toString() != id) {
                newHistory.append(entry);
            } else {
                found = true;
            }
        }
        if (found) {
            success = saveHistoryToFile(newHistory);
        }
#endif

        // Emit signal on main thread
        QMetaObject::invokeMethod(this, [this, success]() {
            emit historyEntryDeleted(success);
        }, Qt::QueuedConnection);

        promise.addResult(QVariant::fromValue(success));
        promise.finish();
        return future;
    });
}

void JsonBridge::clearHistory()
{
    AsyncSerialiser::instance().enqueue("clearHistory", [this]() {
        QPromise<QVariant> promise;
        auto future = promise.future();
        promise.start();

        bool success = false;

#ifdef __EMSCRIPTEN__
        try {
            val window = val::global("window");
            val jsonBridge = window["JsonBridge"];

            if (!jsonBridge.isUndefined() && !jsonBridge.isNull()) {
                val jsPromise = jsonBridge.call<val>("clearHistory");
                val result = jsPromise.await();

                if (!result.isUndefined() && !result.isNull()) {
                    std::string resultStr = result.as<std::string>();
                    QJsonDocument doc = QJsonDocument::fromJson(QString::fromStdString(resultStr).toUtf8());
                    QJsonObject obj = doc.object();
                    success = obj["success"].toBool();
                }
            }
        } catch (const std::exception &e) {
            qWarning() << "Failed to clear history:" << e.what();
        } catch (...) {
            qWarning() << "Failed to clear history";
        }
#else
        // Desktop native implementation
        success = saveHistoryToFile(QJsonArray());
#endif

        // Emit signal on main thread
        QMetaObject::invokeMethod(this, [this, success]() {
            emit historyCleared(success);
        }, Qt::QueuedConnection);

        promise.addResult(QVariant::fromValue(success));
        promise.finish();
        return future;
    });
}

bool JsonBridge::isHistoryAvailable()
{
#ifdef __EMSCRIPTEN__
    try {
        val window = val::global("window");
        val jsonBridge = window["JsonBridge"];

        if (!jsonBridge.isUndefined() && !jsonBridge.isNull()) {
            return jsonBridge.call<bool>("isHistoryAvailable");
        }
    } catch (...) {
        qWarning() << "Failed to check history availability";
    }
    return false;
#else
    // Desktop native implementation - history is always available
    return true;
#endif
}

void JsonBridge::requestRenderMarkdown(const QString &input)
{
    AsyncSerialiser::instance().enqueue("renderMarkdown", [this, input]() {
        QPromise<QVariant> promise;
        auto future = promise.future();
        promise.start();

        QString html;
        QString error;
        bool success = false;

#ifdef __EMSCRIPTEN__
        try {
            val window = val::global("window");
            val jsonBridge = window["JsonBridge"];

            if (jsonBridge.isUndefined() || jsonBridge.isNull()) {
                error = "JsonBridge not available";
            } else {
                std::string inputStd = input.toStdString();

                // Call returns JSON string
                std::string jsonResultStr = jsonBridge.call<std::string>("renderMarkdown", inputStd);

                // Parse the JSON response
                QJsonDocument doc = QJsonDocument::fromJson(QString::fromStdString(jsonResultStr).toUtf8());
                if (doc.isNull() || !doc.isObject()) {
                    error = "Failed to parse renderMarkdown response";
                } else {
                    QJsonObject obj = doc.object();
                    success = obj["success"].toBool();

                    if (success) {
                        html = obj["html"].toString();
                    } else {
                        error = obj["error"].toString();
                    }
                }
            }
        } catch (const std::exception &e) {
            error = QString("Exception: %1").arg(e.what());
        } catch (...) {
            error = "Unknown error in renderMarkdown";
        }
#else
        // Desktop: Markdown rendering only available in WASM build
        error = "Markdown rendering only available in WASM build";
#endif

        // Emit signal on main thread
        QMetaObject::invokeMethod(this, [this, success, html, error]() {
            if (success) {
                emit markdownRendered(html);
            } else {
                emit markdownRenderError(error);
            }
        }, Qt::QueuedConnection);

        promise.addResult(QVariant::fromValue(success));
        promise.finish();
        return future;
    });
}

void JsonBridge::requestRenderMarkdownWithMermaid(const QString &input, const QString &theme)
{
    AsyncSerialiser::instance().enqueue("renderMarkdownWithMermaid", [this, input, theme]() {
        QPromise<QVariant> promise;
        auto future = promise.future();
        promise.start();

        QString html;
        QString error;
        QStringList warnings;
        bool success = false;

#ifdef __EMSCRIPTEN__
        try {
            val window = val::global("window");
            val jsonBridge = window["JsonBridge"];

            if (jsonBridge.isUndefined() || jsonBridge.isNull()) {
                error = "JsonBridge not available";
            } else {
                std::string inputStd = input.toStdString();
                std::string themeStd = theme.toStdString();

                // renderMarkdownWithMermaid is async - use await
                val jsPromise = jsonBridge.call<val>("renderMarkdownWithMermaid", inputStd, themeStd);
                val jsResult = jsPromise.await();

                // Parse the JSON string result
                std::string jsonResultStr = jsResult.as<std::string>();
                QJsonDocument doc = QJsonDocument::fromJson(QString::fromStdString(jsonResultStr).toUtf8());

                if (doc.isNull() || !doc.isObject()) {
                    error = "Failed to parse renderMarkdownWithMermaid response";
                } else {
                    QJsonObject obj = doc.object();
                    success = obj["success"].toBool();

                    if (success) {
                        html = obj["html"].toString();
                        // Parse warnings array if present
                        if (obj.contains("warnings") && obj["warnings"].isArray()) {
                            QJsonArray warningsArray = obj["warnings"].toArray();
                            for (const QJsonValue &v : warningsArray) {
                                warnings.append(v.toString());
                            }
                        }
                    } else {
                        error = obj["error"].toString();
                    }
                }
            }
        } catch (const std::exception &e) {
            error = QString("Exception: %1").arg(e.what());
        } catch (...) {
            error = "Unknown error in renderMarkdownWithMermaid";
        }
#else
        // Desktop: Markdown+Mermaid rendering only available in WASM build
        error = "Markdown+Mermaid rendering only available in WASM build";
#endif

        // Emit signal on main thread
        QMetaObject::invokeMethod(this, [this, success, html, error, warnings]() {
            if (success) {
                emit markdownWithMermaidRendered(html, warnings);
            } else {
                emit markdownWithMermaidError(error);
            }
        }, Qt::QueuedConnection);

        promise.addResult(QVariant::fromValue(success));
        promise.finish();
        return future;
    });
}

void JsonBridge::renderMermaid(const QString &code, const QString &theme)
{
    AsyncSerialiser::instance().enqueue("renderMermaid", [this, code, theme]() {
        QPromise<QVariant> promise;
        auto future = promise.future();
        promise.start();

        QVariantMap result;
        result["success"] = false;

#ifdef __EMSCRIPTEN__
        try {
            val window = val::global("window");
            val renderFn = window["renderMermaid"];

            if (renderFn.isUndefined() || renderFn.isNull()) {
                result["error"] = "renderMermaid not available";
            } else {
                std::string codeStd = code.toStdString();
                std::string themeStd = theme.toStdString();

                // renderMermaid is async — use jsPromise.await() for Asyncify
                val jsPromise = renderFn(codeStd, themeStd);
                val jsResult = jsPromise.await();

                // Parse the JavaScript object result
                bool success = jsResult["success"].as<bool>();
                result["success"] = success;

                if (success) {
                    result["svg"] = QString::fromStdString(jsResult["svg"].as<std::string>());
                } else {
                    result["error"] = QString::fromStdString(jsResult["error"].as<std::string>());
                    if (!jsResult["details"].isUndefined()) {
                        result["details"] = QString::fromStdString(jsResult["details"].as<std::string>());
                    }
                }
            }
        } catch (const std::exception& e) {
            result["error"] = QString("Exception: %1").arg(e.what());
        } catch (...) {
            result["error"] = "Unknown error in renderMermaid";
        }
#else
        // Desktop: Mermaid rendering only available in WASM build
        result["error"] = "Mermaid rendering only available in WASM build";
#endif

        // Emit signal on main thread
        QMetaObject::invokeMethod(this, [this, result]() {
            emit renderMermaidCompleted(result);
        }, Qt::QueuedConnection);

        promise.addResult(QVariant::fromValue(result));
        promise.finish();
        return future;
    });
}

// ============================================================================
// XML Operations (Story 8.3)
// ============================================================================

void JsonBridge::formatXml(const QString &input, const QString &indentType)
{
    AsyncSerialiser::instance().enqueue("formatXml", [this, input, indentType]() {
        QPromise<QVariant> promise;
        auto future = promise.future();
        promise.start();

        QVariantMap result;
        result["success"] = false;

#ifdef __EMSCRIPTEN__
        try {
            val window = val::global("window");
            val jsonBridge = window["JsonBridge"];

            if (jsonBridge.isUndefined() || jsonBridge.isNull()) {
                result["error"] = "JsonBridge not available";
            } else {
                std::string inputStd = input.toStdString();
                std::string indentStd = indentType.toStdString();

                // Call returns JSON string to avoid embind property access issues
                std::string jsonResultStr = jsonBridge.call<std::string>("formatXml", inputStd, indentStd);

                // Parse the JSON response
                QJsonDocument doc = QJsonDocument::fromJson(QString::fromStdString(jsonResultStr).toUtf8());
                if (doc.isNull() || !doc.isObject()) {
                    result["error"] = "Failed to parse formatXml response";
                } else {
                    QJsonObject obj = doc.object();
                    bool success = obj["success"].toBool();
                    result["success"] = success;

                    if (success) {
                        result["result"] = obj["result"].toString();
                    } else {
                        result["error"] = obj["error"].toString();
                    }
                }
            }
        } catch (const std::exception &e) {
            result["error"] = QString("Exception: %1").arg(e.what());
        } catch (...) {
            result["error"] = "Unknown error in formatXml";
        }
#else
        // Desktop: XML formatting not available (requires Rust WASM)
        Q_UNUSED(indentType);
        result["error"] = "XML formatting not available in desktop build";
#endif

        // Emit signal on main thread
        QMetaObject::invokeMethod(this, [this, result]() {
            emit formatXmlCompleted(result);
        }, Qt::QueuedConnection);

        promise.addResult(QVariant::fromValue(result));
        promise.finish();
        return future;
    });
}

void JsonBridge::minifyXml(const QString &input)
{
    AsyncSerialiser::instance().enqueue("minifyXml", [this, input]() {
        QPromise<QVariant> promise;
        auto future = promise.future();
        promise.start();

        QVariantMap result;
        result["success"] = false;

#ifdef __EMSCRIPTEN__
        try {
            val window = val::global("window");
            val jsonBridge = window["JsonBridge"];

            if (jsonBridge.isUndefined() || jsonBridge.isNull()) {
                result["error"] = "JsonBridge not available";
            } else {
                std::string inputStd = input.toStdString();

                // Call returns JSON string to avoid embind property access issues
                std::string jsonResultStr = jsonBridge.call<std::string>("minifyXml", inputStd);

                // Parse the JSON response
                QJsonDocument doc = QJsonDocument::fromJson(QString::fromStdString(jsonResultStr).toUtf8());
                if (doc.isNull() || !doc.isObject()) {
                    result["error"] = "Failed to parse minifyXml response";
                } else {
                    QJsonObject obj = doc.object();
                    bool success = obj["success"].toBool();
                    result["success"] = success;

                    if (success) {
                        result["result"] = obj["result"].toString();
                    } else {
                        result["error"] = obj["error"].toString();
                    }
                }
            }
        } catch (const std::exception &e) {
            result["error"] = QString("Exception: %1").arg(e.what());
        } catch (...) {
            result["error"] = "Unknown error in minifyXml";
        }
#else
        // Desktop: XML minification not available (requires Rust WASM)
        result["error"] = "XML minification not available in desktop build";
#endif

        // Emit signal on main thread
        QMetaObject::invokeMethod(this, [this, result]() {
            emit minifyXmlCompleted(result);
        }, Qt::QueuedConnection);

        promise.addResult(QVariant::fromValue(result));
        promise.finish();
        return future;
    });
}

// ============================================================================
// Format Auto-Detection (Story 8.4 + Story 10.4 Markdown Extension)
// ============================================================================

// Story 10.4: Helper function to detect likely Markdown content
// Uses heuristic pattern matching for common Markdown syntax
static bool isLikelyMarkdown(const QString &input)
{
    // Static regex patterns for performance (compiled once)
    // Heading: # followed by space (ATX heading) - must have space after hash
    static QRegularExpression headingRe(QStringLiteral("^#{1,6}\\s"));
    // Code block: triple backticks at line start
    static QRegularExpression codeBlockRe(QStringLiteral("^```"));
    // YAML frontmatter: --- at document start
    static QRegularExpression frontmatterRe(QStringLiteral("^---\\s*$"));
    // Unordered list: - or * followed by space
    static QRegularExpression unorderedListRe(QStringLiteral("^[-*]\\s"));
    // Ordered list: digit(s) followed by . and space
    static QRegularExpression orderedListRe(QStringLiteral("^\\d+\\.\\s"));
    // Blockquote: > followed by space
    static QRegularExpression blockquoteRe(QStringLiteral("^>\\s"));

    // Validate static regex patterns (fail gracefully if malformed)
    if (!headingRe.isValid() || !codeBlockRe.isValid() || !frontmatterRe.isValid() ||
        !unorderedListRe.isValid() || !orderedListRe.isValid() || !blockquoteRe.isValid()) {
        return false;
    }

    // Check first line for patterns
    int newlinePos = input.indexOf(QLatin1Char('\n'));
    QString firstLine = (newlinePos > 0) ? input.left(newlinePos) : input;
    QString firstLineTrimmed = firstLine.trimmed();

    // First line pattern checks
    if (headingRe.match(firstLineTrimmed).hasMatch()) return true;
    if (codeBlockRe.match(firstLineTrimmed).hasMatch()) return true;
    if (frontmatterRe.match(firstLineTrimmed).hasMatch()) return true;
    if (unorderedListRe.match(firstLineTrimmed).hasMatch()) return true;
    if (orderedListRe.match(firstLineTrimmed).hasMatch()) return true;
    if (blockquoteRe.match(firstLineTrimmed).hasMatch()) return true;

    // Check for patterns mid-document (limit search to first 2KB for performance)
    QString searchArea = input.left(2000);

    // Multi-line pattern: heading at start of any line
    static QRegularExpression anyHeadingRe(QStringLiteral("\\n#{1,6}\\s"));
    // Multi-line pattern: code block at start of any line
    static QRegularExpression anyCodeBlockRe(QStringLiteral("\\n```"));
    // Link pattern: [text](url)
    static QRegularExpression linkRe(QStringLiteral("\\[.+?\\]\\(.+?\\)"));

    // Validate multi-line regex patterns
    if (!anyHeadingRe.isValid() || !anyCodeBlockRe.isValid() || !linkRe.isValid()) {
        return false;
    }

    if (anyHeadingRe.match(searchArea).hasMatch()) return true;
    if (anyCodeBlockRe.match(searchArea).hasMatch()) return true;
    if (linkRe.match(searchArea).hasMatch()) return true;

    return false;
}

QString JsonBridge::detectFormat(const QString &input)
{
    QString trimmed = input.trimmed();

    if (trimmed.isEmpty()) {
        emit formatDetected(QStringLiteral("unknown"));
        return QStringLiteral("unknown");
    }

    QChar first = trimmed.at(0);

    // Priority 1: XML (strict syntax - starts with <)
    if (first == QLatin1Char('<')) {
        emit formatDetected(QStringLiteral("xml"));
        return QStringLiteral("xml");
    }

    // Priority 2: JSON (strict syntax - starts with { or [)
    if (first == QLatin1Char('{') || first == QLatin1Char('[')) {
        emit formatDetected(QStringLiteral("json"));
        return QStringLiteral("json");
    }

    // Priority 3: Markdown (heuristic patterns) - Story 10.4
    if (isLikelyMarkdown(trimmed)) {
        emit formatDetected(QStringLiteral("markdown"));
        return QStringLiteral("markdown");
    }

    // Priority 4: Unknown (default fallback)
    emit formatDetected(QStringLiteral("unknown"));
    return QStringLiteral("unknown");
}

QString JsonBridge::highlightXml(const QString &input)
{
#ifdef __EMSCRIPTEN__
    try {
        val window = val::global("window");
        val jsonBridge = window["JsonBridge"];

        if (jsonBridge.isUndefined() || jsonBridge.isNull()) {
            // Return escaped HTML if bridge not available
            QString escaped = input;
            escaped.replace("&", "&amp;");
            escaped.replace("<", "&lt;");
            escaped.replace(">", "&gt;");
            return escaped;
        }

        std::string inputStd = input.toStdString();
        std::string result = jsonBridge.call<std::string>("highlightXml", inputStd);
        return QString::fromStdString(result);
    } catch (const std::exception &e) {
        qWarning() << "highlightXml error:" << e.what();
    } catch (...) {
        qWarning() << "Unknown error in highlightXml";
    }
    // Fallback: return escaped HTML
    QString escaped = input;
    escaped.replace("&", "&amp;");
    escaped.replace("<", "&lt;");
    escaped.replace(">", "&gt;");
    return escaped;
#else
    // Desktop: XML highlighting not available (requires Rust WASM)
    // Return escaped HTML as fallback
    QString escaped = input;
    escaped.replace("&", "&amp;");
    escaped.replace("<", "&lt;");
    escaped.replace(">", "&gt;");
    return "<pre style=\"margin:0; font-family:monospace; white-space:pre-wrap;\">" + escaped + "</pre>";
#endif
}
