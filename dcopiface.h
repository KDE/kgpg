/***************************************************************************
                          kgpg.h  -  description
                             -------------------
    begin                : Mon Jul 21 2003
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

#ifndef KGPGDCOPIF_H
#define KGPGDCOPIF_H

#include <dcopobject.h>

class KeyInterface : virtual public DCOPObject
{
  K_DCOP
  k_dcop:
  virtual void showKeyInfo(QString keyID) =0;
  virtual bool importRemoteKey(QString keyID)=0;
  virtual void showOptions()=0;
  virtual void showKeyServer()=0;
  virtual void showKeyManager()=0;
};
  
#endif // KGPGDCOPIF_H

