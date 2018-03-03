/*
 * Copyright (C) 2008,2009,2010,2012,2017 Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KGPGCHANGEPASS_H
#define KGPGCHANGEPASS_H

#include <QObject>

#include "kgpgtransaction.h"

/**
 * @brief set a new passphrase for a key pair
 */
class KGpgChangePass: public KGpgTransaction {
	Q_OBJECT

	Q_DISABLE_COPY(KGpgChangePass)
public:
	KGpgChangePass(QObject *parent, const QString &keyid);
	virtual ~KGpgChangePass();

protected:
	bool nextLine(const QString &line) override;
	bool preStart() override;
	bool passphraseRequested() override;
	bool passphraseReceived() override;
	bool hintLine(const KGpgTransaction::ts_hintType hint, const QString & args) override;

private:
	bool m_seenold;		///< old password correctly entered
};

#endif // KGPGCHANGEPASS_H
