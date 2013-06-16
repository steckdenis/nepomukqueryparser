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

#include "pass_splitunits.h"
#include "pass_numbers.h"
#include "pass_filesize.h"
#include "pass_typehints.h"
#include "pass_sentby.h"

#include <nepomuk2/literalterm.h>
#include <soprano/literalvalue.h>
#include <klocalizedstring.h>

#include <QVector>
#include <QRegExp>
#include <QtDebug>

struct Parser::Private
{
    QStringList split(const QString &query, bool split_separators);

    template<typename T>
    bool runPass(const T &pass, const QString &pattern, int match_count);
    bool match(const Nepomuk2::Query::Term &term, const QString &pattern, int &index);

    // Terms on which the parser works
    QVector<Nepomuk2::Query::Term> terms;

    // Parsing passes (they cache translations, queries, etc)
    PassSplitUnits pass_splitunits;
    PassNumbers pass_numbers;
    PassFileSize pass_filesize;
    PassTypeHints pass_typehints;
    PassSentBy pass_sentby;
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

void Parser::parse(const QString &query)
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

        progress |= d->runPass(d->pass_splitunits,
            "<string0>", 1);
        progress |= d->runPass(d->pass_numbers,
            "<string0>", 1);
        progress |= d->runPass(d->pass_filesize,
            "<integer0> <string1>", 2);
        progress |= d->runPass(d->pass_typehints,
            "<string0>", 1);
        progress |= d->runPass(d->pass_sentby,
            i18nc("Sender of an email", "sent by <string0>;from <string0>"), 1);
    }

    // Print the terms
    Q_FOREACH(const Nepomuk2::Query::Term &term, d->terms) {
        qDebug() << term.toString();
    }
}

QStringList Parser::Private::split(const QString &query, bool split_separators)
{
    QString separators;
    QStringList parts;
    QString part;
    int size = query.size();
    bool between_quotes = false;

    if (split_separators) {
        separators = i18nc("Characters that are kept in the query for further processing but are considered word boundaries", ".,;:!?()[]{}<>=");
    }

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
bool Parser::Private::runPass(const T &pass, const QString &pattern, int match_count)
{
    QVector<Nepomuk2::Query::Term> matched_terms(match_count);
    bool progress = false;

    // Split the pattern at ";" characters, as a locale can have more than one
    // pattern that can be used for a given rule
    QStringList rules = pattern.split(QLatin1Char(';'));

    Q_FOREACH(const QString &rule, rules) {
        // Split the rule into parts that have to be matched
        QStringList parts = split(rule, false);
        int part_index = 0;
        int first_match_index = -1;

        // Try to match the rule into the list of terms
        for (int i=0; i<terms.size(); ++i) {
            const Nepomuk2::Query::Term &term = terms.at(i);
            int index;

            if (match(term, parts.at(part_index), index)) {
                matched_terms[index] = term;

                if (first_match_index == -1) {
                    first_match_index = i;
                }

                if (++part_index == parts.size()) {
                    // Match complete, run the pass on it
                    QVector<Nepomuk2::Query::Term> replacement = pass.run(matched_terms);

                    if (replacement.count() > 0) {
                        // Replace terms first_match_index..i with replacement
                        terms.remove(first_match_index, 1 + i - first_match_index);

                        for (int j=replacement.count()-1; j>=0; --j) {
                            terms.insert(first_match_index, replacement.at(j));
                        }

                        // Re-explore the terms vector as indexes have changed
                        progress = true;
                        i = -1;
                    }

                    // Reinitialize the matching machinery
                    first_match_index = -1;
                    part_index = 0;
                }
            } else {
                // The begin of the pattern matched but not the end
                first_match_index = -1;
                part_index = 0;
            }
        }
    }

    return progress;
}

bool Parser::Private::match(const Nepomuk2::Query::Term &term, const QString &pattern, int &index)
{
    if (pattern.size() < 2) {
        return false;
    }

    // Match immediate values
    if (pattern.at(0) != QLatin1Char('<') ||
        pattern.at(pattern.size() - 1) != QLatin1Char('>') ||
        !pattern.at(pattern.size() - 2).isDigit())
    {
        if (!term.isLiteralTerm()) {
            return false;
        }

        QString value = term.toLiteralTerm().value().toString();
        QRegExp regexp(pattern, Qt::CaseInsensitive, QRegExp::RegExp2);

        return regexp.exactMatch(value);
    }

    // Parse the index and the name of a pattern like "<name0>"
    QString name = pattern.mid(1, pattern.size() - 3);
    index = pattern.at(pattern.size() - 2).digitValue();

    // Match type hints
    if (name == "type") {
        return term.isResourceTerm();
    }

    // Match literal types
    if (term.isLiteralTerm())
    {
        Soprano::LiteralValue value = term.toLiteralTerm().value();

        if (name == "literal") {
            return true;
        } else if (name == "integer") {
            return value.isInt() || value.isInt64();
        } else if (name == "double") {
            return value.isDouble();
        } else if (name == "string") {
            return value.isString();
        }
    }

    return false;
}

