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

#ifndef __PASS_DATEPERIODS_H__
#define __PASS_DATEPERIODS_H__

#include <QString>
#include <QList>
#include <QHash>
#include <QUrl>

namespace Nepomuk2 { namespace Query { class Term; }}

class PassDatePeriods
{
    public:
        enum Period {
            Year = 0,
            Month,
            Week,
            DayOfWeek,
            Day,
            Hour,
            Minute,
            Second,
            VariablePeriod,
            MaxPeriod = VariablePeriod
        };

        enum ValueType {
            Value,
            Offset,
            InvertedOffset
        };

    public:
        PassDatePeriods();

        void setKind(Period period, ValueType value_type, int value = 0);

        QList<Nepomuk2::Query::Term> run(const QList<Nepomuk2::Query::Term> &match) const;

        Period periodFromName(const QString &name) const;
        static QString nameOfPeriod(Period period);
        static QUrl propertyUrl(Period period, bool offset);

    private:
        void registerPeriod(Period period, const QString &names);

    private:
        QHash<QString, Period> periods;

        Period period;
        ValueType value_type;
        int value;
};

#endif