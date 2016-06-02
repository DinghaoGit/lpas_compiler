#include <iostream>
#include <stdio.h>
#include "Parser.h"

using namespace std;

int main()
{
    Parser parser;
    parser.start();
    parser.printSymbolTable();
    parser.generateMIPSCode();

    getchar();
    return 0;
}
