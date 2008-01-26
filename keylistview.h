/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KEYLISTVIEW_H
#define KEYLISTVIEW_H

#include <QColorGroup>
#include <QPixmap>


#include <K3ListViewSearchLine>
#include <K3ListView>
#include <KUrl>

#include "kgpgkey.h"

using namespace KgpgCore;

class Q3ListViewItem;
class QDragMoveEvent;
class QDropEvent;
class QPainter;

class KeyListView;

class KeyListViewItem : public K3ListViewItem
{
public:
    enum ItemTypeFlag
    {
        Group = 1,
        Secret = 2,
        Public = 4,
        Pair = Secret | Public,
        GSecret = Group | Secret,
        GPublic = Group | Public,
        GPair = Group | Pair,
        Sub = 8,
        Uid = 16,
        Uat = 32,
        RevSign = 64,
        Sign = 128
    };
    Q_DECLARE_FLAGS(ItemType, ItemTypeFlag)

    KeyListViewItem(K3ListView *parent, const KgpgKey &key, const bool isbold);
    KeyListViewItem(K3ListViewItem *parent, const KgpgKeySign &sig);
    ~KeyListViewItem();

    ItemType itemType() const;

    bool isDefault() const;

    virtual QString key(int column, bool) const;
    virtual KeyListViewItem *parent() const { return static_cast<KeyListViewItem*>(K3ListViewItem::parent()); }
    virtual KeyListViewItem *nextSibling() const { return static_cast<KeyListViewItem*>(K3ListViewItem::nextSibling()); }
    virtual KeyListViewItem *firstChild() const { return static_cast<KeyListViewItem*>(K3ListViewItem::firstChild()); }
    virtual const QString keyId(void) const { return m_key ? m_key->fullId() : m_sig ? m_sig->fullId() : groupId ? *groupId : text(6); }
    KgpgKeyTrust trust(void) const { return m_key ? m_key->trust() : TRUST_NOKEY; }

private:
    bool m_def; /// Is set to \em true if it is the default key, \em false otherwise.
    bool m_exp; /// Is set to \em true if the key is expired, \em false otherwise.
    ItemType m_type;
    KgpgKey *m_key;
    KgpgKeySign *m_sig;
    QString *groupId;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(KeyListViewItem::ItemType)


class KeyListView : public K3ListView
{
    Q_OBJECT
    friend class KeysManager;

public:
    KeyListView(QWidget *parent = 0);

    QPixmap trustbad;

signals:
    void statusMessage(QString, int, bool keep = false);

public slots:
    virtual KeyListViewItem *firstChild() { return static_cast<KeyListViewItem*>(K3ListView::firstChild()); }
    QString statusCountMessage(void);

protected:
    virtual KeyListViewItem *currentItem() const { return static_cast<KeyListViewItem*>(K3ListView::currentItem()); }
    virtual KeyListViewItem *findItem (const QString &text, int column, ComparisonFlags compare = ExactMatch | Qt::CaseSensitive) const
		{ return static_cast<KeyListViewItem *>(K3ListView::findItem(text, column, compare)); }
    virtual QList<KeyListViewItem *> selectedItems(void);
    virtual KeyListViewItem *lastChild() const { return static_cast<KeyListViewItem*>(K3ListView::lastChild()); }
    virtual KeyListViewItem *itemAtIndex(int index) { return static_cast<KeyListViewItem*>(K3ListView::itemAtIndex(index)); }

private:
    QPixmap trustunknown;
    QPixmap trustrevoked;
    QPixmap trustgood;
    QPixmap trustultimate;
    QPixmap trustexpired;
    QPixmap trustmarginal;

    int groupNb;
    int m_previewsize;
    bool m_displayphoto;
};

#endif // KEYLISTVIEW_H
