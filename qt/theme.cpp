#include "theme.h"
#include <QGuiApplication>
#include <QStyleHints>
#include <QSettings>

Theme::Theme(QObject *parent)
    : QObject(parent)
{
    // Load saved preference or use system default
    QSettings settings;
    if (settings.contains("theme/darkMode")) {
        m_darkMode = settings.value("theme/darkMode").toBool();
    } else {
        // Use system preference as default
        m_darkMode = systemPrefersDark();
    }
}

void Theme::setDarkMode(bool dark)
{
    if (m_darkMode == dark)
        return;

    m_darkMode = dark;

    // Save preference
    QSettings settings;
    settings.setValue("theme/darkMode", m_darkMode);

    emit darkModeChanged();
    emit themeChanged();
}

bool Theme::systemPrefersDark() const
{
    // Qt 6.5+ has colorScheme() in QStyleHints
    auto hints = QGuiApplication::styleHints();
    if (hints) {
        // colorScheme returns Qt::ColorScheme enum (Dark, Light, Unknown)
        return hints->colorScheme() == Qt::ColorScheme::Dark;
    }
    // Default to dark if detection fails
    return true;
}

void Theme::toggleTheme()
{
    setDarkMode(!m_darkMode);
}
