/*
    SPDX-FileCopyrightText: 2009-2022 Rolf Eike Beer <kde@opensource.sf-tec.de>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KGPGPRIMARYUID_H
#define KGPGPRIMARYUID_H

#include <QObject>

#include "kgpguidtransaction.h"

class KGpgUidNode;

/**
 * @brief transaction class to change the primary user id of a key
 */
class KGpgPrimaryUid: public KGpgUidTransaction {
	Q_OBJECT

	Q_DISABLE_COPY(KGpgPrimaryUid)
	KGpgPrimaryUid() = delete;

public:
	/**
	 * @brief constructor
	 * @param parent parent object
	 * @param uid user id to become new primary one
	 */
	KGpgPrimaryUid(QObject *parent, KGpgUidNode *uid);
	/**
	 * @brief destructor
	 */
	~KGpgPrimaryUid() override = default;

protected:
	bool nextLine(const QString &line) override;
	bool passphraseReceived() override;
};

#endif // KGPGPRIMARYUID_H
