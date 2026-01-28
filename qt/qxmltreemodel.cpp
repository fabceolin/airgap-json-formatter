#include "qxmltreemodel.h"
#include <QXmlStreamReader>
#include <QStack>
#include <QDebug>

QXmlTreeModel::QXmlTreeModel(QObject* parent)
    : QAbstractItemModel(parent)
    , m_rootItem(nullptr)
    , m_lastErrorLine(0)
    , m_lastErrorColumn(0)
    , m_nodeCount(0)
{
}

QXmlTreeModel::~QXmlTreeModel()
{
    delete m_rootItem;
}

QModelIndex QXmlTreeModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    QXmlTreeItem* parentItem;
    if (!parent.isValid()) {
        parentItem = m_rootItem;
    } else {
        parentItem = static_cast<QXmlTreeItem*>(parent.internalPointer());
    }

    if (!parentItem)
        return QModelIndex();

    QXmlTreeItem* childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);

    return QModelIndex();
}

QModelIndex QXmlTreeModel::parent(const QModelIndex& index) const
{
    if (!index.isValid())
        return QModelIndex();

    QXmlTreeItem* childItem = static_cast<QXmlTreeItem*>(index.internalPointer());
    if (!childItem)
        return QModelIndex();

    QXmlTreeItem* parentItem = childItem->parent();

    if (!parentItem || parentItem == m_rootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int QXmlTreeModel::rowCount(const QModelIndex& parent) const
{
    if (parent.column() > 0)
        return 0;

    QXmlTreeItem* parentItem;
    if (!parent.isValid()) {
        parentItem = m_rootItem;
    } else {
        parentItem = static_cast<QXmlTreeItem*>(parent.internalPointer());
    }

    if (!parentItem)
        return 0;

    return parentItem->childCount();
}

int QXmlTreeModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return 1;
}

QVariant QXmlTreeModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    QXmlTreeItem* item = static_cast<QXmlTreeItem*>(index.internalPointer());
    if (!item)
        return QVariant();

    switch (role) {
        case KeyRole: {
            // For elements with namespace prefix, show as "ns:element"
            if (item->type() == QXmlTreeItem::ItemType::Element && !item->namespacePrefix().isEmpty()) {
                return item->namespacePrefix() + QStringLiteral(":") + item->key();
            }
            return item->key();
        }
        case ValueRole:
            return item->value();
        case ValueTypeRole:
            return item->typeName();
        case XmlPathRole:
            return item->xmlPath();
        case ChildCountRole:
            return item->childCount();
        case IsExpandableRole:
            return item->isExpandable();
        case IsLastChildRole:
            return item->isLastChild();
        case NamespacePrefixRole:
            return item->namespacePrefix();
        case Qt::DisplayRole: {
            // For display, combine key and value
            QString displayKey = item->key();
            if (item->type() == QXmlTreeItem::ItemType::Element && !item->namespacePrefix().isEmpty()) {
                displayKey = item->namespacePrefix() + QStringLiteral(":") + item->key();
            }
            if (item->value().isEmpty()) {
                return displayKey;
            }
            return displayKey + QStringLiteral(": ") + item->value();
        }
        default:
            return QVariant();
    }
}

QHash<int, QByteArray> QXmlTreeModel::roleNames() const
{
    return {
        {KeyRole, "key"},
        {ValueRole, "value"},
        {ValueTypeRole, "valueType"},
        {XmlPathRole, "jsonPath"},  // Use jsonPath for compatibility with TreeView
        {ChildCountRole, "childCount"},
        {IsExpandableRole, "isExpandable"},
        {IsLastChildRole, "isLastChild"},
        {NamespacePrefixRole, "namespacePrefix"}
    };
}

bool QXmlTreeModel::loadXml(const QString& xmlString)
{
    beginResetModel();

    delete m_rootItem;
    m_rootItem = nullptr;
    m_lastError.clear();
    m_lastErrorLine = 0;
    m_lastErrorColumn = 0;
    m_nodeCount = 0;

    if (xmlString.trimmed().isEmpty()) {
        endResetModel();
        return true;
    }

    bool success = parseXml(xmlString);

    endResetModel();
    return success;
}

bool QXmlTreeModel::parseXml(const QString& xmlString)
{
    m_rootItem = new QXmlTreeItem(QXmlTreeItem::ItemType::Root, QString(), QString());
    m_nodeCount = 1;

    QXmlStreamReader reader(xmlString);
    QStack<QXmlTreeItem*> stack;
    stack.push(m_rootItem);

    while (!reader.atEnd()) {
        QXmlStreamReader::TokenType token = reader.readNext();

        switch (token) {
        case QXmlStreamReader::StartElement: {
            // Check node count limit (PERF-001 mitigation)
            if (m_nodeCount >= MAX_NODE_COUNT) {
                m_lastError = QStringLiteral("Document exceeds maximum node limit of %1").arg(MAX_NODE_COUNT);
                m_lastErrorLine = static_cast<int>(reader.lineNumber());
                m_lastErrorColumn = static_cast<int>(reader.columnNumber());
                emit loadError(m_lastError, m_lastErrorLine, m_lastErrorColumn);
                delete m_rootItem;
                m_rootItem = nullptr;
                return false;
            }

            auto* item = new QXmlTreeItem(
                QXmlTreeItem::ItemType::Element,
                reader.name().toString(),
                QString(),
                stack.top()
            );
            item->setNamespacePrefix(reader.prefix().toString());
            stack.top()->appendChild(item);
            m_nodeCount++;

            // Add attributes as children
            const auto attributes = reader.attributes();
            for (const auto& attr : attributes) {
                // Check node count limit for each attribute
                if (m_nodeCount >= MAX_NODE_COUNT) {
                    m_lastError = QStringLiteral("Document exceeds maximum node limit of %1").arg(MAX_NODE_COUNT);
                    m_lastErrorLine = static_cast<int>(reader.lineNumber());
                    m_lastErrorColumn = static_cast<int>(reader.columnNumber());
                    emit loadError(m_lastError, m_lastErrorLine, m_lastErrorColumn);
                    delete m_rootItem;
                    m_rootItem = nullptr;
                    return false;
                }

                QString attrName = QStringLiteral("@");
                if (!attr.prefix().isEmpty()) {
                    attrName += attr.prefix().toString() + QStringLiteral(":") + attr.name().toString();
                } else {
                    attrName += attr.name().toString();
                }
                auto* attrItem = new QXmlTreeItem(
                    QXmlTreeItem::ItemType::Attribute,
                    attrName,
                    attr.value().toString(),
                    item
                );
                item->appendChild(attrItem);
                m_nodeCount++;
            }

            stack.push(item);
            break;
        }
        case QXmlStreamReader::EndElement:
            if (!stack.isEmpty() && stack.top() != m_rootItem) {
                stack.pop();
            }
            break;
        case QXmlStreamReader::Characters:
            if (!reader.isWhitespace()) {
                // Check node count limit
                if (m_nodeCount >= MAX_NODE_COUNT) {
                    m_lastError = QStringLiteral("Document exceeds maximum node limit of %1").arg(MAX_NODE_COUNT);
                    m_lastErrorLine = static_cast<int>(reader.lineNumber());
                    m_lastErrorColumn = static_cast<int>(reader.columnNumber());
                    emit loadError(m_lastError, m_lastErrorLine, m_lastErrorColumn);
                    delete m_rootItem;
                    m_rootItem = nullptr;
                    return false;
                }

                auto* textItem = new QXmlTreeItem(
                    QXmlTreeItem::ItemType::Text,
                    QString(),
                    reader.text().toString(),
                    stack.top()
                );
                stack.top()->appendChild(textItem);
                m_nodeCount++;
            }
            break;
        case QXmlStreamReader::Comment: {
            // Check node count limit
            if (m_nodeCount >= MAX_NODE_COUNT) {
                m_lastError = QStringLiteral("Document exceeds maximum node limit of %1").arg(MAX_NODE_COUNT);
                m_lastErrorLine = static_cast<int>(reader.lineNumber());
                m_lastErrorColumn = static_cast<int>(reader.columnNumber());
                emit loadError(m_lastError, m_lastErrorLine, m_lastErrorColumn);
                delete m_rootItem;
                m_rootItem = nullptr;
                return false;
            }

            auto* commentItem = new QXmlTreeItem(
                QXmlTreeItem::ItemType::Comment,
                QStringLiteral("<!-- -->"),
                reader.text().toString(),
                stack.top()
            );
            stack.top()->appendChild(commentItem);
            m_nodeCount++;
            break;
        }
        case QXmlStreamReader::DTD:
        case QXmlStreamReader::ProcessingInstruction:
        case QXmlStreamReader::StartDocument:
        case QXmlStreamReader::EndDocument:
        case QXmlStreamReader::EntityReference:
        case QXmlStreamReader::NoToken:
        case QXmlStreamReader::Invalid:
            // Handle these cases or ignore as needed
            break;
        }
    }

    if (reader.hasError()) {
        m_lastError = reader.errorString();
        m_lastErrorLine = static_cast<int>(reader.lineNumber());
        m_lastErrorColumn = static_cast<int>(reader.columnNumber());
        emit loadError(m_lastError, m_lastErrorLine, m_lastErrorColumn);
        delete m_rootItem;
        m_rootItem = nullptr;
        return false;
    }

    return true;
}

void QXmlTreeModel::clear()
{
    beginResetModel();
    delete m_rootItem;
    m_rootItem = nullptr;
    m_lastError.clear();
    m_lastErrorLine = 0;
    m_lastErrorColumn = 0;
    m_nodeCount = 0;
    endResetModel();
}

QString QXmlTreeModel::serializeNode(const QModelIndex& index) const
{
    if (!index.isValid())
        return QString();

    QXmlTreeItem* item = static_cast<QXmlTreeItem*>(index.internalPointer());
    if (!item)
        return QString();

    return item->toXmlString();
}

QString QXmlTreeModel::getXmlPath(const QModelIndex& index) const
{
    if (!index.isValid())
        return QString();

    QXmlTreeItem* item = static_cast<QXmlTreeItem*>(index.internalPointer());
    if (!item)
        return QString();

    return item->xmlPath();
}

int QXmlTreeModel::totalNodeCount() const
{
    return m_nodeCount;
}

int QXmlTreeModel::countNodes(QXmlTreeItem* item) const
{
    if (!item)
        return 0;

    int count = 1;  // Count this node
    for (int i = 0; i < item->childCount(); ++i) {
        count += countNodes(item->child(i));
    }
    return count;
}
