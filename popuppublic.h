/***************************************************************************
                          popuppublic.h  -  description
                             -------------------
    begin                : Sat Jun 29 2002
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
#ifndef POPUPPUBLIC_H
#define POPUPPUBLIC_H

#include <kdialogbase.h>

//#include <kiconloader.h>
#include <kshortcut.h>


class QPushButton;
class QCheckBox;
class KListView;
class QButtonGroup;
class KProcIO;

class popupPublic : public KDialogBase //QDialog
{
        Q_OBJECT
public:

        popupPublic(QWidget *parent=0, const char *name=0,QString sfile="",bool filemode=false,KShortcut goDefaultKey=QKeySequence(CTRL+Qt::Key_Home));
	~popupPublic();
        KListView *keysList;
        QCheckBox *CBarmor,*CBuntrusted,*CBshred,*CBsymmetric,*CBhideid;
        bool fmode,trusted;
        QPixmap keyPair,keySingle,keyGroup;
        QString seclist;
	QStringList untrustedList;

private:
        KConfig *config;
        QButtonGroup *boutonboxoptions;
        QString customOptions;

private slots:
        void customOpts(const QString &);
        void slotprocread(KProcIO *);
        void slotpreselect();
        void refreshkeys();
        void refresh(bool state);
        void isSymetric(bool state);
        void sort();
        void enable();
	void slotGotoDefaultKey();
	
public slots:
void slotAccept();
void slotSetVisible();

protected slots:
virtual void slotOk();
	
signals:
        void selectedKey(QStringList ,QStringList,bool,bool);
	void keyListFilled();

};

#endif // POPUPPUBLIC_H

