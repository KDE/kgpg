/***************************************************************************
    begin                : Thu Jul 4 2002
    copyright          : (C) 2002 by Jean-Baptiste Mardelle
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


#include "kgpgrevokewidget.h"

#include "core/KGpgKeyNode.h"

#include <QDir>

KgpgRevokeWidget::KgpgRevokeWidget(QWidget* parent)
	: QWidget(parent),
	Ui_KgpgRevokeWidget()
{
	setupUi(this);
	connect(cbSave, SIGNAL(toggled(bool)), SLOT(cbSave_toggled(bool)));
}

void KgpgRevokeWidget::cbSave_toggled(bool isOn)
{
	outputFile->setEnabled(isOn);
}

KGpgRevokeDialog::KGpgRevokeDialog(QWidget* parent, const KGpgKeyNode *node)
	: KDialog(parent),
	m_revWidget(new KgpgRevokeWidget(this)),
	m_id(node->getId())
{
	setCaption(i18n("Create Revocation Certificate"));
	setButtons(KDialog::Ok | KDialog::Cancel);
	setDefaultButton(KDialog::Ok);
	setModal(true);

	m_revWidget->keyID->setText(i18nc("<Name> (<Email>) ID: <KeyId>", "%1 (%2) ID: %3",
				node->getName(), node->getEmail(), m_id));
	m_revWidget->outputFile->setUrl(QString(QDir::homePath() + QLatin1Char( '/' ) + node->getEmail().section( QLatin1Char( '@' ), 0, 0 )  + QLatin1String( ".revoke" ) ));
	m_revWidget->outputFile->setMode(KFile::File);

	setMinimumSize(m_revWidget->sizeHint());
	setMainWidget(m_revWidget);
}

QString KGpgRevokeDialog::getDescription() const
{
	return m_revWidget->textDescription->toPlainText();
}

int KGpgRevokeDialog::getReason() const
{
	return m_revWidget->comboBox1->currentIndex();
}

KUrl KGpgRevokeDialog::saveUrl() const
{
	if (m_revWidget->cbSave->isChecked())
		return m_revWidget->outputFile->url();
	else
		return KUrl();
}

QString KGpgRevokeDialog::getId() const
{
	return m_id;
}

bool KGpgRevokeDialog::importChecked()
{
	return m_revWidget->cbImport->isChecked();
}

bool KGpgRevokeDialog::printChecked()
{
	return m_revWidget->cbPrint->isChecked();
}

#include "kgpgrevokewidget.moc"
