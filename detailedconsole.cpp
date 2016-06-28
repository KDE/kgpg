/***************************************************************************
                          detailedconsole.cpp  -  description
                             -------------------
    begin                : Mon Jul 8 2002
    copyright            : (C) 2002 by Jean-Baptiste Mardelle
    email                : bj@altern.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "detailedconsole.h"

#include <KLocalizedString>
#include <KMessageBox>
#include <QDialogButtonBox>
#include <QPushButton>

KgpgDetailedInfo::KgpgDetailedInfo(QWidget *parent, const QString &boxLabel, const QString &errormessage,
		const QStringList &keysList, const QString &caption)
	: QDialog(parent)
{
	if (!caption.isEmpty())
		setWindowTitle(caption);
	else
		setWindowTitle(i18n("Info"));
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
	QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
	okButton->setDefault(true);
	okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
	connect(buttonBox, &QDialogButtonBox::accepted, this, &KgpgDetailedInfo::accept);
	connect(buttonBox, &QDialogButtonBox::rejected, this, &KgpgDetailedInfo::reject);
	okButton->setDefault(true);
	setModal(true);
	KMessageBox::createKMessageBox(this, buttonBox, QMessageBox::Information, boxLabel, keysList, QString(), Q_NULLPTR, 0, errormessage);
}
