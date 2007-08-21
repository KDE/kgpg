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

#include <KTextEdit>
#include <KUrl>

class QDragEnterEvent;
class QDropEvent;

class KgpgInterface;

class KgpgTextEdit : public KTextEdit
{
    Q_OBJECT

public:
    explicit KgpgTextEdit(QWidget *parent = 0);
    ~KgpgTextEdit();

signals:
    void newText();

public slots:
    void slotDroppedFile(const KUrl &url);

    bool slotCheckFile(const QString &filetocheck, const bool &checkforpgpmessage = true);
    void slotDecodeFile(const QString &fname);

    void slotEncode();
    void slotDecode();
    void slotSign();
    void slotVerify();

protected:
    void dragEnterEvent(QDragEnterEvent *e);
    void dropEvent(QDropEvent *e);

private:
    void deleteFile();
    bool checkForUtf8(const QString &text);

private slots:
    void editorUpdateDecryptedtxt(const QString &content, KgpgInterface *interface);
    void editorFailedDecryptedtxt(const QString &content, KgpgInterface *interface);

    void slotEncodeUpdate(const QString &content, KgpgInterface *interface);
    void slotDecodeUpdateSuccess(const QString &content, KgpgInterface *interface);
    void slotDecodeUpdateFailed(const QString &content, KgpgInterface *interface);
    void slotSignUpdate(const QString &content, KgpgInterface *interface);
    void slotVerifySuccess(const QString &content, const QString &log, KgpgInterface *interface);
    void slotVerifyKeyNeeded(const QString &id, KgpgInterface *interface);

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
    KgpgView(QWidget *parent = 0);

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

    void slotHighlightText(const QString &, const int &matchingindex, const int &matchedlength);

    void encodeTxt(QStringList selec, QStringList encryptoptions, const bool, const bool symmetric);
    void updateTxt(const QString &newtxt, KgpgInterface *interface);

private:
    bool checkForUtf8(const QString &text);
};

#endif // KGPGVIEW_H
