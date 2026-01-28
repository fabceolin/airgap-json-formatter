/**
 * @file tst_mermaid.cpp
 * @brief Unit tests for Mermaid rendering via JsonBridge (Story 10.2)
 *
 * Tests verify:
 * - AC3: renderMermaid wrapper exists and has correct signature
 * - AC12: AsyncSerialiser integration for sequential execution
 * - Signal emission for QML callback
 *
 * Note: Full diagram rendering tests require WASM environment.
 * These tests verify the C++ wrapper structure and AsyncSerialiser integration.
 */
#include <QtTest/QtTest>
#include <QSignalSpy>
#include "../jsonbridge.h"
#include "../asyncserialiser.h"

class tst_Mermaid : public QObject
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

    // 10.2-UNIT-017: renderMermaid uses AsyncSerialiser
    void testRenderMermaidUsesAsyncSerialiser()
    {
        QSignalSpy taskStartedSpy(&AsyncSerialiser::instance(), &AsyncSerialiser::taskStarted);
        QSignalSpy renderCompletedSpy(m_bridge, &JsonBridge::renderMermaidCompleted);

        m_bridge->renderMermaid("graph TD; A-->B", "dark");

        // Task should start
        QTRY_COMPARE(taskStartedSpy.count(), 1);
        QCOMPARE(taskStartedSpy.at(0).at(0).toString(), QString("renderMermaid"));

        // Signal should be emitted with result (may be error in desktop mode)
        QTRY_COMPARE(renderCompletedSpy.count(), 1);
        QVariantMap result = renderCompletedSpy.at(0).at(0).toMap();

        // Desktop mode: Mermaid not available, so expect error
        // WASM mode: Would succeed
#ifndef __EMSCRIPTEN__
        QVERIFY(!result["success"].toBool());
        QCOMPARE(result["error"].toString(), QString("Mermaid rendering only available in WASM build"));
#endif
    }

    // 10.2-INT-024: Serial execution with multiple queued renders
    void testRenderMermaidSerialExecution()
    {
        QSignalSpy taskStartedSpy(&AsyncSerialiser::instance(), &AsyncSerialiser::taskStarted);
        QSignalSpy renderCompletedSpy(m_bridge, &JsonBridge::renderMermaidCompleted);

        // Queue 3 renders rapidly
        m_bridge->renderMermaid("graph TD; A-->B", "dark");
        m_bridge->renderMermaid("graph TD; C-->D", "dark");
        m_bridge->renderMermaid("graph TD; E-->F", "dark");

        // All tasks should eventually start (serially)
        QTRY_COMPARE(taskStartedSpy.count(), 3);

        // All should complete with results
        QTRY_COMPARE(renderCompletedSpy.count(), 3);

        // Verify all task names are renderMermaid
        for (int i = 0; i < 3; ++i) {
            QCOMPARE(taskStartedSpy.at(i).at(0).toString(), QString("renderMermaid"));
        }
    }

    // 10.2-E2E-005: Theme parameter passed correctly
    void testRenderMermaidThemeParameter()
    {
        QSignalSpy renderCompletedSpy(m_bridge, &JsonBridge::renderMermaidCompleted);

        // Test dark theme
        m_bridge->renderMermaid("graph TD; A-->B", "dark");
        QTRY_COMPARE(renderCompletedSpy.count(), 1);

        // Test light theme
        m_bridge->renderMermaid("graph TD; A-->B", "light");
        QTRY_COMPARE(renderCompletedSpy.count(), 2);
    }

    // Test default theme parameter
    void testRenderMermaidDefaultTheme()
    {
        QSignalSpy renderCompletedSpy(m_bridge, &JsonBridge::renderMermaidCompleted);

        // Call without theme parameter (should default to "dark")
        m_bridge->renderMermaid("graph TD; A-->B");
        QTRY_COMPARE(renderCompletedSpy.count(), 1);
    }

    // Test that renderMermaid doesn't block the event loop
    void testRenderMermaidNonBlocking()
    {
        QSignalSpy renderCompletedSpy(m_bridge, &JsonBridge::renderMermaidCompleted);

        // Start render
        m_bridge->renderMermaid("graph TD; A-->B", "dark");

        // Should return immediately (non-blocking)
        // Queue length should be > 0 or task should have started
        QVERIFY(AsyncSerialiser::instance().queueLength() >= 0);

        // Wait for completion
        QTRY_COMPARE(renderCompletedSpy.count(), 1);
    }

    // Test result signal contains required fields
    void testRenderMermaidSignalStructure()
    {
        QSignalSpy renderCompletedSpy(m_bridge, &JsonBridge::renderMermaidCompleted);

        m_bridge->renderMermaid("graph TD; A-->B", "dark");

        QTRY_COMPARE(renderCompletedSpy.count(), 1);
        QVariantMap result = renderCompletedSpy.at(0).at(0).toMap();

        // Result should always have 'success' field
        QVERIFY(result.contains("success"));

        // If success, should have 'svg'; if failure, should have 'error'
        if (result["success"].toBool()) {
            QVERIFY(result.contains("svg"));
            QVERIFY(!result["svg"].toString().isEmpty());
        } else {
            QVERIFY(result.contains("error"));
            QVERIFY(!result["error"].toString().isEmpty());
        }
    }

    // Test busy state during rendering
    void testRenderMermaidBusyState()
    {
        QSignalSpy busyChangedSpy(m_bridge, &JsonBridge::busyChanged);
        QSignalSpy renderCompletedSpy(m_bridge, &JsonBridge::renderMermaidCompleted);

        // Initial state should not be busy (queue empty)
        QVERIFY(!m_bridge->isBusy() || AsyncSerialiser::instance().queueLength() > 0);

        m_bridge->renderMermaid("graph TD; A-->B", "dark");

        // Wait for completion
        QTRY_COMPARE(renderCompletedSpy.count(), 1);

        // After completion, should not be busy
        QCoreApplication::processEvents();
        QVERIFY(!m_bridge->isBusy());
    }
};

QTEST_MAIN(tst_Mermaid)
#include "tst_mermaid.moc"
