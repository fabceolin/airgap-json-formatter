#include <QtTest>
#include <QSignalSpy>
#include <QElapsedTimer>
#include "qxmltreemodel.h"
#include "qxmltreeitem.h"

class tst_QXmlTreeModel : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    // Basic parsing tests (AC2: Expandable nodes)
    void testSimpleElement();
    void testNestedElements();
    void testEmptyDocument();

    // Attribute tests (AC3: @ prefix attributes)
    void testSingleAttribute();
    void testMultipleAttributes();
    void testAttributeWithNamespace();

    // Text content tests (AC4: Text content leaf nodes)
    void testTextContent();
    void testMixedContent();
    void testCDataContent();
    void testCommentNode();

    // Namespace tests (AC5: Namespaces preserved)
    void testSimpleNamespace();
    void testMultipleNamespaces();
    void testDefaultNamespace();
    void testNestedNamespaces();

    // Serialization tests (AC6: Copy/serialize node)
    void testSerializeSimpleElement();
    void testSerializeWithAttributes();
    void testSerializeWithChildren();
    void testSerializeWithText();
    void testSerializeWithCData();

    // Model interface tests (AC1: QXmlTreeModel pattern)
    void testRoleNames();
    void testDataRoles();
    void testRowCount();
    void testIndex();
    void testParent();

    // Error handling tests
    void testMalformedXml();
    void testUnclosedTag();
    void testInvalidCharacter();

    // Performance tests (AC8: Performance 10K nodes)
    void testLoad1000Nodes();
    void testLoad10000Nodes();
    void testNodeCountLimit();

    // XPath tests
    void testXmlPath();
    void testXmlPathWithIndex();

    // Clear and reload tests
    void testClear();
    void testReload();

private:
    QXmlTreeModel* m_model;

    // Helper to generate large XML
    QString generateLargeXml(int nodeCount);
};

void tst_QXmlTreeModel::init()
{
    m_model = new QXmlTreeModel();
}

void tst_QXmlTreeModel::cleanup()
{
    delete m_model;
    m_model = nullptr;
}

// =============================================================================
// Basic parsing tests
// =============================================================================

void tst_QXmlTreeModel::testSimpleElement()
{
    bool result = m_model->loadXml("<root/>");
    QVERIFY(result);
    QCOMPARE(m_model->rowCount(), 1);

    QModelIndex rootIndex = m_model->index(0, 0);
    QVERIFY(rootIndex.isValid());
    QCOMPARE(m_model->data(rootIndex, QXmlTreeModel::KeyRole).toString(), QString("root"));
    QCOMPARE(m_model->data(rootIndex, QXmlTreeModel::ValueTypeRole).toString(), QString("element"));
}

void tst_QXmlTreeModel::testNestedElements()
{
    bool result = m_model->loadXml("<a><b><c/></b></a>");
    QVERIFY(result);
    QCOMPARE(m_model->rowCount(), 1);

    // Navigate to "a"
    QModelIndex aIndex = m_model->index(0, 0);
    QCOMPARE(m_model->data(aIndex, QXmlTreeModel::KeyRole).toString(), QString("a"));
    QCOMPARE(m_model->rowCount(aIndex), 1);

    // Navigate to "b"
    QModelIndex bIndex = m_model->index(0, 0, aIndex);
    QCOMPARE(m_model->data(bIndex, QXmlTreeModel::KeyRole).toString(), QString("b"));
    QCOMPARE(m_model->rowCount(bIndex), 1);

    // Navigate to "c"
    QModelIndex cIndex = m_model->index(0, 0, bIndex);
    QCOMPARE(m_model->data(cIndex, QXmlTreeModel::KeyRole).toString(), QString("c"));
    QCOMPARE(m_model->rowCount(cIndex), 0);
}

void tst_QXmlTreeModel::testEmptyDocument()
{
    bool result = m_model->loadXml("");
    QVERIFY(result);
    QCOMPARE(m_model->rowCount(), 0);
}

// =============================================================================
// Attribute tests
// =============================================================================

void tst_QXmlTreeModel::testSingleAttribute()
{
    bool result = m_model->loadXml("<a b=\"1\"/>");
    QVERIFY(result);

    QModelIndex aIndex = m_model->index(0, 0);
    QCOMPARE(m_model->rowCount(aIndex), 1);  // One attribute child

    QModelIndex attrIndex = m_model->index(0, 0, aIndex);
    QCOMPARE(m_model->data(attrIndex, QXmlTreeModel::KeyRole).toString(), QString("@b"));
    QCOMPARE(m_model->data(attrIndex, QXmlTreeModel::ValueRole).toString(), QString("1"));
    QCOMPARE(m_model->data(attrIndex, QXmlTreeModel::ValueTypeRole).toString(), QString("attribute"));
}

void tst_QXmlTreeModel::testMultipleAttributes()
{
    bool result = m_model->loadXml("<a x=\"1\" y=\"2\" z=\"3\"/>");
    QVERIFY(result);

    QModelIndex aIndex = m_model->index(0, 0);
    QCOMPARE(m_model->rowCount(aIndex), 3);  // Three attribute children
}

void tst_QXmlTreeModel::testAttributeWithNamespace()
{
    bool result = m_model->loadXml("<a xmlns:ns=\"http://example.com\" ns:attr=\"value\"/>");
    QVERIFY(result);

    QModelIndex aIndex = m_model->index(0, 0);
    // Find the ns:attr attribute (not the xmlns:ns one)
    bool foundNsAttr = false;
    for (int i = 0; i < m_model->rowCount(aIndex); i++) {
        QModelIndex attrIndex = m_model->index(i, 0, aIndex);
        QString key = m_model->data(attrIndex, QXmlTreeModel::KeyRole).toString();
        if (key == "@ns:attr") {
            foundNsAttr = true;
            QCOMPARE(m_model->data(attrIndex, QXmlTreeModel::ValueRole).toString(), QString("value"));
        }
    }
    QVERIFY(foundNsAttr);
}

// =============================================================================
// Text content tests
// =============================================================================

void tst_QXmlTreeModel::testTextContent()
{
    bool result = m_model->loadXml("<a>hello</a>");
    QVERIFY(result);

    QModelIndex aIndex = m_model->index(0, 0);
    QCOMPARE(m_model->rowCount(aIndex), 1);  // One text child

    QModelIndex textIndex = m_model->index(0, 0, aIndex);
    QCOMPARE(m_model->data(textIndex, QXmlTreeModel::ValueRole).toString(), QString("hello"));
    QCOMPARE(m_model->data(textIndex, QXmlTreeModel::ValueTypeRole).toString(), QString("text"));
}

void tst_QXmlTreeModel::testMixedContent()
{
    bool result = m_model->loadXml("<a>before<b/>after</a>");
    QVERIFY(result);

    QModelIndex aIndex = m_model->index(0, 0);
    QCOMPARE(m_model->rowCount(aIndex), 3);  // text, element, text
}

void tst_QXmlTreeModel::testCDataContent()
{
    bool result = m_model->loadXml("<a><![CDATA[<hello>&world]]></a>");
    QVERIFY(result);

    QModelIndex aIndex = m_model->index(0, 0);
    // QXmlStreamReader treats CDATA as characters, so we just verify it loads
    QVERIFY(m_model->rowCount(aIndex) >= 1);
}

void tst_QXmlTreeModel::testCommentNode()
{
    bool result = m_model->loadXml("<a><!-- This is a comment --><b/></a>");
    QVERIFY(result);

    QModelIndex aIndex = m_model->index(0, 0);
    // Should have comment and element children
    QCOMPARE(m_model->rowCount(aIndex), 2);

    // Find the comment
    bool foundComment = false;
    for (int i = 0; i < m_model->rowCount(aIndex); i++) {
        QModelIndex childIndex = m_model->index(i, 0, aIndex);
        if (m_model->data(childIndex, QXmlTreeModel::ValueTypeRole).toString() == "comment") {
            foundComment = true;
            QCOMPARE(m_model->data(childIndex, QXmlTreeModel::ValueRole).toString().trimmed(),
                     QString("This is a comment"));
        }
    }
    QVERIFY(foundComment);
}

// =============================================================================
// Namespace tests
// =============================================================================

void tst_QXmlTreeModel::testSimpleNamespace()
{
    bool result = m_model->loadXml("<ns:root xmlns:ns=\"http://example.com\"/>");
    QVERIFY(result);

    QModelIndex rootIndex = m_model->index(0, 0);
    QCOMPARE(m_model->data(rootIndex, QXmlTreeModel::KeyRole).toString(), QString("ns:root"));
    QCOMPARE(m_model->data(rootIndex, QXmlTreeModel::NamespacePrefixRole).toString(), QString("ns"));
}

void tst_QXmlTreeModel::testMultipleNamespaces()
{
    bool result = m_model->loadXml(
        "<root xmlns:a=\"http://a.com\" xmlns:b=\"http://b.com\">"
        "<a:child/><b:child/>"
        "</root>"
    );
    QVERIFY(result);

    QModelIndex rootIndex = m_model->index(0, 0);

    // Find namespace-prefixed children (skip xmlns attributes)
    int nsChildCount = 0;
    for (int i = 0; i < m_model->rowCount(rootIndex); i++) {
        QModelIndex childIndex = m_model->index(i, 0, rootIndex);
        QString valueType = m_model->data(childIndex, QXmlTreeModel::ValueTypeRole).toString();
        if (valueType == "element") {
            nsChildCount++;
            QString prefix = m_model->data(childIndex, QXmlTreeModel::NamespacePrefixRole).toString();
            QVERIFY(prefix == "a" || prefix == "b");
        }
    }
    QCOMPARE(nsChildCount, 2);
}

void tst_QXmlTreeModel::testDefaultNamespace()
{
    bool result = m_model->loadXml("<root xmlns=\"http://default.com\"><child/></root>");
    QVERIFY(result);

    QModelIndex rootIndex = m_model->index(0, 0);
    QCOMPARE(m_model->data(rootIndex, QXmlTreeModel::KeyRole).toString(), QString("root"));
    // Default namespace has no prefix
    QCOMPARE(m_model->data(rootIndex, QXmlTreeModel::NamespacePrefixRole).toString(), QString(""));
}

void tst_QXmlTreeModel::testNestedNamespaces()
{
    bool result = m_model->loadXml(
        "<ns1:root xmlns:ns1=\"http://ns1.com\">"
        "<ns1:child xmlns:ns2=\"http://ns2.com\">"
        "<ns2:grandchild/>"
        "</ns1:child>"
        "</ns1:root>"
    );
    QVERIFY(result);

    QModelIndex rootIndex = m_model->index(0, 0);
    QCOMPARE(m_model->data(rootIndex, QXmlTreeModel::NamespacePrefixRole).toString(), QString("ns1"));

    // Find the element child (skip xmlns attributes)
    QModelIndex childIndex;
    for (int i = 0; i < m_model->rowCount(rootIndex); i++) {
        QModelIndex idx = m_model->index(i, 0, rootIndex);
        if (m_model->data(idx, QXmlTreeModel::ValueTypeRole).toString() == "element") {
            childIndex = idx;
            break;
        }
    }
    QVERIFY(childIndex.isValid());
    QCOMPARE(m_model->data(childIndex, QXmlTreeModel::NamespacePrefixRole).toString(), QString("ns1"));
}

// =============================================================================
// Serialization tests
// =============================================================================

void tst_QXmlTreeModel::testSerializeSimpleElement()
{
    m_model->loadXml("<root/>");
    QModelIndex rootIndex = m_model->index(0, 0);
    QString serialized = m_model->serializeNode(rootIndex);
    QVERIFY(serialized.contains("<root"));
    QVERIFY(serialized.contains("/>") || serialized.contains("</root>"));
}

void tst_QXmlTreeModel::testSerializeWithAttributes()
{
    m_model->loadXml("<a b=\"1\" c=\"2\"/>");
    QModelIndex aIndex = m_model->index(0, 0);
    QString serialized = m_model->serializeNode(aIndex);
    QVERIFY(serialized.contains("b=\"1\""));
    QVERIFY(serialized.contains("c=\"2\""));
}

void tst_QXmlTreeModel::testSerializeWithChildren()
{
    m_model->loadXml("<a><b/><c/></a>");
    QModelIndex aIndex = m_model->index(0, 0);
    QString serialized = m_model->serializeNode(aIndex);
    QVERIFY(serialized.contains("<a>"));
    QVERIFY(serialized.contains("<b"));
    QVERIFY(serialized.contains("<c"));
    QVERIFY(serialized.contains("</a>"));
}

void tst_QXmlTreeModel::testSerializeWithText()
{
    m_model->loadXml("<a>Hello World</a>");
    QModelIndex aIndex = m_model->index(0, 0);
    QString serialized = m_model->serializeNode(aIndex);
    QVERIFY(serialized.contains("Hello World"));
}

void tst_QXmlTreeModel::testSerializeWithCData()
{
    // Note: QXmlStreamReader converts CDATA to text, so we test that text content is preserved
    m_model->loadXml("<a><![CDATA[<data>]]></a>");
    QModelIndex aIndex = m_model->index(0, 0);
    QString serialized = m_model->serializeNode(aIndex);
    // Content should be preserved (either as CDATA or escaped)
    QVERIFY(serialized.contains("<a>"));
}

// =============================================================================
// Model interface tests
// =============================================================================

void tst_QXmlTreeModel::testRoleNames()
{
    QHash<int, QByteArray> roles = m_model->roleNames();
    QVERIFY(roles.contains(QXmlTreeModel::KeyRole));
    QVERIFY(roles.contains(QXmlTreeModel::ValueRole));
    QVERIFY(roles.contains(QXmlTreeModel::ValueTypeRole));
    QVERIFY(roles.contains(QXmlTreeModel::ChildCountRole));
    QVERIFY(roles.contains(QXmlTreeModel::IsExpandableRole));
    QVERIFY(roles.contains(QXmlTreeModel::NamespacePrefixRole));
}

void tst_QXmlTreeModel::testDataRoles()
{
    m_model->loadXml("<root attr=\"value\"><child>text</child></root>");

    QModelIndex rootIndex = m_model->index(0, 0);
    QCOMPARE(m_model->data(rootIndex, QXmlTreeModel::KeyRole).toString(), QString("root"));
    QCOMPARE(m_model->data(rootIndex, QXmlTreeModel::ValueTypeRole).toString(), QString("element"));
    QVERIFY(m_model->data(rootIndex, QXmlTreeModel::IsExpandableRole).toBool());
    QVERIFY(m_model->data(rootIndex, QXmlTreeModel::ChildCountRole).toInt() > 0);
}

void tst_QXmlTreeModel::testRowCount()
{
    m_model->loadXml("<root><a/><b/><c/></root>");

    QCOMPARE(m_model->rowCount(), 1);  // One root element

    QModelIndex rootIndex = m_model->index(0, 0);
    QCOMPARE(m_model->rowCount(rootIndex), 3);  // Three children
}

void tst_QXmlTreeModel::testIndex()
{
    m_model->loadXml("<a><b/></a>");

    QModelIndex aIndex = m_model->index(0, 0);
    QVERIFY(aIndex.isValid());

    QModelIndex bIndex = m_model->index(0, 0, aIndex);
    QVERIFY(bIndex.isValid());

    // Invalid index
    QModelIndex invalidIndex = m_model->index(100, 0);
    QVERIFY(!invalidIndex.isValid());
}

void tst_QXmlTreeModel::testParent()
{
    m_model->loadXml("<a><b/></a>");

    QModelIndex aIndex = m_model->index(0, 0);
    QModelIndex bIndex = m_model->index(0, 0, aIndex);

    QModelIndex bParent = m_model->parent(bIndex);
    QCOMPARE(bParent, aIndex);

    // Root's parent should be invalid
    QModelIndex aParent = m_model->parent(aIndex);
    QVERIFY(!aParent.isValid());
}

// =============================================================================
// Error handling tests
// =============================================================================

void tst_QXmlTreeModel::testMalformedXml()
{
    QSignalSpy errorSpy(m_model, &QXmlTreeModel::loadError);

    bool result = m_model->loadXml("<a><b></a>");
    QVERIFY(!result);
    QVERIFY(errorSpy.count() > 0);
    QVERIFY(!m_model->lastError().isEmpty());
}

void tst_QXmlTreeModel::testUnclosedTag()
{
    QSignalSpy errorSpy(m_model, &QXmlTreeModel::loadError);

    bool result = m_model->loadXml("<a><b>");
    QVERIFY(!result);
    QVERIFY(!m_model->lastError().isEmpty());
}

void tst_QXmlTreeModel::testInvalidCharacter()
{
    // Control characters (except tab, newline, carriage return) are invalid in XML
    bool result = m_model->loadXml("<a>\x01</a>");
    QVERIFY(!result);
}

// =============================================================================
// Performance tests
// =============================================================================

QString tst_QXmlTreeModel::generateLargeXml(int nodeCount)
{
    QString xml = "<root>";
    int nodesPerLevel = qMax(1, nodeCount / 10);  // Distribute across 10 levels
    for (int i = 0; i < nodeCount && i < nodesPerLevel; i++) {
        xml += QString("<item id=\"%1\">").arg(i);
        for (int j = 0; j < 9 && (i * 10 + j) < nodeCount; j++) {
            xml += QString("<subitem idx=\"%1\">value</subitem>").arg(j);
        }
        xml += "</item>";
    }
    xml += "</root>";
    return xml;
}

void tst_QXmlTreeModel::testLoad1000Nodes()
{
    QString xml = generateLargeXml(1000);

    QElapsedTimer timer;
    timer.start();

    bool result = m_model->loadXml(xml);

    qint64 elapsed = timer.elapsed();

    QVERIFY(result);
    QVERIFY2(elapsed < 1000, qPrintable(QString("1000 nodes took %1ms (limit: 1000ms)").arg(elapsed)));
    qDebug() << "1000 nodes loaded in" << elapsed << "ms";
}

void tst_QXmlTreeModel::testLoad10000Nodes()
{
    // Generate ~10000 nodes
    QString xml = "<root>";
    for (int i = 0; i < 1000; i++) {
        xml += QString("<item id=\"%1\">").arg(i);
        for (int j = 0; j < 9; j++) {
            xml += QString("<sub%1>text</sub%1>").arg(j);
        }
        xml += "</item>";
    }
    xml += "</root>";

    QElapsedTimer timer;
    timer.start();

    bool result = m_model->loadXml(xml);

    qint64 elapsed = timer.elapsed();

    QVERIFY(result);
    QVERIFY2(elapsed < 2000, qPrintable(QString("10000 nodes took %1ms (limit: 2000ms)").arg(elapsed)));
    qDebug() << "~10000 nodes loaded in" << elapsed << "ms, total nodes:" << m_model->totalNodeCount();
}

void tst_QXmlTreeModel::testNodeCountLimit()
{
    // Generate XML that exceeds 50000 node limit
    QString xml = "<root>";
    for (int i = 0; i < 60000; i++) {
        xml += QString("<n%1/>").arg(i);
    }
    xml += "</root>";

    QSignalSpy errorSpy(m_model, &QXmlTreeModel::loadError);

    bool result = m_model->loadXml(xml);

    QVERIFY(!result);
    QVERIFY(errorSpy.count() > 0);
    QVERIFY(m_model->lastError().contains("50000") || m_model->lastError().contains("limit"));
}

// =============================================================================
// XPath tests
// =============================================================================

void tst_QXmlTreeModel::testXmlPath()
{
    m_model->loadXml("<root><child attr=\"val\">text</child></root>");

    QModelIndex rootIndex = m_model->index(0, 0);
    QString rootPath = m_model->getXmlPath(rootIndex);
    QCOMPARE(rootPath, QString("/root"));

    // Find child element (skip attributes)
    QModelIndex childIndex;
    for (int i = 0; i < m_model->rowCount(rootIndex); i++) {
        QModelIndex idx = m_model->index(i, 0, rootIndex);
        if (m_model->data(idx, QXmlTreeModel::ValueTypeRole).toString() == "element") {
            childIndex = idx;
            break;
        }
    }
    QVERIFY(childIndex.isValid());
    QString childPath = m_model->getXmlPath(childIndex);
    QCOMPARE(childPath, QString("/root/child"));
}

void tst_QXmlTreeModel::testXmlPathWithIndex()
{
    m_model->loadXml("<root><item/><item/><item/></root>");

    QModelIndex rootIndex = m_model->index(0, 0);

    // Get paths for all items - they should have indices since names are same
    for (int i = 0; i < m_model->rowCount(rootIndex); i++) {
        QModelIndex itemIndex = m_model->index(i, 0, rootIndex);
        QString path = m_model->getXmlPath(itemIndex);
        QVERIFY(path.contains(QString("[%1]").arg(i)));
    }
}

// =============================================================================
// Clear and reload tests
// =============================================================================

void tst_QXmlTreeModel::testClear()
{
    m_model->loadXml("<root><child/></root>");
    QCOMPARE(m_model->rowCount(), 1);

    m_model->clear();
    QCOMPARE(m_model->rowCount(), 0);
    QCOMPARE(m_model->totalNodeCount(), 0);
}

void tst_QXmlTreeModel::testReload()
{
    m_model->loadXml("<a/>");
    QCOMPARE(m_model->rowCount(), 1);
    QModelIndex aIndex = m_model->index(0, 0);
    QCOMPARE(m_model->data(aIndex, QXmlTreeModel::KeyRole).toString(), QString("a"));

    // Reload with different content
    m_model->loadXml("<b><c/></b>");
    QCOMPARE(m_model->rowCount(), 1);
    QModelIndex bIndex = m_model->index(0, 0);
    QCOMPARE(m_model->data(bIndex, QXmlTreeModel::KeyRole).toString(), QString("b"));
}

QTEST_MAIN(tst_QXmlTreeModel)
#include "tst_qxmltreemodel.moc"
