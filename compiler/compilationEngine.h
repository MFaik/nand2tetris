#pragma once
#include <string>
#include <vector>

#include "symbol_table.h"
#include "token.h"
#include "vmwriter.h"

class CompilationEngine {
    int ptr = 0;
    bool isInsideMethod = false;
    std::string currentSubroutineType;
    std::string currentClassName;
    int labelCounter = 0;

    std::vector<Token> tokens;
    VMWriter out;

    SymbolTable classSymbols, subroutineSymbols;

    void compileClass();
    void compileClassVarDec();
    void compileSubroutine();
    void compileParameterList();
    void compileVarDec();
    void compileStatements();
    void compileLet();
    void compileIf();
    void compileWhile();
    void skipWhileExpression();
    void compileDo();
    void compileReturn();
    void compileExpression();
    void compileTerm();
    void compileSubroutineCall();
    int compileExpressionList();
    std::string eatType();
    void eatKeyword(Keyword keyword);
    void eatSymbol(char c);
    bool isOP();

    public:
        CompilationEngine(std::vector<Token> tokens);
        std::string compile();
};
