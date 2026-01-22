#include "qjsontreeitem.h"
#include <QJsonDocument>

QJsonTreeItem::QJsonTreeItem(QJsonTreeItem* parent)
    : m_parent(parent)
    , m_type(Type::Null)
{
}

QJsonTreeItem::~QJsonTreeItem()
{
    qDeleteAll(m_children);
}

void QJsonTreeItem::appendChild(QJsonTreeItem* child)
{
    m_children.append(child);
}

QJsonTreeItem* QJsonTreeItem::child(int row) const
{
    if (row < 0 || row >= m_children.size())
        return nullptr;
    return m_children.at(row);
}

int QJsonTreeItem::childCount() const
{
    return m_children.size();
}

int QJsonTreeItem::row() const
{
    if (m_parent) {
        return m_parent->m_children.indexOf(const_cast<QJsonTreeItem*>(this));
    }
    return 0;
}

QJsonTreeItem* QJsonTreeItem::parentItem() const
{
    return m_parent;
}

QString QJsonTreeItem::key() const
{
    return m_key;
}

void QJsonTreeItem::setKey(const QString& key)
{
    m_key = key;
}

QVariant QJsonTreeItem::value() const
{
    return m_value;
}

void QJsonTreeItem::setValue(const QVariant& value)
{
    m_value = value;
}

QJsonTreeItem::Type QJsonTreeItem::type() const
{
    return m_type;
}

void QJsonTreeItem::setType(Type type)
{
    m_type = type;
}

QString QJsonTreeItem::typeName() const
{
    switch (m_type) {
        case Type::Object:  return QStringLiteral("object");
        case Type::Array:   return QStringLiteral("array");
        case Type::String:  return QStringLiteral("string");
        case Type::Number:  return QStringLiteral("number");
        case Type::Boolean: return QStringLiteral("boolean");
        case Type::Null:    return QStringLiteral("null");
    }
    return QStringLiteral("unknown");
}

QString QJsonTreeItem::jsonPath() const
{
    if (!m_parent) {
        return QStringLiteral("$");
    }

    QString parentPath = m_parent->jsonPath();

    // Skip items with empty key (virtual root level)
    if (m_key.isEmpty()) {
        return parentPath;
    }

    if (m_parent->type() == Type::Array) {
        return parentPath + QStringLiteral("[") + m_key + QStringLiteral("]");
    } else {
        // Check if key needs bracket notation (contains special chars)
        bool needsBracket = m_key.contains('.') || m_key.contains(' ') ||
                           m_key.contains('[') || m_key.contains(']');
        if (needsBracket) {
            return parentPath + QStringLiteral("[\"") + m_key + QStringLiteral("\"]");
        }
        return parentPath + QStringLiteral(".") + m_key;
    }
}

bool QJsonTreeItem::isExpandable() const
{
    return (m_type == Type::Object || m_type == Type::Array) && m_children.size() > 0;
}

QString QJsonTreeItem::toJsonString(int indentLevel) const
{
    const QString indent(indentLevel * 2, ' ');
    const QString childIndent((indentLevel + 1) * 2, ' ');

    switch (m_type) {
        case Type::Object: {
            if (m_children.isEmpty()) {
                return QStringLiteral("{}");
            }
            QStringList items;
            for (const auto* child : m_children) {
                QString item = childIndent + QStringLiteral("\"") + child->key() + QStringLiteral("\": ") +
                              child->toJsonString(indentLevel + 1);
                items.append(item);
            }
            return QStringLiteral("{\n") + items.join(QStringLiteral(",\n")) + QStringLiteral("\n") + indent + QStringLiteral("}");
        }
        case Type::Array: {
            if (m_children.isEmpty()) {
                return QStringLiteral("[]");
            }
            QStringList items;
            for (const auto* child : m_children) {
                items.append(childIndent + child->toJsonString(indentLevel + 1));
            }
            return QStringLiteral("[\n") + items.join(QStringLiteral(",\n")) + QStringLiteral("\n") + indent + QStringLiteral("]");
        }
        case Type::String:
            return QStringLiteral("\"") + m_value.toString().replace(QStringLiteral("\\"), QStringLiteral("\\\\"))
                                                          .replace(QStringLiteral("\""), QStringLiteral("\\\""))
                                                          .replace(QStringLiteral("\n"), QStringLiteral("\\n"))
                                                          .replace(QStringLiteral("\r"), QStringLiteral("\\r"))
                                                          .replace(QStringLiteral("\t"), QStringLiteral("\\t")) + QStringLiteral("\"");
        case Type::Number:
            return m_value.toString();
        case Type::Boolean:
            return m_value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
        case Type::Null:
            return QStringLiteral("null");
    }
    return QStringLiteral("null");
}

QJsonTreeItem* QJsonTreeItem::load(const QJsonValue& value, QJsonTreeItem* parent)
{
    QJsonTreeItem* item = new QJsonTreeItem(parent);

    if (value.isObject()) {
        item->setType(Type::Object);
        QJsonObject obj = value.toObject();
        for (auto it = obj.begin(); it != obj.end(); ++it) {
            QJsonTreeItem* child = load(it.value(), item);
            child->setKey(it.key());
            item->appendChild(child);
        }
    } else if (value.isArray()) {
        item->setType(Type::Array);
        QJsonArray arr = value.toArray();
        for (int i = 0; i < arr.size(); ++i) {
            QJsonTreeItem* child = load(arr.at(i), item);
            child->setKey(QString::number(i));
            item->appendChild(child);
        }
    } else if (value.isString()) {
        item->setType(Type::String);
        item->setValue(value.toString());
    } else if (value.isDouble()) {
        item->setType(Type::Number);
        // Preserve integer vs double representation
        double d = value.toDouble();
        if (d == static_cast<qint64>(d) && d >= -9007199254740992.0 && d <= 9007199254740992.0) {
            item->setValue(static_cast<qint64>(d));
        } else {
            item->setValue(d);
        }
    } else if (value.isBool()) {
        item->setType(Type::Boolean);
        item->setValue(value.toBool());
    } else {
        item->setType(Type::Null);
        item->setValue(QVariant());
    }

    return item;
}
