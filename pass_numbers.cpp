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

#include "pass_numbers.h"

#include <nepomuk2/literalterm.h>
#include <soprano/literalvalue.h>

#include <QtDebug>

PassNumbers::PassNumbers()
{
}

QVector<Nepomuk2::Query::Term> PassNumbers::run(const QVector<Nepomuk2::Query::Term> &match) const
{
    QString value = match.at(0).toLiteralTerm().value().toString();
    QVector<Nepomuk2::Query::Term> rs;

    // Integer or double
    bool is_integer = false;
    bool is_double = false;
    long long int as_integer = value.toLongLong(&is_integer);
    double as_double = value.toDouble(&is_double);

    // Prefer integers over doubles
    if (is_integer) {
        rs.append(Nepomuk2::Query::LiteralTerm(as_integer));
    } else if (is_double) {
        rs.append(Nepomuk2::Query::LiteralTerm(as_double));
    }

    return rs;
}
