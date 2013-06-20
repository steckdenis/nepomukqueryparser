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

#include "pass_tags.h"
#include "utils.h"

#include <nepomuk2/resourcemanager.h>
#include <nepomuk2/comparisonterm.h>
#include <nepomuk2/resourceterm.h>
#include <nepomuk2/property.h>
#include <soprano/nao.h>
#include <soprano/rdfs.h>
#include <soprano/node.h>
#include <soprano/model.h>
#include <soprano/queryresultiterator.h>

#include <QtDebug>

PassTags::PassTags()
: cache_filled(false)
{
}

const QHash<QString, QUrl> &PassTags::tags() const
{
    if (!cache_filled) {
        const_cast<PassTags *>(this)->fillCache();
    }

    return cached_tags;
}

void PassTags::fillCache()
{
    cache_filled = true;

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

QList<Nepomuk2::Query::Term> PassTags::run(const QList<Nepomuk2::Query::Term> &match) const
{
    QList<Nepomuk2::Query::Term> rs;
    QString tagname = termStringValue(match.at(0));

    if (!tagname.isEmpty() && tags().contains(tagname)) {
        // As tags are cached, a ResourceTerm can be used instead of a string literal
        // that must be resolved by the database.
        rs.append(Nepomuk2::Query::ComparisonTerm(
            Soprano::Vocabulary::NAO::hasTag(),
            Nepomuk2::Query::ResourceTerm(tags().value(tagname)),
            Nepomuk2::Query::ComparisonTerm::Equal
        ));
    }

    return rs;
}