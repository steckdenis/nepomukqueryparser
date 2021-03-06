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

#include "pass_splitunits.h"
#include "utils.h"

#include <nepomuk2/literalterm.h>
#include <soprano/literalvalue.h>
#include <klocalizedstring.h>

#include <QtDebug>

PassSplitUnits::PassSplitUnits()
: known_units(
    QSet<QString>::fromList(
        i18nc(
            "List of lowercase prefixes or suffix that need to be split from values",
            "k m g b kb mb gb tb kib mib gib tib h am pm th rd nd st"
        ).split(QLatin1Char(' '))
    )
  )
{
}

QList<Nepomuk2::Query::Term> PassSplitUnits::run(const QList<Nepomuk2::Query::Term> &match) const
{
    QList<Nepomuk2::Query::Term> rs;
    Nepomuk2::Query::LiteralTerm value_term;
    Nepomuk2::Query::LiteralTerm unit_term;

    QString value = termStringValue(match.at(0));
    int value_position = match.at(0).position();

    if (value.isNull()) {
        return rs;
    }

    // Possible prefix
    QString prefix;

    for (int i=0; i<value.size() && value.at(i).isLetter(); ++i) {
        prefix.append(value.at(i).toLower());
    }

    if (prefix.size() < value.size() && known_units.contains(prefix)) {
        unit_term.setValue(prefix);
        unit_term.setPosition(value_position, prefix.size());

        value = value.mid(prefix.size());
        value_position += prefix.size();
    }

    // Possible postfix
    QString postfix;

    for (int i=value.size()-1; i>=0 && value.at(i).isLetter(); --i) {
        postfix.prepend(value.at(i).toLower());
    }

    if (postfix.size() < value.size() && known_units.contains(postfix)) {
        value.resize(value.size() - postfix.size());

        unit_term.setValue(postfix);
        unit_term.setPosition(value_position + value.size(), postfix.size());
    }

    // Value
    value_term.setValue(value);
    value_term.setPosition(value_position, value.size());

    if (unit_term.isValid()) {
        rs.append(value_term);
        rs.append(unit_term);
    }

    return rs;
}
