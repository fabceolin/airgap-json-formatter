// Theme.qml unit tests (Story 10.7)
// Tests: Property existence, contrast ratios, theme toggle reactivity, Mermaid theme structure

#include <QtTest>
#include <QtQuick>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QColor>
#include <cmath>

class TestTheme : public QObject
{
    Q_OBJECT

private:
    QQmlEngine *m_engine = nullptr;
    QObject *m_theme = nullptr;

    // WCAG relative luminance calculation
    // See: https://www.w3.org/WAI/GL/wiki/Relative_luminance
    double relativeLuminance(const QColor &color) {
        auto sRGBtoLinear = [](double channel) {
            double c = channel / 255.0;
            return c <= 0.03928 ? c / 12.92 : std::pow((c + 0.055) / 1.055, 2.4);
        };

        double r = sRGBtoLinear(color.red());
        double g = sRGBtoLinear(color.green());
        double b = sRGBtoLinear(color.blue());

        return 0.2126 * r + 0.7152 * g + 0.0722 * b;
    }

    // WCAG contrast ratio calculation
    // See: https://www.w3.org/WAI/GL/wiki/Contrast_ratio
    double contrastRatio(const QColor &fg, const QColor &bg) {
        double l1 = relativeLuminance(fg);
        double l2 = relativeLuminance(bg);

        double lighter = std::max(l1, l2);
        double darker = std::min(l1, l2);

        return (lighter + 0.05) / (darker + 0.05);
    }

    QColor getColorProperty(const QString &name) {
        return m_theme->property(name.toLatin1().constData()).value<QColor>();
    }

    bool getBoolProperty(const QString &name) {
        return m_theme->property(name.toLatin1().constData()).toBool();
    }

    QVariant getProperty(const QString &name) {
        return m_theme->property(name.toLatin1().constData());
    }

private slots:
    void initTestCase()
    {
        m_engine = new QQmlEngine(this);

        // Add QML import path
        m_engine->addImportPath(QML_PATH);

        // Register Theme singleton
        qmlRegisterSingletonType(QUrl::fromLocalFile(QString(QML_PATH) + "/Theme.qml"), "Theme", 1, 0, "Theme");

        QQmlComponent component(m_engine, QUrl::fromLocalFile(QString(QML_PATH) + "/Theme.qml"));
        if (component.isError()) {
            qWarning() << "Component errors:" << component.errors();
        }
        QVERIFY2(!component.isError(), "Theme.qml should load without errors");

        m_theme = component.create();
        QVERIFY2(m_theme != nullptr, "Theme singleton should be created");
    }

    void cleanupTestCase()
    {
        delete m_theme;
        delete m_engine;
    }

    // ========================================
    // AC1: Markdown Syntax Colors Exist
    // ========================================

    void testSyntaxMdPropertiesExist()
    {
        QStringList syntaxMdProps = {
            "syntaxMdHeading",
            "syntaxMdBold",
            "syntaxMdItalic",
            "syntaxMdStrike",
            "syntaxMdLink",
            "syntaxMdUrl",
            "syntaxMdCodeInlineBg",
            "syntaxMdCodeBlockBg",
            "syntaxMdCodeBorder",
            "syntaxMdBlockquote",
            "syntaxMdBlockquoteBorder",
            "syntaxMdListMarker",
            "syntaxMdMermaidBg",
            "syntaxMdMermaidBorder"
        };

        for (const QString &prop : syntaxMdProps) {
            QVariant value = getProperty(prop);
            QVERIFY2(value.isValid(), qPrintable(QString("%1 property should exist").arg(prop)));
            QVERIFY2(value.canConvert<QColor>(), qPrintable(QString("%1 should be a color type").arg(prop)));
        }
    }

    void testSyntaxMdHeadingValues()
    {
        // Dark mode: #569cd6, Light mode: #0066cc
        m_theme->setProperty("darkMode", true);
        QColor darkValue = getColorProperty("syntaxMdHeading");
        QCOMPARE(darkValue.name(QColor::HexRgb).toLower(), QString("#569cd6"));

        m_theme->setProperty("darkMode", false);
        QColor lightValue = getColorProperty("syntaxMdHeading");
        QCOMPARE(lightValue.name(QColor::HexRgb).toLower(), QString("#0066cc"));

        // Reset to dark mode
        m_theme->setProperty("darkMode", true);
    }

    void testSyntaxMdLinkUrlDistinct()
    {
        // Link and URL colors should be visually distinct
        QColor link = getColorProperty("syntaxMdLink");
        QColor url = getColorProperty("syntaxMdUrl");
        QVERIFY2(link != url, "syntaxMdLink and syntaxMdUrl should be different colors");
    }

    void testSyntaxMdCodeBgDistinct()
    {
        // Inline and block code backgrounds should be distinct
        QColor inlineBg = getColorProperty("syntaxMdCodeInlineBg");
        QColor blockBg = getColorProperty("syntaxMdCodeBlockBg");
        QVERIFY2(inlineBg != blockBg, "syntaxMdCodeInlineBg and syntaxMdCodeBlockBg should differ");
    }

    // ========================================
    // AC2: Preview Pane Colors Exist
    // ========================================

    void testPreviewPropertiesExist()
    {
        QStringList previewProps = {
            "previewBackground",
            "previewText",
            "previewHeading",
            "previewLink",
            "previewCodeBg",
            "previewCodeText",
            "previewTableBorder",
            "previewTableHeaderBg",
            "previewBlockquoteBorder",
            "previewHrColor"
        };

        for (const QString &prop : previewProps) {
            QVariant value = getProperty(prop);
            QVERIFY2(value.isValid(), qPrintable(QString("%1 property should exist").arg(prop)));
            QVERIFY2(value.canConvert<QColor>(), qPrintable(QString("%1 should be a color type").arg(prop)));
        }
    }

    // ========================================
    // AC3: Mermaid Theme Configuration
    // ========================================

    void testMermaidThemePropertiesExist()
    {
        QVERIFY2(getProperty("mermaidTheme").isValid(), "mermaidTheme should exist");
        QVERIFY2(getProperty("mermaidDarkTheme").isValid(), "mermaidDarkTheme should exist");
        QVERIFY2(getProperty("mermaidLightTheme").isValid(), "mermaidLightTheme should exist");
    }

    void testMermaidDarkThemeStructure()
    {
        QVariant darkThemeVar = getProperty("mermaidDarkTheme");
        QVERIFY2(darkThemeVar.canConvert<QVariantMap>(), "mermaidDarkTheme should be an object");

        QVariantMap darkTheme = darkThemeVar.toMap();
        QCOMPARE(darkTheme["theme"].toString(), QString("dark"));
        QVERIFY2(darkTheme.contains("themeVariables"), "should have themeVariables");

        QVariantMap vars = darkTheme["themeVariables"].toMap();
        QStringList requiredKeys = {
            "primaryColor", "primaryTextColor", "primaryBorderColor",
            "lineColor", "secondaryColor", "tertiaryColor", "background",
            "mainBkg", "nodeBorder", "titleColor", "nodeTextColor"
        };

        for (const QString &key : requiredKeys) {
            QVERIFY2(vars.contains(key), qPrintable(QString("themeVariables should contain %1").arg(key)));
        }
    }

    void testMermaidLightThemeStructure()
    {
        QVariant lightThemeVar = getProperty("mermaidLightTheme");
        QVERIFY2(lightThemeVar.canConvert<QVariantMap>(), "mermaidLightTheme should be an object");

        QVariantMap lightTheme = lightThemeVar.toMap();
        QCOMPARE(lightTheme["theme"].toString(), QString("default"));
        QVERIFY2(lightTheme.contains("themeVariables"), "should have themeVariables");

        QVariantMap vars = lightTheme["themeVariables"].toMap();
        QStringList requiredKeys = {
            "primaryColor", "primaryTextColor", "primaryBorderColor",
            "lineColor", "secondaryColor", "tertiaryColor", "background",
            "mainBkg", "nodeBorder", "titleColor", "nodeTextColor"
        };

        for (const QString &key : requiredKeys) {
            QVERIFY2(vars.contains(key), qPrintable(QString("themeVariables should contain %1").arg(key)));
        }
    }

    void testMermaidThemeSwitches()
    {
        m_theme->setProperty("darkMode", true);
        QVariantMap darkTheme = getProperty("mermaidTheme").toMap();
        QCOMPARE(darkTheme["theme"].toString(), QString("dark"));

        m_theme->setProperty("darkMode", false);
        QVariantMap lightTheme = getProperty("mermaidTheme").toMap();
        QCOMPARE(lightTheme["theme"].toString(), QString("default"));

        m_theme->setProperty("darkMode", true);
    }

    // ========================================
    // AC4: WCAG AA Contrast Ratios (4.5:1)
    // ========================================

    void testDarkModeContrastRatios()
    {
        m_theme->setProperty("darkMode", true);
        QColor bg = getColorProperty("background");
        QColor previewBg = getColorProperty("previewBackground");
        QColor codeBg = getColorProperty("previewCodeBg");

        // Each pair should have >= 4.5:1 contrast ratio for WCAG AA
        struct ColorPair {
            QString fgName;
            QColor fg;
            QString bgName;
            QColor bg;
        };

        QList<ColorPair> pairs = {
            {"syntaxMdHeading", getColorProperty("syntaxMdHeading"), "background", bg},
            {"syntaxMdLink", getColorProperty("syntaxMdLink"), "background", bg},
            {"syntaxMdUrl", getColorProperty("syntaxMdUrl"), "background", bg},
            {"syntaxMdStrike", getColorProperty("syntaxMdStrike"), "background", bg},
            {"syntaxMdBlockquote", getColorProperty("syntaxMdBlockquote"), "background", bg},
            {"syntaxMdListMarker", getColorProperty("syntaxMdListMarker"), "background", bg},
            {"previewText", getColorProperty("previewText"), "previewBackground", previewBg},
            {"previewHeading", getColorProperty("previewHeading"), "previewBackground", previewBg},
            {"previewLink", getColorProperty("previewLink"), "previewBackground", previewBg},
            {"previewCodeText", getColorProperty("previewCodeText"), "previewCodeBg", codeBg},
        };

        const double minRatio = 4.5;

        for (const ColorPair &pair : pairs) {
            double ratio = contrastRatio(pair.fg, pair.bg);
            QVERIFY2(ratio >= minRatio,
                qPrintable(QString("Dark mode: %1 on %2 contrast ratio %.2f < %.1f (WCAG AA)")
                    .arg(pair.fgName).arg(pair.bgName).arg(ratio).arg(minRatio)));
        }
    }

    void testLightModeContrastRatios()
    {
        m_theme->setProperty("darkMode", false);
        QColor bg = getColorProperty("background");
        QColor previewBg = getColorProperty("previewBackground");
        QColor codeBg = getColorProperty("previewCodeBg");

        struct ColorPair {
            QString fgName;
            QColor fg;
            QString bgName;
            QColor bg;
        };

        QList<ColorPair> pairs = {
            {"syntaxMdHeading", getColorProperty("syntaxMdHeading"), "background", bg},
            {"syntaxMdLink", getColorProperty("syntaxMdLink"), "background", bg},
            {"syntaxMdUrl", getColorProperty("syntaxMdUrl"), "background", bg},
            {"syntaxMdStrike", getColorProperty("syntaxMdStrike"), "background", bg},
            {"syntaxMdBlockquote", getColorProperty("syntaxMdBlockquote"), "background", bg},
            {"syntaxMdListMarker", getColorProperty("syntaxMdListMarker"), "background", bg},
            {"previewText", getColorProperty("previewText"), "previewBackground", previewBg},
            {"previewHeading", getColorProperty("previewHeading"), "previewBackground", previewBg},
            {"previewLink", getColorProperty("previewLink"), "previewBackground", previewBg},
            {"previewCodeText", getColorProperty("previewCodeText"), "previewCodeBg", codeBg},
        };

        const double minRatio = 4.5;

        for (const ColorPair &pair : pairs) {
            double ratio = contrastRatio(pair.fg, pair.bg);
            QVERIFY2(ratio >= minRatio,
                qPrintable(QString("Light mode: %1 on %2 contrast ratio %.2f < %.1f (WCAG AA)")
                    .arg(pair.fgName).arg(pair.bgName).arg(ratio).arg(minRatio)));
        }

        // Reset to dark mode
        m_theme->setProperty("darkMode", true);
    }

    void testMermaidThemeContrastRatios()
    {
        // Test dark theme Mermaid contrast
        QVariantMap darkVars = getProperty("mermaidDarkTheme").toMap()["themeVariables"].toMap();
        QColor darkPrimaryText = QColor(darkVars["primaryTextColor"].toString());
        QColor darkPrimaryBg = QColor(darkVars["primaryColor"].toString());

        double darkRatio = contrastRatio(darkPrimaryText, darkPrimaryBg);
        QVERIFY2(darkRatio >= 4.5,
            qPrintable(QString("Mermaid dark: primaryTextColor on primaryColor ratio %.2f < 4.5").arg(darkRatio)));

        // Test light theme Mermaid contrast
        QVariantMap lightVars = getProperty("mermaidLightTheme").toMap()["themeVariables"].toMap();
        QColor lightPrimaryText = QColor(lightVars["primaryTextColor"].toString());
        QColor lightPrimaryBg = QColor(lightVars["primaryColor"].toString());

        double lightRatio = contrastRatio(lightPrimaryText, lightPrimaryBg);
        QVERIFY2(lightRatio >= 4.5,
            qPrintable(QString("Mermaid light: primaryTextColor on primaryColor ratio %.2f < 4.5").arg(lightRatio)));
    }

    // ========================================
    // AC7: Theme Toggle Reactivity
    // ========================================

    void testAllPropertiesRespondToToggle()
    {
        QStringList newProps = {
            "syntaxMdHeading", "syntaxMdStrike", "syntaxMdLink", "syntaxMdUrl",
            "syntaxMdCodeInlineBg", "syntaxMdCodeBlockBg", "syntaxMdCodeBorder",
            "syntaxMdBlockquote", "syntaxMdBlockquoteBorder", "syntaxMdListMarker",
            "syntaxMdMermaidBg", "syntaxMdMermaidBorder",
            "previewBackground", "previewText", "previewHeading", "previewLink",
            "previewCodeBg", "previewCodeText", "previewTableBorder",
            "previewTableHeaderBg", "previewBlockquoteBorder", "previewHrColor"
        };

        m_theme->setProperty("darkMode", true);
        QMap<QString, QColor> darkValues;
        for (const QString &prop : newProps) {
            darkValues[prop] = getColorProperty(prop);
        }

        m_theme->setProperty("darkMode", false);
        QMap<QString, QColor> lightValues;
        for (const QString &prop : newProps) {
            lightValues[prop] = getColorProperty(prop);
        }

        // At least some colors should change between modes (not all need to, e.g. accent colors)
        int changedCount = 0;
        for (const QString &prop : newProps) {
            if (darkValues[prop] != lightValues[prop]) {
                changedCount++;
            }
        }

        QVERIFY2(changedCount > 0, "At least some properties should change between dark and light modes");

        // Reset to dark
        m_theme->setProperty("darkMode", true);
    }

    void testRapidToggleStability()
    {
        // Toggle 10 times rapidly and verify no crashes or undefined values
        for (int i = 0; i < 10; i++) {
            m_theme->setProperty("darkMode", !getBoolProperty("darkMode"));

            // Verify critical properties are still valid
            QVERIFY(getProperty("syntaxMdHeading").isValid());
            QVERIFY(getProperty("previewText").isValid());
            QVERIFY(getProperty("mermaidTheme").isValid());
        }

        // Ensure we're back to a known state
        m_theme->setProperty("darkMode", true);
    }
};

QTEST_MAIN(TestTheme)
#include "tst_theme.moc"
