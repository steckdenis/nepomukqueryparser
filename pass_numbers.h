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