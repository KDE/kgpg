/*
 * Copyright (C) 2009,2010,2012 Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _KGPGCAFF_P_H
#define _KGPGCAFF_P_H

#include "caff.h"
#include "core/KGpgSignableNode.h"
#include "transactions/kgpgsigntransactionhelper.h"

#include <QList>
#include <QObject>
#include <QScopedPointer>
#include <QString>
#include <QStringList>

class KTempDir;

class KGpgCaffPrivate : public QObject {
	Q_OBJECT

	KGpgCaff * const q_ptr;
	Q_DECLARE_PUBLIC(KGpgCaff)
	Q_DISABLE_COPY(KGpgCaffPrivate)

	QScopedPointer<KTempDir> m_tempdir;
	QStringList m_signers;
	QString m_secringfile;
	const KGpgCaff::OperationFlags m_flags;
	const KGpgSignTransactionHelper::carefulCheck m_checklevel;

	void reexportKey(const KGpgSignableNode *node);
	void abortOperation(int result);
	void checkNextLoop();
public:
	KGpgCaffPrivate(KGpgCaff *parent, const KGpgSignableNode::List &ids, const QStringList &signers,
			const KGpgCaff::OperationFlags flags, const KGpgSignTransactionHelper::carefulCheck checklevel);
	~KGpgCaffPrivate();

	KGpgSignableNode::List m_allids;
	KGpgSignableNode::const_List m_noEncIds;	///< keys without encryption capability that were skipped
	KGpgSignableNode::const_List m_alreadyIds;	///< ids already signed

private slots:
	void slotSigningFinished(int result);
	void slotDelUidFinished(int result);
	void slotExportFinished(int result);
	void slotTextEncrypted(int result);
	void slotReimportDone(int result);
};

#endif /* _KGPGCAFF_P_H */
