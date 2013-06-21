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

#include "parser.h"
#include "patternmatcher.h"
#include "utils.h"

#include "pass_splitunits.h"
#include "pass_numbers.h"
#include "pass_filesize.h"
#include "pass_typehints.h"
#include "pass_properties.h"
#include "pass_dateperiods.h"
#include "pass_datevalues.h"
#include "pass_periodnames.h"
#include "pass_subqueries.h"
#include "pass_comparators.h"
#include "pass_tags.h"

#include <nepomuk2/literalterm.h>
#include <nepomuk2/property.h>
#include <nepomuk2/nfo.h>
#include <nepomuk2/nmo.h>
#include <nepomuk2/nie.h>
#include <soprano/literalvalue.h>
#include <soprano/nao.h>

#include <klocale.h>
#include <kcalendarsystem.h>
#include <klocalizedstring.h>

#include <QList>
#include <QRegExp>
#include <QtDebug>

struct Field {
    enum Flags {
        Unset = 0,
        Absolute,
        Relative
    };

    int value;
    Flags flags;

    void reset()
    {
        value = 0;
        flags = Unset;
    }
};

struct DateTimeSpec {
    Field fields[PassDatePeriods::MaxPeriod];

    void reset()
    {
        for (int i=0; i<int(PassDatePeriods::MaxPeriod); ++i) {
            fields[i].reset();
        }
    }
};

struct Parser::Private
{
    Private()
    : separators(i18nc(
        "Characters that are kept in the query for further processing but are considered word boundaries",
        ",;:!?()[]{}<>=#+-"))
    {}

    QStringList split(const QString &query, bool split_separators);

    template<typename T>
    bool runPass(const T &pass, const QString &pattern);
    bool foldDateTimes();
    void handleDateTimeComparison(DateTimeSpec &spec, const Nepomuk2::Query::ComparisonTerm &term);

    // Terms on which the parser works
    QList<Nepomuk2::Query::Term> terms;

    // Parsing passes (they cache translations, queries, etc)
    PassSplitUnits pass_splitunits;
    PassNumbers pass_numbers;
    PassFileSize pass_filesize;
    PassTypeHints pass_typehints;
    PassComparators pass_comparators;
    PassProperties pass_properties;
    PassDatePeriods pass_dateperiods;
    PassDateValues pass_datevalues;
    PassPeriodNames pass_periodnames;
    PassSubqueries pass_subqueries;
    PassTags pass_tags;

    // Locale-specific
    QString separators;
};

Parser::Parser()
: d(new Private)
{
}

Parser::Parser(const Parser &other)
: d(new Private(*other.d))
{
}

Parser::~Parser()
{
    delete d;
}

void Parser::reset()
{
    d->terms.clear();
}

Nepomuk2::Query::Query Parser::parse(const QString &query)
{
    reset();

    // Split the query into terms
    QStringList parts = d->split(query, true);

    Q_FOREACH(const QString &part, parts) {
        d->terms.append(Nepomuk2::Query::LiteralTerm(part));
    }

    // Parse passes
    bool progress = true;

    while (progress) {
        progress = false;

        progress |= d->runPass(d->pass_splitunits, "%1");
        progress |= d->runPass(d->pass_numbers, "%1");
        progress |= d->runPass(d->pass_filesize, "%1 %2");
        progress |= d->runPass(d->pass_typehints, "%1");
        progress |= d->runPass(d->pass_tags, i18nc(
            "A document is associated with a tag", "tagged as %1;has tag %1;tag is %1;# %1"));

        // Comparators
        d->pass_comparators.setComparator(Nepomuk2::Query::ComparisonTerm::Contains);
        progress |= d->runPass(d->pass_comparators,
            i18nc("Equality", "(contains|containing) %1"));
        d->pass_comparators.setComparator(Nepomuk2::Query::ComparisonTerm::Greater);
        progress |= d->runPass(d->pass_comparators,
            i18nc("Strictly greater", "(greater|bigger|more) than %1;at least %1;\\> %1"));
        d->pass_comparators.setComparator(Nepomuk2::Query::ComparisonTerm::Smaller);
        progress |= d->runPass(d->pass_comparators,
            i18nc("Strictly smaller", "(smaller|less|lesser) than %1;at most %1;\\< %1"));
        d->pass_comparators.setComparator(Nepomuk2::Query::ComparisonTerm::Equal);
        progress |= d->runPass(d->pass_comparators,
            i18nc("Equality", "(equal|equals|=) %1;equal to %1"));

        // Email-related properties
        d->pass_properties.setProperty(Nepomuk2::Vocabulary::NMO::messageFrom());
        progress |= d->runPass(d->pass_properties,
            i18nc("Sender of an e-mail", "sent by %1;from %1;sender is %1;sender %1"));
        d->pass_properties.setProperty(Nepomuk2::Vocabulary::NMO::messageSubject());
        progress |= d->runPass(d->pass_properties,
            i18nc("Title of an e-mail", "title %1"));
        d->pass_properties.setProperty(Nepomuk2::Vocabulary::NMO::messageRecipient());
        progress |= d->runPass(d->pass_properties,
            i18nc("Recipient of an e-mail", "sent to %1;to %1;recipient is %1;recipient %1"));

        // File-related properties
        d->pass_properties.setProperty(Nepomuk2::Vocabulary::NFO::fileSize());
        progress |= d->runPass(d->pass_properties,
            i18nc("Size of a file", "size is %1;size %1;being %1 large;%1 large"));
        d->pass_properties.setProperty(Nepomuk2::Vocabulary::NFO::fileName());
        progress |= d->runPass(d->pass_properties,
            i18nc("Name of a file", "name %1;named %1"));

        // Date-time periods
        progress |= d->runPass(d->pass_periodnames, "%1");

        d->pass_dateperiods.setKind(PassDatePeriods::VariablePeriod, PassDatePeriods::Offset);
        progress |= d->runPass(d->pass_dateperiods,
            i18nc("Adding an offset to a period of time (%1=period, %2=offset)", "in %2 %1"));

        d->pass_dateperiods.setKind(PassDatePeriods::VariablePeriod, PassDatePeriods::InvertedOffset);
        progress |= d->runPass(d->pass_dateperiods,
            i18nc("Removing an offset from a period of time (%1=period, %2=offset)", "%2 %1 ago"));

        d->pass_dateperiods.setKind(PassDatePeriods::VariablePeriod, PassDatePeriods::Offset, 1);
        progress |= d->runPass(d->pass_dateperiods,
            i18nc("Adding 1 to a period of time", "next %1"));

        d->pass_dateperiods.setKind(PassDatePeriods::VariablePeriod, PassDatePeriods::Offset, -1);
        progress |= d->runPass(d->pass_dateperiods,
            i18nc("Removing 1 to a period of time", "last %1"));

        d->pass_dateperiods.setKind(PassDatePeriods::Day, PassDatePeriods::Offset, 1);
        progress |= d->runPass(d->pass_dateperiods,
            i18nc("In one day", "tomorrow"));
        d->pass_dateperiods.setKind(PassDatePeriods::Day, PassDatePeriods::Offset, -1);
        progress |= d->runPass(d->pass_dateperiods,
            i18nc("One day ago", "yesterday"));
        d->pass_dateperiods.setKind(PassDatePeriods::Day, PassDatePeriods::Offset, 0);
        progress |= d->runPass(d->pass_dateperiods,
            i18nc("The current day", "today"));

        d->pass_dateperiods.setKind(PassDatePeriods::VariablePeriod, PassDatePeriods::Value, 1);
        progress |= d->runPass(d->pass_dateperiods,
            i18nc("First period (first day, month, etc)", "first %1"));
        d->pass_dateperiods.setKind(PassDatePeriods::VariablePeriod, PassDatePeriods::Value, -1);
        progress |= d->runPass(d->pass_dateperiods,
            i18nc("Last period (last day, month, etc)", "last %1"));
        d->pass_dateperiods.setKind(PassDatePeriods::VariablePeriod, PassDatePeriods::Value);
        progress |= d->runPass(d->pass_dateperiods,
            i18nc("Setting the value of a period, as in 'third week' (%1=period, %2=value)", "%2 %1"));

        // Setting values of date-time periods (14:30, June 6, etc)
        d->pass_datevalues.setPm(true);
        progress |= d->runPass(d->pass_datevalues,
            i18nc("An hour (%5) and an optional minute (%6), PM", "%5 : %6 pm;%5 h pm;%5 pm"));
        d->pass_datevalues.setPm(false);
        progress |= d->runPass(d->pass_datevalues,
            i18nc("An hour (%5) and an optional minute (%6), AM", "%5 : %6 am;%5 h am;%5 am"));

        progress |= d->runPass(d->pass_datevalues, i18nc(
            "A year (%1), month (%2), day (%3), day of week (%4), hour (%5), "
                "minute (%6), second (%7), in every combination supported by your language",
            "%3 of %2 %1;%3 (st|nd|rd|th) %2 %1;%3 (st|nd|rd|th) of %2 %1;"
            "%3 of %2;%3 (st|nd|rd|th) %2;%3 (st|nd|rd|th) of %2;%2 %3;%2 %1;"
            "%1 - %2 - %3;%1 - %2;%3 / %2 / %1;%3 / %2;"
            "in %1; in %2 %1;, %1;"
            "%5 : %6;%5 : %6 : %7;%5 h;"
        ));

        // Fold date-time properties into real DateTime values
        progress |= d->foldDateTimes();

        // Different kinds of properties that need subqueries
        d->pass_subqueries.setProperty(Nepomuk2::Vocabulary::NIE::relatedTo());
        progress |= d->runPass(d->pass_subqueries,
            i18nc("Related to a subquery", "related to ... ,"));
    }

    // Fuse the terms into a big AND term and produce the query
    int end_index;
    Nepomuk2::Query::Term final_term = fuseTerms(d->terms, 0, end_index);

    qDebug() << final_term;

    return Nepomuk2::Query::Query(final_term);
}

QStringList Parser::Private::split(const QString &query, bool split_separators)
{
    QStringList parts;
    QString part;
    int size = query.size();
    bool between_quotes = false;

    for (int i=0; i<size; ++i) {
        QChar c = query.at(i);

        if (!between_quotes && (c.isSpace() || (split_separators && separators.contains(c)))) {
            // A part may be empty if more than one space are found in block in the input
            if (part.size() > 0) {
                parts.append(part);
                part.clear();
            }

            // Add a separator, if any
            if (split_separators && separators.contains(c)) {
                parts.append(QString(c));
            }
        } else if (c == '"') {
            between_quotes = !between_quotes;
        } else {
            part.append(c);
        }
    }

    if (part.size() > 0) {
        parts.append(part);
    }

    return parts;
}

template<typename T>
bool Parser::Private::runPass(const T &pass, const QString &pattern)
{
    bool progress = false;

    // Split the pattern at ";" characters, as a locale can have more than one
    // pattern that can be used for a given rule
    QStringList rules = pattern.split(QLatin1Char(';'));

    Q_FOREACH(const QString &rule, rules) {
        // Split the rule into parts that have to be matched
        QStringList parts = split(rule, false);
        PatternMatcher matcher(terms, parts);

        matcher.runPass(pass);
    }

    return progress;
}

/*
 * Datetime-folding
 */
void Parser::Private::handleDateTimeComparison(DateTimeSpec &spec, const Nepomuk2::Query::ComparisonTerm &term)
{
    QUrl property_url = term.property().uri(); // URL like date://<property>/<offset|value>
    int value = term.subTerm().toLiteralTerm().value().toInt();

    // Populate the field corresponding to the property being compared to
    Field &field = spec.fields[pass_dateperiods.periodFromName(property_url.host())];

    field.value = value;
    field.flags =
        (property_url.path() == QLatin1String("/offset") ? Field::Relative : Field::Absolute);
}

static int valueForFlags(const Field &field, int if_unset, int if_absolute, int if_relative)
{
    switch (field.flags)
    {
        case Field::Unset:
            return if_unset;
        case Field::Absolute:
            return if_absolute;
        case Field::Relative:
            return if_relative;
    }

    return 0;
}

static int fieldIsSet(const Field &field, int if_yes, int if_no)
{
    return (field.flags != Field::Unset ? if_yes : if_no);
}

static int fieldIsRelative(const Field &field, int if_yes, int if_no)
{
    return (field.flags == Field::Relative ? if_yes : if_no);
}

static Nepomuk2::Query::LiteralTerm buildDateTimeLiteral(const DateTimeSpec &spec)
{
    KCalendarSystem *calendar = KCalendarSystem::create(KGlobal::locale()->calendarSystem());
    QDate cdate = QDate::currentDate();
    QTime ctime = QTime::currentTime();
    QDate date;

    const Field &year = spec.fields[PassDatePeriods::Year];
    const Field &month = spec.fields[PassDatePeriods::Month];
    const Field &week = spec.fields[PassDatePeriods::Week];
    const Field &day = spec.fields[PassDatePeriods::Day];
    const Field &dayofweek = spec.fields[PassDatePeriods::DayOfWeek];
    const Field &hour = spec.fields[PassDatePeriods::Hour];
    const Field &minute = spec.fields[PassDatePeriods::Minute];
    const Field &second = spec.fields[PassDatePeriods::Second];

    // Absolute year, month, day of month
    if (month.flags != Field::Unset)
    {
        // Month set, day of month
        calendar->setDate(
            date,
            valueForFlags(year,
                          calendar->year(cdate),
                          year.value,
                          calendar->year(cdate)),
            fieldIsRelative(month,
                            calendar->month(cdate),
                            month.value),
            valueForFlags(day,
                          1,
                          day.value,
                          calendar->day(cdate))
        );
    } else {
        calendar->setDate(
            date,
            valueForFlags(year,
                          calendar->year(cdate),
                          year.value,
                          calendar->year(cdate)),
            valueForFlags(day,
                          fieldIsSet(year, 1, calendar->dayOfYear(cdate)),
                          day.value,
                          calendar->dayOfYear(cdate))
        );
    }

    // Absolute week and day of week
    int isoyear;
    int isoweek = calendar->week(date, KLocale::IsoWeekNumber, &isoyear);

    calendar->setDateIsoWeek(
        date,
        isoyear,
        valueForFlags(week,
                      isoweek,
                      week.value,
                      isoweek),
        valueForFlags(dayofweek,
                      calendar->dayOfWeek(date),
                      dayofweek.value,
                      calendar->dayOfWeek(date)
        )
    );

    // Relative year, month, week, day of month
    if (year.flags == Field::Relative) {
        date = calendar->addYears(date, year.value);
    }
    if (month.flags == Field::Relative) {
        date = calendar->addMonths(date, month.value);
    }
    if (week.flags == Field::Relative) {
        date = calendar->addDays(date, week.value * 7);
    }
    if (day.flags == Field::Relative) {
        date = calendar->addDays(date, day.value);
    }

    // Absolute time
    QTime time = QTime(
        valueForFlags(hour,
                      ctime.hour(),
                      hour.value,
                      ctime.hour()),
        valueForFlags(minute,
                      fieldIsSet(hour,
                                 0,
                                 ctime.minute()),
                      minute.value,
                      ctime.minute()),
        valueForFlags(second,
                      fieldIsSet(minute,
                                 0,
                                 fieldIsSet(hour,
                                            0,
                                            ctime.second())),
                      second.value,
                      ctime.second())
    );

    // Relative time
    time.addSecs(
        fieldIsRelative(hour, hour.value * 60 * 60, 0) +
        fieldIsRelative(minute, minute.value * 60, 0) +
        fieldIsRelative(second, second.value, 0)
    );

    delete calendar;
    return Nepomuk2::Query::LiteralTerm(QDateTime(date, time));
}

bool Parser::Private::foldDateTimes()
{
    QList<Nepomuk2::Query::Term> new_terms;

    DateTimeSpec spec;
    bool progress = false;
    bool spec_contains_interesting_data = false;

    spec.reset();

    Q_FOREACH(const Nepomuk2::Query::Term &term, terms) {
        bool comparison_encountered = false;

        if (term.isComparisonTerm()) {
            Nepomuk2::Query::ComparisonTerm comparison = term.toComparisonTerm();

            if (comparison.property().uri().scheme() == QLatin1String("date")) {
                handleDateTimeComparison(spec, comparison);

                progress = true;
                spec_contains_interesting_data = true;
                comparison_encountered = true;
            }
        }

        if (!comparison_encountered) {
            if (spec_contains_interesting_data) {
                // End a date-time spec and emit its xsd:DateTime value
                new_terms.append(buildDateTimeLiteral(spec));

                spec.reset();
                spec_contains_interesting_data = false;
            }

            new_terms.append(term);     // Preserve non-datetime terms
        }
    }

    if (spec_contains_interesting_data) {
        // Query ending with a date-time, don't forget to build it
        new_terms.append(buildDateTimeLiteral(spec));
    }

    terms.swap(new_terms);
    return progress;
}