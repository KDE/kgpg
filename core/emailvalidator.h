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

#ifndef EMAILVALIDATOR_H
#define EMAILVALIDATOR_H

#include <QValidator>

namespace KgpgCore
{

class EmailValidator : public QValidator
{
public:
    explicit EmailValidator(QObject *parent = nullptr) : QValidator(parent) { }
    QValidator::State validate(QString &input, int &pos) const override;
};

} // namespace KgpgCore

#endif // EMAILVALIDATOR_H
