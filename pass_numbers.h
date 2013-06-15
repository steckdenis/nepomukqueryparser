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

#ifndef __PASS_NUMBERS_H__
#define __PASS_NUMBERS_H__

#include <QVector>
#include <QHash>

namespace Nepomuk2 { namespace Query { class Term; }}

class PassNumbers
{
    public:
        PassNumbers();

        QVector<Nepomuk2::Query::Term> run(const QVector<Nepomuk2::Query::Term> &match) const;

    private:
        struct FileSizeMultiplier
        {
            FileSizeMultiplier(long long int m, int s)
            : multiplier(m), size(s)
            {}

            long long int multiplier;
            int size;
        };

        QHash<QString, FileSizeMultiplier> filesize_multipliers;
};

#endif