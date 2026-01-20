#ifndef THEME_H
#define THEME_H

#include <QObject>
#include <QColor>
#include <QString>

class Theme : public QObject
{
    Q_OBJECT

    // Background colors
    Q_PROPERTY(QColor background READ background CONSTANT)
    Q_PROPERTY(QColor backgroundSecondary READ backgroundSecondary CONSTANT)
    Q_PROPERTY(QColor backgroundTertiary READ backgroundTertiary CONSTANT)

    // Text colors
    Q_PROPERTY(QColor textPrimary READ textPrimary CONSTANT)
    Q_PROPERTY(QColor textSecondary READ textSecondary CONSTANT)
    Q_PROPERTY(QColor textError READ textError CONSTANT)
    Q_PROPERTY(QColor textSuccess READ textSuccess CONSTANT)

    // Accent colors
    Q_PROPERTY(QColor accent READ accent CONSTANT)
    Q_PROPERTY(QColor border READ border CONSTANT)
    Q_PROPERTY(QColor splitHandle READ splitHandle CONSTANT)

    // Focus indicators
    Q_PROPERTY(QColor focusRing READ focusRing CONSTANT)
    Q_PROPERTY(int focusRingWidth READ focusRingWidth CONSTANT)

    // Typography
    Q_PROPERTY(QString monoFont READ monoFont CONSTANT)
    Q_PROPERTY(int monoFontSize READ monoFontSize CONSTANT)

public:
    explicit Theme(QObject *parent = nullptr) : QObject(parent) {}

    // Background colors
    QColor background() const { return QColor("#1e1e1e"); }
    QColor backgroundSecondary() const { return QColor("#252526"); }
    QColor backgroundTertiary() const { return QColor("#2d2d2d"); }

    // Text colors
    QColor textPrimary() const { return QColor("#d4d4d4"); }
    QColor textSecondary() const { return QColor("#808080"); }
    QColor textError() const { return QColor("#f44747"); }
    QColor textSuccess() const { return QColor("#4ec9b0"); }

    // Accent colors
    QColor accent() const { return QColor("#0078d4"); }
    QColor border() const { return QColor("#3c3c3c"); }
    QColor splitHandle() const { return QColor("#505050"); }

    // Focus indicators
    QColor focusRing() const { return QColor("#0078d4"); }
    int focusRingWidth() const { return 2; }

    // Typography
    QString monoFont() const { return QStringLiteral("Consolas, Monaco, 'Courier New', monospace"); }
    int monoFontSize() const { return 14; }
};

#endif // THEME_H
