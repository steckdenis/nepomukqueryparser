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
#include "utils.h"

#include <nepomuk2/literalterm.h>
#include <soprano/literalvalue.h>
#include <klocalizedstring.h>

#include <QtDebug>

PassNumbers::PassNumbers()
{
    registerNames(0, i18nc("Space-separated list of words meaning 0", "zero naught null"));
    registerNames(1, i18nc("Space-separated list of words meaning 1", "one a first"));
    registerNames(2, i18nc("Space-separated list of words meaning 2", "two second"));
    registerNames(3, i18nc("Space-separated list of words meaning 3", "three third"));
    registerNames(4, i18nc("Space-separated list of words meaning 4", "four fourth"));
    registerNames(5, i18nc("Space-separated list of words meaning 5", "five fifth"));
    registerNames(6, i18nc("Space-separated list of words meaning 6", "six sixth"));
    registerNames(7, i18nc("Space-separated list of words meaning 7", "seven seventh"));
    registerNames(8, i18nc("Space-separated list of words meaning 8", "eight eighth"));
    registerNames(9, i18nc("Space-separated list of words meaning 9", "nine nineth"));
    registerNames(10, i18nc("Space-separated list of words meaning 10", "ten tenth"));
}

void PassNumbers::registerNames(long long int number, const QString &names)
{
    Q_FOREACH(const QString &name, names.split(QLatin1Char(' '))) {
        number_names.insert(name, number);
    }
}

QList<Nepomuk2::Query::Term> PassNumbers::run(const QList<Nepomuk2::Query::Term> &match) const
{
    QList<Nepomuk2::Query::Term> rs;

    // Single integer number
    QString value = termStringValue(match.at(0)).toLower();

    if (value.isNull()) {
        return rs;
    }

    // Named integer
    if (number_names.contains(value)) {
        rs.append(Nepomuk2::Query::LiteralTerm(number_names.value(value)));
    } else {
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
    }

    return rs;
}
