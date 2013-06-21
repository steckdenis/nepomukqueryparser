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

#include <nepomuk2/literalterm.h>
#include <nepomuk2/andterm.h>
#include <nepomuk2/orterm.h>
#include <nepomuk2/negationterm.h>
#include <soprano/literalvalue.h>

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

Nepomuk2::Query::Term fuseTerms(const QList<Nepomuk2::Query::Term> &terms, int first_term_index, int& end_term_index)
{
    Nepomuk2::Query::Term fused_term;
    bool build_and = true;
    bool build_not = false;

    // Fuse terms in nested AND and OR terms. "a AND b OR c" is fused as
    // "(a AND b) OR c"
    for (end_term_index=first_term_index; end_term_index<terms.size(); ++end_term_index) {
        Nepomuk2::Query::Term term = terms.at(end_term_index);

        if (term.isLiteralTerm()) {
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
