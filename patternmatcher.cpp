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

#include "patternmatcher.h"

#include <nepomuk2/literalterm.h>
#include <QRegExp>

PatternMatcher::PatternMatcher(QList<Nepomuk2::Query::Term> &terms, QStringList pattern)
: terms(terms),
  pattern(pattern),
  capture_count(captureCount())
{
}

int PatternMatcher::captureCount() const
{
    int max_capture = 0;
    int capture;

    Q_FOREACH(const QString &p, pattern) {
        if (p.at(0) == QLatin1Char('%')) {
            capture = p.mid(1).toInt();

            if (capture > max_capture) {
                max_capture = capture;
            }
        }
    }

    return max_capture;
}

int PatternMatcher::matchPattern(QList<Nepomuk2::Query::Term> &matched_terms, int index) const
{
    int pattern_index = 0;
    int term_index = index;
    bool match_anything = false;        // Match "..."
    bool contains_catchall = false;

    while (pattern_index < pattern.count() && term_index < terms.count()) {
        const Nepomuk2::Query::Term &term = terms.at(term_index);
        int capture_index = -1;

        if (pattern.at(pattern_index) == QLatin1String("...")) {
            // Start to match anything
            match_anything = true;
            contains_catchall = true;
            ++pattern_index;

            continue;
        }

        bool match = matchTerm(term, pattern.at(pattern_index), capture_index);

        if (match_anything) {
            if (!match) {
                // The stop pattern is not yet matched, continue to match
                // anything
                matched_terms.append(term);
            } else {
                // The terminating pattern is matched, stop to match anything
                match_anything = false;
                ++pattern_index;
            }
        } else if (match) {
            if (capture_index != -1) {
                matched_terms[capture_index] = term;
            }

            // Try to match the next pattern
            ++pattern_index;
        } else {
            // The pattern does not match, abort
            return 0;
        }

        // Match the next term
        ++term_index;
    }

    if (!contains_catchall && pattern_index != pattern.count()) {
        // The pattern was not fully matched
        // Patterns containing "..." typically end with an optional terminating
        // term. Allow them to match even if we reach the end of the term list
        // without encountering the terminating term.
        return 0;
    } else {
        return (term_index - index);
    }
}

bool PatternMatcher::matchTerm(const Nepomuk2::Query::Term& term, const QString& pattern, int& capture_index) const
{
    if (pattern.at(0) == QLatin1Char('%')) {
        // Placeholder
        capture_index = pattern.mid(1).toInt() - 1;

        return true;
    } else {
        // Literal value that has to be matched against a regular expression
        if (!term.isLiteralTerm()) {
            return false;
        }

        QString value = term.toLiteralTerm().value().toString();
        QRegExp regexp(pattern, Qt::CaseInsensitive, QRegExp::RegExp2);

        return regexp.exactMatch(value);
    }
}