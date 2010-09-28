/* Copyright 2008,2009,2010 Rolf Eike Beer <kde@opensource.sf-tec.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "KGpgUatNode.h"

#include <KLocale>

#include <QPixmap>
#include <QDateTime>

#include "kgpginterface.h"
#include "KGpgKeyNode.h"

class KGpgUatNodePrivate {
public:
	KGpgUatNodePrivate(const unsigned int index, const QStringList &sl);

	QPixmap m_pixmap;
	const QString m_idx;
	QDateTime m_creation;
};

KGpgUatNodePrivate::KGpgUatNodePrivate(const unsigned int index, const QStringList &sl)
	: m_idx(QString::number(index))
{
	if (sl.count() < 6)
		return;
	m_creation = QDateTime::fromTime_t(sl.at(5).toUInt());
}

KGpgUatNode::KGpgUatNode(KGpgKeyNode *parent, const unsigned int index, const QStringList &sl)
	: KGpgSignableNode(parent),
	d_ptr(new KGpgUatNodePrivate(index, sl))
{
	d_ptr->m_pixmap = KgpgInterface::loadPhoto(parent->getKeyId(), d_ptr->m_idx);
}

KGpgUatNode::~KGpgUatNode()
{
	delete d_ptr;
}

QString
KGpgUatNode::getName() const
{
	return i18n("Photo id");
}

QString
KGpgUatNode::getSize() const
{
	const Q_D(KGpgUatNode);

	return QString::number(d->m_pixmap.width()) + QLatin1Char( 'x' ) + QString::number(d->m_pixmap.height());
}

QDateTime
KGpgUatNode::getCreation() const
{
	const Q_D(KGpgUatNode);

	return d->m_creation;
}

KGpgKeyNode *
KGpgUatNode::getParentKeyNode() const
{
	return m_parent->toKeyNode();
}

void
KGpgUatNode::readChildren()
{
}

KgpgCore::KgpgItemType
KGpgUatNode::getType() const
{
	return KgpgCore::ITYPE_UAT;
}

KgpgCore::KgpgKeyTrust
KGpgUatNode::getTrust() const
{
	return KgpgCore::TRUST_NOKEY;
}

const QPixmap &
KGpgUatNode::getPixmap() const
{
	const Q_D(KGpgUatNode);

	return d->m_pixmap;
}

QString
KGpgUatNode::getId() const
{
	const Q_D(KGpgUatNode);

	return d->m_idx;
}

KGpgKeyNode *
KGpgUatNode::getKeyNode(void)
{
	return getParentKeyNode()->toKeyNode();
}

const KGpgKeyNode *
KGpgUatNode::getKeyNode(void) const
{
	return getParentKeyNode()->toKeyNode();
}
