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

#include "pass_filesize.h"
#include "utils.h"

#include <nepomuk2/literalterm.h>
#include <nepomuk2/comparisonterm.h>
#include <nepomuk2/property.h>
#include <soprano/literalvalue.h>
#include <klocalizedstring.h>

PassFileSize::PassFileSize()
{
    // File size units
    registerUnits(1000LL, i18nc("Lower-case units corresponding to a kilobyte", "kb kilobyte kilobytes"));
    registerUnits(1000000LL, i18nc("Lower-case units corresponding to a megabyte", "mb megabyte megabytes"));
    registerUnits(1000000000LL, i18nc("Lower-case units corresponding to a gigabyte", "gb gigabyte gigabytes"));
    registerUnits(1000000000000LL, i18nc("Lower-case units corresponding to a terabyte", "tb terabyte terabytes"));

    registerUnits(1LL << 10, i18nc("Lower-case units corresponding to a kibibyte", "kib k kibibyte kibibytes"));
    registerUnits(1LL << 20, i18nc("Lower-case units corresponding to a mebibyte", "mib m mebibyte mebibytes"));
    registerUnits(1LL << 30, i18nc("Lower-case units corresponding to a gibibyte", "gib g gibibyte gibibytes"));
    registerUnits(1LL << 40, i18nc("Lower-case units corresponding to a tebibyte", "tib t tebibyte tebibytes"));
}

void PassFileSize::registerUnits(long long int multiplier, const QString &units)
{
    Q_FOREACH(const QString &unit, units.split(QLatin1Char(' '))) {
        multipliers.insert(unit, multiplier);
    }
}

QList<Nepomuk2::Query::Term> PassFileSize::run(const QList<Nepomuk2::Query::Term> &match) const
{
    QList<Nepomuk2::Query::Term> rs;

    if (!match.at(0).isLiteralTerm() || !match.at(1).isLiteralTerm()) {
        return rs;
    }

    // Unit
    QString unit = match.at(1).toLiteralTerm().value().toString().toLower();

    if (multipliers.contains(unit)) {
        long long int multiplier = multipliers.value(unit);
        Nepomuk2::Query::LiteralTerm term = match.at(0).toLiteralTerm();

        if (term.value().isDouble()) {
            term.setValue(term.value().toDouble() * double(multiplier));
        } else {
            term.setValue(term.value().toInt64() * multiplier);
        }

        rs.append(term);
    }

    return rs;
}
