/***************************************************************************
                          popupimport.h  -  description
                             -------------------
    begin                : Mon Jul 8 2002
    copyright            : (C) 2002 by y0k0
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
#ifndef POPUPIMPORT_H
#define POPUPIMPORT_H


#include <kdialogbase.h>
#include <kurl.h>

class QRadioButton;
class QHButtonGroup;
class KLineEdit;
class QButtonGroup;
class QCheckBox;
class QPushButton;
class QGridLayout;

class popupImport : public KDialogBase
{
        Q_OBJECT
public:

        popupImport(const QString& caption, QWidget *parent=0, const char *name=0, KURL oldnam=KURL());

        QRadioButton *checkFile,*checkClipboard;
        //  QVButtonGroup *vgroup;
        QHButtonGroup *hgroup;
        KLineEdit *newFilename;
        KURL path;
        QButtonGroup* bGroupSources;
//        QCheckBox *importSecretKeys;
        QPushButton* buttonToolbar;

protected:
        QGridLayout* bGroupSourcesLayout;

public slots:
        void slotchooseurl();
        void slotenable(bool);
};

#endif
