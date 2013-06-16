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

#include "pass_comparators.h"

#include <nepomuk2/property.h>

PassComparators::PassComparators()
: comparator(Nepomuk2::Query::ComparisonTerm::Equal)
{
}

void PassComparators::setComparator(Nepomuk2::Query::ComparisonTerm::Comparator comparator)
{
    this->comparator = comparator;
}

QVector<Nepomuk2::Query::Term> PassComparators::run(const QVector<Nepomuk2::Query::Term> &match) const
{
    QVector<Nepomuk2::Query::Term> rs;
    Nepomuk2::Query::ComparisonTerm term;

    if (match.at(0).isComparisonTerm()) {
        term = match.at(0).toComparisonTerm();
    } else if (match.at(0).isLiteralTerm()) {
        // Comparison with a literal term. The property is not given, as it will
        // be parsed in a later pass ("age > 5" first matches "> 5" then "age <comparison>")
        term = Nepomuk2::Query::ComparisonTerm(
            Nepomuk2::Types::Property(),
            match.at(0),
            this->comparator
        );
    } else {
        return rs;
    }

    // Set the comparison operator of the term
    term.setComparator(comparator);

    // Use this updated term in place of the old one
    rs.append(term);

    return rs;
}
