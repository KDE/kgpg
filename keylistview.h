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

    explicit KeyListViewItem(KeyListView *parent = 0, const QString &name = QString(), const QString &email = QString(), const QString &trust = QString(), const QString &expiration = QString(), const QString &size = QString(), const QString &creation = QString(), const QString &id = QString() , const bool isdefault = false, const bool isexpired = false, ItemType type = Public);
    explicit KeyListViewItem(KeyListViewItem *parent = 0, const QString &name = QString(), const QString &email = QString(), const QString &trust = QString(), const QString &expiration = QString(), const QString &size = QString(), const QString &creation = QString(), const QString &id = QString(), const bool isdefault = false, const bool isexpired = false, ItemType type = Public);
    KeyListViewItem(K3ListView *parent, const KgpgKey &key, const bool isbold);
    KeyListViewItem(K3ListViewItem *parent, const KgpgKeySign &sig);
    ~KeyListViewItem();

    void setItemType(const ItemType &type);
    ItemType itemType() const;

    void setDefault(const bool &def);
    bool isDefault() const;

    void setExpired(const bool &exp);
    bool isExpired() const;

    void setGroupId(const QString &nid) { delete groupId; groupId = new QString(nid); }

    virtual void paintCell(QPainter *p, const QColorGroup &cg, int col, int width, int align);
    virtual int compare(Q3ListViewItem *item, int c, bool ascending) const;
    virtual QString key(int column, bool) const;
    virtual KeyListViewItem *parent() const { return static_cast<KeyListViewItem*>(K3ListViewItem::parent()); }
    virtual KeyListViewItem *nextSibling() const { return static_cast<KeyListViewItem*>(K3ListViewItem::nextSibling()); }
    virtual KeyListViewItem *firstChild() const { return static_cast<KeyListViewItem*>(K3ListViewItem::firstChild()); }
    virtual KgpgKey* getKey() { return m_key; }
    virtual QString keyId(void) const { return m_key ? m_key->fullId() : m_sig ? m_sig->fullId() : groupId ? *groupId : text(6); }

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

    void setPreviewSize(const int &size);
    int previewSize() const;

    void setDisplayPhoto(const bool &display);
    bool displayPhoto() const;

    QPixmap trustbad;

signals:
    void statusMessage(QString, int, bool keep = false);
/*    void expanded(KeyListViewItem *);
    void returnPressed(KeyListViewItem *);
    void doubleClicked(KeyListViewItem *, QPoint, int);
    void contextMenuRequested(KeyListViewItem*, QPoint, int);*/

public slots:
    void slotAddColumn(const int &c);
    void slotRemoveColumn(const int &c);
    virtual KeyListViewItem *firstChild() { return static_cast<KeyListViewItem*>(K3ListView::firstChild()); }

protected:
    virtual void contentsDragMoveEvent(QDragMoveEvent *e);
    virtual void contentsDropEvent(QDropEvent *e);
    virtual void startDrag();
    virtual KeyListViewItem *currentItem() const { return static_cast<KeyListViewItem*>(K3ListView::currentItem()); }
    virtual KeyListViewItem *findItem (const QString &text, int column, ComparisonFlags compare = ExactMatch | Qt::CaseSensitive) const
		{ return static_cast<KeyListViewItem *>(K3ListView::findItem(text, column, compare)); }
    virtual QList<KeyListViewItem *> selectedItems(void);
    virtual KeyListViewItem *lastChild() const { return static_cast<KeyListViewItem*>(K3ListView::lastChild()); }
    virtual KeyListViewItem *itemAtIndex(int index) { return static_cast<KeyListViewItem*>(K3ListView::itemAtIndex(index)); }
    virtual KeyListViewItem *findItemByKeyId(const QString &id);

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
    void refreshTrust(int color, const QColor &newColor);

    void expandKey(Q3ListViewItem *item);
    void expandGroup(KeyListViewItem *item);
    void insertSigns(KeyListViewItem *item, const KgpgCore::KgpgKeySignList &list);

private:
    QPixmap getTrustPix(const KgpgCore::KgpgKeyTrust &trust, const bool &isvalid);

private:
    QString photoKeysList;

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


class KeyListViewSearchLine : public K3ListViewSearchLine
{
    Q_OBJECT

public:
    explicit KeyListViewSearchLine(QWidget *parent = 0, KeyListView *listView = 0);
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
