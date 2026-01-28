#ifndef QXMLTREEITEM_H
#define QXMLTREEITEM_H

#include <QString>
#include <QList>

class QXmlTreeItem
{
public:
    enum class ItemType {
        Root,
        Element,
        Attribute,
        Text,
        Comment,
        CData
    };

    explicit QXmlTreeItem(ItemType type,
                          const QString& key,
                          const QString& value,
                          QXmlTreeItem* parent = nullptr);
    ~QXmlTreeItem();

    void appendChild(QXmlTreeItem* child);
    QXmlTreeItem* child(int row) const;
    int childCount() const;
    int row() const;
    QXmlTreeItem* parent() const;

    ItemType type() const { return m_type; }
    QString key() const { return m_key; }      // Tag name or "@attrName"
    QString value() const { return m_value; }  // Text content or attr value
    QString namespacePrefix() const { return m_nsPrefix; }
    void setNamespacePrefix(const QString& prefix) { m_nsPrefix = prefix; }

    // Type name for display
    QString typeName() const;

    // XPath-like path: /root/child[0]
    QString xmlPath() const;

    // Is expandable (has children)
    bool isExpandable() const;

    // Is this the last child of its parent
    bool isLastChild() const;

    // Serialize this node and its children to XML string
    QString toXmlString(int indentLevel = 0) const;

private:
    ItemType m_type;
    QString m_key;
    QString m_value;
    QString m_nsPrefix;
    QList<QXmlTreeItem*> m_children;
    QXmlTreeItem* m_parent;
};

#endif // QXMLTREEITEM_H
