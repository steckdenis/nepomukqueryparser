#ifndef __PARSER_H__
#define __PARSER_H__

#include <QString>

class Parser
{
    public:
        Parser();
        Parser(const Parser &other);
        ~Parser();

        void reset();
        void parse(const QString &query);

    private:
        struct Private;
        Private *d;
};

#endif