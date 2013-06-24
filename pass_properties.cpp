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

#include "pass_properties.h"
#include "utils.h"

#include <nepomuk2/literalterm.h>
#include <nepomuk2/comparisonterm.h>
#include <nepomuk2/property.h>
#include <nepomuk2/resourcemanager.h>
#include <nepomuk2/resourceterm.h>
#include <soprano/nao.h>
#include <soprano/rdfs.h>
#include <soprano/model.h>
#include <soprano/queryresultiterator.h>

PassProperties::PassProperties()
: cached_tags_filled(false)
{
}

void PassProperties::setProperty(const QUrl &property, Types range)
{
    this->property = property;
    this->range = range;
}

const QHash<QString, QUrl> &PassProperties::tags() const
{
    if (!cached_tags_filled) {
        const_cast<PassProperties *>(this)->fillTagsCache();
    }

    return cached_tags;
}

void PassProperties::fillTagsCache()
{
    cached_tags_filled = true;

    // Get the tags URIs and their label in one SPARQL query
    QString query = QString::fromLatin1("select ?tag ?label where { "
                                        "?tag a %1 . "
                                        "?tag %2 ?label . "
                                        "}")
                    .arg(Soprano::Node::resourceToN3(Soprano::Vocabulary::NAO::Tag()),
                         Soprano::Node::resourceToN3(Soprano::Vocabulary::RDFS::label()));

    Soprano::QueryResultIterator it =
        Nepomuk2::ResourceManager::instance()->mainModel()->executeQuery(query, Soprano::Query::QueryLanguageSparql);

    while(it.next()) {
        cached_tags.insert(
            it["label"].toString(),
            QUrl(it["tag"].toString())
        );
    }
}

Nepomuk2::Query::Term PassProperties::convertToRange(const Nepomuk2::Query::LiteralTerm& term) const
{
    Soprano::LiteralValue value = term.value();

    switch (range)
    {
        case Integer:
            if (value.isInt() || value.isInt64()) {
                return term;
            }
            break;

        case IntegerOrDouble:
            if (value.isInt() || value.isInt64() || value.isDouble()) {
                return term;
            }
            break;

        case String:
            if (value.isString()) {
                return term;
            }
            break;

        case DateTime:
            if (value.isDateTime()) {
                return term;
            }
            break;

        case Tag:
            if (value.isString() && tags().contains(value.toString())) {
                return Nepomuk2::Query::ResourceTerm(cached_tags.value(value.toString()));
            }
            break;
    }

    return Nepomuk2::Query::Term();
}

QList<Nepomuk2::Query::Term> PassProperties::run(const QList<Nepomuk2::Query::Term> &match) const
{
    QList<Nepomuk2::Query::Term> rs;
    Nepomuk2::Query::Term term = match.at(0);
    Nepomuk2::Query::Term subterm;
    Nepomuk2::Query::ComparisonTerm::Comparator comparator;

    if (term.isComparisonTerm()) {
        Nepomuk2::Query::ComparisonTerm &comparison = term.toComparisonTerm();

        if (comparison.subTerm().isLiteralTerm()) {
            subterm = convertToRange(comparison.subTerm().toLiteralTerm());
            comparator = comparison.comparator();
        }
    } else if (term.isLiteralTerm()) {
        // Property followed by a value, the comparator is "contains" for strings
        // and the equality for everything else
        subterm = convertToRange(term.toLiteralTerm());
        comparator = (
            (subterm.isLiteralTerm() && subterm.toLiteralTerm().value().isString()) ?
            Nepomuk2::Query::ComparisonTerm::Contains :
            Nepomuk2::Query::ComparisonTerm::Equal
        );
    }

    if (subterm.isValid()) {
        rs.append(Nepomuk2::Query::ComparisonTerm(
            property,
            subterm,
            comparator
        ));
    }

    return rs;
}
