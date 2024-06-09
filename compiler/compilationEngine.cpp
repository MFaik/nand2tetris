#include "compilationEngine.h"
#include "symbol_table.h"
#include "token.h"
#include <stdexcept>

CompilationEngine::CompilationEngine(std::vector<Token> tokens) 
    : tokens(tokens), out() {
    //TODO: fix boundary errors
    // tokens.push_back(Token{TokenType::SYMBOL, "", -1, Keyword::notfound, 0});
    // tokens.push_back(Token{TokenType::SYMBOL, "", -1, Keyword::notfound, 0});
    // tokens.push_back(Token{TokenType::SYMBOL, "", -1, Keyword::notfound, 0});
    // tokens.push_back(Token{TokenType::SYMBOL, "", -1, Keyword::notfound, 0});
}

std::string CompilationEngine::compile() {
    try {
        compileClass();
    } catch(std::runtime_error e) {
        throw std::runtime_error(std::string(e.what()) + 
                " at token " + std::to_string(ptr) + ": " +
                token_to_str(tokens[ptr]));
    }
    return out.get();
}

void CompilationEngine::compileClass() {
    eatKeyword(Keyword::CLASS);   
    if(tokens[ptr].type != TokenType::IDENTIFIER) 
        throw std::runtime_error("expected class name");

    currentClassName = tokens[ptr].str;
    ptr++;
    eatSymbol('{');

    while(tokens[ptr].type == TokenType::KEYWORD &&
            (tokens[ptr].keyword == Keyword::STATIC ||
             tokens[ptr].keyword == Keyword::FIELD)) {
        compileClassVarDec();
    }

    while(tokens[ptr].type == TokenType::KEYWORD && 
            (tokens[ptr].keyword == Keyword::FUNCTION ||
             tokens[ptr].keyword == Keyword::CONSTRUCTOR ||
             tokens[ptr].keyword == Keyword::METHOD)) {
        compileSubroutine();
    }

    eatSymbol('}');
}

void CompilationEngine::compileClassVarDec() {
    SymbolKind kind = SymbolKind::STATIC;
    if(tokens[ptr].keyword == Keyword::FIELD)
        kind = SymbolKind::FIELD;
    ptr++;

    std::string type = eatType();

    if(tokens[ptr].type != TokenType::IDENTIFIER)
        throw std::runtime_error("expected variable name");

    classSymbols.define(tokens[ptr].str, type, kind);
    ptr++;

    while(tokens[ptr].type == TokenType::SYMBOL &&
            tokens[ptr].symbol == ',') {
        ptr++;//eatSymbol(',');
        if(tokens[ptr].type != TokenType::IDENTIFIER)
            throw std::runtime_error("expected variable name");

        classSymbols.define(tokens[ptr].str, type, kind);
        ptr++;
    }

    eatSymbol(';');
}

void CompilationEngine::compileSubroutine() {
    if(tokens[ptr].type != TokenType::KEYWORD)
        throw std::runtime_error("expected subroutine declaration");


    Keyword keyword = tokens[ptr].keyword;
    ptr++;

    if(tokens[ptr].type == TokenType::KEYWORD &&
            tokens[ptr].keyword == Keyword::VOID) {
        currentSubroutineType = "void";
        ptr++;
    } else {
        currentSubroutineType = eatType();
    }

    if(tokens[ptr].type != TokenType::IDENTIFIER)
        throw std::runtime_error("expected function name");

    std::string name = currentClassName + '.' + tokens[ptr].str;
    ptr++;

    eatSymbol('(');
    subroutineSymbols.reset();
    if(keyword == Keyword::METHOD)
        subroutineSymbols.define("-1", "-1", SymbolKind::ARG); 
    compileParameterList();
    eatSymbol(')');

    eatSymbol('{');
    if(tokens[ptr].type == TokenType::KEYWORD) {
        while(tokens[ptr].keyword == Keyword::VAR) {
            compileVarDec();
        }
    }
    out.writeFunction(name, subroutineSymbols.varCnt(SymbolKind::VAR));
    if(keyword == Keyword::CONSTRUCTOR) {
        int size = classSymbols.varCnt(SymbolKind::FIELD);
        out.writePush(Segment::CONSTANT, size);
        out.writeCall("Memory.alloc", 1);
        out.writePop(Segment::POINTER, 0);
        isInsideMethod = true;
    } else if(keyword == Keyword::METHOD) {
        out.writePush(Segment::ARGUMENT, 0);
        out.writePop(Segment::POINTER, 0);
        isInsideMethod = true;
    } else if(keyword == Keyword::FUNCTION) {
        isInsideMethod = false;
    } else {
        throw std::runtime_error("expected subroutine declaration");
    }
    compileStatements();
    eatSymbol('}');
    //TODO: maybe add a safety net return for functions
}

void CompilationEngine::compileParameterList() {
    if(tokens[ptr].type != TokenType::SYMBOL) {
        std::string var_type = eatType();

        if(tokens[ptr].type != TokenType::IDENTIFIER)
            throw std::runtime_error("expected parameter");

        subroutineSymbols.define(tokens[ptr].str, var_type, SymbolKind::ARG);
        ptr++;
    }
    while(tokens[ptr].type == TokenType::SYMBOL &&
            tokens[ptr].symbol == ',') {
        ptr++;//eatSymbol(',');

        std::string var_type = eatType();

        if(tokens[ptr].type != TokenType::IDENTIFIER)
            throw std::runtime_error("expected parameter");

        subroutineSymbols.define(tokens[ptr].str, var_type, SymbolKind::ARG);
        ptr++;
    }
}

void CompilationEngine::compileVarDec() {
    eatKeyword(Keyword::VAR);
    std::string var_type = eatType();

    if(tokens[ptr].type != TokenType::IDENTIFIER)
        throw std::runtime_error("expected identifier");

    subroutineSymbols.define(tokens[ptr].str, var_type, SymbolKind::VAR);
    ptr++;

    while(tokens[ptr].type == TokenType::SYMBOL &&
            tokens[ptr].symbol == ',') {
        ptr++;//eatSymbol(',');
        if(tokens[ptr].type != TokenType::IDENTIFIER)
            throw std::runtime_error("expected identifier");

        subroutineSymbols.define(tokens[ptr].str, var_type, SymbolKind::VAR);
        ptr++;
    }

    eatSymbol(';');
}

void CompilationEngine::compileStatements() {
    while(tokens[ptr].type == TokenType::KEYWORD) {
        switch(tokens[ptr].keyword) {
            case Keyword::LET:
                compileLet();
                break;
            case Keyword::IF:
                compileIf();
                break;
            case Keyword::WHILE:
                compileWhile();
                break;
            case Keyword::DO:
                compileDo();
                break;
            case Keyword::RETURN:
                compileReturn();
                break;
            default:
                return;
        }
    }
}

void CompilationEngine::compileLet() {
    eatKeyword(Keyword::LET);
    if(tokens[ptr].type != TokenType::IDENTIFIER)
        throw std::runtime_error("expected variable name");

    std::string name = tokens[ptr].str;
    Symbol symbol = subroutineSymbols.getSymbol(name);
    if(symbol.kind == SymbolKind::NONE) {
        symbol = classSymbols.getSymbol(tokens[ptr].str);
    }
    if(symbol.kind == SymbolKind::NONE) {
        throw std::runtime_error("symbol not found: " + tokens[ptr].str);
    }
    Segment symbol_seg;
    switch(symbol.kind) {
        case SymbolKind::STATIC:
            symbol_seg = Segment::STATIC;
            break;
        case SymbolKind::ARG:
            symbol_seg = Segment::ARGUMENT;
            if(isInsideMethod)
                symbol.index++;
            break;
        case SymbolKind::FIELD:
            symbol_seg = Segment::THIS;
            break;
        case SymbolKind::VAR:
            symbol_seg = Segment::LOCAL;
            break;
        default:break;
    }
    ptr++;

    if(tokens[ptr].type == TokenType::SYMBOL &&
            tokens[ptr].symbol == '[') {
        //array
        ptr++;//eatSymbol('[');
        out.writePush(symbol_seg, symbol.index);
        compileExpression();
        out.writeArithmetic(Command::ADD);
        eatSymbol(']');
        eatSymbol('=');
        compileExpression();
        out.writePop(Segment::TEMP, 0);
        out.writePop(Segment::POINTER, 1);
        out.writePush(Segment::TEMP, 0);
        out.writePop(Segment::THAT, 0);
    } else {
        eatSymbol('=');
        compileExpression();
        out.writePop(symbol_seg, symbol.index);
    }
    eatSymbol(';');
}

void CompilationEngine::compileIf() {
    //TODO: remove class name from labels
    std::string elseLabel = "L" + currentClassName + std::to_string(labelCounter);
    labelCounter++;
    eatKeyword(Keyword::IF);
    eatSymbol('(');
    compileExpression();
    eatSymbol(')');
    out.writeArithmetic(Command::NOT);
    out.writeIf(elseLabel);
    eatSymbol('{');
    compileStatements();
    eatSymbol('}');
    if(tokens[ptr].type == TokenType::KEYWORD &&
            tokens[ptr].keyword == Keyword::ELSE) {
        //TODO: remove class name from labels
        std::string endLabel = "L" + currentClassName + std::to_string(labelCounter);
        labelCounter++;
        ptr++;//eatKeyword(Keyword::ELSE);
        out.writeGoto(endLabel);
        eatSymbol('{');
        out.writeLabel(elseLabel);
        compileStatements();
        eatSymbol('}');
        out.writeLabel(endLabel);
    } else {
        out.writeLabel(elseLabel);
    }
}

void CompilationEngine::compileWhile() {
    //TODO: remove class name from labels
    std::string startLabel = "L" + currentClassName + std::to_string(labelCounter);
    std::string endLabel = "L" + currentClassName + std::to_string(labelCounter+1);
    labelCounter += 2;

    eatKeyword(Keyword::WHILE);
    int expressionPtr = ptr;
    skipWhileExpression();
    out.writeGoto(endLabel);
    eatSymbol('{');
    out.writeLabel(startLabel);
    compileStatements();
    eatSymbol('}');
    out.writeLabel(endLabel);
    int endPtr = ptr;
    ptr = expressionPtr;
    compileExpression();
    out.writeIf(startLabel);
    ptr = endPtr;
}

void CompilationEngine::skipWhileExpression() {
    int depth = 0;
    do {
        if(tokens[ptr].type == TokenType::SYMBOL) {
            if(tokens[ptr].symbol == '(')
                depth++;
            else if(tokens[ptr].symbol == ')')
                depth--;
        }
        ptr++;
    } while(depth);
}

void CompilationEngine::compileDo() {
    eatKeyword(Keyword::DO);
    compileSubroutineCall();   
    out.writePop(Segment::TEMP, 0);
    eatSymbol(';');
}

void CompilationEngine::compileReturn() {
    eatKeyword(Keyword::RETURN);
    // std::cout << currentSubroutineType << std::endl;
    if(currentSubroutineType == "void") {
        out.writePush(Segment::CONSTANT, 0);
    } else {
        compileExpression();
    }
    out.writeReturn();
    eatSymbol(';');
}

void CompilationEngine::compileExpression() {
    // std::cout << "compile expression at: " << ptr << tokens[ptr].symbol << std::endl;
    compileTerm();
    while(isOP()) {
        Command op;
        char special = 0;
        switch(tokens[ptr].symbol) {
            case '+':
                op = Command::ADD;
                break;
            case '-':
                op = Command::SUB;
                break;
            case '*':
            case '/':
                special = tokens[ptr].symbol;
                break;
            case '&':
                op = Command::AND;
                break;
            case '|':
                op = Command::OR;
                break;
            case '<':
                op = Command::LT;
                break;
            case '>':
                op = Command::GT;
                break;
            case '=':
                op = Command::EQ;
                break;
        }
        ptr++;

        compileTerm();
        if(!special) {
            out.writeArithmetic(op);
        } else {
            if(special == '*') {
                out.writeCall("Math.multiply", 2);
            } else {
                out.writeCall("Math.divide", 2);
            }
        }
    }
    // std::cout << ", finished expression at: " << ptr << std::endl;
}

void CompilationEngine::compileTerm() {
    switch(tokens[ptr].type) {
        case TokenType::INTEGER:
            out.writePush(Segment::CONSTANT, tokens[ptr].i);
            ptr++;
            break;
        case TokenType::STRING:
            out.writePush(Segment::CONSTANT, tokens[ptr].str.size());
            out.writeCall("String.new", 1);
            for(char c : tokens[ptr].str) {
                out.writePush(Segment::CONSTANT, (int)c);
                out.writeCall("String.appendChar", 2);
            }
            ptr++;
            break;
        case TokenType::KEYWORD:
            switch(tokens[ptr].keyword) {
                case Keyword::TRUE:
                    out.writePush(Segment::CONSTANT, 1);
                    out.writeArithmetic(Command::NEG);
                    break;
                case Keyword::FALSE:
                case Keyword::NULL_:
                    out.writePush(Segment::CONSTANT, 0);
                    break;
                case Keyword::THIS:
                    if(!isInsideMethod)
                        throw std::runtime_error(
                                "can only call \"this\" inside a method");
                    out.writePush(Segment::POINTER, 0);
                    break;
                default:
                    throw std::runtime_error("expected term");
            }
            ptr++;
            break;
        case TokenType::SYMBOL:
            if(tokens[ptr].symbol == '~') {
                ptr++;//eatSymbol('~');
                compileTerm();
                out.writeArithmetic(Command::NOT);
            } else if(tokens[ptr].symbol == '-') {
                ptr++;//eatSymbol('-');
                compileTerm();
                out.writeArithmetic(Command::NEG);
            } else if('('){
                ptr++;//eatSymbol('(');
                compileExpression();
                eatSymbol(')');
            } else {
                throw std::runtime_error("expected term");
            }
            break;
        case TokenType::IDENTIFIER:
            Symbol symbol = subroutineSymbols.getSymbol(tokens[ptr].str);
            if(symbol.kind == SymbolKind::NONE) {
                symbol = classSymbols.getSymbol(tokens[ptr].str);
            }
            Segment symbol_seg;
            switch(symbol.kind) {
                case SymbolKind::STATIC:
                    symbol_seg = Segment::STATIC;
                    break;
                case SymbolKind::ARG:
                    symbol_seg = Segment::ARGUMENT;
                    break;
                case SymbolKind::FIELD:
                    symbol_seg = Segment::THIS;
                    break;
                case SymbolKind::VAR:
                    symbol_seg = Segment::LOCAL;
                    break;
                default:break;
            }

            if(tokens[ptr+1].type == TokenType::SYMBOL) {
                if(tokens[ptr+1].symbol == '[') {
                    if(symbol.kind == SymbolKind::NONE) {
                        throw std::runtime_error("symbol not found: " + tokens[ptr].str);
                    }
                    //array 
                    out.writePush(symbol_seg, symbol.index);
                    // std::cout << "compile expression inside []";
                    ptr+=2;
                    compileExpression();
                    out.writeArithmetic(Command::ADD);
                    out.writePop(Segment::POINTER, 1);
                    out.writePush(Segment::THAT, 0);
                    eatSymbol(']');
                    break;
                } else if(tokens[ptr+1].symbol == '(' || tokens[ptr+1].symbol == '.'){
                    compileSubroutineCall();
                    break;
                } else {
                    if(symbol.kind == SymbolKind::NONE) {
                        throw std::runtime_error("symbol not found: " + tokens[ptr].str);
                    }
                    out.writePush(symbol_seg, symbol.index);
                    ptr++;
                    break;
                }
            } else {
                if(symbol.kind == SymbolKind::NONE) {
                    throw std::runtime_error("symbol not found: " + tokens[ptr].str);
                }
                out.writePush(symbol_seg, symbol.index);
                break;
            }
            throw std::runtime_error("expected term");
    }
}

void CompilationEngine::compileSubroutineCall() {
    if(tokens[ptr].type != TokenType::IDENTIFIER ||
            tokens[ptr+1].type != TokenType::SYMBOL) {
        throw std::runtime_error("expected subroutine call");
    }

    int parameterCount = 0;
    std::string functionName;

    if(tokens[ptr+1].symbol == '(') {
        //method call
        if(!isInsideMethod)
            throw std::runtime_error(
                    "can only call methods by name, while inside a method");

        out.writePush(Segment::POINTER, 0);
        parameterCount++;
        functionName = currentClassName + "." + tokens[ptr].str;
        ptr += 1;
    } else if(tokens[ptr+1].symbol == '.') {
        if(tokens[ptr+2].type != TokenType::IDENTIFIER)
            throw std::runtime_error("expected identifier after .");

        Symbol symbol = subroutineSymbols.getSymbol(tokens[ptr].str);
        if(symbol.kind == SymbolKind::NONE) {
            symbol = classSymbols.getSymbol(tokens[ptr].str);
        }
        if(symbol.kind == SymbolKind::NONE) {
            //function call
            functionName = tokens[ptr].str + "." + tokens[ptr+2].str;
        } else {
            Segment symbol_seg;
            switch(symbol.kind) {
                case SymbolKind::STATIC:
                    symbol_seg = Segment::STATIC;
                    break;
                case SymbolKind::ARG:
                    symbol_seg = Segment::ARGUMENT;
                    if(isInsideMethod)
                        symbol.index++;
                    break;
                case SymbolKind::FIELD:
                    symbol_seg = Segment::THIS;
                    break;
                case SymbolKind::VAR:
                    symbol_seg = Segment::LOCAL;
                    break;
                default:break;
            }
            functionName = symbol.type + "." + tokens[ptr+2].str;
            out.writePush(symbol_seg, symbol.index);
            parameterCount++;
        }
        ptr += 3;
    } else {
        // std::cout << "\"" << tokens[ptr].str << "\"\n";
        throw std::runtime_error("expected subroutine call1, " + std::string(1, tokens[ptr+1].symbol));
    }
    eatSymbol('(');
    parameterCount += compileExpressionList();
    out.writeCall(functionName, parameterCount);
    eatSymbol(')');
}

int CompilationEngine::compileExpressionList() {
    int expressionCnt = 0;
    if(tokens[ptr].type != TokenType::SYMBOL || tokens[ptr].symbol != ')') {
        compileExpression();
        expressionCnt++;
        while(tokens[ptr].type == TokenType::SYMBOL 
                && tokens[ptr].symbol == ',') {
            ptr++;//eatSymbol(',');
            compileExpression();
            expressionCnt++;
        }
    }
    return expressionCnt;
}

std::string CompilationEngine::eatType() {
    ptr++;
    if(tokens[ptr-1].type == TokenType::KEYWORD) {
        switch(tokens[ptr-1].keyword) {
            case Keyword::INT:
                return "int";
            case Keyword::CHAR:
                return "char";
            case Keyword::BOOLEAN:
                return "boolean";
            default:
                throw std::runtime_error("expected type");
        }
    } else if(tokens[ptr-1].type == TokenType::IDENTIFIER) {
        return tokens[ptr-1].str;
    } else {
        throw std::runtime_error("expected type");
    }
}

void CompilationEngine::eatKeyword(Keyword keyword) {
    if(tokens[ptr].type == TokenType::KEYWORD &&
            tokens[ptr].keyword == keyword) {
        ptr++;
    } else {
        throw std::runtime_error("expected keyword: " + keywordTableReversed[keyword]);
    }
}

void CompilationEngine::eatSymbol(char c) {
    if(tokens[ptr].type == TokenType::SYMBOL && 
            tokens[ptr].symbol == c) {
        ptr++;
    } else {
        throw std::runtime_error("expected symbol: " + std::string(1, c));
    }
}

bool CompilationEngine::isOP() {
    const auto symbols = std::string("+-*/&|<>=");
    return tokens[ptr].type == TokenType::SYMBOL && 
        symbols.find(tokens[ptr].symbol) != std::string::npos;
}
