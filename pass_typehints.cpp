#include "pass_typehints.h"

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

QVector<Nepomuk2::Query::Term> PassTypeHints::run(const QVector<Nepomuk2::Query::Term> &match) const
{
    QString value = match.at(0).toLiteralTerm().value().toString();
    QVector<Nepomuk2::Query::Term> rs;

    if (type_hints.contains(value)) {
        rs.append(Nepomuk2::Query::ResourceTypeTerm(
            Nepomuk2::Types::Class(type_hints.value(value))
        ));
    }

    return rs;
}
