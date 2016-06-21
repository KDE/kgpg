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
#include <KConfigGroup>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

KgpgDetailedInfo::KgpgDetailedInfo(QWidget *parent, const QString &boxLabel, const QString &errormessage,
		const QStringList &keysList, const QString &caption)
	: QDialog(parent)
{
	if (!caption.isEmpty())
		setWindowTitle(caption);
	else
		setWindowTitle(i18n("Info"));
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
	QWidget *mainWidget = new QWidget(this);
	QVBoxLayout *mainLayout = new QVBoxLayout(this);
	setLayout(mainLayout);
	mainLayout->addWidget(mainWidget);
	QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
	okButton->setDefault(true);
	okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
	connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
	connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
	mainLayout->addWidget(buttonBox);
	okButton->setDefault(true);
	setModal(true);
// 	KMessageBox::createKMessageBox(this, QMessageBox::Information, // krazy:exclude=qtclasses
// 				   boxLabel, keysList, QString(), Q_NULLPTR, 0, errormessage); FIXME: KF5
}
