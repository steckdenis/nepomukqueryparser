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

#ifndef __PASS_PROPERTIES_H__
#define __PASS_PROPERTIES_H__

#include <QList>
#include <QUrl>

#include <nepomuk2/literalterm.h>

class PassProperties
{
    public:
        enum Types {
            Integer,
            IntegerOrDouble,
            String,
            DateTime,
            Tag,
        };

        PassProperties();

        void setProperty(const QUrl &property, Types range);

        const QHash<QString, QUrl> &tags() const;
        QList<Nepomuk2::Query::Term> run(const QList<Nepomuk2::Query::Term> &match) const;

    private:
        Nepomuk2::Query::Term convertToRange(const Nepomuk2::Query::LiteralTerm &term) const;
        void fillTagsCache();

    private:
        QUrl property;
        Types range;

        // Cache for tags
        QHash<QString, QUrl> cached_tags;
        bool cached_tags_filled;
};

#endif