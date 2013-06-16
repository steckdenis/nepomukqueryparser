#include "utils.h"

#include <nepomuk2/literalterm.h>
#include <soprano/literalvalue.h>

QString termStringValue(const Nepomuk2::Query::Term &term)
{
    if (!term.isLiteralTerm()) {
        return QString();
    }

    Soprano::LiteralValue value = term.toLiteralTerm().value();

    if (value.isString()) {
        return value.toString();
    } else {
        return QString();
    }
}