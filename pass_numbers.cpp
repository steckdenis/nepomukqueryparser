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

#include "pass_numbers.h"

#include <nepomuk2/literalterm.h>
#include <soprano/literalvalue.h>

#include <QtDebug>

PassNumbers::PassNumbers()
{
    filesize_multipliers.insert("KB", FileSizeMultiplier(1000LL, 2));
    filesize_multipliers.insert("MB", FileSizeMultiplier(1000000LL, 2));
    filesize_multipliers.insert("GB", FileSizeMultiplier(1000000000LL, 2));
    filesize_multipliers.insert("TB", FileSizeMultiplier(1000000000000LL, 2));

    filesize_multipliers.insert("KiB", FileSizeMultiplier(1LL << 10, 3));
    filesize_multipliers.insert("MiB", FileSizeMultiplier(1LL << 20, 3));
    filesize_multipliers.insert("GiB", FileSizeMultiplier(1LL << 30, 3));
    filesize_multipliers.insert("TiB", FileSizeMultiplier(1LL << 40, 3));
}

QVector<Nepomuk2::Query::Term> PassNumbers::run(const QVector<Nepomuk2::Query::Term> &match) const
{
    QString value = match.at(0).toLiteralTerm().value().toString();
    QVector<Nepomuk2::Query::Term> rs;

    // File size multiplier
    QHash<QString, FileSizeMultiplier>::const_iterator it;
    long long int multiplier = 1;
    bool is_suffix = true;

    it = filesize_multipliers.find(value.right(2));
    if (it == filesize_multipliers.end()) {
        it = filesize_multipliers.find(value.right(3));
    }

    if (it == filesize_multipliers.end()) {
        it = filesize_multipliers.find(value.left(2));
        is_suffix = false;
    }
    if (it == filesize_multipliers.end()) {
        it = filesize_multipliers.find(value.left(3));
    }

    if (it != filesize_multipliers.end()) {
        multiplier = it->multiplier;

        if (is_suffix) {
            value.resize(value.size() - it->size);
        } else {
            value = value.mid(it->size);
        }
    }

    // Integer or double
    bool is_integer = false;
    bool is_double = false;
    long long int as_integer = value.toLongLong(&is_integer);
    double as_double = value.toDouble(&is_double);

    // Prefer integers over doubles
    if (is_integer) {
        rs.append(Nepomuk2::Query::LiteralTerm(as_integer * multiplier));
    } else if (is_double) {
        rs.append(Nepomuk2::Query::LiteralTerm(as_double * double(multiplier)));
    }

    return rs;
}
