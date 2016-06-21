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

#include <QUrl>
#include <QDir>
#include <KConfigGroup>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

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
	: QDialog(parent),
	m_revWidget(new KgpgRevokeWidget(this)),
	m_id(node->getId())
{
	setWindowTitle(i18n("Create Revocation Certificate"));
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
	QWidget *mainWidget = new QWidget(this);
	QVBoxLayout *mainLayout = new QVBoxLayout(this);
	setLayout(mainLayout);
	mainLayout->addWidget(mainWidget);
	QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
	okButton->setDefault(true);
	okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
	connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
	connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
	buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
	setModal(true);

	m_revWidget->keyID->setText(i18nc("<Name> (<Email>) ID: <KeyId>", "%1 (%2) ID: %3",
				node->getName(), node->getEmail(), m_id));
	m_revWidget->outputFile->setUrl(QUrl(QDir::homePath() + QLatin1Char( '/' ) + node->getEmail().section( QLatin1Char( '@' ), 0, 0 )  + QLatin1String( ".revoke" ) ));
	m_revWidget->outputFile->setMode(KFile::File);

	setMinimumSize(m_revWidget->sizeHint());
	mainLayout->addWidget(m_revWidget);
	mainLayout->addWidget(buttonBox);
}

QString KGpgRevokeDialog::getDescription() const
{
	return m_revWidget->textDescription->toPlainText();
}

int KGpgRevokeDialog::getReason() const
{
	return m_revWidget->comboBox1->currentIndex();
}

QUrl KGpgRevokeDialog::saveUrl() const
{
	if (m_revWidget->cbSave->isChecked())
		return m_revWidget->outputFile->url();
	else
		return QUrl();
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
