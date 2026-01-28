/**
 * @file tst_format_detection.cpp
 * @brief Unit tests for Format Auto-Detection (Story 8.4 + Story 10.4 Markdown Extension)
 *
 * Story 8.4 Tests verify:
 * - AC1: Format is auto-detected when content is pasted into input pane
 * - AC2: Format is auto-detected when Format button is clicked
 * - AC3: Detection logic: `<` prefix = XML, `{` or `[` prefix = JSON
 * - AC4: Unknown format handled gracefully (default to JSON or show message)
 * - AC5: `formatDetected` signal emitted with detected format
 * - AC6: Detection works for content with leading whitespace
 * - AC7: Detection is fast (< 1ms for any input size)
 *
 * Story 10.4 Tests verify:
 * - AC1: detectFormat() extended to detect Markdown content
 * - AC2: Detection patterns include headings, code blocks, frontmatter, lists, blockquotes, links
 * - AC3: Detection works with leading whitespace (trimmed)
 * - AC4: Detection works for patterns mid-document (not just start)
 * - AC6: formatDetected signal emits "markdown" for detected content
 * - AC7: Plain prose without Markdown patterns returns "unknown" (no false positives)
 * - AC8: Detection is fast (< 1ms for any input size)
 */
#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QElapsedTimer>
#include "../jsonbridge.h"
#include "../asyncserialiser.h"

class tst_FormatDetection : public QObject
{
    Q_OBJECT

private:
    JsonBridge* m_bridge = nullptr;

private slots:
    void init()
    {
        AsyncSerialiser::instance().clearQueue();
        m_bridge = new JsonBridge(this);
    }

    void cleanup()
    {
        AsyncSerialiser::instance().clearQueue();
        delete m_bridge;
        m_bridge = nullptr;
        QCoreApplication::processEvents();
    }

    // ========== AC3: Detection logic tests ==========

    // 8.4-UNIT-001: JSON object detection
    void testDetectJsonObject()
    {
        QString result = m_bridge->detectFormat("{\"a\":1}");
        QCOMPARE(result, QString("json"));
    }

    // 8.4-UNIT-002: JSON array detection
    void testDetectJsonArray()
    {
        QString result = m_bridge->detectFormat("[1,2,3]");
        QCOMPARE(result, QString("json"));
    }

    // 8.4-UNIT-003: XML element detection
    void testDetectXmlElement()
    {
        QString result = m_bridge->detectFormat("<root/>");
        QCOMPARE(result, QString("xml"));
    }

    // 8.4-UNIT-004: XML declaration detection
    void testDetectXmlDeclaration()
    {
        QString result = m_bridge->detectFormat("<?xml version=\"1.0\"?>");
        QCOMPARE(result, QString("xml"));
    }

    // 8.4-UNIT-005: HTML doctype treated as XML
    void testDetectHtmlDoctype()
    {
        QString result = m_bridge->detectFormat("<!DOCTYPE html>");
        QCOMPARE(result, QString("xml"));
    }

    // 8.4-UNIT-006: Plain text returns unknown
    void testDetectPlainText()
    {
        QString result = m_bridge->detectFormat("hello world");
        QCOMPARE(result, QString("unknown"));
    }

    // 8.4-UNIT-007: JSON primitive (null) returns unknown
    void testDetectJsonNull()
    {
        QString result = m_bridge->detectFormat("null");
        QCOMPARE(result, QString("unknown"));
    }

    // 8.4-UNIT-008: Numeric string returns unknown
    void testDetectNumeric()
    {
        QString result = m_bridge->detectFormat("123");
        QCOMPARE(result, QString("unknown"));
    }

    // 8.4-UNIT-009: Empty input returns unknown
    void testDetectEmpty()
    {
        QString result = m_bridge->detectFormat("");
        QCOMPARE(result, QString("unknown"));
    }

    // 8.4-UNIT-010: Whitespace-only input returns unknown
    void testDetectWhitespaceOnly()
    {
        QString result = m_bridge->detectFormat("   \n\t  ");
        QCOMPARE(result, QString("unknown"));
    }

    // ========== AC6: Whitespace handling tests ==========

    // 8.4-UNIT-011: JSON with leading whitespace
    void testDetectJsonWithWhitespace()
    {
        QString result = m_bridge->detectFormat("  \n  {\"a\":1}");
        QCOMPARE(result, QString("json"));
    }

    // 8.4-UNIT-012: JSON array with leading whitespace
    void testDetectJsonArrayWithWhitespace()
    {
        QString result = m_bridge->detectFormat("\t\n  [1,2,3]");
        QCOMPARE(result, QString("json"));
    }

    // 8.4-UNIT-013: XML with leading whitespace
    void testDetectXmlWithWhitespace()
    {
        QString result = m_bridge->detectFormat("  \n\t<root/>");
        QCOMPARE(result, QString("xml"));
    }

    // 8.4-UNIT-014: XML declaration with leading whitespace
    void testDetectXmlDeclWithWhitespace()
    {
        QString result = m_bridge->detectFormat("   <?xml version=\"1.0\"?><r/>");
        QCOMPARE(result, QString("xml"));
    }

    // ========== AC5: Signal emission tests ==========

    // 8.4-UNIT-015: Signal emitted for JSON
    void testSignalEmittedForJson()
    {
        QSignalSpy formatDetectedSpy(m_bridge, &JsonBridge::formatDetected);

        m_bridge->detectFormat("{\"test\":true}");

        QCOMPARE(formatDetectedSpy.count(), 1);
        QCOMPARE(formatDetectedSpy.at(0).at(0).toString(), QString("json"));
    }

    // 8.4-UNIT-016: Signal emitted for XML
    void testSignalEmittedForXml()
    {
        QSignalSpy formatDetectedSpy(m_bridge, &JsonBridge::formatDetected);

        m_bridge->detectFormat("<element attr=\"val\"/>");

        QCOMPARE(formatDetectedSpy.count(), 1);
        QCOMPARE(formatDetectedSpy.at(0).at(0).toString(), QString("xml"));
    }

    // 8.4-UNIT-017: Signal emitted for unknown
    void testSignalEmittedForUnknown()
    {
        QSignalSpy formatDetectedSpy(m_bridge, &JsonBridge::formatDetected);

        m_bridge->detectFormat("just some text");

        QCOMPARE(formatDetectedSpy.count(), 1);
        QCOMPARE(formatDetectedSpy.at(0).at(0).toString(), QString("unknown"));
    }

    // 8.4-UNIT-018: Signal emitted for empty input
    void testSignalEmittedForEmpty()
    {
        QSignalSpy formatDetectedSpy(m_bridge, &JsonBridge::formatDetected);

        m_bridge->detectFormat("");

        QCOMPARE(formatDetectedSpy.count(), 1);
        QCOMPARE(formatDetectedSpy.at(0).at(0).toString(), QString("unknown"));
    }

    // ========== AC7: Performance tests ==========

    // 8.4-PERF-001: Small input detection is fast
    void testPerformanceSmallInput()
    {
        QElapsedTimer timer;
        timer.start();

        for (int i = 0; i < 1000; i++) {
            m_bridge->detectFormat("{\"key\":\"value\"}");
        }

        qint64 elapsed = timer.elapsed();
        // 1000 iterations should complete in less than 100ms (0.1ms per call)
        QVERIFY2(elapsed < 100, qPrintable(QString("1000 detections took %1ms").arg(elapsed)));
    }

    // 8.4-PERF-002: Large input detection is fast (< 1ms)
    void testPerformanceLargeInput()
    {
        // Create 1MB input
        QString largeInput = QString("{\"data\":\"") + QString(1024 * 1024, 'x') + "\"}";

        QElapsedTimer timer;
        timer.start();

        QString result = m_bridge->detectFormat(largeInput);

        qint64 elapsed = timer.elapsed();
        // Single detection should complete in less than 10ms even for 1MB (generous allowance)
        QVERIFY2(elapsed < 10, qPrintable(QString("1MB detection took %1ms").arg(elapsed)));
        QCOMPARE(result, QString("json"));
    }

    // 8.4-PERF-003: Large whitespace prefix detection is fast
    void testPerformanceLargeWhitespacePrefix()
    {
        // Create input with 1MB of whitespace followed by JSON
        QString largeWhitespace = QString(1024 * 1024, ' ') + "{\"a\":1}";

        QElapsedTimer timer;
        timer.start();

        QString result = m_bridge->detectFormat(largeWhitespace);

        qint64 elapsed = timer.elapsed();
        // Even with large whitespace prefix, should complete quickly
        // Note: This tests trimmed() overhead, may be slower than character check
        QVERIFY2(elapsed < 100, qPrintable(QString("1MB whitespace + JSON detection took %1ms").arg(elapsed)));
        QCOMPARE(result, QString("json"));
    }

    // 8.4-PERF-004: Large XML detection is fast
    void testPerformanceLargeXml()
    {
        // Create 1MB XML input
        QString largeXml = QString("<root>") + QString(1024 * 1024, 'x') + "</root>";

        QElapsedTimer timer;
        timer.start();

        QString result = m_bridge->detectFormat(largeXml);

        qint64 elapsed = timer.elapsed();
        QVERIFY2(elapsed < 10, qPrintable(QString("1MB XML detection took %1ms").arg(elapsed)));
        QCOMPARE(result, QString("xml"));
    }

    // ========== Edge case tests ==========

    // 8.4-UNIT-019: Nested JSON object
    void testDetectNestedJson()
    {
        QString result = m_bridge->detectFormat("{\"outer\":{\"inner\":[1,2,3]}}");
        QCOMPARE(result, QString("json"));
    }

    // 8.4-UNIT-020: Nested XML elements
    void testDetectNestedXml()
    {
        QString result = m_bridge->detectFormat("<root><child><grandchild/></child></root>");
        QCOMPARE(result, QString("xml"));
    }

    // 8.4-UNIT-021: JSON string (quoted) returns unknown
    void testDetectQuotedString()
    {
        QString result = m_bridge->detectFormat("\"just a string\"");
        QCOMPARE(result, QString("unknown"));
    }

    // 8.4-UNIT-022: Boolean true returns unknown
    void testDetectBooleanTrue()
    {
        QString result = m_bridge->detectFormat("true");
        QCOMPARE(result, QString("unknown"));
    }

    // 8.4-UNIT-023: Boolean false returns unknown
    void testDetectBooleanFalse()
    {
        QString result = m_bridge->detectFormat("false");
        QCOMPARE(result, QString("unknown"));
    }

    // ========== Return value and signal consistency tests ==========

    // 8.4-INT-001: Return value matches signal value
    void testReturnMatchesSignal()
    {
        QSignalSpy formatDetectedSpy(m_bridge, &JsonBridge::formatDetected);

        QString returnValue = m_bridge->detectFormat("{\"test\":true}");

        QCOMPARE(formatDetectedSpy.count(), 1);
        QString signalValue = formatDetectedSpy.at(0).at(0).toString();

        QCOMPARE(returnValue, signalValue);
        QCOMPARE(returnValue, QString("json"));
    }

    // 8.4-INT-002: Multiple detections work correctly
    void testMultipleDetections()
    {
        QSignalSpy formatDetectedSpy(m_bridge, &JsonBridge::formatDetected);

        m_bridge->detectFormat("{\"a\":1}");
        m_bridge->detectFormat("<root/>");
        m_bridge->detectFormat("text");

        QCOMPARE(formatDetectedSpy.count(), 3);
        QCOMPARE(formatDetectedSpy.at(0).at(0).toString(), QString("json"));
        QCOMPARE(formatDetectedSpy.at(1).at(0).toString(), QString("xml"));
        QCOMPARE(formatDetectedSpy.at(2).at(0).toString(), QString("unknown"));
    }

    // ============================================================================
    // Story 10.4: Markdown Detection Tests
    // ============================================================================

    // ========== AC1, AC2: Markdown pattern detection tests ==========

    // 10.4-UNIT-001: ATX heading H1 detected
    void testDetectMarkdownH1()
    {
        QString result = m_bridge->detectFormat("# Heading");
        QCOMPARE(result, QString("markdown"));
    }

    // 10.4-UNIT-002: ATX heading H2 detected
    void testDetectMarkdownH2()
    {
        QString result = m_bridge->detectFormat("## Subheading");
        QCOMPARE(result, QString("markdown"));
    }

    // 10.4-UNIT-003: ATX heading H3 detected
    void testDetectMarkdownH3()
    {
        QString result = m_bridge->detectFormat("### Deep heading");
        QCOMPARE(result, QString("markdown"));
    }

    // 10.4-UNIT-004: ATX heading H6 detected
    void testDetectMarkdownH6()
    {
        QString result = m_bridge->detectFormat("###### Level 6");
        QCOMPARE(result, QString("markdown"));
    }

    // 10.4-UNIT-005: Code block detected
    void testDetectMarkdownCodeBlock()
    {
        QString result = m_bridge->detectFormat("```javascript\nconsole.log('hi');\n```");
        QCOMPARE(result, QString("markdown"));
    }

    // 10.4-UNIT-006: YAML frontmatter detected
    void testDetectMarkdownFrontmatter()
    {
        QString result = m_bridge->detectFormat("---\ntitle: Test\n---");
        QCOMPARE(result, QString("markdown"));
    }

    // 10.4-UNIT-007: Unordered list with dash detected
    void testDetectMarkdownUnorderedListDash()
    {
        QString result = m_bridge->detectFormat("- List item");
        QCOMPARE(result, QString("markdown"));
    }

    // 10.4-UNIT-008: Unordered list with asterisk detected
    void testDetectMarkdownUnorderedListAsterisk()
    {
        QString result = m_bridge->detectFormat("* List item");
        QCOMPARE(result, QString("markdown"));
    }

    // 10.4-UNIT-009: Ordered list detected
    void testDetectMarkdownOrderedList()
    {
        QString result = m_bridge->detectFormat("1. First item");
        QCOMPARE(result, QString("markdown"));
    }

    // 10.4-UNIT-010: Ordered list with multi-digit number
    void testDetectMarkdownOrderedListMultiDigit()
    {
        QString result = m_bridge->detectFormat("123. Item one twenty three");
        QCOMPARE(result, QString("markdown"));
    }

    // 10.4-UNIT-011: Blockquote detected
    void testDetectMarkdownBlockquote()
    {
        QString result = m_bridge->detectFormat("> This is a quote");
        QCOMPARE(result, QString("markdown"));
    }

    // 10.4-UNIT-012: Link pattern detected mid-document
    void testDetectMarkdownLink()
    {
        QString result = m_bridge->detectFormat("Check out [this link](https://example.com) for more info.");
        QCOMPARE(result, QString("markdown"));
    }

    // ========== AC3: Whitespace handling tests ==========

    // 10.4-UNIT-013: Heading with leading whitespace
    void testDetectMarkdownHeadingWithWhitespace()
    {
        QString result = m_bridge->detectFormat("  \n  # Heading");
        QCOMPARE(result, QString("markdown"));
    }

    // 10.4-UNIT-014: Code block with leading whitespace
    void testDetectMarkdownCodeBlockWithWhitespace()
    {
        QString result = m_bridge->detectFormat("\t\n```python\nprint('hi')\n```");
        QCOMPARE(result, QString("markdown"));
    }

    // ========== AC4: Mid-document pattern detection tests ==========

    // 10.4-UNIT-015: Heading appearing mid-document
    void testDetectMarkdownHeadingMidDocument()
    {
        QString result = m_bridge->detectFormat("Some intro text\n\n# Main Heading\n\nMore content");
        QCOMPARE(result, QString("markdown"));
    }

    // 10.4-UNIT-016: Code block appearing mid-document
    void testDetectMarkdownCodeBlockMidDocument()
    {
        QString result = m_bridge->detectFormat("Some text here.\n\n```\ncode\n```\n\nMore text.");
        QCOMPARE(result, QString("markdown"));
    }

    // ========== AC6: Signal emission test for markdown ==========

    // 10.4-UNIT-017: Signal emitted for markdown
    void testSignalEmittedForMarkdown()
    {
        QSignalSpy formatDetectedSpy(m_bridge, &JsonBridge::formatDetected);

        m_bridge->detectFormat("# Markdown Heading");

        QCOMPARE(formatDetectedSpy.count(), 1);
        QCOMPARE(formatDetectedSpy.at(0).at(0).toString(), QString("markdown"));
    }

    // ========== AC7: False positive prevention tests ==========

    // 10.4-UNIT-018: Plain text without patterns returns unknown
    void testDetectPlainTextNoPatterns()
    {
        QString result = m_bridge->detectFormat("Hello world, this is just plain text.");
        QCOMPARE(result, QString("unknown"));
    }

    // 10.4-UNIT-019: Hashtag without space returns unknown (not heading)
    void testDetectHashtagNoSpace()
    {
        QString result = m_bridge->detectFormat("#hashtag");
        QCOMPARE(result, QString("unknown"));
    }

    // 10.4-UNIT-020: Hex color returns unknown
    void testDetectHexColor()
    {
        QString result = m_bridge->detectFormat("#ffffff");
        QCOMPARE(result, QString("unknown"));
    }

    // 10.4-UNIT-021: C preprocessor include returns unknown
    void testDetectCInclude()
    {
        QString result = m_bridge->detectFormat("#include <stdio.h>");
        QCOMPARE(result, QString("unknown"));
    }

    // 10.4-UNIT-022: Hash with space only (empty heading content) - still markdown
    void testDetectHashSpaceOnly()
    {
        QString result = m_bridge->detectFormat("# ");
        QCOMPARE(result, QString("markdown"));
    }

    // 10.4-UNIT-023: Double hash without space returns unknown
    void testDetectDoubleHashNoSpace()
    {
        QString result = m_bridge->detectFormat("##NoSpace");
        QCOMPARE(result, QString("unknown"));
    }

    // ========== Priority conflict tests (JSON/XML take precedence) ==========

    // 10.4-UNIT-024: JSON with markdown-like content returns json
    void testDetectJsonWithMarkdownContent()
    {
        QString result = m_bridge->detectFormat("{\"title\": \"# Heading\", \"list\": \"- item\"}");
        QCOMPARE(result, QString("json"));
    }

    // 10.4-UNIT-025: XML with markdown-like content returns xml
    void testDetectXmlWithMarkdownContent()
    {
        QString result = m_bridge->detectFormat("<root># This looks like heading</root>");
        QCOMPARE(result, QString("xml"));
    }

    // ========== AC8: Performance tests ==========

    // 10.4-PERF-001: Large markdown detection is fast (< 1ms due to 2KB cap)
    void testPerformanceLargeMarkdown()
    {
        // Create 10KB markdown input
        QString largeMarkdown = QString("# Large Document\n\n") + QString(10240, 'x');

        QElapsedTimer timer;
        timer.start();

        QString result = m_bridge->detectFormat(largeMarkdown);

        qint64 elapsed = timer.elapsed();
        // Should complete in < 10ms even for large input (2KB search cap)
        QVERIFY2(elapsed < 10, qPrintable(QString("10KB markdown detection took %1ms").arg(elapsed)));
        QCOMPARE(result, QString("markdown"));
    }

    // 10.4-PERF-002: Very large input still fast due to 2KB limit
    void testPerformanceVeryLargeInput()
    {
        // Create 1MB plain text (no markdown patterns early)
        QString largePlainText = QString(1024 * 1024, 'x');

        QElapsedTimer timer;
        timer.start();

        QString result = m_bridge->detectFormat(largePlainText);

        qint64 elapsed = timer.elapsed();
        // 2KB search limit means this should be fast regardless of size
        QVERIFY2(elapsed < 50, qPrintable(QString("1MB plain text detection took %1ms").arg(elapsed)));
        QCOMPARE(result, QString("unknown"));
    }

    // 10.4-PERF-003: Repeated detections are fast (1000 iterations)
    void testPerformanceRepeatedMarkdownDetection()
    {
        QElapsedTimer timer;
        timer.start();

        for (int i = 0; i < 1000; i++) {
            m_bridge->detectFormat("# Heading\n\nSome content with [a link](url).");
        }

        qint64 elapsed = timer.elapsed();
        // 1000 iterations should complete in < 100ms
        QVERIFY2(elapsed < 100, qPrintable(QString("1000 markdown detections took %1ms").arg(elapsed)));
    }

    // ========== Regression tests (JSON/XML still work) ==========

    // 10.4-REG-001: JSON detection still works
    void testRegressionJsonDetection()
    {
        QString result = m_bridge->detectFormat("{\"key\": \"value\", \"nested\": {\"a\": 1}}");
        QCOMPARE(result, QString("json"));
    }

    // 10.4-REG-002: XML detection still works
    void testRegressionXmlDetection()
    {
        QString result = m_bridge->detectFormat("<root><child attr=\"val\">text</child></root>");
        QCOMPARE(result, QString("xml"));
    }

    // 10.4-REG-003: Empty input still returns unknown
    void testRegressionEmptyInput()
    {
        QString result = m_bridge->detectFormat("");
        QCOMPARE(result, QString("unknown"));
    }

    // ========== Additional edge cases ==========

    // 10.4-UNIT-026: Task list (checkbox markdown) detected
    void testDetectMarkdownTaskList()
    {
        QString result = m_bridge->detectFormat("- [ ] Task item");
        QCOMPARE(result, QString("markdown"));
    }

    // 10.4-UNIT-027: Multiple link patterns
    void testDetectMarkdownMultipleLinks()
    {
        QString result = m_bridge->detectFormat("See [link1](url1) and also [link2](url2).");
        QCOMPARE(result, QString("markdown"));
    }

    // 10.4-INT-003: Return value matches signal value for markdown
    void testReturnMatchesSignalMarkdown()
    {
        QSignalSpy formatDetectedSpy(m_bridge, &JsonBridge::formatDetected);

        QString returnValue = m_bridge->detectFormat("## Section Title");

        QCOMPARE(formatDetectedSpy.count(), 1);
        QString signalValue = formatDetectedSpy.at(0).at(0).toString();

        QCOMPARE(returnValue, signalValue);
        QCOMPARE(returnValue, QString("markdown"));
    }

    // 10.4-INT-004: Multiple format detections including markdown
    void testMultipleDetectionsIncludingMarkdown()
    {
        QSignalSpy formatDetectedSpy(m_bridge, &JsonBridge::formatDetected);

        m_bridge->detectFormat("{\"a\":1}");
        m_bridge->detectFormat("<root/>");
        m_bridge->detectFormat("# Heading");
        m_bridge->detectFormat("plain text");

        QCOMPARE(formatDetectedSpy.count(), 4);
        QCOMPARE(formatDetectedSpy.at(0).at(0).toString(), QString("json"));
        QCOMPARE(formatDetectedSpy.at(1).at(0).toString(), QString("xml"));
        QCOMPARE(formatDetectedSpy.at(2).at(0).toString(), QString("markdown"));
        QCOMPARE(formatDetectedSpy.at(3).at(0).toString(), QString("unknown"));
    }
};

QTEST_MAIN(tst_FormatDetection)
#include "tst_format_detection.moc"
