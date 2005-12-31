/**************************************************************************
                          kgpgview.h  -  description
                             -------------------
    begin                : Tue Jul  2 12:31:38 GMT 2002
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

#ifndef KGPGVIEW_H
#define KGPGVIEW_H

#include <ktextedit.h>
#include <kurl.h>

class QDragEnterEvent;
class QDropEvent;

class KgpgInterface;

class KgpgTextEdit : public KTextEdit
{
    Q_OBJECT

public:
    KgpgTextEdit(QWidget *parent = 0, const char *name = 0);
    ~KgpgTextEdit();

public slots:
    void slotDroppedFile(const KURL &url);
    bool slotCheckContent(const QString &filetocheck, const bool &checkforpgpmessage = true);
    void slotDecodeFile(const QString &fname);

protected:
    void contentsDragEnterEvent(QDragEnterEvent *e);
    void contentsDropEvent(QDropEvent *e);

private slots:
    void editorUpdateDecryptedtxt(const QString &newtxt, KgpgInterface *interface);
    void editorFailedDecryptedtxt(const QString &newtxt, KgpgInterface *interface);

private:
    QString m_tempfile;
};

class KgpgView : public QWidget
{
    Q_OBJECT

public:
    /**
     * Constructor for the main view
     */
    KgpgView(QWidget *parent = 0, const char *name = 0);

    /**
     * Destructor for the main view
     */
    ~KgpgView();

    KgpgTextEdit *editor;

signals:
    void newText();
    void textChanged();
    void resetEncoding(bool);
    void verifyFinished();

public slots:
    void slotSignVerify();
    void slotEncode();
    void slotDecode();

private slots:
    void updateDecryptedtxt(const QString &newtxt, KgpgInterface *interface);
    void failedDecryptedtxt(const QString &newtxt, KgpgInterface *interface);

    void slotAskForImport(const QString &id, KgpgInterface *interface);
    void slotVerifyResult(const QString &mssge, const QString &log, KgpgInterface *interface);
    void slotSignResult(const QString &signResult, KgpgInterface *interface);

    void encodeTxt(QStringList selec, QStringList encryptoptions, bool, bool symmetric);
    void updateTxt(const QString &newtxt, KgpgInterface *interface);

private:
    bool checkForUtf8(const QString &text);
};

#endif // KGPGVIEW_H
