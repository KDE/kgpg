/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "keylistview.h"

#include <QDragMoveEvent>
#include <QDropEvent>
#include <QPainter>
#include <QRect>

#include <Q3ListViewItem>
#include <Q3TextDrag>
#include <Q3Header>

#include <kabc/stdaddressbook.h>
#include <KStandardDirs>
#include <KApplication>
#include <KIconLoader>
#include <KMessageBox>
#include <KLocale>

#include "kgpgsettings.h"
#include "kgpgoptions.h"
#include "convert.h"
#include "images.h"

KeyListViewItem::KeyListViewItem(KeyListView *parent, const QString &name, const QString &email, const QString &trust, const QString &expiration, const QString &size, const QString &creation, const QString &id, const bool isdefault, bool isexpired, ItemType type)
               : K3ListViewItem(parent)
{
    m_def = isdefault;
    m_exp = isexpired;
    m_type = type;
    m_key = NULL;
    m_sig = NULL;
    setText(0, name);
    setText(1, email);
    setText(2, trust);
    setText(3, expiration);
    setText(4, size);
    setText(5, creation);
    setText(6, id);
}

KeyListViewItem::KeyListViewItem(KeyListViewItem *parent, const QString &name, const QString &email, const QString &trust, const QString &expiration, const QString &size, const QString &creation, const QString &id, const bool isdefault, const bool isexpired, ItemType type)
               : K3ListViewItem(parent)
{
    m_def = isdefault;
    m_exp = isexpired;
    m_type = type;
    m_key = NULL;
    m_sig = NULL;
    setText(0, name);
    setText(1, email);
    setText(2, trust);
    setText(3, expiration);
    setText(4, size);
    setText(5, creation);
    setText(6, id);
}

KeyListViewItem::KeyListViewItem(K3ListView *parent, const KgpgKey &key, const bool isbold)
		: K3ListViewItem(parent)
{
	m_def = isbold;
	m_exp = (key.trust() == TRUST_EXPIRED);
	m_type = Public;
	m_key = new KgpgKey(key);
	m_sig = NULL;
	if (key.secret())
		m_type |= Secret;
	setText(0, key.name());
	setText(1, key.email());
	setText(2, QString());
	setText(3, key.expiration());
	setText(4, key.size());
	setText(5, key.creation());
	setText(6, key.id());
}

KeyListViewItem::KeyListViewItem(K3ListViewItem *parent, const KgpgKeySign &sig)
		: K3ListViewItem(parent)
{
	m_def = false;
	m_exp = false;	// TODO: sign expiration
	m_type = Sign;
	m_key = NULL;
	m_sig = new KgpgKeySign(sig);

	QString tmpname = sig.name();
	if (!sig.comment().isEmpty())
		tmpname += " (" + sig.comment() + ')';

	setText(0, tmpname);

	QString tmpemail = sig.email();
	if (sig.local())
		tmpemail += i18n(" [local]");

	if (sig.revocation()) {
		tmpemail += i18n(" [Revocation signature]");
		setPixmap(0, Images::revoke());
	} else
		setPixmap(0, Images::signature());

	setText(1, tmpemail);
	setText(2, "-");
	setText(3, sig.expiration());
	setText(4, "-");
	setText(5, sig.creation());
	setText(6, sig.id());
}

KeyListViewItem::~KeyListViewItem()
{
	delete m_key;
	delete m_sig;
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

int KeyListViewItem::compare(Q3ListViewItem *itemx, int c, bool ascending) const
{
    KeyListViewItem *item = static_cast<KeyListViewItem *>(itemx);

	switch (c) {
	case 3:		// expiration date
	case 5: {	// creation date
		QDate d = KGlobal::locale()->readDate(text(c));
		QDate itemDate = KGlobal::locale()->readDate(item->text(c));

		bool thisDateValid = d.isValid();
		bool itemDateValid = itemDate.isValid();

		if (thisDateValid) {
			if (itemDateValid) {
				if (d < itemDate) return -1;
				if (d > itemDate) return  1;
			} else
				return -1;
		} else if (itemDateValid)
			return 1;

		return 0;
	}
	case 2: {	// pixmap
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
	case 0: {
		ItemType item1 = itemType();
		ItemType item2 = item->itemType();

		bool test1 = (item1 & KeyListViewItem::Public) && !(item1 & KeyListViewItem::Secret); // only a public key
		bool test2 = (item2 & KeyListViewItem::Public) && !(item2 & KeyListViewItem::Secret); // only a public key

		// key-pair goes before simple public key
		if (item1 == KeyListViewItem::Pair && test2) return -1;
		if (item2 == KeyListViewItem::Pair && test1) return 1;

		if (item1 < item2) return -1;
		if (item1 > item2) return 1;

		// fallthrough
		}
	default:
		return K3ListViewItem::compare(item, c, ascending);
	}
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

    trustultimate.load(KStandardDirs::locate("appdata", "pics/kgpg_fill.png"));
    trustultimate.fill(KGpgSettings::colorUltimate());
    QPainter(&trustultimate).drawPixmap(rect, blankFrame);

    trustmarginal.load(KStandardDirs::locate("appdata", "pics/kgpg_fill.png"));
    trustmarginal.fill(KGpgSettings::colorMarginal());
    QPainter(&trustmarginal).drawPixmap(rect, blankFrame);

    trustexpired.load(KStandardDirs::locate("appdata", "pics/kgpg_fill.png"));
    trustexpired.fill(KGpgSettings::colorExpired());
    QPainter(&trustexpired).drawPixmap(rect, blankFrame);

    connect(this, SIGNAL(expanded(Q3ListViewItem *)), this, SLOT(expandKey(Q3ListViewItem *)));

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
    KeyListViewItem *ki = currentItem();
    QString keyid = ki->text(6);

	if (!(ki->itemType() & KeyListViewItem::Public))
		return;

    KgpgInterface *interface = new KgpgInterface();
    QString keytxt = interface->getKeys(true, NULL, QStringList(keyid));
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

    ensureItemVisible(this->findItemByKeyId(keyids.last()));
    emit statusMessage(i18n("%1 Keys, %2 Groups", childCount() - groupNb, groupNb), 1);
    emit statusMessage(i18n("Ready"), 0);
}

void KeyListView::refreshAll()
{
    emit statusMessage(i18n("Loading Keys..."), 0, true);
    kapp->processEvents();

    // update display of keys in main management window
    kDebug(2100) << "Refreshing All" ;

    // get current position.
    KeyListViewItem *current = currentItem();
    if(current != 0)
    {
        while(current->depth() > 0)
            current = current->parent();
        takeItem(current);
    }

    // clear the list
    clear();

    if (refreshKeys())
    {
        kDebug(2100) << "No key found" ;
        emit statusMessage(i18n("Ready"), 0);
        return;
    }

    refreshGroups();

    KeyListViewItem *newPos = NULL;
    if(current != 0)
    {
        // select previous selected
        if (!current->text(6).isEmpty())
            newPos = findItemByKeyId(current->keyId());
        else
            newPos = findItem(current->text(0), 0);
        delete current;
    }

    if (newPos != NULL)
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
    kDebug(2100) << "Refresh Finished" ;
}

bool KeyListView::refreshKeys(const QStringList &ids)
{
    KgpgInterface *interface = new KgpgInterface();
    KgpgKeyList secretlist = interface->readSecretKeys(ids);

    QStringList issec;
    for (int i = 0; i < secretlist.size(); ++i)
        issec << secretlist.at(i).fullId();

    KgpgKeyList publiclist = interface->readPublicKeys(true, ids);
    delete interface;

    KeyListViewItem *item = 0;
    QString defaultkey = KGpgSettings::defaultKey();
    for (int i = 0; i < publiclist.size(); ++i)
    {
        KgpgKey key = publiclist.at(i);

        bool isbold;
        if (defaultkey.length() == 16)
          isbold = (key.fullId() == defaultkey);
        else
          isbold = (key.id() == defaultkey);
        int index = issec.indexOf(key.fullId());
        if (index != -1)
        {
            key.setSecret(true);
            issec.removeAt(index);
        }

        item = new KeyListViewItem(this, key, isbold);
        item->setPixmap(2, getTrustPix(key.trust(), key.valid()));
        item->setVisible(true);
        item->setExpandable(true);

        if (key.secret())
            item->setPixmap(0, Images::pair());
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

void KeyListView::refreshcurrentkey(KeyListViewItem *current)
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
        refreshcurrentkey(currentItem());
    else
        refreshcurrentkey(currentItem()->parent());
}

void KeyListView::slotReloadOrphaned()
{
    QStringList issec;

    KgpgInterface *interface = new KgpgInterface();
    KgpgKeyList listkeys;

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
        if (findItemByKeyId(*it) == NULL)
            list += *it;

    if (list.size() != 0)
        insertOrphans(list);

    setSelected(findItemByKeyId(*it), true);
    emit statusMessage(i18n("%1 Keys, %2 Groups", childCount() - groupNb, groupNb), 1);
    emit statusMessage(i18n("Ready"), 0);
}

void KeyListView::insertOrphans(const QStringList &ids)
{
    KgpgInterface *interface = new KgpgInterface();
    KgpgKeyList keys = interface->readSecretKeys(ids);
    delete interface;
    QStringList orphanList;

    KeyListViewItem *item = 0;
    for (int i = 0; i < keys.count(); ++i)
    {
        KgpgKey key = keys.at(i);

        orphanList << key.fullId();

        item = new KeyListViewItem(this, key, false);
	item->setItemType(KeyListViewItem::Secret);
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
    kDebug(2100) << "Refreshing groups..." ;
    KeyListViewItem *item = firstChild();
    while (item)
    {
        if (item->itemType() == KeyListViewItem::Group)
        {
            KeyListViewItem *item2 = item->nextSibling();
            delete item;
            item = item2;
        }
        else
            item = item->nextSibling();
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
        case kgpgOptions::UltimateColor:
            trustFinger = trustultimate.serialNumber();
            trustultimate = newtrust;
            break;

        case kgpgOptions::GoodColor:
            trustFinger = trustgood.serialNumber();
            trustgood = newtrust;
            break;

        case kgpgOptions::MarginalColor:
            trustFinger = trustmarginal.serialNumber();
            trustmarginal = newtrust;
            break;

        case kgpgOptions::ExpiredColor:
            trustFinger = trustexpired.serialNumber();
            trustexpired = newtrust;
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

    KeyListViewItem *item = firstChild();
    while (item)
    {
        if (item->pixmap(2))
            if (item->pixmap(2)->serialNumber() == trustFinger)
                item->setPixmap(2, newtrust);
        item = item->nextSibling();
    }
}

void KeyListView::expandKey(Q3ListViewItem *item2)
{
    KeyListViewItem *item = static_cast<KeyListViewItem *>(item2);

    if (item->childCount() != 0)
        return;   // key has already been expanded

    QString keyid = item->keyId();

    KgpgInterface *interface = new KgpgInterface();
    KgpgKeyList keys = interface->readPublicKeys(true, QStringList(keyid), true);
    KgpgKey key = keys.at(0);

    KeyListViewItem *tmpitem;


    /********* insertion of sub keys ********/
    for (int i = 0; i < key.subList()->size(); ++i)
    {
        KgpgKeySub sub = key.subList()->at(i);

        QString algo = i18n("%1 subkey", Convert::toString(sub.algorithm()));
        tmpitem = new KeyListViewItem(item, algo, QString(), QString(), sub.expiration(), QString::number(sub.size()), sub.creation(), sub.id(), false, false, KeyListViewItem::Sub);
        tmpitem->setPixmap(0, Images::single());
        tmpitem->setPixmap(2, getTrustPix(sub.trust(), sub.valid()));
        insertSigns(tmpitem, sub.signList());
    }
    /****************************************/


    /********* insertion of users id ********/
    for (int i = 0; i < key.uidList()->size(); ++i)
    {
        KgpgKeyUid uid = key.uidList()->at(i);
        QString index;

        index.setNum(uid.index());

        tmpitem = new KeyListViewItem(item, uid.name(), uid.email(), QString(), "-", "-", "-", index, false, false, KeyListViewItem::Uid);
        tmpitem->setPixmap(2, getTrustPix(key.trust(), key.valid()));
        tmpitem->setPixmap(0, Images::userId());
        insertSigns(tmpitem, uid.signList());
    }
    /****************************************/


    /******** insertion of photos id ********/
    QStringList photolist = key.photoList();
    for (int i = 0; i < photolist.size(); ++i)
    {
        tmpitem = new KeyListViewItem(item, i18n("Photo id"), QString(), QString(), "-", "-", "-", photolist.at(i), false, false, KeyListViewItem::Uat);
        tmpitem->setPixmap(2, getTrustPix(key.trust(), key.valid()));

        if (m_displayphoto)
        {
            QPixmap pixmap = interface->loadPhoto(keyid, photolist.at(i), true);
            tmpitem->setPixmap(0, pixmap.scaled(m_previewsize + 5, m_previewsize, Qt::KeepAspectRatio));
        }
        else
            tmpitem->setPixmap(0, Images::photo());

        KgpgKeyUat uat = key.uatList()->at(i);
        insertSigns(tmpitem, uat.signList());
    }
    /****************************************/

    delete interface;

    /******** insertion of signature ********/
    insertSigns(item, key.signList());
    /****************************************/
}

void KeyListView::insertSigns(KeyListViewItem *item, const KgpgKeySignList &list)
{
    for (int i = 0; i < list.size(); ++i)
    {
        (void) new KeyListViewItem(item, list.at(i));
    }
}

void KeyListView::expandGroup(KeyListViewItem *item)
{
    QStringList keysGroup = KgpgInterface::getGpgGroupSetting(item->text(0), KGpgSettings::gpgConfigPath());

    kDebug(2100) << keysGroup ;

    for (QStringList::Iterator it = keysGroup.begin(); it != keysGroup.end(); ++it)
    {
        KeyListViewItem *item2 = new KeyListViewItem(item, QString(*it));
        item2->setPixmap(0, Images::group());
        item2->setExpandable(false);
    }
}

QPixmap KeyListView::getTrustPix(const KgpgKeyTrust &trust, const bool &isvalid)
{
    if (!isvalid)
        return trustbad;
    switch (trust) {
	case TRUST_ULTIMATE:	return trustultimate;
	case TRUST_FULL:	return trustgood;
	case TRUST_REVOKED:	return trustrevoked;
	case TRUST_INVALID:
	case TRUST_DISABLED:	return trustbad;
	case TRUST_EXPIRED:	return trustexpired;
	case TRUST_MARGINAL:	return trustmarginal;
	case TRUST_UNKNOWN:
	case TRUST_UNDEFINED:
	case TRUST_NONE:	return trustunknown;
	default:
kDebug(3125) << "Oops, unmatched trust value " << trust ;
				return trustunknown;
    }
}

QList<KeyListViewItem *> KeyListView::selectedItems(void)
{
	QList<KeyListViewItem *> list;

	Q3ListViewItemIterator it(this, Q3ListViewItemIterator::Selected);

	for(; it.current(); ++it)
		list.append(static_cast<KeyListViewItem*>(it.current()));

	return list;
}

/**
 * Find the item that is a primary key with the given id. Match will be
 * by full id if possible, else by short id. Passing a fingerprint is
 * explicitly allowed (forward compatibility) but currently matching
 * is only done by full id.
 */
KeyListViewItem *KeyListView::findItemByKeyId(const QString &id)
{
	QString fullid = id.right(16);
	KeyListViewItem *cur = findItem(fullid.right(8), 6);

	if ((cur == NULL) || ((fullid.length() < 16) && (cur->getKey() != NULL)))
		return cur;

	KgpgKey *key = cur->getKey();
	if ((key != NULL) && (key->fullId() == id))
		return cur;

	// The first hit doesn't match the full id. Do deep scanning.
	Q3ListViewItemIterator it(this);

	for(; it.current(); ++it) {
		cur = static_cast<KeyListViewItem*>(it.current());
		key = cur->getKey();
		if (key == NULL)
			continue;
		if (key->fullId() == fullid)
			return cur;
	}
	return NULL;
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
        KeyListViewItem *item = m_searchlistview->firstChild();
        while (item)
        {
            if (item->isVisible())
            {
                if (m_hidepublic)
                    if ((item->itemType() == KeyListViewItem::Public) && (item->itemType() != KeyListViewItem::Secret)) {
                        item->setVisible(false);
                        continue;
                    }

                if (m_hidedisabled)
                    if (item->isExpired() || (item->getKey()->trust() == TRUST_REVOKED))
                        item->setVisible(false);

            }
            item = item->nextSibling();
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
