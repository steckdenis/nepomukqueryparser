/* This file is part of the Nepomuk query parser
   Copyright (c) 2013 Denis Steckelmacher <steckdenis@yahoo.fr>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2.1 as published by the Free Software Foundation,
   or any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __PASS_NUMBERS_H__
#define __PASS_NUMBERS_H__

#include <QList>
#include <QHash>

namespace Nepomuk2 { namespace Query { class Term; }}

class PassNumbers
{
    public:
        PassNumbers();

        QList<Nepomuk2::Query::Term> run(const QList<Nepomuk2::Query::Term> &match) const;

    private:
        void registerNames(long long int number, const QString &names);

    private:
        QHash<QString, long long int> number_names;
};

#endif