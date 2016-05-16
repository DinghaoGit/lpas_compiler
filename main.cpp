#include <iostream>
#include <stdio.h>
#include "Parser.h"

using namespace std;

int main()
{
    Parser parser;
    parser.start();
    parser.printSymbolTable();
    parser.printMIPS();

    getchar();
    return 0;
}
