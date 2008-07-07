/***************************************************************************
 *   Copyright (C) 2006 by Jimmy Gilles                                    *
 *   jimmygilles@gmail.com                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA          *
 ***************************************************************************/

#include "emailvalidator.h"

#include <QRegExp>

namespace KgpgCore
{

QValidator::State EmailValidator::validate(QString &input, int &) const
{
    static QString pattern = "^[a-z][a-z|0-9|]*([_][a-z|0-9]+)*([.][a-z|"
                             "0-9]+([_][a-z|0-9]+)*)?@[a-z][a-z|0-9|-]*\\.([a-z]"
                             "[a-z|0-9|-]*(\\.[a-z][a-z|0-9|-]*)?)$";

    static QRegExp regexp(pattern, Qt::CaseInsensitive);

    if (regexp.exactMatch(input))
        return QValidator::Acceptable;
    else
        return QValidator::Invalid;
}

} // namespace KgpgCore
