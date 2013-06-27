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

#include "pass_periodnames.h"
#include "pass_dateperiods.h"
#include "utils.h"

#include <nepomuk2/comparisonterm.h>
#include <nepomuk2/literalterm.h>
#include <nepomuk2/property.h>
#include <klocalizedstring.h>

PassPeriodNames::PassPeriodNames()
{
    registerNames(day_names, i18nc(
        "Day names, starting at the first day of the week (Monday for the Gregorian Calendar)",
        "monday tuesday wednesday thursday friday saturday sunday"
    ));
    registerNames(month_names, i18nc(
        "Month names, starting at the first of the year",
        "january february march april may june july augustus september october november september"
    ));
}

void PassPeriodNames::registerNames(QHash<QString, int> &table, const QString &names)
{
    QStringList list = names.split(QLatin1Char(' '));

    for (int i=0; i<list.count(); ++i) {
        table.insert(list.at(i), i + 1);    // Count from 1 as calendars do this
    }
}

QList<Nepomuk2::Query::Term> PassPeriodNames::run(const QList<Nepomuk2::Query::Term> &match) const
{
    QList<Nepomuk2::Query::Term> rs;
    QString name = termStringValue(match.at(0)).toLower();

    Nepomuk2::Query::LiteralTerm value_term;
    PassDatePeriods::Period period;

    if (day_names.contains(name)) {
        period = PassDatePeriods::DayOfWeek;
        value_term.setValue(day_names.value(name));
    } else if (month_names.contains(name)) {
        period = PassDatePeriods::Month;
        value_term.setValue(month_names.value(name));
    }

    if (value_term.isValid()) {
        value_term.setPosition(match.at(0));

        rs.append(Nepomuk2::Query::ComparisonTerm(
            PassDatePeriods::propertyUrl(period, false),
            value_term,
            Nepomuk2::Query::ComparisonTerm::Equal
        ));
    }

    return rs;
}
