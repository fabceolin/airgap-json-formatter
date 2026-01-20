#include "jsonbridge.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

#ifdef __EMSCRIPTEN__
#include <emscripten/val.h>
using namespace emscripten;
#endif

JsonBridge::JsonBridge(QObject *parent)
    : QObject(parent)
{
    checkReady();
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
    m_ready = false;
#endif
    emit readyChanged();
}

bool JsonBridge::isReady() const
{
    return m_ready;
}

QVariantMap JsonBridge::formatJson(const QString &input, const QString &indentType)
{
    QVariantMap result;
    result["success"] = false;

#ifdef __EMSCRIPTEN__
    try {
        val window = val::global("window");
        val jsonBridge = window["JsonBridge"];

        if (jsonBridge.isUndefined() || jsonBridge.isNull()) {
            result["error"] = "JsonBridge not available";
            return result;
        }

        std::string inputStd = input.toStdString();
        std::string indentStd = indentType.toStdString();

        val jsResult = jsonBridge.call<val>("formatJson", inputStd, indentStd);

        bool success = jsResult["success"].as<bool>();
        result["success"] = success;

        if (success) {
            std::string resultStr = jsResult["result"].as<std::string>();
            result["result"] = QString::fromStdString(resultStr);
        } else {
            std::string errorStr = jsResult["error"].as<std::string>();
            result["error"] = QString::fromStdString(errorStr);
        }
    } catch (const std::exception &e) {
        result["error"] = QString("Exception: %1").arg(e.what());
    } catch (...) {
        result["error"] = "Unknown error in formatJson";
    }
#else
    result["error"] = "Not running in WebAssembly";
#endif

    return result;
}

QVariantMap JsonBridge::minifyJson(const QString &input)
{
    QVariantMap result;
    result["success"] = false;

#ifdef __EMSCRIPTEN__
    try {
        val window = val::global("window");
        val jsonBridge = window["JsonBridge"];

        if (jsonBridge.isUndefined() || jsonBridge.isNull()) {
            result["error"] = "JsonBridge not available";
            return result;
        }

        std::string inputStd = input.toStdString();

        val jsResult = jsonBridge.call<val>("minifyJson", inputStd);

        bool success = jsResult["success"].as<bool>();
        result["success"] = success;

        if (success) {
            std::string resultStr = jsResult["result"].as<std::string>();
            result["result"] = QString::fromStdString(resultStr);
        } else {
            std::string errorStr = jsResult["error"].as<std::string>();
            result["error"] = QString::fromStdString(errorStr);
        }
    } catch (const std::exception &e) {
        result["error"] = QString("Exception: %1").arg(e.what());
    } catch (...) {
        result["error"] = "Unknown error in minifyJson";
    }
#else
    result["error"] = "Not running in WebAssembly";
#endif

    return result;
}

QVariantMap JsonBridge::validateJson(const QString &input)
{
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
            return result;
        }

        std::string inputStd = input.toStdString();

        val jsResult = jsonBridge.call<val>("validateJson", inputStd);

        bool isValid = jsResult["isValid"].as<bool>();
        result["isValid"] = isValid;

        if (isValid) {
            QVariantMap stats;
            val jsStats = jsResult["stats"];
            if (!jsStats.isUndefined()) {
                stats["object_count"] = jsStats["object_count"].isUndefined() ? 0 : jsStats["object_count"].as<int>();
                stats["array_count"] = jsStats["array_count"].isUndefined() ? 0 : jsStats["array_count"].as<int>();
                stats["string_count"] = jsStats["string_count"].isUndefined() ? 0 : jsStats["string_count"].as<int>();
                stats["number_count"] = jsStats["number_count"].isUndefined() ? 0 : jsStats["number_count"].as<int>();
                stats["boolean_count"] = jsStats["boolean_count"].isUndefined() ? 0 : jsStats["boolean_count"].as<int>();
                stats["null_count"] = jsStats["null_count"].isUndefined() ? 0 : jsStats["null_count"].as<int>();
                stats["total_keys"] = jsStats["total_keys"].isUndefined() ? 0 : jsStats["total_keys"].as<int>();
                stats["max_depth"] = jsStats["max_depth"].isUndefined() ? 0 : jsStats["max_depth"].as<int>();
            }
            result["stats"] = stats;
        } else {
            val jsError = jsResult["error"];
            QVariantMap error;
            error["message"] = jsError["message"].isUndefined() ? QString("Unknown error") : QString::fromStdString(jsError["message"].as<std::string>());
            error["line"] = jsError["line"].isUndefined() ? 0 : jsError["line"].as<int>();
            error["column"] = jsError["column"].isUndefined() ? 0 : jsError["column"].as<int>();
            result["error"] = error;
            result["stats"] = QVariantMap();
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
    QVariantMap error;
    error["message"] = "Not running in WebAssembly";
    error["line"] = 0;
    error["column"] = 0;
    result["error"] = error;
    result["stats"] = QVariantMap();
#endif

    return result;
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
#endif
    // Fallback: return escaped HTML
    QString escaped = input;
    escaped.replace("&", "&amp;");
    escaped.replace("<", "&lt;");
    escaped.replace(">", "&gt;");
    return escaped;
}

void JsonBridge::copyToClipboard(const QString &text)
{
#ifdef __EMSCRIPTEN__
    try {
        val window = val::global("window");
        val jsonBridge = window["JsonBridge"];

        if (!jsonBridge.isUndefined() && !jsonBridge.isNull()) {
            std::string textStd = text.toStdString();
            jsonBridge.call<void>("copyToClipboard", textStd);
        }
    } catch (...) {
        qWarning() << "Failed to copy to clipboard";
    }
#else
    qDebug() << "Clipboard copy not available outside WebAssembly";
#endif
}

QString JsonBridge::readFromClipboard()
{
#ifdef __EMSCRIPTEN__
    try {
        val window = val::global("window");
        val jsonBridge = window["JsonBridge"];

        if (!jsonBridge.isUndefined() && !jsonBridge.isNull()) {
            // Note: This is a synchronous call to an async JS function
            // The JS side handles the promise internally
            val promise = jsonBridge.call<val>("readFromClipboard");
            val result = promise.await();
            if (!result.isUndefined() && !result.isNull()) {
                std::string text = result.as<std::string>();
                return QString::fromStdString(text);
            }
        }
    } catch (const std::exception &e) {
        qWarning() << "Failed to read from clipboard:" << e.what();
    } catch (...) {
        qWarning() << "Failed to read from clipboard";
    }
#else
    qDebug() << "Clipboard read not available outside WebAssembly";
#endif
    return QString();
}
