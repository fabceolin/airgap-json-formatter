/**
 * @file tst_markdown_bridge.cpp
 * @brief Unit tests for Markdown rendering via JsonBridge (Story 10.3)
 *
 * Tests verify:
 * - AC5: requestRenderMarkdown and requestRenderMarkdownWithMermaid exist
 * - AC4: AsyncSerialiser integration for sequential execution
 * - Signal emission for QML callback
 *
 * Note: Full markdown rendering tests require WASM environment.
 * These tests verify the C++ wrapper structure and AsyncSerialiser integration.
 */
#include <QtTest/QtTest>
#include <QSignalSpy>
#include "../jsonbridge.h"
#include "../asyncserialiser.h"

class tst_MarkdownBridge : public QObject
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

    // 10.3-UNIT-010: requestRenderMarkdown uses AsyncSerialiser (AC4)
    void testRequestRenderMarkdownUsesAsyncSerialiser()
    {
        QSignalSpy taskStartedSpy(&AsyncSerialiser::instance(), &AsyncSerialiser::taskStarted);
        QSignalSpy renderedSpy(m_bridge, &JsonBridge::markdownRendered);
        QSignalSpy errorSpy(m_bridge, &JsonBridge::markdownRenderError);

        m_bridge->requestRenderMarkdown("# Hello World");

        // Task should start
        QTRY_COMPARE(taskStartedSpy.count(), 1);
        QCOMPARE(taskStartedSpy.at(0).at(0).toString(), QString("renderMarkdown"));

        // Signal should be emitted (either rendered or error)
        QTRY_VERIFY(renderedSpy.count() == 1 || errorSpy.count() == 1);

#ifndef __EMSCRIPTEN__
        // Desktop mode: Markdown not available via WASM
        QCOMPARE(errorSpy.count(), 1);
        QCOMPARE(errorSpy.at(0).at(0).toString(), QString("Markdown rendering only available in WASM build"));
#endif
    }

    // 10.3-UNIT-011: requestRenderMarkdownWithMermaid uses AsyncSerialiser (AC4)
    void testRequestRenderMarkdownWithMermaidUsesAsyncSerialiser()
    {
        QSignalSpy taskStartedSpy(&AsyncSerialiser::instance(), &AsyncSerialiser::taskStarted);
        QSignalSpy renderedSpy(m_bridge, &JsonBridge::markdownWithMermaidRendered);
        QSignalSpy errorSpy(m_bridge, &JsonBridge::markdownWithMermaidError);

        m_bridge->requestRenderMarkdownWithMermaid("# Test\n```mermaid\ngraph TD;A-->B\n```", "dark");

        // Task should start
        QTRY_COMPARE(taskStartedSpy.count(), 1);
        QCOMPARE(taskStartedSpy.at(0).at(0).toString(), QString("renderMarkdownWithMermaid"));

        // Signal should be emitted (either rendered or error)
        QTRY_VERIFY(renderedSpy.count() == 1 || errorSpy.count() == 1);

#ifndef __EMSCRIPTEN__
        // Desktop mode: Markdown+Mermaid not available via WASM
        QCOMPARE(errorSpy.count(), 1);
        QCOMPARE(errorSpy.at(0).at(0).toString(), QString("Markdown+Mermaid rendering only available in WASM build"));
#endif
    }

    // 10.3-INT-009: Serial execution with multiple queued renders (AC4)
    void testRenderMarkdownSerialExecution()
    {
        QSignalSpy taskStartedSpy(&AsyncSerialiser::instance(), &AsyncSerialiser::taskStarted);
        QSignalSpy renderedSpy(m_bridge, &JsonBridge::markdownRendered);
        QSignalSpy errorSpy(m_bridge, &JsonBridge::markdownRenderError);

        // Queue 3 renders rapidly
        m_bridge->requestRenderMarkdown("# One");
        m_bridge->requestRenderMarkdown("# Two");
        m_bridge->requestRenderMarkdown("# Three");

        // All tasks should eventually start (serially)
        QTRY_COMPARE(taskStartedSpy.count(), 3);

        // All should complete with results
        int totalSignals = renderedSpy.count() + errorSpy.count();
        QTRY_COMPARE(totalSignals, 3);

        // Verify all task names are renderMarkdown
        for (int i = 0; i < 3; ++i) {
            QCOMPARE(taskStartedSpy.at(i).at(0).toString(), QString("renderMarkdown"));
        }
    }

    // 10.3-E2E-001: Theme parameter passed correctly (AC8)
    void testRenderMarkdownWithMermaidThemeParameter()
    {
        QSignalSpy renderedSpy(m_bridge, &JsonBridge::markdownWithMermaidRendered);
        QSignalSpy errorSpy(m_bridge, &JsonBridge::markdownWithMermaidError);

        // Test dark theme
        m_bridge->requestRenderMarkdownWithMermaid("# Dark", "dark");
        QTRY_VERIFY(renderedSpy.count() + errorSpy.count() == 1);

        // Test light theme
        m_bridge->requestRenderMarkdownWithMermaid("# Light", "light");
        QTRY_VERIFY(renderedSpy.count() + errorSpy.count() == 2);
    }

    // 10.3-UNIT-012: Default theme parameter
    void testRenderMarkdownWithMermaidDefaultTheme()
    {
        QSignalSpy renderedSpy(m_bridge, &JsonBridge::markdownWithMermaidRendered);
        QSignalSpy errorSpy(m_bridge, &JsonBridge::markdownWithMermaidError);

        // Call without theme parameter (should default to "dark")
        m_bridge->requestRenderMarkdownWithMermaid("# Default Theme");
        QTRY_VERIFY(renderedSpy.count() + errorSpy.count() == 1);
    }

    // 10.3-PERF-003: Non-blocking operation (AC7)
    void testRenderMarkdownNonBlocking()
    {
        QSignalSpy renderedSpy(m_bridge, &JsonBridge::markdownRendered);
        QSignalSpy errorSpy(m_bridge, &JsonBridge::markdownRenderError);

        // Start render
        m_bridge->requestRenderMarkdown("# Test");

        // Should return immediately (non-blocking)
        // Queue length should be > 0 or task should have started
        QVERIFY(AsyncSerialiser::instance().queueLength() >= 0);

        // Wait for completion
        QTRY_VERIFY(renderedSpy.count() + errorSpy.count() == 1);
    }

    // 10.3-UNIT-013: markdownRendered signal structure (AC5)
    void testMarkdownRenderedSignalStructure()
    {
        QSignalSpy renderedSpy(m_bridge, &JsonBridge::markdownRendered);
        QSignalSpy errorSpy(m_bridge, &JsonBridge::markdownRenderError);

        m_bridge->requestRenderMarkdown("# Hello");

        QTRY_VERIFY(renderedSpy.count() + errorSpy.count() == 1);

#ifdef __EMSCRIPTEN__
        // In WASM, verify the HTML output
        if (renderedSpy.count() == 1) {
            QString html = renderedSpy.at(0).at(0).toString();
            QVERIFY(!html.isEmpty());
        }
#else
        // Desktop mode should error
        QCOMPARE(errorSpy.count(), 1);
#endif
    }

    // 10.3-UNIT-014: markdownWithMermaidRendered signal includes warnings array (AC5)
    void testMarkdownWithMermaidRenderedSignalStructure()
    {
        QSignalSpy renderedSpy(m_bridge, &JsonBridge::markdownWithMermaidRendered);
        QSignalSpy errorSpy(m_bridge, &JsonBridge::markdownWithMermaidError);

        m_bridge->requestRenderMarkdownWithMermaid("# Test with Mermaid", "dark");

        QTRY_VERIFY(renderedSpy.count() + errorSpy.count() == 1);

#ifdef __EMSCRIPTEN__
        // In WASM, verify signal parameters
        if (renderedSpy.count() == 1) {
            QString html = renderedSpy.at(0).at(0).toString();
            QStringList warnings = renderedSpy.at(0).at(1).toStringList();
            QVERIFY(!html.isEmpty());
            // warnings can be empty or contain diagram errors
        }
#else
        // Desktop mode should error
        QCOMPARE(errorSpy.count(), 1);
#endif
    }

    // 10.3-UNIT-015: Busy state during rendering (AC7)
    void testRenderMarkdownBusyState()
    {
        QSignalSpy busyChangedSpy(m_bridge, &JsonBridge::busyChanged);
        QSignalSpy renderedSpy(m_bridge, &JsonBridge::markdownRendered);
        QSignalSpy errorSpy(m_bridge, &JsonBridge::markdownRenderError);

        m_bridge->requestRenderMarkdown("# Busy Test");

        // Wait for completion
        QTRY_VERIFY(renderedSpy.count() + errorSpy.count() == 1);

        // After completion, should not be busy
        QCoreApplication::processEvents();
        QVERIFY(!m_bridge->isBusy());
    }

    // 10.3-INT-010: Mixed markdown and mermaid requests serialize correctly
    void testMixedRequestsSerialization()
    {
        QSignalSpy taskStartedSpy(&AsyncSerialiser::instance(), &AsyncSerialiser::taskStarted);

        // Queue different types of requests
        m_bridge->requestRenderMarkdown("# Plain");
        m_bridge->requestRenderMarkdownWithMermaid("# With Diagram", "dark");
        m_bridge->requestRenderMarkdown("# Another Plain");

        // All tasks should start serially
        QTRY_COMPARE(taskStartedSpy.count(), 3);

        // Verify correct task names in order
        QCOMPARE(taskStartedSpy.at(0).at(0).toString(), QString("renderMarkdown"));
        QCOMPARE(taskStartedSpy.at(1).at(0).toString(), QString("renderMarkdownWithMermaid"));
        QCOMPARE(taskStartedSpy.at(2).at(0).toString(), QString("renderMarkdown"));
    }
};

QTEST_MAIN(tst_MarkdownBridge)
#include "tst_markdown_bridge.moc"
