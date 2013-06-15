#include "pass_sentby.h"

#include <nepomuk2/literalterm.h>
#include <nepomuk2/comparisonterm.h>
#include <nepomuk2/property.h>
#include <nepomuk2/nmo.h>

QVector<Nepomuk2::Query::Term> PassSentBy::run(const QVector<Nepomuk2::Query::Term> &match) const
{
    QString value = match.at(0).toLiteralTerm().value().toString();
    QVector<Nepomuk2::Query::Term> rs;

    // TODO: Query Nepomuk for the resource identifier of the sender
    rs.append(Nepomuk2::Query::ComparisonTerm(
        Nepomuk2::Vocabulary::NMO::sender(),
        Nepomuk2::Query::LiteralTerm(value),
        Nepomuk2::Query::ComparisonTerm::Equal
    ));

    return rs;
}
