#ifndef __PATTERNMATCHER_H__
#define __PATTERNMATCHER_H__

#include <nepomuk2/term.h>
#include <QStringList>

class PatternMatcher
{
    public:
        PatternMatcher(QList<Nepomuk2::Query::Term> &terms, QStringList pattern);

        template<typename T>
        bool runPass(const T &pass)
        {
            QList<Nepomuk2::Query::Term> matched_terms;
            bool progress = false;

            for (int i=0; i<capture_count; ++i) {
                matched_terms.append(Nepomuk2::Query::Term());
            }

            // Try to start to match the pattern at every position in the term list
            for (int index=0; index<terms.count(); ++index) {
                int matched_length = matchPattern(matched_terms, index);

                if (matched_length > 0) {
                    // The pattern matched, run the pass on the matching terms
                    QList<Nepomuk2::Query::Term> replacement = pass.run(matched_terms);

                    if (replacement.count() > 0) {
                        // Replace terms first_match_index..i with replacement
                        for (int i=0; i<matched_length; ++i) {
                            terms.removeAt(index);
                        }

                        for (int i=replacement.count()-1; i>=0; --i) {
                            terms.insert(index, replacement.at(i));
                        }

                        // Re-explore the terms vector as indexes have changed
                        progress = true;
                        index = -1;
                    }

                    // If the pattern contains "...", terms are appended to the end
                    // of matched_terms. Remove them now that they are not needed anymore
                    while (matched_terms.count() > capture_count) {
                        matched_terms.removeLast();
                    }
                }
            }

            return progress;
        }

    private:
        int captureCount() const;
        int matchPattern(QList<Nepomuk2::Query::Term> &matched_terms, int index) const;
        bool matchTerm(const Nepomuk2::Query::Term &term, const QString &pattern, int &capture_index) const;

    private:
        QList<Nepomuk2::Query::Term> &terms;
        QStringList pattern;
        int capture_count;
};

#endif