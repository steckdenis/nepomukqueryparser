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
#include <klocalizedstring.h>

#include <QList>
#include <QRegExp>
#include <QtDebug>

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

        d->pass_dateperiods.setKind(PassDatePeriods::VariablePeriod, PassDatePeriods::Value, 1);
        progress |= d->runPass(d->pass_dateperiods,
            i18nc("First period (first day, month, etc)", "first %1"));
        d->pass_dateperiods.setKind(PassDatePeriods::VariablePeriod, PassDatePeriods::Value, -1);
        progress |= d->runPass(d->pass_dateperiods,
            i18nc("Last period (last day, month, etc)", "last %1"));
        d->pass_dateperiods.setKind(PassDatePeriods::VariablePeriod, PassDatePeriods::Value);
        progress |= d->runPass(d->pass_dateperiods,
            i18nc("Setting the value of a period, as in 'third week' (%1=period, %2=value)", "%2 %1"));

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
