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
#include <QString>

#include <k3listviewsearchline.h>
#include <k3listview.h>
#include <kurl.h>

#include "kgpgkey.h"

using namespace KgpgCore;

class Q3ListViewItem;
class QDragMoveEvent;
class QDropEvent;
class QPainter;

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

    KeyListViewItem(K3ListView *parent = 0, QString name = QString::null, QString email = QString::null, QString trust = QString::null, QString expiration = QString::null, QString size = QString::null, QString creation = QString::null, QString id = QString::null, bool isdefault = false, bool isexpired = false, ItemType type = Public);
    KeyListViewItem(K3ListViewItem *parent = 0, QString name = QString::null, QString email = QString::null, QString trust = QString::null, QString expiration = QString::null, QString size = QString::null, QString creation = QString::null, QString id = QString::null, bool isdefault = false, bool isexpired = false, ItemType type = Public);

    void setItemType(const ItemType &type);
    ItemType itemType() const;

    void setDefault(const bool &def);
    bool isDefault() const;

    void setExpired(const bool &exp);
    bool isExpired() const;

    virtual void paintCell(QPainter *p, const QColorGroup &cg, int col, int width, int align);
    virtual int compare(Q3ListViewItem *item, int c, bool ascending) const;
    virtual QString key(int column, bool) const;

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
    QString secretList;

signals:
    void statusMessage(QString, int, bool keep = false);

public slots:
    void slotAddColumn(const int &c);
    void slotRemoveColumn(const int &c);

protected:
    virtual void contentsDragMoveEvent(QDragMoveEvent *e);
    virtual void contentsDropEvent(QDropEvent *e);
    virtual void startDrag();

private slots:
    void droppedFile(const KUrl &url);

    void slotReloadKeys(const QStringList &keyids);
    void refreshAll();

    bool refreshKeys(QStringList ids = QStringList());
    void refreshcurrentkey(K3ListViewItem *current);
    void refreshselfkey();

    void slotReloadOrphaned();
    void insertOrphans(QStringList ids);

    void refreshGroups();
    void refreshTrust(int color, QColor newColor);

    void expandKey(Q3ListViewItem *item);
    void expandGroup(K3ListViewItem *item);
    void insertSigns(K3ListViewItem *item, const KgpgCore::KeySignList &list);

private:
    QPixmap getTrustPix(const KgpgCore::KeyTrust &trust, const bool &isvalid);

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
    virtual void updateSearch(const QString &s = QString::null);

protected:
    virtual bool itemMatches(const Q3ListViewItem *item, const QString &s)  const;

private:
    KeyListView *m_searchlistview;
    bool m_hidepublic;
    bool m_hidedisabled;
};

#endif // KEYLISTVIEW_H
