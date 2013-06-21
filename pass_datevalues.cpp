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

#include "pass_datevalues.h"
#include "pass_dateperiods.h"
#include "utils.h"

#include <nepomuk2/literalterm.h>
#include <nepomuk2/comparisonterm.h>
#include <nepomuk2/property.h>
#include <soprano/literalvalue.h>

PassDateValues::PassDateValues()
: pm(false)
{
}

void PassDateValues::setPm(bool pm)
{
    this->pm = pm;
}

QList<Nepomuk2::Query::Term> PassDateValues::run(const QList<Nepomuk2::Query::Term> &match) const
{
    QList<Nepomuk2::Query::Term> rs;
    bool valid_input = true;
    bool progress = false;

    static const PassDatePeriods::Period periods[7] = {
        PassDatePeriods::Year, PassDatePeriods::Month, PassDatePeriods::Day,
        PassDatePeriods::DayOfWeek, PassDatePeriods::Hour, PassDatePeriods::Minute,
        PassDatePeriods::Second
    };
    // Conservative minimum values (not every calendar already reached year 2000+)
    static const int min_values[7] = {
        0 /* Y */, 1 /* M */, 1 /* D */, 1 /* DW */, 0 /* H */, 0 /* M */, 0 /* S */
    };
    // Conservative maximum values (some calendars may have months of 100+ days)
    static const int max_values[7] = {
        1<<30 /* Y */, 60 /* M */, 500 /* D */, 7 /* DW */, 24 /* H */, 60 /* M */, 60 /* S */
    };

    // See if a match sets a value for any period
    for (int i=0; i<7; ++i) {
        PassDatePeriods::Period period = periods[i];

        if (i < match.count() && match.at(i).isValid()) {
            const Nepomuk2::Query::Term &term = match.at(i);
            int value;

            if (!termIntValue(match.at(i), value)) {
                // The term is not a literal integer, but may be a typed comparison
                if (!term.isComparisonTerm()) {
                    valid_input = false;
                    break;
                }

                Nepomuk2::Query::ComparisonTerm comparison = term.toComparisonTerm();

                if (comparison.property().uri() != PassDatePeriods::propertyUrl(period, false)) {
                    valid_input = false;
                    break;
                }

                // Keep the comparison, it is already good. No need to extract
                // its value only to build a new comparison exactly the same.
                rs.append(comparison);
                continue;
            }

            if (value < min_values[i] || value > max_values[i]) {
                valid_input = false;
                break;
            }

            if (period == PassDatePeriods::Hour && pm) {
                value += 12;
            }

            // Build a comparison of the right type
            progress = true;

            rs.append(Nepomuk2::Query::ComparisonTerm(
                PassDatePeriods::propertyUrl(period, false),
                Nepomuk2::Query::LiteralTerm(value),
                Nepomuk2::Query::ComparisonTerm::Equal
            ));
        }
    }

    if (!valid_input || !progress) {
        rs.clear();
    }

    return rs;
}
