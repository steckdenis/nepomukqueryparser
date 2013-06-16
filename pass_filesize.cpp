#include "pass_filesize.h"

#include <nepomuk2/literalterm.h>
#include <soprano/literalvalue.h>
#include <klocalizedstring.h>

PassFileSize::PassFileSize()
{
    // File size units
    registerUnits(1000LL, i18nc("Lower-case units corresponding to a kilobyte", "kb"));
    registerUnits(1000000LL, i18nc("Lower-case units corresponding to a megabyte", "mb"));
    registerUnits(1000000000LL, i18nc("Lower-case units corresponding to a gigabyte", "gb"));
    registerUnits(1000000000000LL, i18nc("Lower-case units corresponding to a terabyte", "tb"));

    registerUnits(1LL << 10, i18nc("Lower-case units corresponding to a kibibyte", "kib"));
    registerUnits(1LL << 20, i18nc("Lower-case units corresponding to a mebibyte", "mib"));
    registerUnits(1LL << 30, i18nc("Lower-case units corresponding to a gibibyte", "gib"));
    registerUnits(1LL << 40, i18nc("Lower-case units corresponding to a tebibyte", "tib"));
}

void PassFileSize::registerUnits(long long int multiplier, const QString &units)
{
    Q_FOREACH(const QString &unit, units.split(QLatin1Char(' '))) {
        multipliers.insert(unit, multiplier);
    }
}

QVector<Nepomuk2::Query::Term> PassFileSize::run(const QVector<Nepomuk2::Query::Term> &match) const
{
    QVector<Nepomuk2::Query::Term> rs;

    // Number and unit
    long long int number = match.at(0).toLiteralTerm().value().toInt64();
    QString unit = match.at(1).toLiteralTerm().value().toString();

    if (multipliers.contains(unit)) {
        rs.append(Nepomuk2::Query::LiteralTerm(number * multipliers.value(unit)));
    }

    return rs;
}
