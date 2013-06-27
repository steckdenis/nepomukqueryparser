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

#include "pass_subqueries.h"
#include "utils.h"

#include <nepomuk2/literalterm.h>
#include <nepomuk2/comparisonterm.h>
#include <nepomuk2/property.h>

void PassSubqueries::setProperty(const QUrl &property)
{
    this->property = property;
}

QList<Nepomuk2::Query::Term> PassSubqueries::run(const QList<Nepomuk2::Query::Term> &match) const
{
    QList<Nepomuk2::Query::Term> rs;

    // Fuse the matched terms (... in "related to ... ,") into a subquery
    int end_index;

    rs.append(Nepomuk2::Query::ComparisonTerm(
        property,
        fuseTerms(match, 0, end_index),
        Nepomuk2::Query::ComparisonTerm::Equal
    ));

    return rs;
}
