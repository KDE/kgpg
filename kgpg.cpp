/***************************************************************************
                          kgpg.cpp  -  description
                             -------------------
    begin                : Mon Nov 18 2002
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

#include "kgpg.h"

#include <QApplication>
#include <QtDBus/QtDBus>

#include <KCmdLineArgs>
#include <KMessageBox>
#include <KWindowSystem>
#include <KActionCollection>
#include <KMimeType>

#include "images.h"
#include "kgpgeditor.h"
#include "kgpgsettings.h"   // automatically created by compilation
#include "keysmanager.h"
#include "kgpg_interface.h"
#include "kgpgexternalactions.h"
#include "kgpgview.h"

using namespace KgpgCore;

KGpgApp::KGpgApp()
             : KUniqueApplication(),
	     running(false),
	     w(NULL)
{
}

KGpgApp::~KGpgApp()
{
	delete s_keyManager;
	delete w;
}

void KGpgApp::slotHandleQuit()
{
	s_keyManager->saveToggleOpts();
	quit();
}

void KGpgApp::assistantOver(const QString &defaultKeyId)
{
	if (!defaultKeyId.isEmpty())
		s_keyManager->slotSetDefaultKey(defaultKeyId);

	s_keyManager->show();
	s_keyManager->raise();
}

int KGpgApp::newInstance()
{
	if (!running) {
		running = true;

		const QString gpgPath(KGpgSettings::gpgConfigPath());
		if (!gpgPath.isEmpty())
			if (KUrl::fromPath(gpgPath).directory(KUrl::AppendTrailingSlash) != (QDir::homePath() + "/.gnupg/"))
				setenv("GNUPGHOME", KUrl::fromPath(gpgPath).directory(KUrl::AppendTrailingSlash).toAscii(), 1);

		s_keyManager = new KeysManager();

		w = new KGpgExternalActions(s_keyManager, s_keyManager->getModel());
		connect(w, SIGNAL(importDrop(const QString &)), s_keyManager, SLOT(slotImport(const QString &)));

		connect(s_keyManager, SIGNAL(encryptFiles(KUrl::List)), w, SLOT(encryptFiles(KUrl::List)));
		connect(s_keyManager->s_kgpgEditor, SIGNAL(encryptFiles(KUrl::List)), w, SLOT(encryptFiles(KUrl::List)));

		connect(s_keyManager, SIGNAL(readAgainOptions()), w, SLOT(readOptions()));
		connect(w, SIGNAL(updateDefault(QString)), SLOT(assistantOver(QString)));
		connect(w, SIGNAL(createNewKey()), s_keyManager, SLOT(slotGenerateKey()));

		if (!gpgPath.isEmpty()) {
			if ((KgpgInterface::getGpgBoolSetting("use-agent", gpgPath)) && (qgetenv("GPG_AGENT_INFO").isEmpty()))
				KMessageBox::sorry(0, i18n("<qt>The use of <b>GnuPG Agent</b> is enabled in GnuPG's configuration file (%1).<br />"
					"However, the agent does not seem to be running. This could result in problems with signing/decryption.<br />"
					"Please disable GnuPG Agent from KGpg settings, or fix the agent.</qt>", gpgPath));
		}
	}

	args = KCmdLineArgs::parsedArgs();

	goHome = qobject_cast<KAction *>(s_keyManager->actionCollection()->action("go_default_key"))->shortcut();
	w->goDefaultKey = goHome;

	// parsing of command line args
	if (args->isSet("k") || (!KGpgSettings::showSystray() && (args->count() == 0) && !args->isSet("d"))) {
		s_keyManager->show();
		KWindowSystem::setOnDesktop(s_keyManager->winId(), KWindowSystem::currentDesktop());  //set on the current desktop
		KWindowSystem::unminimizeWindow(s_keyManager->winId());  //de-iconify window
		s_keyManager->raise();  // set on top
	} else if (args->isSet("d")) {
		s_keyManager->slotOpenEditor();
		s_keyManager->hide();
	} else if (args->count() > 0) {
		urlList.clear();
		for (int ct = 0; ct < args->count(); ct++)
			urlList.append(args->url(ct));

		if (urlList.empty())
			return 0;

		w->droppedUrl = urlList.first();

		bool directoryInside = false;
		const QStringList lst = urlList.toStringList();
		for (QStringList::const_iterator it = lst.begin(); it != lst.end(); ++it)
			if (KMimeType::findByUrl(KUrl(*it))->name() == "inode/directory")
				directoryInside = true;

		if ((directoryInside) && (lst.count() > 1)) {
			KMessageBox::sorry(0, i18n("Unable to perform requested operation.\nPlease select only one folder, or several files, but do not mix files and folders."));
			return 0;
		}

		w->droppedUrls = urlList;

		if (args->isSet("e")) {
			if (!directoryInside)
				w->encryptDroppedFile();
			else
				w->encryptDroppedFolder();
		} else if (args->isSet("s")) {
			if (!directoryInside)
				w->showDroppedFile();
			else
				KMessageBox::sorry(0, i18n("Cannot decrypt and show folder."));
		} else if (args->isSet("S")) {
			if (!directoryInside)
				w->signDroppedFile();
			else
				KMessageBox::sorry(0, i18n("Cannot sign folder."));
		} else if (args->isSet("V") != 0) {
			if (!directoryInside)
				w->slotVerifyFile();
			else
				KMessageBox::sorry(0, i18n("Cannot verify folder."));
		} else {
			if (w->droppedUrl.fileName().endsWith(QLatin1String(".sig"))) {
				w->slotVerifyFile();
			} else {
				bool haskeys = false;
				bool hastext = false;
				foreach (const KUrl &url, urlList) {
					QFile qfile(url.path());
					if (qfile.open(QIODevice::ReadOnly)) {
						QTextStream t(&qfile);
						QString result(t.read(1024));
						qfile.close();

						if (KgpgTextEdit::checkForKey(result))
							haskeys = true;
						else
							hastext = true;
					}
				}

				if (hastext)
					w->decryptDroppedFile();
				else
					s_keyManager->slotImport(urlList);
			}
		}
	}
	return 0;
}

#include "kgpg.moc"
