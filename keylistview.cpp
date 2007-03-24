/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QDragMoveEvent>
#include <QDropEvent>
#include <QPainter>
#include <QRect>

#include <Q3ListViewItem>
#include <Q3TextDrag>
#include <Q3Header>

#include <kabc/stdaddressbook.h>
#include <kstandarddirs.h>
#include <kapplication.h>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <klocale.h>

#include "kgpgsettings.h"
#include "keylistview.h"
#include "kgpgoptions.h"
#include "convert.h"
#include "images.h"

KeyListViewItem::KeyListViewItem(K3ListView *parent, const QString &name, const QString &email, const QString &trust, const QString &expiration, const QString &size, const QString &creation, const QString &id, const bool isdefault, bool isexpired, ItemType type)
               : K3ListViewItem(parent)
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

KeyListViewItem::KeyListViewItem(K3ListViewItem *parent, const QString &name, const QString &email, const QString &trust, const QString &expiration, const QString &size, const QString &creation, const QString &id, const bool isdefault, const bool isexpired, ItemType type)
               : K3ListViewItem(parent)
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
            _cg.setColor(QPalette::Text, Qt::red);
    }
    else
    if (column < 2)
    {
        QFont font(p->font());
        font.setItalic(true);
        p->setFont(font);
    }

    K3ListViewItem::paintCell(p, _cg, column, width, alignment);
}

int KeyListViewItem::compare(Q3ListViewItem *item2, int c, bool ascending) const
{
    KeyListViewItem *item = static_cast<KeyListViewItem*>(item2);

    if (c == 0)
    {
        ItemType item1 = itemType();
        ItemType item2 = item->itemType();

        bool test1 = (item1 & KeyListViewItem::Public) && !(item1 & KeyListViewItem::Secret); // only a public key
        bool test2 = (item2 & KeyListViewItem::Public) && !(item2 & KeyListViewItem::Secret); // only a public key

        // key-pair goes before simple public key
        if (item1 == KeyListViewItem::Pair && test2) return -1;
        if (item2 == KeyListViewItem::Pair && test1) return 1;

        if (item1 < item2) return -1;
        if (item1 > item2) return 1;

        return K3ListViewItem::compare(item, c, ascending);
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

    return K3ListViewItem::compare(item, c, ascending);
}

QString KeyListViewItem::key(int column, bool) const
{
    return text(column).toLower();
}

KeyListView::KeyListView(QWidget *parent)
           : K3ListView(parent)
{
    setRootIsDecorated(true);
    addColumn(i18n("Name"), 200);
    addColumn(i18n("Email"), 200);
    addColumn(i18n("Trust"), 60);
    addColumn(i18n("Expiration"), 100);
    addColumn(i18n("Size"), 100);
    addColumn(i18n("Creation"), 100);
    addColumn(i18n("Id"), 100);
    setShowSortIndicator(true);
    setAllColumnsShowFocus(true);
    setFullWidth(true);
    setAcceptDrops(true) ;
    setSelectionModeExt(Extended);

    QPixmap blankFrame(KStandardDirs::locate("appdata", "pics/kgpg_blank.png"));
    QRect rect(0, 0, 50, 15);

    trustunknown.load(KStandardDirs::locate("appdata", "pics/kgpg_fill.png"));
    trustunknown.fill(KGpgSettings::colorUnknown());
    QPainter(&trustunknown).drawPixmap(rect, blankFrame);

    trustbad.load(KStandardDirs::locate("appdata", "pics/kgpg_fill.png"));
    trustbad.fill(KGpgSettings::colorBad());
    QPainter(&trustbad).drawPixmap(rect, blankFrame);

    trustrevoked.load(KStandardDirs::locate("appdata", "pics/kgpg_fill.png"));
    trustrevoked.fill(KGpgSettings::colorRev());
    QPainter(&trustrevoked).drawPixmap(rect, blankFrame);

    trustgood.load(KStandardDirs::locate("appdata", "pics/kgpg_fill.png"));
    trustgood.fill(KGpgSettings::colorGood());
    QPainter(&trustgood).drawPixmap(rect, blankFrame);

    connect(this, SIGNAL(expanded(Q3ListViewItem*)), this, SLOT(expandKey(Q3ListViewItem *)));

    header()->setMovingEnabled(false);
    setAcceptDrops(true);
    setDragEnabled(true);
}

void KeyListView::setPreviewSize(const int &size)
{
    m_previewsize = size;
}

int KeyListView::previewSize() const
{
    return m_previewsize;
}

void KeyListView::setDisplayPhoto(const bool &display)
{
    m_displayphoto = display;
}

bool KeyListView::displayPhoto() const
{
    return m_displayphoto;
}

void KeyListView::slotAddColumn(const int &c)
{
    header()->setResizeEnabled(true, c);
    adjustColumn(c);
}

void KeyListView::slotRemoveColumn(const int &c)
{
    hideColumn(c);
    header()->setResizeEnabled(false, c);
    header()->setStretchEnabled(true, 6);
}

void KeyListView::contentsDragMoveEvent(QDragMoveEvent *e)
{
    e->setAccepted(KUrl::List::canDecode(e->mimeData()));
}

void  KeyListView::contentsDropEvent(QDropEvent *o)
{
    KUrl::List uriList = KUrl::List::fromMimeData(o->mimeData());
    if (!uriList.isEmpty())
        droppedFile(uriList.first());
}

void KeyListView::startDrag()
{
    KeyListViewItem *ki = static_cast<KeyListViewItem *>(currentItem());
    QString keyid = ki->text(6);

	if (!(ki->itemType() & KeyListViewItem::Public))
		return;

    KgpgInterface *interface = new KgpgInterface();
    QString keytxt = interface->getKeys(true, true, QStringList(keyid));
    delete interface;

    Q3DragObject *d = new Q3TextDrag(keytxt, this);
    d->dragCopy();
    // do NOT delete d.
}

void KeyListView::droppedFile(const KUrl &url)
{
    if (KMessageBox::questionYesNo(this, i18n("<p>Do you want to import file <b>%1</b> into your key ring?</p>", url.path()), QString(), KGuiItem(i18n("Import")), KGuiItem(i18n("Do Not Import"))) != KMessageBox::Yes)
        return;

    KgpgInterface *interface = new KgpgInterface();
    connect(interface, SIGNAL(importKeyFinished(QStringList)), this, SLOT(slotReloadKeys(QStringList)));
    interface->importKey(url);
}

void KeyListView::slotReloadKeys(const QStringList &keyids)
{
    if (keyids.isEmpty())
        return;

    if (keyids.first() == "ALL")
    {
        refreshAll();
        return;
    }

    refreshKeys(keyids);

    ensureItemVisible(this->findItem((keyids.last()).right(8), Q3ListView::BeginsWith | Q3ListView::EndsWith));
    emit statusMessage(i18n("%1 Keys, %2 Groups", childCount() - groupNb, groupNb), 1);
    emit statusMessage(i18n("Ready"), 0);
}

void KeyListView::refreshAll()
{
    emit statusMessage(i18n("Loading Keys..."), 0, true);
    kapp->processEvents();

    // update display of keys in main management window
    kDebug(2100) << "Refreshing All" << endl;

    // get current position.
    K3ListViewItem *current = static_cast<K3ListViewItem*>(currentItem());
    if(current != 0)
    {
        while(current->depth() > 0)
            current = static_cast<K3ListViewItem*>(current->parent());
        takeItem(current);
    }

    // clear the list
    clear();

    orphanList = QStringList();
    if (refreshKeys())
    {
        kDebug(2100) << "No key found" << endl;
        emit statusMessage(i18n("Ready"), 0);
        return;
    }

    refreshGroups();

    K3ListViewItem *newPos = 0L;
    if(current != 0)
    {
        // select previous selected
        if (!current->text(6).isEmpty())
            newPos = static_cast<K3ListViewItem*>(findItem(current->text(6), 6));
        else
            newPos = static_cast<K3ListViewItem*>(findItem(current->text(0), 0));
        delete current;
    }

    if (newPos != 0L)
    {
        setCurrentItem(newPos);
        setSelected(newPos, true);
        ensureItemVisible(newPos);
    }
    else
    {
        setCurrentItem(firstChild());
        setSelected(firstChild(), true);
    }

    emit statusMessage(i18n("%1 Keys, %2 Groups", childCount() - groupNb, groupNb), 1);
    emit statusMessage(i18n("Ready"),0);
    kDebug(2100) << "Refresh Finished" << endl;
}

bool KeyListView::refreshKeys(const QStringList &ids)
{
    KgpgInterface *interface = new KgpgInterface();
    KeyList secretlist = interface->readSecretKeys();

    QStringList issec;
    for (int i = 0; i < secretlist.size(); ++i)
        issec << secretlist.at(i).id();

    KeyList publiclist = interface->readPublicKeys(true, ids);
    delete interface;

    KeyListViewItem *item = 0;
    QString defaultkey = KGpgSettings::defaultKey();
    for (int i = 0; i < publiclist.size(); ++i)
    {
        Key key = publiclist.at(i);

        bool isbold = (key.id() == defaultkey);
        bool isexpired = (key.trust() == 'e');

        item = new KeyListViewItem(this, key.name(), key.email(), QString(), key.expiration(), key.size(), key.creation(), key.id(), isbold, isexpired, KeyListViewItem::Public);
        item->setPixmap(2, getTrustPix(key.trust(), key.valide()));
        item->setVisible(true);
        item->setExpandable(true);

        int index = issec.indexOf(key.id());
        if (index != -1)
        {
            item->setPixmap(0, Images::pair());
            item->setItemType(item->itemType() | KeyListViewItem::Secret);
            issec.removeAt(index);
        }
        else
            item->setPixmap(0, Images::single());
    }

    if (!issec.isEmpty())
        insertOrphans(issec);

    if (publiclist.size() == 0)
        return 1;
    else
    {
        if (publiclist.size() == 1)
        {
            clearSelection();
            setCurrentItem(item);
        }
        return 0;
    }
}

void KeyListView::refreshcurrentkey(K3ListViewItem *current)
{
    if (!current)
        return;

    QString keyUpdate = current->text(6);
    if (keyUpdate.isEmpty())
        return;
    bool keyIsOpen = current->isOpen();

    delete current;

    refreshKeys(QStringList(keyUpdate));

    if (currentItem())
        if (currentItem()->text(6) == keyUpdate)
            currentItem()->setOpen(keyIsOpen);
}

void KeyListView::refreshselfkey()
{
    if (currentItem()->depth() == 0)
        refreshcurrentkey(static_cast<K3ListViewItem*>(currentItem()));
    else
        refreshcurrentkey(static_cast<K3ListViewItem*>(currentItem()->parent()));
}

void KeyListView::slotReloadOrphaned()
{
    QStringList issec;

    KgpgInterface *interface = new KgpgInterface();
    KeyList listkeys;

    listkeys = interface->readSecretKeys();
    for (int i = 0; i < listkeys.size(); ++i)
        issec << listkeys.at(i).id();
    listkeys = interface->readPublicKeys(true);
    for (int i = 0; i < listkeys.size(); ++i)
        issec.removeAll(listkeys.at(i).id());

    delete interface;

    QStringList::Iterator it;
    QStringList list;
    for (it = issec.begin(); it != issec.end(); ++it)
        if (findItem(*it, 6) == 0)
            list += *it;

    if (list.size() != 0)
        insertOrphans(list);

    setSelected(findItem(*it, 6), true);
    emit statusMessage(i18n("%1 Keys, %2 Groups", childCount() - groupNb, groupNb), 1);
    emit statusMessage(i18n("Ready"), 0);
}

void KeyListView::insertOrphans(const QStringList &ids)
{
    KgpgInterface *interface = new KgpgInterface();
    KeyList keys = interface->readSecretKeys(ids);
    delete interface;

    KeyListViewItem *item = 0;
    for (int i = 0; i < keys.count(); ++i)
    {
        Key key = keys.at(i);

        bool isexpired = (key.trust() == 'e');

        orphanList << key.id();

        item = new KeyListViewItem(this, key.name(), key.email(), QString(), key.expiration(), key.size(), key.creation(), key.id(), false, isexpired, KeyListViewItem::Secret);
        item->setPixmap(0, Images::orphan());
    }

    if (ids.size() == 1)
    {
        if (keys.isEmpty())
        {
            orphanList.removeAll(ids.at(0));
            setSelected(currentItem(), true);
        }
        else
        {
            clearSelection();
            setCurrentItem(item);
            setSelected(item, true);
        }
    }
}

void KeyListView::refreshGroups()
{
    kDebug(2100) << "Refreshing groups..." << endl;
    KeyListViewItem *item = static_cast<KeyListViewItem*>(firstChild());
    while (item)
    {
        if (item->itemType() == KeyListViewItem::Group)
        {
            KeyListViewItem *item2 = static_cast<KeyListViewItem*>(item->nextSibling());
            delete item;
            item = item2;
        }
        else
            item = static_cast<KeyListViewItem*>(item->nextSibling());
    }

    QStringList groups = KgpgInterface::getGpgGroupNames(KGpgSettings::gpgConfigPath());
    groupNb = groups.count();

    for (QStringList::Iterator it = groups.begin(); it != groups.end(); ++it)
        if (!QString(*it).isEmpty())
        {
            item = new KeyListViewItem(this, QString(*it), "-", "-", "-", "-", "-", "-", false, false, KeyListViewItem::Group);
            item->setPixmap(0, Images::group());
            item->setExpandable(false);
        }

    emit statusMessage(i18n("%1 Keys, %2 Groups", childCount() - groupNb, groupNb), 1);
    emit statusMessage(i18n("Ready"), 0);
}

void KeyListView::refreshTrust(int color, QColor newColor)
{
    if (!newColor.isValid())
        return;

    QPixmap blankFrame;
    QPixmap newtrust;
    int trustFinger = 0;

    blankFrame.load(KStandardDirs::locate("appdata", "pics/kgpg_blank.png"));
    newtrust.load(KStandardDirs::locate("appdata", "pics/kgpg_fill.png"));
    newtrust.fill(newColor);

    bitBlt(&newtrust, 0, 0, &blankFrame, 0, 0, 50, 15);

    switch (color)
    {
        case kgpgOptions::GoodColor:
            trustFinger = trustgood.serialNumber();
            trustgood = newtrust;
            break;

        case kgpgOptions::BadColor:
            trustFinger = trustbad.serialNumber();
            trustbad = newtrust;
            break;

        case kgpgOptions::UnknownColor:
            trustFinger = trustunknown.serialNumber();
            trustunknown = newtrust;
            break;

        case kgpgOptions::RevColor:
            trustFinger = trustrevoked.serialNumber();
            trustrevoked = newtrust;
            break;
    }

    K3ListViewItem *item = static_cast<K3ListViewItem*>(firstChild());
    while (item)
    {
        if (item->pixmap(2))
            if (item->pixmap(2)->serialNumber() == trustFinger)
                item->setPixmap(2, newtrust);
        item = static_cast<K3ListViewItem*>(item->nextSibling());
    }
}

void KeyListView::expandKey(Q3ListViewItem *item2)
{
    K3ListViewItem *item = static_cast<K3ListViewItem*>(item2);
    if (item->childCount() != 0)
        return;   // key has already been expanded

    QString keyid = item->text(6);

    KgpgInterface *interface = new KgpgInterface();
    KeyList keys = interface->readPublicKeys(true, QStringList(keyid), true);
    Key key = keys.at(0);

    KeyListViewItem *tmpitem;


    /********* insertion of sub keys ********/
    for (int i = 0; i < key.subList()->size(); ++i)
    {
        KeySub sub = key.subList()->at(i);

        QString algo = i18n("%1 subkey", Convert::toString(sub.algorithme()));
        tmpitem = new KeyListViewItem(item, algo, QString(), QString(), sub.expiration(), QString::number(sub.size()), sub.creation(), sub.id(), false, false, KeyListViewItem::Sub);
        tmpitem->setPixmap(0, Images::single());
        tmpitem->setPixmap(2, getTrustPix(sub.trust(), sub.valide()));
        insertSigns(tmpitem, sub.signList());
    }
    /****************************************/


    /********* insertion of users id ********/
    for (int i = 0; i < key.uidList()->size(); ++i)
    {
        KeyUid uid = key.uidList()->at(i);

        tmpitem = new KeyListViewItem(item, uid.name(), uid.email(), QString(), "-", "-", "-", "-", false, false, KeyListViewItem::Uid);
        tmpitem->setPixmap(2, getTrustPix(key.trust(), key.valide()));
        tmpitem->setPixmap(0, Images::userId());
        insertSigns(tmpitem, uid.signList());
    }
    /****************************************/


    /******** insertion of photos id ********/
    QStringList photolist = key.photoList();
    for (int i = 0; i < photolist.size(); ++i)
    {
        tmpitem = new KeyListViewItem(item, i18n("Photo id"), QString(), QString(), "-", "-", "-", photolist.at(i), false, false, KeyListViewItem::Uat);
        tmpitem->setPixmap(2, getTrustPix(key.trust(), key.valide()));

        if (m_displayphoto)
        {
            QPixmap pixmap = interface->loadPhoto(keyid, photolist.at(i), true);
            tmpitem->setPixmap(0, pixmap.scaled(m_previewsize + 5, m_previewsize, Qt::KeepAspectRatio));
        }
        else
            tmpitem->setPixmap(0, Images::photo());

        KeyUat uat = key.uatList()->at(i);
        insertSigns(tmpitem, uat.signList());
    }
    /****************************************/


    /******** insertion of signature ********/
    insertSigns(item, key.signList());
    /****************************************/

    delete interface;
}

void KeyListView::insertSigns(K3ListViewItem *item, const KeySignList &list)
{
    KeyListViewItem *newitem;
    for (int i = 0; i < list.size(); ++i)
    {
        const KeySign sign = list.at(i);

        QString tmpname = sign.name();
        if (!sign.comment().isEmpty())
            tmpname += " (" + sign.comment() + ')';

        QString tmpemail = sign.email();
        if (sign.local())
            tmpemail += i18n(" [local]");

        if (sign.revocation())
        {
            tmpemail += i18n(" [Revocation signature]");
            newitem = new KeyListViewItem(item, tmpname, tmpemail, "-", "-", "-", sign.creation(), sign.id(), false, false, KeyListViewItem::Rev);
            newitem->setPixmap(0, Images::revoke());
        }
        else
        {
            newitem = new KeyListViewItem(item, tmpname, tmpemail, "-", sign.expiration(), "-", sign.creation(), sign.id(), false, false, KeyListViewItem::Sign);
            newitem->setPixmap(0, Images::signature());
        }
    }
}

void KeyListView::expandGroup(K3ListViewItem *item)
{
    QStringList keysGroup = KgpgInterface::getGpgGroupSetting(item->text(0), KGpgSettings::gpgConfigPath());

    kDebug(2100) << keysGroup << endl;

    for (QStringList::Iterator it = keysGroup.begin(); it != keysGroup.end(); ++it)
    {
        KeyListViewItem *item2 = new KeyListViewItem(item, QString(*it));
        item2->setPixmap(0, Images::group());
        item2->setExpandable(false);
    }
}

QPixmap KeyListView::getTrustPix(const KeyTrust &trust, const bool &isvalid)
{
    if (!isvalid)
        return trustbad;
    if (trust == 'o')
        return trustunknown;
    if (trust == 'i')
        return trustbad;
    if (trust == 'd')
        return trustbad;
    if (trust == 'r')
        return trustrevoked;
    if (trust == 'e')
        return trustbad;
    if (trust == 'q')
        return trustunknown;
    if (trust == 'n')
        return trustunknown;
    if (trust == 'm')
        return trustbad;
    if (trust == 'f')
        return trustgood;
    if (trust == 'u')
        return trustgood;
    return trustunknown;
}

KeyListViewSearchLine::KeyListViewSearchLine(QWidget *parent, KeyListView *listView)
                     : K3ListViewSearchLine(parent, listView)
{
    m_searchlistview = listView;
    setKeepParentsVisible(false);
    m_hidepublic = false;
    m_hidedisabled = false;
}

void KeyListViewSearchLine::setHidePublic(const bool &hidepublic)
{
    m_hidepublic = hidepublic;
}

bool KeyListViewSearchLine::hidePublic() const
{
    return m_hidepublic;
}

void KeyListViewSearchLine::setHideDisabled(const bool &hidedisabled)
{
    m_hidedisabled = hidedisabled;
}

bool KeyListViewSearchLine::hideDisabled() const
{
    return m_hidedisabled;
}

void KeyListViewSearchLine::updateSearch(const QString& s)
{
    K3ListViewSearchLine::updateSearch(s);

    if (m_hidepublic || m_hidedisabled)
    {
        KeyListViewItem *item = static_cast<KeyListViewItem*>(m_searchlistview->firstChild());
        while (item)
        {
            if (item->isVisible())
            {
                if (m_hidepublic)
                    if ((item->itemType() == KeyListViewItem::Public) && (item->itemType() != KeyListViewItem::Secret))
                        item->setVisible(false);

                if (m_hidedisabled)
                    if (item->isExpired())
                        item->setVisible(false);
            }
            item = static_cast<KeyListViewItem*>(item->nextSibling());
        }
    }
}

bool KeyListViewSearchLine::itemMatches(const Q3ListViewItem *item, const QString &s) const
{
    if (item->depth() != 0)
        return true;
    else
        return K3ListViewSearchLine::itemMatches(item, s);
}

#include "keylistview.moc"
