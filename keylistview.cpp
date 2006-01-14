/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QPainter>
#include <QString>

#include <Q3ListViewItem>

#include <kabc/stdaddressbook.h>
#include <klocale.h>

#include "keylistview.h"

KeyListViewItem::KeyListViewItem(KListView *parent, QString name, QString email, QString trust, QString expiration, QString size, QString creation, QString id, bool isdefault, bool isexpired, ItemType type)
               : KListViewItem(parent)
{
    m_def = isdefault;
    m_exp = isexpired;
    m_type = type;
    setText(0, name);
    setText(1, email);
    setText(2, trust);
    setText(3, expiration);
    setText(4, size);
    setText(5, creation);
    setText(6, id);
}

KeyListViewItem::KeyListViewItem(KListViewItem *parent, QString name, QString email, QString trust, QString expiration, QString size, QString creation, QString id, bool isdefault, bool isexpired, ItemType type)
               : KListViewItem(parent)
{
    m_def = isdefault;
    m_exp = isexpired;
    m_type = type;
    setText(0, name);
    setText(1, email);
    setText(2, trust);
    setText(3, expiration);
    setText(4, size);
    setText(5, creation);
    setText(6, id);
}

void KeyListViewItem::setItemType(const ItemType &type)
{
    m_type = type;
}

KeyListViewItem::ItemType KeyListViewItem::itemType() const
{
    return m_type;
}

void KeyListViewItem::setDefault(const bool &def)
{
    m_def = def;
}

bool KeyListViewItem::isDefault() const
{
    return m_def;
}

void KeyListViewItem::setExpired(const bool &exp)
{
    m_exp = exp;
}

bool KeyListViewItem::isExpired() const
{
    return m_exp;
}

void KeyListViewItem::paintCell(QPainter *p, const QColorGroup &cg, int column, int width, int alignment)
{
    QColorGroup _cg(cg);

    if (itemType() & Group)
        return;

    if (itemType() & Public)
    {
        if (m_def && (column < 2))
        {
            QFont font(p->font());
            font.setBold(true);
            p->setFont(font);
        }
        else
        if (m_exp && (column == 3))
            _cg.setColor(QColorGroup::Text, Qt::red);
    }
    else
    if (column < 2)
    {
        QFont font(p->font());
        font.setItalic(true);
        p->setFont(font);
    }

    KListViewItem::paintCell(p, _cg, column, width, alignment);
}

int KeyListViewItem::compare(Q3ListViewItem *item2, int c, bool ascending) const
{
    KeyListViewItem *item = static_cast<KeyListViewItem*>(item2);

    if (c == 0)
    {
        ItemType item1 = itemType();
        ItemType item2 = item->itemType();

        if (item1 < item2) return -1;
        if (item1 > item2) return 1;

        return KListViewItem::compare(item, c, ascending);
    }

    if ((c == 3) || (c == 5))  // by (3) expiration date or (5) creation date
    {
        QDate d = KGlobal::locale()->readDate(text(c));
        QDate itemDate = KGlobal::locale()->readDate(item->text(c));

        bool thisDateValid = d.isValid();
        bool itemDateValid = itemDate.isValid();

        if (thisDateValid)
        {
            if (itemDateValid)
            {
                if (d < itemDate) return -1;
                if (d > itemDate) return  1;
            }
            else
                return -1;
        }
        else
        if (itemDateValid)
            return 1;

        return 0;
    }

    if (c == 2)  // sorting by pixmap
    {
        const QPixmap* pix = pixmap(c);
        const QPixmap* itemPix = item->pixmap(c);

        int serial;
        int itemSerial;

        if (!pix)
            serial = 0;
        else
            serial = pix->serialNumber();

        if (!itemPix)
            itemSerial = 0;
        else
            itemSerial = itemPix->serialNumber();

        if (serial < itemSerial) return -1;
        if (serial > itemSerial) return  1;
        return 0;
    }

    return KListViewItem::compare(item, c, ascending);
}

QString KeyListViewItem::key(int column, bool) const
{
    return text(column).toLower();
}

#include "keylistview.moc"
