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

#include <QStringList>
#include <QColorGroup>
#include <QPixmap>


#include <k3listviewsearchline.h>
#include <k3listview.h>
#include <kurl.h>

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
        Sub = 8,
        Uid = 16,
        Uat = 32,
        Rev = 64,
        Sign = 128
    };
    Q_DECLARE_FLAGS(ItemType, ItemTypeFlag)

    KeyListViewItem(KeyListView *parent = 0, const QString &name = QString(), const QString &email = QString(), const QString &trust = QString(), const QString &expiration = QString(), const QString &size = QString(), const QString &creation = QString(), const QString &id = QString() , const bool isdefault = false, const bool isexpired = false, ItemType type = Public);
    KeyListViewItem(KeyListViewItem *parent = 0, const QString &name = QString(), const QString &email = QString(), const QString &trust = QString(), const QString &expiration = QString(), const QString &size = QString(), const QString &creation = QString(), const QString &id = QString(), const bool isdefault = false, const bool isexpired = false, ItemType type = Public);

    void setItemType(const ItemType &type);
    ItemType itemType() const;

    void setDefault(const bool &def);
    bool isDefault() const;

    void setExpired(const bool &exp);
    bool isExpired() const;

    virtual void paintCell(QPainter *p, const QColorGroup &cg, int col, int width, int align);
    virtual int compare(KeyListViewItem *item, int c, bool ascending) const;
    virtual QString key(int column, bool) const;
    virtual KeyListViewItem *parent() const { return static_cast<KeyListViewItem*>(parent()); }
    virtual KeyListViewItem *nextSibling() const { return static_cast<KeyListViewItem*>(nextSibling()); }
    virtual KeyListViewItem *firstChild() const { return static_cast<KeyListViewItem*>(firstChild()); }

private:
    bool m_def; /// Is set to \em true if it is the default key, \em false otherwise.
    bool m_exp; /// Is set to \em true if the key is expired, \em false otherwise.
    ItemType m_type;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(KeyListViewItem::ItemType)


class KeyListView : public K3ListView
{
    Q_OBJECT
    friend class KeysManager;

public:
    KeyListView(QWidget *parent = 0);

    void setPreviewSize(const int &size);
    int previewSize() const;

    void setDisplayPhoto(const bool &display);
    bool displayPhoto() const;

    QPixmap trustbad;

signals:
    void statusMessage(QString, int, bool keep = false);

public slots:
    void slotAddColumn(const int &c);
    void slotRemoveColumn(const int &c);
    virtual KeyListViewItem *firstChild() { return static_cast<KeyListViewItem*>(firstChild()); }

protected:
    virtual void contentsDragMoveEvent(QDragMoveEvent *e);
    virtual void contentsDropEvent(QDropEvent *e);
    virtual void startDrag();
    virtual KeyListViewItem *currentItem() const { return static_cast<KeyListViewItem*>(currentItem()); }
    virtual KeyListViewItem *findItem (const QString &text, int column, ComparisonFlags compare = ExactMatch | Qt::CaseSensitive) const
		{ return static_cast<KeyListViewItem *>(findItem(text, column, compare)); }
    virtual QList<KeyListViewItem *> selectedItems(void);
    virtual KeyListViewItem *lastChild() const { return static_cast<KeyListViewItem*>(lastChild()); }

private slots:
    void droppedFile(const KUrl &url);

    void slotReloadKeys(const QStringList &keyids);
    void refreshAll();

    bool refreshKeys(const QStringList &ids = QStringList());
    void refreshcurrentkey(KeyListViewItem *current);
    void refreshselfkey();

    void slotReloadOrphaned();
    void insertOrphans(const QStringList &ids);

    void refreshGroups();
    void refreshTrust(int color, QColor newColor);

    void expandKey(KeyListViewItem *item);
    void expandGroup(KeyListViewItem *item);
    void insertSigns(KeyListViewItem *item, const KgpgCore::KgpgKeySignList &list);

private:
    QPixmap getTrustPix(const KgpgCore::KgpgKeyTrust &trust, const bool &isvalid);

private:
    QStringList orphanList;
    QString photoKeysList;

    QPixmap trustunknown;
    QPixmap trustrevoked;
    QPixmap trustgood;

    int groupNb;
    int m_previewsize;
    bool m_displayphoto;
};


class KeyListViewSearchLine : public K3ListViewSearchLine
{
    Q_OBJECT

public:
    KeyListViewSearchLine(QWidget *parent = 0, KeyListView *listView = 0);
    virtual ~KeyListViewSearchLine() { }

    void setHidePublic(const bool &hidepublic = true);
    bool hidePublic() const;

    void setHideDisabled(const bool &hidedisabled = true);
    bool hideDisabled() const;

public slots:
    virtual void updateSearch(const QString &s = QString());

protected:
    virtual bool itemMatches(const Q3ListViewItem *item, const QString &s)  const;

private:
    KeyListView *m_searchlistview;
    bool m_hidepublic;
    bool m_hidedisabled;
};

#endif // KEYLISTVIEW_H
