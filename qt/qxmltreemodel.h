#ifndef QXMLTREEMODEL_H
#define QXMLTREEMODEL_H

#include <QAbstractItemModel>
#include "qxmltreeitem.h"

class QXmlTreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum Roles {
        KeyRole = Qt::UserRole + 1,      // Tag name or @attribute
        ValueRole,                        // Text content or attribute value
        ValueTypeRole,                    // "element", "attribute", "text", "comment", "cdata"
        XmlPathRole,                      // XPath-like path: /root/child[0]
        ChildCountRole,                   // Number of children
        IsExpandableRole,                 // Has children
        IsLastChildRole,                  // For tree line drawing
        NamespacePrefixRole               // "ns" for ns:element
    };
    Q_ENUM(Roles)

    explicit QXmlTreeModel(QObject* parent = nullptr);
    ~QXmlTreeModel() override;

    // QAbstractItemModel interface
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& index) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // XML loading
    Q_INVOKABLE bool loadXml(const QString& xmlString);
    Q_INVOKABLE void clear();

    // Serialization for copy functionality
    Q_INVOKABLE QString serializeNode(const QModelIndex& index) const;
    Q_INVOKABLE QString getXmlPath(const QModelIndex& index) const;

    // Node counting for performance guard
    Q_INVOKABLE int totalNodeCount() const;

    // Error information
    Q_INVOKABLE QString lastError() const { return m_lastError; }
    Q_INVOKABLE int lastErrorLine() const { return m_lastErrorLine; }
    Q_INVOKABLE int lastErrorColumn() const { return m_lastErrorColumn; }

    // Maximum node limit (PERF-001 mitigation)
    static constexpr int MAX_NODE_COUNT = 50000;

signals:
    void loadError(const QString& error, int line, int column);

private:
    int countNodes(QXmlTreeItem* item) const;
    bool parseXml(const QString& xmlString);

    QXmlTreeItem* m_rootItem;
    QString m_lastError;
    int m_lastErrorLine;
    int m_lastErrorColumn;
    int m_nodeCount;
};

#endif // QXMLTREEMODEL_H
