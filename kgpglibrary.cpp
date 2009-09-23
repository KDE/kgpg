/***************************************************************************
                          kgpglibrary.cpp  -  description
                             -------------------
    begin                : Mon Jul 8 2002
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

#include "kgpglibrary.h"

#include <QDesktopWidget>
#include <QApplication>
#include <QTextStream>
#include <QFile>

#include <KFileDialog>
#include <KPassivePopup>
#include <kio/renamedialog.h>
#include <KMessageBox>
#include <KLocale>

#include "images.h"
#include "selectpublickeydialog.h"
#include "kgpgsettings.h"
#include "kgpginterface.h"
#include "kgpgtextinterface.h"
#include "kgpgitemmodel.h"

using namespace KgpgCore;

KgpgLibrary::KgpgLibrary(QWidget *parent)
	: m_pop(NULL)
{
	m_extension = ".gpg";

	m_popisactive = false;
	m_panel = parent;
}

void KgpgLibrary::setFileExtension(const QString &ext)
{
	m_extension = ext;
}

void KgpgLibrary::slotFileEnc(const KUrl::List &urls, const QStringList &opts, KGpgItemModel *model, const KShortcut &goDefaultKey, const QString &defaultKey)
{
	if (urls.empty())
		return;

	m_urlselecteds = urls;
	QString fileNames(urls.first().fileName());
	if (urls.count() > 1)
		fileNames += ", ...";	// FIXME

	KgpgSelectPublicKeyDlg *dialog = new KgpgSelectPublicKeyDlg(0, model, goDefaultKey, fileNames);
	if (dialog->exec() == KDialog::Accepted) {
		QStringList options(opts);
		if (dialog->getUntrusted())
			options.append("--always-trust");
		if (dialog->getArmor())
			options.append("--armor");
		if (dialog->getHideId())
			options.append("--throw-keyid");

		if (KGpgSettings::allowCustomEncryptionOptions()) {
			const QString customopts(dialog->getCustomOptions().isEmpty());

			if (!customopts.isEmpty())
				options << customopts.split(' ', QString::SkipEmptyParts);
		}

		QStringList keys(dialog->selectedKeys());
		if (!defaultKey.isEmpty() && !keys.contains(defaultKey))
			keys.append(defaultKey);
		startEncode(keys, options, dialog->getSymmetric());
	}

	delete dialog;
}

void KgpgLibrary::startEncode(const QStringList &encryptkeys, const QStringList &encryptoptions, const bool symetric)
{
	m_popisactive = false;
	m_encryptkeys = encryptkeys;
	m_encryptoptions = encryptoptions;
	m_symetric = symetric;
	fastEncode(m_urlselecteds.first(), encryptkeys, encryptoptions, symetric);
}

void KgpgLibrary::fastEncode(const KUrl &filetocrypt, const QStringList &encryptkeys, const QStringList &encryptoptions, const bool symetric)
{
	if ((encryptkeys.isEmpty()) && (!symetric)) {
		KMessageBox::sorry(0, i18n("You have not chosen an encryption key."));
		return;
	}

	m_urlselected = filetocrypt;

	KUrl dest;
	if (encryptoptions.contains("--armor"))
		dest.setPath(m_urlselected.path() + ".asc");
	else
		dest.setPath(m_urlselected.path() + m_extension);

	QFile fgpg(dest.path());
	if (fgpg.exists()) {
		KIO::RenameDialog over(0, i18n("File Already Exists"), KUrl(), dest, KIO::M_OVERWRITE);
		if (over.exec() == QDialog::Rejected) {
			emit systemMessage(QString(), true);
			return;
		}

		dest = over.newDestUrl();
	}

	int filesToEncode = m_urlselecteds.count();
	emit systemMessage(i18np("<p><b>Encrypting </b>%2</p>", "<p><b>%1 Files left.</b><br /><b>Encrypting </b>%2</p>", filesToEncode, m_urlselecteds.first().path()));

	m_pop = new KPassivePopup(m_panel);

	KGpgTextInterface *cryptFileProcess = new KGpgTextInterface();
	connect(cryptFileProcess, SIGNAL(fileEncryptionFinished(KUrl, KGpgTextInterface*)), SLOT(processEnc(KUrl, KGpgTextInterface*)));
	connect(cryptFileProcess, SIGNAL(errorMessage(QString, KGpgTextInterface*)), SLOT(processEncError(QString, KGpgTextInterface*)));
	cryptFileProcess->encryptFile(encryptkeys, m_urlselected, dest, encryptoptions, symetric);

	m_popisactive = true;
}

void KgpgLibrary::processEnc(const KUrl &, KGpgTextInterface *i)
{
	i->deleteLater();
	emit systemMessage(QString());

	m_urlselecteds.pop_front();

	if (m_urlselecteds.count() > 0)
		fastEncode(m_urlselecteds.first(), m_encryptkeys, m_encryptoptions, m_symetric);
	else
		emit encryptionOver();
}

void KgpgLibrary::processEncError(const QString &mssge, KGpgTextInterface *i)
{
	i->deleteLater();
	m_popisactive = false;
	emit systemMessage(QString(), true);
	KMessageBox::detailedSorry(m_panel, i18n("<p><b>Process halted</b>.<br />Not all files were encrypted.</p>"), mssge);
}

void KgpgLibrary::slotFileDec(const KUrl &src, const KUrl &dest, const QStringList &customDecryptOption)
{
	// decode file from konqueror or menu
	m_pop = new KPassivePopup();
	m_urlselected = src;

	KGpgTextInterface *decryptFileProcess = new KGpgTextInterface();
	decryptFileProcess->decryptFile(src, dest, customDecryptOption);
	connect(decryptFileProcess, SIGNAL(decryptFileStarted(KUrl)), SLOT(processEncPopup(KUrl)));
	connect(decryptFileProcess, SIGNAL(decryptFileFinished(int, KGpgTextInterface*)), SLOT(processDecOver(int, KGpgTextInterface*)));
}

void KgpgLibrary::processDecOver(int ret, KGpgTextInterface *iface)
{
	emit systemMessage(QString());
	delete m_pop;
	m_pop = NULL;
	iface->deleteLater();
	if (ret != 0)
		emit decryptionOver(this, m_urlselected);
	else
		emit decryptionOver(this, KUrl());
}

void KgpgLibrary::slotImportOver(KgpgInterface *iface, const QStringList &keys)
{
	iface->deleteLater();
	emit importOver(this, keys);
}

void KgpgLibrary::processEncPopup(const KUrl &url)
{
	emit systemMessage(i18n("Decrypting %1", url.pathOrUrl()));

	m_pop->setTimeout(0);
	m_pop->setView(i18n("Processing decryption"), i18n("Please wait..."), Images::kgpg());
	m_pop->show();

	QRect qRect(QApplication::desktop()->screenGeometry());
	int iXpos = qRect.width() / 2 - m_pop->width() / 2;
	int iYpos = qRect.height() / 2 - m_pop->height() / 2;
	m_pop->move(iXpos, iYpos);
}

#include "kgpglibrary.moc"
