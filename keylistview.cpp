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
#include <KMessageBox>
#include <KLocale>

#include "kgpgsettings.h"
#include "kgpgoptions.h"
#include "convert.h"
#include "images.h"

KeyListViewItem::KeyListViewItem(K3ListView *parent, const KgpgKey &key, const bool isbold)
		: K3ListViewItem(parent)
{
	m_def = isbold;
	m_exp = (key.trust() == TRUST_EXPIRED);
	m_type = Public;
	m_key = new KgpgKey(key);
	m_sig = NULL;
	groupId = NULL;
	if (key.secret())
		m_type |= Secret;
	if (key.comment().isEmpty())
		setText(0, key.name());
	else
		setText(0, i18nc("Name (Comment)", "%1 (%2)", key.name(), key.comment()));
	setText(1, key.email());
	setText(2, QString());
	setText(3, key.expiration());
	setText(4, QString::number(key.size()));
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
	groupId = NULL;
	m_sig = new KgpgKeySign(sig);

	QString tmpname = sig.name();
	if (!sig.comment().isEmpty())
		tmpname += " (" + sig.comment() + ')';

	setText(0, tmpname);

	QString tmpemail = sig.email();
	if (sig.local())
		tmpemail += i18n(" [local signature]");

	if (sig.revocation()) {
		tmpemail += i18n(" [Revocation signature]");
		setPixmap(0, Images::revoke());
		m_type = RevSign;
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
	delete groupId;
}

KeyListViewItem::ItemType KeyListViewItem::itemType() const
{
    return m_type;
}

QString KeyListViewItem::key(int column, bool) const
{
    return text(column).toLower();
}

KeyListView::KeyListView(QWidget *parent)
           : K3ListView(parent)
{
    setRootIsDecorated(true);
    addColumn(i18nc("Name of key owner", "Name"), 230);
    addColumn(i18nc("Email address of key owner", "Email"), 220);
    addColumn(i18n("Trust"), 60);
    addColumn(i18n("Expiration"), 100);
    addColumn(i18n("Size"), 50);
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
    trustunknown.fill(Convert::toColor(TRUST_UNKNOWN));
    QPainter(&trustunknown).drawPixmap(rect, blankFrame);

    trustbad.load(KStandardDirs::locate("appdata", "pics/kgpg_fill.png"));
    trustbad.fill(Convert::toColor(TRUST_DISABLED));
    QPainter(&trustbad).drawPixmap(rect, blankFrame);

    trustrevoked.load(KStandardDirs::locate("appdata", "pics/kgpg_fill.png"));
    trustrevoked.fill(Convert::toColor(TRUST_REVOKED));
    QPainter(&trustrevoked).drawPixmap(rect, blankFrame);

    trustgood.load(KStandardDirs::locate("appdata", "pics/kgpg_fill.png"));
    trustgood.fill(Convert::toColor(TRUST_FULL));
    QPainter(&trustgood).drawPixmap(rect, blankFrame);

    trustultimate.load(KStandardDirs::locate("appdata", "pics/kgpg_fill.png"));
    trustultimate.fill(Convert::toColor(TRUST_ULTIMATE));
    QPainter(&trustultimate).drawPixmap(rect, blankFrame);

    trustmarginal.load(KStandardDirs::locate("appdata", "pics/kgpg_fill.png"));
    trustmarginal.fill(Convert::toColor(TRUST_MARGINAL));
    QPainter(&trustmarginal).drawPixmap(rect, blankFrame);

    trustexpired.load(KStandardDirs::locate("appdata", "pics/kgpg_fill.png"));
    trustexpired.fill(Convert::toColor(TRUST_EXPIRED));
    QPainter(&trustexpired).drawPixmap(rect, blankFrame);

    header()->setMovingEnabled(false);
    setAcceptDrops(true);
    setDragEnabled(true);
}

QList<KeyListViewItem *> KeyListView::selectedItems(void)
{
	QList<KeyListViewItem *> list;

	Q3ListViewItemIterator it(this, Q3ListViewItemIterator::Selected);

	for(; it.current(); ++it) {
		Q3ListViewItem *q = Q3ListViewItem(*it).parent();
		if ((q->depth() > 0) && !q->parent()->isOpen())
			continue;
		list.append(static_cast<KeyListViewItem*>(q));
	}

	return list;
}

QString KeyListView::statusCountMessage(void)
{
	QString kmsg = i18np("1 Key", "%1 Keys", childCount() - groupNb);

	if (groupNb == 0) {
		return kmsg;
	} else {
		QString gmsg = i18np("1 Group", "%1 Groups", groupNb);

		return kmsg + ", " + gmsg;
	}
}

#include "keylistview.moc"
