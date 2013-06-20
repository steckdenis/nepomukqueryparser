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

#include "pass_hourminute.h"
#include "pass_dateperiods.h"
#include "utils.h"

#include <nepomuk2/literalterm.h>
#include <nepomuk2/comparisonterm.h>
#include <nepomuk2/property.h>
#include <soprano/literalvalue.h>

PassHourMinute::PassHourMinute()
: pm(false)
{
}

void PassHourMinute::setPm(bool pm)
{
    this->pm = pm;
}

QList<Nepomuk2::Query::Term> PassHourMinute::run(const QList<Nepomuk2::Query::Term> &match) const
{
    QList<Nepomuk2::Query::Term> rs;
    Nepomuk2::Query::ComparisonTerm minute_comparison;

    // If two terms were matched, the second one represents a minute information
    if (match.count() == 2) {
        int minute;

        if (!termIntValue(match.at(1), minute)) {
            return rs;
        }

        minute_comparison = Nepomuk2::Query::ComparisonTerm(
            PassDatePeriods::propertyUrl(PassDatePeriods::Minute, 0),
            Nepomuk2::Query::LiteralTerm(minute),
            Nepomuk2::Query::ComparisonTerm::Equal
        );
    }

    // Add the hour information
    int hour;

    if (!termIntValue(match.at(0), hour)) {
        return rs;
    }

    rs.append(Nepomuk2::Query::ComparisonTerm(
        PassDatePeriods::propertyUrl(PassDatePeriods::Hour, 0),
        Nepomuk2::Query::LiteralTerm(pm ? hour + 12 : hour),
        Nepomuk2::Query::ComparisonTerm::Equal
    ));

    if (match.count() == 2) {
        rs.append(minute_comparison);
    }

    return rs;
}
