/**
 * @file tst_jsonbridge_xml.cpp
 * @brief Unit tests for JsonBridge XML operations (Story 8.3)
 *
 * Tests verify:
 * - AC1: formatXml(input, indentType) method
 * - AC2: minifyXml(input) method
 * - AC3: highlightXml(input) synchronous method
 * - AC4: Async methods emit completion signals with result/error
 * - AC5: All XML methods integrate with AsyncSerialiser pattern
 * - AC6: Existing JSON methods continue to work unchanged
 * - AC7: Bridge compiles for both native and WASM targets
 */
#include <QtTest/QtTest>
#include <QSignalSpy>
#include "../jsonbridge.h"
#include "../asyncserialiser.h"

class tst_JsonBridgeXml : public QObject
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

    // ========== AC1: formatXml method tests ==========

    // 8.3-UNIT-001: formatXml enqueues task to AsyncSerialiser
    void testFormatXmlUsesAsyncSerialiser()
    {
        QSignalSpy taskStartedSpy(&AsyncSerialiser::instance(), &AsyncSerialiser::taskStarted);
        QSignalSpy formatXmlCompletedSpy(m_bridge, &JsonBridge::formatXmlCompleted);

        m_bridge->formatXml("<root/>", "spaces:4");

        // Task should start
        QTRY_COMPARE(taskStartedSpy.count(), 1);
        QCOMPARE(taskStartedSpy.at(0).at(0).toString(), QString("formatXml"));

        // Signal should be emitted
        QTRY_COMPARE(formatXmlCompletedSpy.count(), 1);
    }

    // 8.3-UNIT-002: formatXmlCompleted signal structure
    void testFormatXmlSignalStructure()
    {
        QSignalSpy formatXmlCompletedSpy(m_bridge, &JsonBridge::formatXmlCompleted);

        m_bridge->formatXml("<root><child/></root>", "spaces:2");

        QTRY_COMPARE(formatXmlCompletedSpy.count(), 1);
        QVariantMap result = formatXmlCompletedSpy.at(0).at(0).toMap();

        // Signal should contain success and result or error
        QVERIFY(result.contains("success"));
        // In native build, XML formatting is not available
#ifndef __EMSCRIPTEN__
        QVERIFY(!result["success"].toBool());
        QVERIFY(result.contains("error"));
#endif
    }

    // ========== AC2: minifyXml method tests ==========

    // 8.3-UNIT-003: minifyXml enqueues task to AsyncSerialiser
    void testMinifyXmlUsesAsyncSerialiser()
    {
        QSignalSpy taskStartedSpy(&AsyncSerialiser::instance(), &AsyncSerialiser::taskStarted);
        QSignalSpy minifyXmlCompletedSpy(m_bridge, &JsonBridge::minifyXmlCompleted);

        m_bridge->minifyXml("<root>\n  <child/>\n</root>");

        QTRY_COMPARE(taskStartedSpy.count(), 1);
        QCOMPARE(taskStartedSpy.at(0).at(0).toString(), QString("minifyXml"));

        QTRY_COMPARE(minifyXmlCompletedSpy.count(), 1);
    }

    // 8.3-UNIT-004: minifyXmlCompleted signal structure
    void testMinifyXmlSignalStructure()
    {
        QSignalSpy minifyXmlCompletedSpy(m_bridge, &JsonBridge::minifyXmlCompleted);

        m_bridge->minifyXml("<root>  <child/>  </root>");

        QTRY_COMPARE(minifyXmlCompletedSpy.count(), 1);
        QVariantMap result = minifyXmlCompletedSpy.at(0).at(0).toMap();

        // Signal should contain success and result or error
        QVERIFY(result.contains("success"));
#ifndef __EMSCRIPTEN__
        QVERIFY(!result["success"].toBool());
        QVERIFY(result.contains("error"));
#endif
    }

    // ========== AC3: highlightXml synchronous method tests ==========

    // 8.3-UNIT-005: highlightXml returns HTML
    void testHighlightXmlReturnsHtml()
    {
        QString result = m_bridge->highlightXml("<root attr=\"value\"/>");

        // Should return non-empty HTML string
        QVERIFY(!result.isEmpty());
        // In native build, should return escaped HTML in a pre tag
#ifndef __EMSCRIPTEN__
        QVERIFY(result.contains("<pre"));
        QVERIFY(result.contains("&lt;root"));
#endif
    }

    // 8.3-UNIT-006: highlightXml handles empty input
    void testHighlightXmlEmptyInput()
    {
        QString result = m_bridge->highlightXml("");

        // Empty input should return some output (escaped empty or wrapper)
        // The native build returns a pre wrapper even for empty input
#ifndef __EMSCRIPTEN__
        QVERIFY(result.contains("<pre"));
#endif
    }

    // ========== AC4: Signal result/error format tests ==========

    // 8.3-INT-001: formatXml success signal format (WASM only)
    // Note: This test verifies the signal is emitted; actual success requires WASM

    // 8.3-INT-003: formatXml error signal contains error field
    void testFormatXmlErrorSignalFormat()
    {
        QSignalSpy formatXmlCompletedSpy(m_bridge, &JsonBridge::formatXmlCompleted);

        m_bridge->formatXml("<root", "spaces:4");  // Invalid XML

        QTRY_COMPARE(formatXmlCompletedSpy.count(), 1);
        QVariantMap result = formatXmlCompletedSpy.at(0).at(0).toMap();

        // Error case should have success=false and error message
        QVERIFY(!result["success"].toBool());
        QVERIFY(result.contains("error"));
        QVERIFY(!result["error"].toString().isEmpty());
    }

    // ========== AC5: AsyncSerialiser integration tests ==========

    // 8.3-INT-016: Multiple XML operations queue correctly
    void testMultipleXmlOperationsQueue()
    {
        QSignalSpy formatXmlCompletedSpy(m_bridge, &JsonBridge::formatXmlCompleted);
        QSignalSpy minifyXmlCompletedSpy(m_bridge, &JsonBridge::minifyXmlCompleted);

        // Queue multiple operations
        m_bridge->formatXml("<root><a/></root>", "spaces:2");
        m_bridge->minifyXml("<root>  <b/>  </root>");
        m_bridge->formatXml("<root><c/></root>", "tabs");

        // All should complete
        QTRY_COMPARE(formatXmlCompletedSpy.count(), 2);
        QTRY_COMPARE(minifyXmlCompletedSpy.count(), 1);
    }

    // 8.3-INT-017: Mixed JSON and XML operations queue correctly (AC5)
    void testMixedJsonXmlOperations()
    {
        QSignalSpy formatCompletedSpy(m_bridge, &JsonBridge::formatCompleted);
        QSignalSpy formatXmlCompletedSpy(m_bridge, &JsonBridge::formatXmlCompleted);

        // Queue mixed operations
        m_bridge->formatJson("{\"a\":1}", "spaces:4");
        m_bridge->formatXml("<root/>", "spaces:4");
        m_bridge->formatJson("{\"b\":2}", "spaces:2");

        // All should complete
        QTRY_COMPARE(formatCompletedSpy.count(), 2);
        QTRY_COMPARE(formatXmlCompletedSpy.count(), 1);
    }

    // 8.3-INT-018: Failed XML operation doesn't block queue
    void testFailedXmlDoesNotBlockQueue()
    {
        QSignalSpy formatXmlCompletedSpy(m_bridge, &JsonBridge::formatXmlCompleted);

        // First with invalid XML
        m_bridge->formatXml("<invalid", "spaces:4");
        // Second with valid XML
        m_bridge->formatXml("<valid/>", "spaces:4");

        QTRY_COMPARE(formatXmlCompletedSpy.count(), 2);

        // Both should have emitted signals (even if both are errors in native build)
        QVariantMap result1 = formatXmlCompletedSpy.at(0).at(0).toMap();
        QVariantMap result2 = formatXmlCompletedSpy.at(1).at(0).toMap();

        QVERIFY(result1.contains("success"));
        QVERIFY(result2.contains("success"));
    }

    // ========== AC6: JSON regression tests ==========

    // 8.3-E2E-001: JSON formatJson still works
    void testJsonFormatStillWorks()
    {
        QSignalSpy formatCompletedSpy(m_bridge, &JsonBridge::formatCompleted);

        m_bridge->formatJson("{\"key\": \"value\"}", "spaces:4");

        QTRY_COMPARE(formatCompletedSpy.count(), 1);
        QVariantMap result = formatCompletedSpy.at(0).at(0).toMap();

        // JSON formatting should work in native build
        QVERIFY(result["success"].toBool());
        QVERIFY(result["result"].toString().contains("key"));
    }

    // 8.3-E2E-002: JSON minifyJson still works
    void testJsonMinifyStillWorks()
    {
        QSignalSpy minifyCompletedSpy(m_bridge, &JsonBridge::minifyCompleted);

        m_bridge->minifyJson("{ \"key\": \"value\" }");

        QTRY_COMPARE(minifyCompletedSpy.count(), 1);
        QVariantMap result = minifyCompletedSpy.at(0).at(0).toMap();

        QVERIFY(result["success"].toBool());
        QCOMPARE(result["result"].toString(), QString("{\"key\":\"value\"}"));
    }

    // 8.3-E2E-003: JSON highlightJson still works
    void testJsonHighlightStillWorks()
    {
        QString result = m_bridge->highlightJson("{\"key\": \"value\"}");

        // Should return highlighted HTML
        QVERIFY(!result.isEmpty());
        QVERIFY(result.contains("<span") || result.contains("<pre"));
    }

    // 8.3-E2E-003: JSON validateJson still works
    void testJsonValidateStillWorks()
    {
        QSignalSpy validateCompletedSpy(m_bridge, &JsonBridge::validateCompleted);

        m_bridge->validateJson("{\"valid\": true}");

        QTRY_COMPARE(validateCompletedSpy.count(), 1);
        QVariantMap result = validateCompletedSpy.at(0).at(0).toMap();

        QVERIFY(result["isValid"].toBool());
    }

    // ========== FIFO order test ==========

    // 8.3-INT-019: XML operations complete in FIFO order
    void testXmlOperationsFIFOOrder()
    {
        QStringList operationOrder;

        QSignalSpy formatXmlCompletedSpy(m_bridge, &JsonBridge::formatXmlCompleted);
        QSignalSpy minifyXmlCompletedSpy(m_bridge, &JsonBridge::minifyXmlCompleted);

        connect(m_bridge, &JsonBridge::formatXmlCompleted, this, [&operationOrder]() {
            operationOrder.append("formatXml");
        });
        connect(m_bridge, &JsonBridge::minifyXmlCompleted, this, [&operationOrder]() {
            operationOrder.append("minifyXml");
        });

        m_bridge->formatXml("<a/>", "spaces:4");
        m_bridge->minifyXml("<b/>");
        m_bridge->formatXml("<c/>", "spaces:2");

        // Wait for all to complete
        QTRY_COMPARE(formatXmlCompletedSpy.count(), 2);
        QTRY_COMPARE(minifyXmlCompletedSpy.count(), 1);

        // Verify FIFO order
        QCOMPARE(operationOrder, QStringList({"formatXml", "minifyXml", "formatXml"}));
    }

    // ========== busyChanged signal test ==========

    // 8.3-INT-020: busyChanged signal emits for XML operations
    void testBusyChangedSignalForXml()
    {
        QSignalSpy busyChangedSpy(m_bridge, &JsonBridge::busyChanged);

        m_bridge->formatXml("<root/>", "spaces:4");

        // Wait for completion
        QSignalSpy formatXmlCompletedSpy(m_bridge, &JsonBridge::formatXmlCompleted);
        QTRY_COMPARE(formatXmlCompletedSpy.count(), 1);

        // busyChanged should have been emitted at least once
        QVERIFY(busyChangedSpy.count() >= 1);
    }
};

QTEST_MAIN(tst_JsonBridgeXml)
#include "tst_jsonbridge_xml.moc"
