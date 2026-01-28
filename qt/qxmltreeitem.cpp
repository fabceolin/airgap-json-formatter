#include "qxmltreeitem.h"

QXmlTreeItem::QXmlTreeItem(ItemType type,
                           const QString& key,
                           const QString& value,
                           QXmlTreeItem* parent)
    : m_type(type)
    , m_key(key)
    , m_value(value)
    , m_parent(parent)
{
}

QXmlTreeItem::~QXmlTreeItem()
{
    qDeleteAll(m_children);
}

void QXmlTreeItem::appendChild(QXmlTreeItem* child)
{
    m_children.append(child);
}

QXmlTreeItem* QXmlTreeItem::child(int row) const
{
    if (row < 0 || row >= m_children.size())
        return nullptr;
    return m_children.at(row);
}

int QXmlTreeItem::childCount() const
{
    return m_children.size();
}

int QXmlTreeItem::row() const
{
    if (m_parent) {
        return m_parent->m_children.indexOf(const_cast<QXmlTreeItem*>(this));
    }
    return 0;
}

QXmlTreeItem* QXmlTreeItem::parent() const
{
    return m_parent;
}

QString QXmlTreeItem::typeName() const
{
    switch (m_type) {
        case ItemType::Root:      return QStringLiteral("root");
        case ItemType::Element:   return QStringLiteral("element");
        case ItemType::Attribute: return QStringLiteral("attribute");
        case ItemType::Text:      return QStringLiteral("text");
        case ItemType::Comment:   return QStringLiteral("comment");
        case ItemType::CData:     return QStringLiteral("cdata");
    }
    return QStringLiteral("unknown");
}

QString QXmlTreeItem::xmlPath() const
{
    if (!m_parent) {
        return QString();
    }

    QString parentPath = m_parent->xmlPath();

    switch (m_type) {
        case ItemType::Root:
            return QString();
        case ItemType::Element: {
            QString elementName = m_nsPrefix.isEmpty() ? m_key : (m_nsPrefix + QStringLiteral(":") + m_key);
            // Count siblings with same name to create index
            int sameNameIndex = 0;
            int sameNameCount = 0;
            for (int i = 0; i < m_parent->childCount(); ++i) {
                QXmlTreeItem* sibling = m_parent->child(i);
                if (sibling->type() == ItemType::Element && sibling->key() == m_key) {
                    if (sibling == this) {
                        sameNameIndex = sameNameCount;
                    }
                    sameNameCount++;
                }
            }
            // Only add index if there are multiple elements with same name
            if (sameNameCount > 1) {
                return parentPath + QStringLiteral("/") + elementName + QStringLiteral("[") + QString::number(sameNameIndex) + QStringLiteral("]");
            }
            return parentPath + QStringLiteral("/") + elementName;
        }
        case ItemType::Attribute:
            return parentPath + QStringLiteral("/@") + m_key.mid(1); // Remove the leading @
        case ItemType::Text:
            return parentPath + QStringLiteral("/text()");
        case ItemType::Comment:
            return parentPath + QStringLiteral("/comment()");
        case ItemType::CData:
            return parentPath + QStringLiteral("/text()");
    }
    return parentPath;
}

bool QXmlTreeItem::isExpandable() const
{
    return !m_children.isEmpty();
}

bool QXmlTreeItem::isLastChild() const
{
    if (!m_parent)
        return true;
    return m_parent->childCount() > 0 && m_parent->child(m_parent->childCount() - 1) == this;
}

QString QXmlTreeItem::toXmlString(int indentLevel) const
{
    const QString indent(indentLevel * 2, ' ');
    const QString childIndent((indentLevel + 1) * 2, ' ');

    switch (m_type) {
        case ItemType::Root: {
            // Serialize all children
            QStringList parts;
            for (const auto* child : m_children) {
                parts.append(child->toXmlString(indentLevel));
            }
            return parts.join(QStringLiteral("\n"));
        }
        case ItemType::Element: {
            QString elementName = m_nsPrefix.isEmpty() ? m_key : (m_nsPrefix + QStringLiteral(":") + m_key);
            QString result = indent + QStringLiteral("<") + elementName;

            // Collect attributes
            QStringList attrStrings;
            QList<QXmlTreeItem*> nonAttrChildren;
            for (auto* child : m_children) {
                if (child->type() == ItemType::Attribute) {
                    QString attrName = child->key().mid(1); // Remove @ prefix
                    QString attrValue = child->value();
                    // Escape attribute value
                    attrValue.replace(QStringLiteral("&"), QStringLiteral("&amp;"));
                    attrValue.replace(QStringLiteral("<"), QStringLiteral("&lt;"));
                    attrValue.replace(QStringLiteral(">"), QStringLiteral("&gt;"));
                    attrValue.replace(QStringLiteral("\""), QStringLiteral("&quot;"));
                    attrStrings.append(attrName + QStringLiteral("=\"") + attrValue + QStringLiteral("\""));
                } else {
                    nonAttrChildren.append(child);
                }
            }

            // Add attributes to opening tag
            for (const auto& attr : attrStrings) {
                result += QStringLiteral(" ") + attr;
            }

            if (nonAttrChildren.isEmpty()) {
                // Self-closing tag
                result += QStringLiteral("/>");
            } else if (nonAttrChildren.size() == 1 &&
                       (nonAttrChildren[0]->type() == ItemType::Text ||
                        nonAttrChildren[0]->type() == ItemType::CData)) {
                // Inline text content
                result += QStringLiteral(">");
                if (nonAttrChildren[0]->type() == ItemType::CData) {
                    result += QStringLiteral("<![CDATA[") + nonAttrChildren[0]->value() + QStringLiteral("]]>");
                } else {
                    QString textVal = nonAttrChildren[0]->value();
                    textVal.replace(QStringLiteral("&"), QStringLiteral("&amp;"));
                    textVal.replace(QStringLiteral("<"), QStringLiteral("&lt;"));
                    textVal.replace(QStringLiteral(">"), QStringLiteral("&gt;"));
                    result += textVal;
                }
                result += QStringLiteral("</") + elementName + QStringLiteral(">");
            } else {
                // Multi-line with children
                result += QStringLiteral(">\n");
                for (auto* child : nonAttrChildren) {
                    result += child->toXmlString(indentLevel + 1) + QStringLiteral("\n");
                }
                result += indent + QStringLiteral("</") + elementName + QStringLiteral(">");
            }
            return result;
        }
        case ItemType::Attribute:
            // Attributes are handled in Element serialization
            return QString();
        case ItemType::Text: {
            QString textVal = m_value;
            textVal.replace(QStringLiteral("&"), QStringLiteral("&amp;"));
            textVal.replace(QStringLiteral("<"), QStringLiteral("&lt;"));
            textVal.replace(QStringLiteral(">"), QStringLiteral("&gt;"));
            return indent + textVal;
        }
        case ItemType::Comment:
            return indent + QStringLiteral("<!--") + m_value + QStringLiteral("-->");
        case ItemType::CData:
            return indent + QStringLiteral("<![CDATA[") + m_value + QStringLiteral("]]>");
    }
    return QString();
}
