#ifndef __PASS_SENTBY_H__
#define __PASS_SENTBY_H__

#include <QVector>

namespace Nepomuk2 { namespace Query { class Term; }}

class PassSentBy
{
    public:
        QVector<Nepomuk2::Query::Term> run(const QVector<Nepomuk2::Query::Term> &match) const;
};

#endif