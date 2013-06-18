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

#include "pass_typehints.h"
#include "utils.h"

#include <nepomuk2/literalterm.h>
#include <nepomuk2/resourcetypeterm.h>
#include <soprano/literalvalue.h>
#include <klocalizedstring.h>

#include <nepomuk2/nfo.h>
#include <nepomuk2/nmo.h>

PassTypeHints::PassTypeHints()
{
    registerHints(Nepomuk2::Vocabulary::NFO::FileDataObject(),
        i18nc("List of words representing a file", "file files"));
    registerHints(Nepomuk2::Vocabulary::NFO::Image(),
        i18nc("List of words representing an image", "image images picture pictures photo photos"));
    registerHints(Nepomuk2::Vocabulary::NFO::Video(),
        i18nc("List of words representing a video", "video videos movie movies film films"));
    registerHints(Nepomuk2::Vocabulary::NFO::Audio(),
        i18nc("List of words representing an audio file", "music musics"));
    registerHints(Nepomuk2::Vocabulary::NFO::Document(),
        i18nc("List of words representing a document", "document documents"));
    registerHints(Nepomuk2::Vocabulary::NMO::Email(),
        i18nc("List of words representing an email", "mail mails email emails e-mail e-mails message messages"));
}

void PassTypeHints::registerHints(const QUrl &type, const QString &hints)
{
    Q_FOREACH(const QString &hint, hints.split(QLatin1Char(' '))) {
        type_hints.insert(hint, type);
    }
}

QList<Nepomuk2::Query::Term> PassTypeHints::run(const QList<Nepomuk2::Query::Term> &match) const
{
    QList<Nepomuk2::Query::Term> rs;
    QString value = termStringValue(match.at(0));

    if (value.isNull()) {
        return rs;
    }

    if (type_hints.contains(value)) {
        rs.append(Nepomuk2::Query::ResourceTypeTerm(
            Nepomuk2::Types::Class(type_hints.value(value))
        ));
    }

    return rs;
}
