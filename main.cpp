#include "parser.h"

#include <QtDebug>

int main(int argc, char **argv)
{
    if (argc != 2)
        return 0;

    Parser parser;

    parser.parse(argv[1]);
}