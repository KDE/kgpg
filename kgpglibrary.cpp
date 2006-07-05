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

#include <QDesktopWidget>
#include <QTextStream>
#include <QFile>

#include <kfiledialog.h>
#include <kpassivepopup.h>
#include <kio/renamedlg.h>
#include <kapplication.h>
#include <kmessagebox.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kconfig.h>
#include <kprocio.h>
#include <kio/job.h>

#include "kgpglibrary.h"
#include "popuppublic.h"
#include "kgpgsettings.h"
#include "kgpginterface.h"

KgpgLibrary::KgpgLibrary(QWidget *parent, const bool &pgpExtension)
{
    if (pgpExtension)
        m_extension = ".pgp";
    else
        m_extension = ".gpg";

    m_popisactive = false;
    m_panel = parent;
}

void KgpgLibrary::slotFileEnc(const KUrl::List &urls, const QStringList &opts, const QStringList &defaultKey, const KShortcut &goDefaultKey)
{
    if (!urls.empty())
    {
        m_urlselecteds = urls;
        if (defaultKey.isEmpty())
        {
            QString fileNames = urls.first().fileName();
            if (urls.count() > 1)
                fileNames += ",...";

            KgpgSelectPublicKeyDlg *dialog = new KgpgSelectPublicKeyDlg(0, fileNames, true, true, goDefaultKey);
            if (dialog->exec() == KDialog::Accepted)
            {
                QStringList options;
                if (dialog->getUntrusted()) options << "--always-trust";
                if (dialog->getArmor())     options << "--armor";
                if (dialog->getHideId())    options << "--throw-keyid";

                if (!dialog->getCustomOptions().isEmpty())
                    if (KGpgSettings::allowCustomEncryptionOptions())
                        options << dialog->getCustomOptions().split(" ");

                startEncode(dialog->selectedKeys(), options, dialog->getShred(), dialog->getSymmetric());
            }

            delete dialog;
        }
        else
            startEncode(defaultKey, opts, false, false);
    }
}

void KgpgLibrary::startEncode(const QStringList &encryptkeys, const QStringList &encryptoptions, const bool &shred, const bool &symetric)
{
    m_popisactive = false;
    //KUrl::List::iterator it;
    //filesToEncode=m_urlselecteds.count();
    m_encryptkeys = encryptkeys;
    m_encryptoptions = encryptoptions;
    m_shred = shred;
    m_symetric = symetric;
    fastEncode(m_urlselecteds.first(), encryptkeys, encryptoptions, symetric);
}

void KgpgLibrary::fastEncode(const KUrl &filetocrypt, const QStringList &encryptkeys, const QStringList &encryptoptions, const bool &symetric)
{
    if ((encryptkeys.isEmpty()) && (!symetric))
    {
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
    if (fgpg.exists())
    {
        KIO::RenameDlg *over = new KIO::RenameDlg(0, i18n("File Already Exists"), QString::null, dest.path(), KIO::M_OVERWRITE);
        if (over->exec() == QDialog::Rejected)
        {
            delete over;
            emit systemMessage(QString::null, true);
            return;
        }

        dest = over->newDestURL();
        delete over;
    }

    int filesToEncode = m_urlselecteds.count();
    if (filesToEncode > 1)
        emit systemMessage(i18n("<b>%1 Files left.</b>\nEncrypting </b>%2", filesToEncode, m_urlselecteds.first().path()));
    else
        emit systemMessage(i18n("<b>Encrypting </b>%2", m_urlselecteds.first().path()));

    m_pop = new KPassivePopup(m_panel);

    KgpgInterface *cryptFileProcess = new KgpgInterface();
    connect(cryptFileProcess, SIGNAL(fileEncryptionFinished(KUrl, KgpgInterface*)), this, SLOT(processEnc(KUrl, KgpgInterface*)));
    connect(cryptFileProcess, SIGNAL(errorMessage(QString, KgpgInterface*)), this, SLOT(processEncError(QString, KgpgInterface*)));
    cryptFileProcess->encryptFile(encryptkeys, m_urlselected, dest, encryptoptions, symetric);

    if (!m_popisactive)
    {
        //connect(cryptFileProcess,SIGNAL(processstarted(QString)),this,SLOT(processpopup2(QString)));
        m_popisactive = true;
    }

}

void KgpgLibrary::processEnc(KUrl, KgpgInterface *i)
{
    delete i;
    emit systemMessage(QString::null);

    if (m_shred)
        shredProcessEnc(m_urlselecteds.first());

    m_urlselecteds.pop_front();

    if (m_urlselecteds.count() > 0)
        fastEncode(m_urlselecteds.first(), m_encryptkeys, m_encryptoptions, m_symetric);
    else
        emit encryptionOver();
}

void KgpgLibrary::processEncError(const QString &mssge, KgpgInterface *i)
{
    delete i;
    m_popisactive = false;
    emit systemMessage(QString::null, true);
    KMessageBox::detailedSorry(m_panel, i18n("<b>Process halted</b>.<br>Not all files were encrypted."), mssge);
}

void KgpgLibrary::slotFileDec(const KUrl &srcUrl, const KUrl &destUrl, const QStringList &customDecryptOption)
{
    // decode file from konqueror or menu
    m_pop = new KPassivePopup();
    m_urlselected = srcUrl;

    KgpgInterface *decryptFileProcess = new KgpgInterface();
    decryptFileProcess->KgpgDecryptFile(srcUrl, destUrl, customDecryptOption);
    connect(decryptFileProcess, SIGNAL(processaborted(bool)), this, SLOT(processdecover()));
    connect(decryptFileProcess, SIGNAL(processstarted(QString)), this, SLOT(processEncPopup(QString)));
    connect(decryptFileProcess, SIGNAL(decryptionfinished()), this, SLOT(processdecover()));
    connect(decryptFileProcess, SIGNAL(errormessage(QString)), this, SLOT(processdecerror(QString)));
}

void KgpgLibrary::processDecOver()
{
    emit systemMessage(QString::null);
    delete m_pop;
    emit decryptionOver();
}

void KgpgLibrary::processDecError(const QString &mssge)
{
    delete m_pop;
    emit systemMessage(QString::null);

    // test if file is a public key
    QFile qfile(QFile::encodeName(m_urlselected.path()));
    if (qfile.open(QIODevice::ReadOnly))
    {
        QTextStream t(&qfile);
        QString result = t.readAll();
        qfile.close();

        // if pgp data found, decode it
        if (result.startsWith("-----BEGIN PGP PUBLIC KEY BLOCK"))
        {
            // dropped file is a public key, ask for import
            int result = KMessageBox::warningContinueCancel(0, i18n("<p>The file <b>%1</b> is a public key.<br>Do you want to import it ?</p>", m_urlselected.path()), i18n("Warning"));
            if (result == KMessageBox::Cancel)
                return;
            else
            {
                KgpgInterface *importKeyProcess = new KgpgInterface();
                importKeyProcess->importKey(m_urlselected);
                connect(importKeyProcess, SIGNAL(importfinished(QStringList)), this, SIGNAL(importOver(QStringList)));
                return;
            }
        }
        else
        if(result.startsWith("-----BEGIN PGP PRIVATE KEY BLOCK"))
        {
            // dropped file is a public key, ask for import
            qfile.close();
            KMessageBox::information(0, i18n("<p>The file <b>%1</b> is a private key block. Please use KGpg key manager to import it.</p>", m_urlselected.path()));
            return;
        }
    }

    KMessageBox::detailedSorry(0, i18n("Decryption failed."), mssge);
}

void KgpgLibrary::processEncPopup(const QString &fileName)
{
    emit systemMessage(i18n("Decrypting %1", fileName));

    m_pop->setTimeout(0);
    m_pop->setView(i18n("Processing decryption"), i18n("Please wait..."), KGlobal::iconLoader()->loadIcon("kgpg", K3Icon::Desktop));
    m_pop->show();

    QRect qRect(QApplication::desktop()->screenGeometry());
    int iXpos = qRect.width() / 2 - m_pop->width() / 2;
    int iYpos = qRect.height() / 2 - m_pop->height() / 2;
    m_pop->move(iXpos, iYpos);
}

void KgpgLibrary::shredProcessEnc(const KUrl::List &filestoshred)
{
    emit systemMessage(i18np("Shredding %n file", "Shredding %n files", filestoshred.count()));
    KIO::Job *job = KIO::del(filestoshred, true, true);
    connect(job, SIGNAL(result(KJob *)), SLOT(slotShredResult(KJob *)));
}

void KgpgLibrary::slotShredResult(KJob *job)
{
    emit systemMessage(QString::null);
    if (job && job->error())
    {
        static_cast<KIO::Job*>(job)->showErrorDialog(static_cast<QWidget*>(parent()));
        emit systemMessage(QString::null, true);

        KPassivePopup::message(i18n("KGpg Error"), i18n("Process halted, not all files were shredded."), KGlobal::iconLoader()->loadIcon("kgpg", K3Icon::Desktop), m_panel, 0);
    }
}

void KgpgLibrary::processPopup2(const QString &fileName)
{
    //m_pop->setTimeout(0);
    m_pop->setView(i18n("Processing encryption (%1)", fileName),i18n("Please wait..."), KGlobal::iconLoader()->loadIcon("kgpg", K3Icon::Desktop));
    m_pop->show();
    /*QRect qRect(QApplication::desktop()->screenGeometry());
    int iXpos=qRect.width()/2-m_pop->width()/2;
    int iYpos=qRect.height()/2-m_pop->height()/2;
    m_pop->move(iXpos,iYpos);*/
}

void KgpgLibrary::addPhoto(QString keyid)
{
    QString mess = i18n("The image must be a JPEG file. Remember that the image is stored within your public key."
                        "If you use a very large picture, your key will become very large as well! Keeping the image "
                        "close to 240x288 is a good size to use.");

    if (KMessageBox::warningContinueCancel(0, mess) != KMessageBox::Continue)
        return;

    QString imagepath = KFileDialog::getOpenFileName(KUrl(), "image/jpeg", 0);
    if (imagepath.isEmpty())
        return;

    KgpgInterface *interface = new KgpgInterface();
    connect(interface, SIGNAL(addPhotoFinished(int, KgpgInterface*)), this, SLOT(slotAddPhotoFinished(int, KgpgInterface*)));
    interface->addPhoto(keyid, imagepath);
}

void KgpgLibrary::slotAddPhotoFinished(int res, KgpgInterface *interface)
{
    delete interface;

    // TODO : add res == 3 (bad passphrase)

    if (res == 2)
        emit photoAdded();
}

#include "kgpglibrary.moc"
