#ifndef THEME_H
#define THEME_H

#include <QObject>
#include <QColor>
#include <QString>
#include <QtQml/qqmlregistration.h>
#include <QQmlEngine>
#include <QJSEngine>

class Theme : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    // Theme mode
    Q_PROPERTY(bool darkMode READ darkMode WRITE setDarkMode NOTIFY darkModeChanged)

    // App version
    Q_PROPERTY(QString appVersion READ appVersion CONSTANT)

    // Background colors
    Q_PROPERTY(QColor background READ background NOTIFY themeChanged)
    Q_PROPERTY(QColor backgroundSecondary READ backgroundSecondary NOTIFY themeChanged)
    Q_PROPERTY(QColor backgroundTertiary READ backgroundTertiary NOTIFY themeChanged)

    // Text colors
    Q_PROPERTY(QColor textPrimary READ textPrimary NOTIFY themeChanged)
    Q_PROPERTY(QColor textSecondary READ textSecondary NOTIFY themeChanged)
    Q_PROPERTY(QColor textError READ textError NOTIFY themeChanged)
    Q_PROPERTY(QColor textSuccess READ textSuccess NOTIFY themeChanged)

    // Accent colors
    Q_PROPERTY(QColor accent READ accent NOTIFY themeChanged)
    Q_PROPERTY(QColor border READ border NOTIFY themeChanged)
    Q_PROPERTY(QColor splitHandle READ splitHandle NOTIFY themeChanged)

    // Focus indicators
    Q_PROPERTY(QColor focusRing READ focusRing NOTIFY themeChanged)
    Q_PROPERTY(int focusRingWidth READ focusRingWidth CONSTANT)

    // Typography
    Q_PROPERTY(QString monoFont READ monoFont CONSTANT)
    Q_PROPERTY(int monoFontSize READ monoFontSize CONSTANT)

    // Badge colors
    Q_PROPERTY(QColor badgeSuccessBg READ badgeSuccessBg NOTIFY themeChanged)
    Q_PROPERTY(QColor badgeSuccessBorder READ badgeSuccessBorder NOTIFY themeChanged)
    Q_PROPERTY(QColor badgeErrorBg READ badgeErrorBg NOTIFY themeChanged)
    Q_PROPERTY(QColor badgeErrorBorder READ badgeErrorBorder NOTIFY themeChanged)

    // Syntax highlighting colors
    Q_PROPERTY(QColor syntaxKey READ syntaxKey NOTIFY themeChanged)
    Q_PROPERTY(QColor syntaxString READ syntaxString NOTIFY themeChanged)
    Q_PROPERTY(QColor syntaxNumber READ syntaxNumber NOTIFY themeChanged)
    Q_PROPERTY(QColor syntaxBoolean READ syntaxBoolean NOTIFY themeChanged)
    Q_PROPERTY(QColor syntaxNull READ syntaxNull NOTIFY themeChanged)
    Q_PROPERTY(QColor syntaxPunctuation READ syntaxPunctuation NOTIFY themeChanged)
    Q_PROPERTY(QColor syntaxBadge READ syntaxBadge NOTIFY themeChanged)

public:
    explicit Theme(QObject *parent = nullptr);

    // Factory method for QML singleton
    static Theme* create(QQmlEngine *qmlEngine, QJSEngine *jsEngine)
    {
        Q_UNUSED(qmlEngine);
        Q_UNUSED(jsEngine);
        static Theme* instance = new Theme();
        return instance;
    }

    // Theme mode
    bool darkMode() const { return m_darkMode; }
    void setDarkMode(bool dark);

    // App version
    QString appVersion() const { return QStringLiteral("0.1.3"); }

    // Background colors
    QColor background() const { return m_darkMode ? QColor("#1e1e1e") : QColor("#f5f5f5"); }
    QColor backgroundSecondary() const { return m_darkMode ? QColor("#252526") : QColor("#ffffff"); }
    QColor backgroundTertiary() const { return m_darkMode ? QColor("#2d2d2d") : QColor("#e8e8e8"); }

    // Text colors
    QColor textPrimary() const { return m_darkMode ? QColor("#d4d4d4") : QColor("#1e1e1e"); }
    QColor textSecondary() const { return m_darkMode ? QColor("#808080") : QColor("#6e6e6e"); }
    QColor textError() const { return m_darkMode ? QColor("#f44747") : QColor("#d32f2f"); }
    QColor textSuccess() const { return m_darkMode ? QColor("#4ec9b0") : QColor("#2e7d32"); }

    // Accent colors
    QColor accent() const { return m_darkMode ? QColor("#0078d4") : QColor("#0066cc"); }
    QColor border() const { return m_darkMode ? QColor("#3c3c3c") : QColor("#d0d0d0"); }
    QColor splitHandle() const { return m_darkMode ? QColor("#505050") : QColor("#c0c0c0"); }

    // Focus indicators
    QColor focusRing() const { return m_darkMode ? QColor("#0078d4") : QColor("#0066cc"); }
    int focusRingWidth() const { return 2; }

    // Typography
    QString monoFont() const { return QStringLiteral("Consolas, Monaco, 'Courier New', monospace"); }
    int monoFontSize() const { return 14; }

    // Badge colors
    QColor badgeSuccessBg() const { return m_darkMode ? QColor("#1a3a1a") : QColor("#e6f4ea"); }
    QColor badgeSuccessBorder() const { return m_darkMode ? QColor("#2d5a2d") : QColor("#34a853"); }
    QColor badgeErrorBg() const { return m_darkMode ? QColor("#4a2d2d") : QColor("#fce8e6"); }
    QColor badgeErrorBorder() const { return m_darkMode ? QColor("#5a3d3d") : QColor("#ea4335"); }

    // Syntax highlighting colors
    // Dark: base16-ocean.dark theme
    // Light: GitHub light theme inspired
    QColor syntaxKey() const { return m_darkMode ? QColor("#8fa1b3") : QColor("#005cc5"); }
    QColor syntaxString() const { return m_darkMode ? QColor("#a3be8c") : QColor("#22863a"); }
    QColor syntaxNumber() const { return m_darkMode ? QColor("#d08770") : QColor("#e36209"); }
    QColor syntaxBoolean() const { return m_darkMode ? QColor("#b48ead") : QColor("#6f42c1"); }
    QColor syntaxNull() const { return m_darkMode ? QColor("#bf616a") : QColor("#d73a49"); }
    QColor syntaxPunctuation() const { return m_darkMode ? QColor("#c0c5ce") : QColor("#586069"); }
    QColor syntaxBadge() const { return m_darkMode ? QColor("#65737e") : QColor("#959da5"); }

    // Detect system preference
    Q_INVOKABLE bool systemPrefersDark() const;

    // Toggle theme
    Q_INVOKABLE void toggleTheme();

signals:
    void darkModeChanged();
    void themeChanged();

private:
    bool m_darkMode = true;
};

#endif // THEME_H
