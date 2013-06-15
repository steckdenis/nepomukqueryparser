#ifndef __PASS_TYPEHINTS_H__
#define __PASS_TYPEHINTS_H__

#include <QString>
#include <QVector>
#include <QHash>
#include <QUrl>

namespace Nepomuk2 { namespace Query { class Term; }}

class PassTypeHints
{
    public:
        PassTypeHints();

        QVector<Nepomuk2::Query::Term> run(const QVector<Nepomuk2::Query::Term> &match) const;

    private:
        void registerHints(const QUrl &type, const QString &hints);

    private:
        QHash<QString, QUrl> type_hints;
};

#endif