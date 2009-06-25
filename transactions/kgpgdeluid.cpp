/**
 * Copyright (C) 2008,2009 Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "kgpgdeluid.h"
#include "gpgproc.h"

#include <QtAlgorithms>

#include "kgpgkey.h"
#include "kgpgitemnode.h"

KGpgDelUid::KGpgDelUid(QObject *parent, const KGpgSignableNode *uid)
	: KGpgUidTransaction(parent, uid->getParentKeyNode()->getId(), uid->getId()),
	m_fixargs(addArgument("deluid"))
{
	setUid(uid);
}

KGpgDelUid::KGpgDelUid(QObject *parent, const QList<const KGpgSignableNode *> &uids)
	: KGpgUidTransaction(parent),
	m_fixargs(addArgument("deluid"))
{
	setUids(uids);
}

KGpgDelUid::KGpgDelUid(QObject *parent, const KGpgKeyNode *keynode, const int uid)
	: KGpgUidTransaction(parent),
	m_fixargs(addArgument("deluid"))
{
	setUids(keynode, uid);
}

KGpgDelUid::~KGpgDelUid()
{
}

void
KGpgDelUid::setUid(const KGpgSignableNode *uid)
{
	QList<const KGpgSignableNode *> uids;

	uids.append(uid);
	setUids(uids);
}

bool
reverseSignNodeLessThan(const KGpgSignableNode *s1, const KGpgSignableNode *s2)
{
	return *s2 < *s1;
}

void
KGpgDelUid::setUids(const QList<const KGpgSignableNode *> &uids)
{
	m_uids = uids;

	GPGProc *proc = getProcess();

	QStringList args(proc->program());
	proc->clearProgram();

	for (int i = args.count() - m_fixargs - 1; i > 0; i--)
		args.removeLast();

	// FIXME: can this use qGreater<>()?
	qSort(m_uids.begin(), m_uids.end(), reverseSignNodeLessThan);

	const KGpgSignableNode *nd = m_uids.first();
	const KGpgExpandableNode *parent;
	if (nd->getType() & ITYPE_PAIR)
		parent = nd;
	else
		parent = nd->getParentKeyNode();

	for (int i = 1; i < m_uids.count(); i++) {
		nd = m_uids.at(i);

		Q_ASSERT((nd->getParentKeyNode() == parent) || (nd == parent));

		args.append("uid");
		if (nd->getType() & ITYPE_PAIR)
			args.append(QString('1'));
		else
			args.append(nd->getId());
		args.append("deluid");
	}

	proc->setProgram(args);
	nd = m_uids.first();

	switch (nd->getType()) {
	case ITYPE_PUBLIC:
	case ITYPE_PAIR:
		KGpgUidTransaction::setUid(1);
		setKeyId(nd->getId());
		break;
	default:
		KGpgUidTransaction::setUid(nd->getId());
		setKeyId(nd->getParentKeyNode()->getId());
		break;
	}
}

void
KGpgDelUid::setUids(const KGpgKeyNode *keynode, const int uid)
{
	Q_ASSERT(uid != 0);

	QList<const KGpgSignableNode *> uids;
	const KGpgSignableNode *uidnode;

	if (uid > 0) {
		uidnode = keynode->getUid(uid);

		Q_ASSERT(uidnode != NULL);
		uids.append(uidnode);
	} else {
		int idx = 0;

		forever {
			idx++;
			if (idx == -uid)
				continue;

			uidnode = keynode->getUid(idx);

			if (uidnode == NULL)
				break;

			// do it like caff: attach UATs to every id when mailing
			if (uidnode->getType() != ITYPE_UAT)
				uids.append(uidnode);
		}
	}

	setUids(uids);
}

bool
KGpgDelUid::nextLine(const QString &line)
{
	if (line.contains("keyedit.remove.uid.okay")) {
		write("YES");
		m_uids.removeFirst();
	} else {
		return standardCommands(line);
	}

	return false;
}

void
KGpgDelUid::finish()
{
	if (!m_uids.isEmpty())
		setSuccess(TS_MSG_SEQUENCE);
}
