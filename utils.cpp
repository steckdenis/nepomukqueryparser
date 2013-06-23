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

#include "utils.h"
#include "pass_dateperiods.h"

#include <nepomuk2/literalterm.h>
#include <nepomuk2/andterm.h>
#include <nepomuk2/orterm.h>
#include <nepomuk2/negationterm.h>
#include <nepomuk2/comparisonterm.h>
#include <nepomuk2/property.h>
#include <soprano/literalvalue.h>

#include <klocale.h>
#include <kcalendarsystem.h>

QString termStringValue(const Nepomuk2::Query::Term &term)
{
    if (!term.isLiteralTerm()) {
        return QString();
    }

    Soprano::LiteralValue value = term.toLiteralTerm().value();

    if (value.isString()) {
        return value.toString();
    } else {
        return QString();
    }
}

bool termIntValue(const Nepomuk2::Query::Term& term, int &value)
{
    if (!term.isLiteralTerm()) {
        return false;
    }

    Soprano::LiteralValue v = term.toLiteralTerm().value();

    if (!v.isInt() && !v.isInt64()) {
        return false;
    }

    value = v.toInt();
    return true;
}

static Nepomuk2::Query::AndTerm intervalComparison(const Nepomuk2::Types::Property &prop,
                                                   const QDateTime &start_date_time)
{
    KCalendarSystem *cal = KCalendarSystem::create(KGlobal::locale()->calendarSystem());
    QDate start_date(start_date_time.date());
    QTime start_time(start_date_time.time());
    QDate end_date(start_date);
    QTime end_time(start_time);
    PassDatePeriods::Period last_defined_period = (PassDatePeriods::Period)(start_time.msec());

    switch (last_defined_period)
    {
        case PassDatePeriods::Year:
            end_date = cal->addYears(start_date, 1);
            break;
        case PassDatePeriods::Month:
            end_date = cal->addMonths(start_date, 1);
            break;
        case PassDatePeriods::Week:
            end_date = cal->addDays(start_date, cal->dayOfWeek(end_date));
            break;
        case PassDatePeriods::DayOfWeek:
        case PassDatePeriods::Day:
            end_date = cal->addDays(start_date, 1);
            break;

        case PassDatePeriods::Hour:
            end_time.addSecs(60 * 60);
            break;
        case PassDatePeriods::Minute:
            end_time.addSecs(60);
            break;
        case PassDatePeriods::Second:
            end_time.addSecs(1);
            break;
        default:
            break;
    }

    return Nepomuk2::Query::AndTerm(
        Nepomuk2::Query::ComparisonTerm(
            prop,
            Nepomuk2::Query::LiteralTerm(start_date_time),
            Nepomuk2::Query::ComparisonTerm::GreaterOrEqual
        ),
        Nepomuk2::Query::ComparisonTerm(
            prop,
            Nepomuk2::Query::LiteralTerm(QDateTime(end_date, end_time)),
            Nepomuk2::Query::ComparisonTerm::SmallerOrEqual
        )
    );
}

Nepomuk2::Query::Term fuseTerms(const QList<Nepomuk2::Query::Term> &terms, int first_term_index, int& end_term_index)
{
    Nepomuk2::Query::Term fused_term;
    bool build_and = true;
    bool build_not = false;

    // Fuse terms in nested AND and OR terms. "a AND b OR c" is fused as
    // "(a AND b) OR c"
    for (end_term_index=first_term_index; end_term_index<terms.size(); ++end_term_index) {
        Nepomuk2::Query::Term term = terms.at(end_term_index);

        if (term.isComparisonTerm()) {
            Nepomuk2::Query::ComparisonTerm &comparison = term.toComparisonTerm();

            if (comparison.comparator() == Nepomuk2::Query::ComparisonTerm::Equal &&
                comparison.subTerm().isLiteralTerm())
            {
                Soprano::LiteralValue value = comparison.subTerm().toLiteralTerm().value();

                if (value.isDateTime()) {
                    // We try to compare exactly with a date-time, which is impossible
                    // (except if you want to find documents edited precisely at
                    // the millisecond you want)
                    // Build a comparison against an interval
                    term = intervalComparison(comparison.property(), value.toDateTime());
                }
            }
        } else if (term.isLiteralTerm()) {
            Soprano::LiteralValue value = term.toLiteralTerm().value();

            if (value.isString()) {
                QString content = value.toString().toLower();

                if (content == QLatin1String("or")) {
                    // Consume the OR term, the next term will be ORed with the previous
                    build_and = false;
                    continue;
                } else if (content == QLatin1String("and") ||
                           content == QLatin1String("+")) {
                    // Consume the AND term
                    build_and = true;
                    continue;
                } else if (content == QLatin1String("!") ||
                           content == QLatin1String("not") ||
                           content == QLatin1String("-")) {
                    // Consume the negation
                    build_not = true;
                    continue;
                } else if (content == QLatin1String("(")) {
                    // Fuse the nested query
                    term = fuseTerms(terms, end_term_index + 1, end_term_index);
                } else if (content == QLatin1String(")")) {
                    // Done
                    return fused_term;
                } else if (content.size() <= 2) {
                    // Ignore small terms, they are typically "to", "a", etc.
                    // NOTE: Some locales may want to have this filter removed
                    continue;
                }
            }
        }

        // Negate the term if needed
        if (build_not) {
            term = Nepomuk2::Query::NegationTerm::negateTerm(term);
        }

        // Add term to the fused term
        if (!fused_term.isValid()) {
            fused_term = term;
        } else if (build_and) {
            if (fused_term.isAndTerm()) {
                fused_term.toAndTerm().addSubTerm(term);
            } else {
                fused_term = Nepomuk2::Query::AndTerm(fused_term, term);
            }
        } else {
            if (fused_term.isOrTerm()) {
                fused_term.toOrTerm().addSubTerm(term);
            } else {
                fused_term = Nepomuk2::Query::OrTerm(fused_term, term);
            }
        }

        // Default to AND, and don't invert terms
        build_and = true;
        build_not = false;
    }

    return fused_term;
}
