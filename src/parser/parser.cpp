#include "parser.h"
#include "../diagnostics/error_reporter.h"

namespace compiler {

Parser::Parser(Lexer& lexer, MemoryPool& pool)
    : lexer_(lexer), pool_(pool), panicMode_(false), hadError_(false) {
    advance(); // Load first token
}

bool Parser::hadError() const { return hadError_; }

void Parser::advance() {
    previous_ = current_;
    current_ = lexer_.nextToken();
    
    if (panicMode_) {
        if (current_.type == TokenType::SEMICOLON || 
            current_.type == TokenType::RBRACE ||
            current_.type == TokenType::END_OF_FILE) {
            panicMode_ = false;
        }
    }
}

bool Parser::check(TokenType type) const {
    return current_.type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::match(std::initializer_list<TokenType> types) {
    for (auto type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

Token Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) return advance(), previous_;
    
    error(message);
    return current_;
}

void Parser::error(const std::string& message) {
    errorAt(current_, message);
}

void Parser::errorAt(const Token& token, const std::string& message) {
    if (panicMode_) return;
    panicMode_ = true;
    hadError_ = true;
    
    std::string loc = token.line > 0 ? 
        std::to_string(token.line) + ":" + std::to_string(token.column) : 
        "end";
    
    REPORT_ERROR(SourceLocation(token.line, token.column, token.offset), 
                 "Parse error at " + loc + ": " + message);
}

void Parser::synchronize() {
    panicMode_ = false;
    
    while (current_.type != TokenType::END_OF_FILE) {
        if (previous_.type == TokenType::SEMICOLON) return;
        
        switch (current_.type) {
            case TokenType::KW_FN:
            case TokenType::KW_LET:
            case TokenType::KW_CONST:
            case TokenType::KW_IF:
            case TokenType::KW_WHILE:
            case TokenType::KW_FOR:
            case TokenType::KW_RETURN:
            case TokenType::KW_STRUCT:
            case TokenType::KW_ENUM:
                return;
            default:
                advance();
        }
    }
}

std::unique_ptr<Program> Parser::parseProgram() {
    SourceLocation loc = SourceLocation(previous_.line, previous_.column, previous_.offset);
    std::vector<std::unique_ptr<Decl>> declarations;
    
    while (!check(TokenType::END_OF_FILE)) {
        try {
            auto decl = parseDeclaration();
            if (decl) declarations.push_back(std::move(decl));
        } catch (const ParseError& e) {
            synchronize();
        }
    }
    
    return std::make_unique<Program>(loc, std::move(declarations));
}

std::unique_ptr<Decl> Parser::parseDeclaration() {
    if (match(TokenType::KW_FN)) return parseFunctionDecl();
    if (match(TokenType::KW_STRUCT)) return parseStructDecl();
    if (match(TokenType::KW_ENUM)) return parseEnumDecl();
    
    // Allow top-level statements (expressions, variable declarations)
    error("Expected declaration");
    return nullptr;
}

std::unique_ptr<FunctionDecl> Parser::parseFunctionDecl() {
    Token name = consume(TokenType::IDENTIFIER, "Expected function name");
    SourceLocation loc(name.line, name.column, name.offset);
    
    consume(TokenType::LPAREN, "Expected '(' after function name");
    auto params = parseParameters();
    consume(TokenType::RPAREN, "Expected ')' after parameters");
    
    std::unique_ptr<Type> returnType = nullptr;
    if (match(TokenType::ARROW)) {
        returnType = parseType();
    }
    
    std::unique_ptr<Stmt> body = nullptr;
    if (match(TokenType::LBRACE)) {
        body = parseBlockStatement();
    } else if (match(TokenType::SEMICOLON)) {
        // Extern function declaration
    } else {
        error("Expected function body or ';' for extern function");
    }
    
    return std::make_unique<FunctionDecl>(loc, name.lexeme, std::move(params),
                                          std::move(returnType), std::move(body));
}

std::vector<ParamDecl> Parser::parseParameters() {
    std::vector<ParamDecl> params;
    
    if (!check(TokenType::RPAREN)) {
        do {
            bool isMut = match(TokenType::KW_MUT);
            Token name = consume(TokenType::IDENTIFIER, "Expected parameter name");
            consume(TokenType::COLON, "Expected ':' after parameter name");
            auto type = parseType();
            
            params.emplace_back(name.lexeme, std::move(type), isMut);
        } while (match(TokenType::COMMA));
    }
    
    return params;
}

std::unique_ptr<StructDecl> Parser::parseStructDecl() {
    Token name = consume(TokenType::IDENTIFIER, "Expected struct name");
    SourceLocation loc(name.line, name.column, name.offset);
    
    consume(TokenType::LBRACE, "Expected '{' after struct name");
    
    std::vector<StructDecl::Field> fields;
    while (!check(TokenType::RBRACE) && !check(TokenType::END_OF_FILE)) {
        Token fieldName = consume(TokenType::IDENTIFIER, "Expected field name");
        consume(TokenType::COLON, "Expected ':' after field name");
        auto fieldType = parseType();
        
        fields.push_back({fieldName.lexeme, std::move(fieldType)});
        
        if (!check(TokenType::RBRACE)) {
            consume(TokenType::COMMA, "Expected ',' or '}' after field");
        } else {
            match(TokenType::COMMA); // Optional trailing comma
        }
    }
    
    consume(TokenType::RBRACE, "Expected '}' after struct fields");
    
    return std::make_unique<StructDecl>(loc, name.lexeme, std::move(fields));
}

std::unique_ptr<EnumDecl> Parser::parseEnumDecl() {
    Token name = consume(TokenType::IDENTIFIER, "Expected enum name");
    SourceLocation loc(name.line, name.column, name.offset);
    
    consume(TokenType::LBRACE, "Expected '{' after enum name");
    
    std::vector<EnumDecl::Variant> variants;
    while (!check(TokenType::RBRACE) && !check(TokenType::END_OF_FILE)) {
        Token variantName = consume(TokenType::IDENTIFIER, "Expected variant name");
        
        std::vector<std::unique_ptr<Type>> types;
        if (match(TokenType::LPAREN)) {
            // Tuple variant
            do {
                types.push_back(parseType());
            } while (match(TokenType::COMMA));
            consume(TokenType::RPAREN, "Expected ')' after variant types");
        }
        
        variants.push_back({variantName.lexeme, std::move(types)});
        
        if (!check(TokenType::RBRACE)) {
            consume(TokenType::COMMA, "Expected ',' or '}' after variant");
        } else {
            match(TokenType::COMMA);
        }
    }
    
    consume(TokenType::RBRACE, "Expected '}' after enum variants");
    
    return std::make_unique<EnumDecl>(loc, name.lexeme, std::move(variants));
}

std::unique_ptr<Stmt> Parser::parseStatement() {
    if (match(TokenType::LBRACE)) return parseBlockStatement();
    if (match(TokenType::KW_IF)) return parseIfStatement();
    if (match(TokenType::KW_WHILE)) return parseWhileStatement();
    if (match(TokenType::KW_FOR)) return parseForStatement();
    if (match(TokenType::KW_RETURN)) return parseReturnStatement();
    if (match(TokenType::KW_BREAK)) return parseBreakStatement();
    if (match(TokenType::KW_CONTINUE)) return parseContinueStatement();
    if (match(TokenType::KW_LET) || match(TokenType::KW_CONST)) {
        return parseVarDeclaration();
    }
    
    return parseExprStatement();
}

std::unique_ptr<Stmt> Parser::parseBlockStatement() {
    SourceLocation loc(previous_.line, previous_.column, previous_.offset);
    std::vector<std::unique_ptr<Stmt>> statements;
    
    while (!check(TokenType::RBRACE) && !check(TokenType::END_OF_FILE)) {
        statements.push_back(parseStatement());
    }
    
    consume(TokenType::RBRACE, "Expected '}' after block");
    
    return std::make_unique<BlockStmt>(loc, std::move(statements));
}

std::unique_ptr<Stmt> Parser::parseVarDeclaration() {
    bool isConst = previous_.type == TokenType::KW_CONST;
    SourceLocation loc(previous_.line, previous_.column, previous_.offset);
    
    bool isMut = match(TokenType::KW_MUT);
    Token name = consume(TokenType::IDENTIFIER, "Expected variable name");
    
    std::unique_ptr<Type> type = nullptr;
    if (match(TokenType::COLON)) {
        type = parseType();
    }
    
    std::unique_ptr<Expr> init = nullptr;
    if (match(TokenType::ASSIGN)) {
        init = parseExpression();
    } else if (!type) {
        error("Variable declaration needs either type annotation or initializer");
    }
    
    match(TokenType::SEMICOLON); // Optional semicolon
    
    return std::make_unique<VarDeclStmt>(SourceLocation(loc.line, loc.column, loc.offset),
                                         name.lexeme, std::move(type), std::move(init),
                                         isMut, isConst);
}

std::unique_ptr<Stmt> Parser::parseIfStatement() {
    SourceLocation loc(previous_.line, previous_.column, previous_.offset);
    
    consume(TokenType::LPAREN, "Expected '(' after 'if'");
    auto condition = parseExpression();
    consume(TokenType::RPAREN, "Expected ')' after condition");
    
    auto thenBranch = parseStatement();
    
    std::unique_ptr<Stmt> elseBranch = nullptr;
    if (match(TokenType::KW_ELSE)) {
        elseBranch = parseStatement();
    }
    
    return std::make_unique<IfStmt>(loc, std::move(condition), 
                                    std::move(thenBranch), std::move(elseBranch));
}

std::unique_ptr<Stmt> Parser::parseWhileStatement() {
    SourceLocation loc(previous_.line, previous_.column, previous_.offset);
    
    consume(TokenType::LPAREN, "Expected '(' after 'while'");
    auto condition = parseExpression();
    consume(TokenType::RPAREN, "Expected ')' after condition");
    
    auto body = parseStatement();
    
    return std::make_unique<WhileStmt>(loc, std::move(condition), std::move(body));
}

std::unique_ptr<Stmt> Parser::parseForStatement() {
    SourceLocation loc(previous_.line, previous_.column, previous_.offset);
    
    consume(TokenType::LPAREN, "Expected '(' after 'for'");
    
    std::unique_ptr<Stmt> init = nullptr;
    if (!check(TokenType::SEMICOLON)) {
        if (match(TokenType::KW_LET) || match(TokenType::KW_CONST)) {
            init = parseVarDeclaration();
        } else {
            init = parseExprStatement();
        }
    } else {
        match(TokenType::SEMICOLON);
    }
    
    std::unique_ptr<Expr> condition = nullptr;
    if (!check(TokenType::SEMICOLON)) {
        condition = parseExpression();
    }
    consume(TokenType::SEMICOLON, "Expected ';' after for condition");
    
    std::unique_ptr<Expr> increment = nullptr;
    if (!check(TokenType::RPAREN)) {
        increment = parseExpression();
    }
    consume(TokenType::RPAREN, "Expected ')' after for clauses");
    
    auto body = parseStatement();
    
    return std::make_unique<ForStmt>(loc, std::move(init), std::move(condition),
                                     std::move(increment), std::move(body));
}

std::unique_ptr<Stmt> Parser::parseReturnStatement() {
    SourceLocation loc(previous_.line, previous_.column, previous_.offset);
    
    std::unique_ptr<Expr> value = nullptr;
    if (!check(TokenType::SEMICOLON) && !check(TokenType::RBRACE)) {
        value = parseExpression();
    }
    
    match(TokenType::SEMICOLON);
    
    return std::make_unique<ReturnStmt>(loc, std::move(value));
}

std::unique_ptr<Stmt> Parser::parseBreakStatement() {
    SourceLocation loc(previous_.line, previous_.column, previous_.offset);
    match(TokenType::SEMICOLON);
    return std::make_unique<BreakStmt>(loc);
}

std::unique_ptr<Stmt> Parser::parseContinueStatement() {
    SourceLocation loc(previous_.line, previous_.column, previous_.offset);
    match(TokenType::SEMICOLON);
    return std::make_unique<ContinueStmt>(loc);
}

std::unique_ptr<Stmt> Parser::parseExprStatement() {
    SourceLocation loc(current_.line, current_.column, current_.offset);
    auto expr = parseExpression();
    match(TokenType::SEMICOLON);
    return std::make_unique<ExprStmt>(loc, std::move(expr));
}

std::unique_ptr<Expr> Parser::parseExpression() {
    return parseAssignment();
}

std::unique_ptr<Expr> Parser::parseAssignment() {
    auto expr = parseOr();
    
    if (match(TokenType::ASSIGN) || match(TokenType::PLUS_ASSIGN) || 
        match(TokenType::MINUS_ASSIGN) || match(TokenType::STAR_ASSIGN) ||
        match(TokenType::SLASH_ASSIGN) || match(TokenType::PERCENT_ASSIGN)) {
        
        Token op = previous_;
        auto value = parseAssignment(); // Right-associative
        
        if (op.type == TokenType::ASSIGN) {
            return std::make_unique<AssignExpr>(SourceLocation(op.line, op.column, op.offset),
                                                std::move(expr), std::move(value));
        }
        
        // Compound assignment: desugar to binary op + assign
        BinaryExpr::Op binOp;
        switch (op.type) {
            case TokenType::PLUS_ASSIGN: binOp = BinaryExpr::Op::Add; break;
            case TokenType::MINUS_ASSIGN: binOp = BinaryExpr::Op::Sub; break;
            case TokenType::STAR_ASSIGN: binOp = BinaryExpr::Op::Mul; break;
            case TokenType::SLASH_ASSIGN: binOp = BinaryExpr::Op::Div; break;
            case TokenType::PERCENT_ASSIGN: binOp = BinaryExpr::Op::Mod; break;
            default: binOp = BinaryExpr::Op::Add; // unreachable
        }
        
        SourceLocation loc(op.line, op.column, op.offset);
        auto binExpr = std::make_unique<BinaryExpr>(loc, binOp, 
            std::unique_ptr<Expr>(dynamic_cast<Expr*>(expr->clone().release())), 
            std::move(value));
        return std::make_unique<AssignExpr>(loc, std::move(expr), std::move(binExpr));
    }
    
    return expr;
}

// Note: clone() not implemented in base, this is simplified
// In production, implement proper cloning or use different approach

std::unique_ptr<Expr> Parser::parseOr() {
    auto expr = parseAnd();
    
    while (match(TokenType::OR)) {
        Token op = previous_;
        auto right = parseAnd();
        expr = std::make_unique<BinaryExpr>(SourceLocation(op.line, op.column, op.offset),
                                            BinaryExpr::Op::Or, std::move(expr), std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::parseAnd() {
    auto expr = parseEquality();
    
    while (match(TokenType::AND)) {
        Token op = previous_;
        auto right = parseEquality();
        expr = std::make_unique<BinaryExpr>(SourceLocation(op.line, op.column, op.offset),
                                            BinaryExpr::Op::And, std::move(expr), std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::parseEquality() {
    auto expr = parseComparison();
    
    while (match(TokenType::EQ) || match(TokenType::NE)) {
        Token op = previous_;
        BinaryExpr::Op binOp = (op.type == TokenType::EQ) ? BinaryExpr::Op::Eq : BinaryExpr::Op::Ne;
        auto right = parseComparison();
        expr = std::make_unique<BinaryExpr>(SourceLocation(op.line, op.column, op.offset),
                                            binOp, std::move(expr), std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::parseComparison() {
    auto expr = parseBitwiseOr();
    
    while (match(TokenType::LT) || match(TokenType::LE) || 
           match(TokenType::GT) || match(TokenType::GE)) {
        Token op = previous_;
        BinaryExpr::Op binOp;
        switch (op.type) {
            case TokenType::LT: binOp = BinaryExpr::Op::Lt; break;
            case TokenType::LE: binOp = BinaryExpr::Op::Le; break;
            case TokenType::GT: binOp = BinaryExpr::Op::Gt; break;
            case TokenType::GE: binOp = BinaryExpr::Op::Ge; break;
            default: binOp = BinaryExpr::Op::Lt;
        }
        auto right = parseBitwiseOr();
        expr = std::make_unique<BinaryExpr>(SourceLocation(op.line, op.column, op.offset),
                                            binOp, std::move(expr), std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::parseBitwiseOr() {
    auto expr = parseBitwiseXor();
    
    while (match(TokenType::BIT_OR)) {
        Token op = previous_;
        auto right = parseBitwiseXor();
        expr = std::make_unique<BinaryExpr>(SourceLocation(op.line, op.column, op.offset),
                                            BinaryExpr::Op::BitOr, std::move(expr), std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::parseBitwiseXor() {
    auto expr = parseBitwiseAnd();
    
    while (match(TokenType::BIT_XOR)) {
        Token op = previous_;
        auto right = parseBitwiseAnd();
        expr = std::make_unique<BinaryExpr>(SourceLocation(op.line, op.column, op.offset),
                                            BinaryExpr::Op::BitXor, std::move(expr), std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::parseBitwiseAnd() {
    auto expr = parseShift();
    
    while (match(TokenType::BIT_AND)) {
        Token op = previous_;
        auto right = parseShift();
        expr = std::make_unique<BinaryExpr>(SourceLocation(op.line, op.column, op.offset),
                                            BinaryExpr::Op::BitAnd, std::move(expr), std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::parseShift() {
    auto expr = parseTerm();
    
    while (match(TokenType::SHL) || match(TokenType::SHR)) {
        Token op = previous_;
        BinaryExpr::Op binOp = (op.type == TokenType::SHL) ? BinaryExpr::Op::Shl : BinaryExpr::Op::Shr;
        auto right = parseTerm();
        expr = std::make_unique<BinaryExpr>(SourceLocation(op.line, op.column, op.offset),
                                            binOp, std::move(expr), std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::parseTerm() {
    auto expr = parseFactor();
    
    while (match(TokenType::PLUS) || match(TokenType::MINUS)) {
        Token op = previous_;
        BinaryExpr::Op binOp = (op.type == TokenType::PLUS) ? BinaryExpr::Op::Add : BinaryExpr::Op::Sub;
        auto right = parseFactor();
        expr = std::make_unique<BinaryExpr>(SourceLocation(op.line, op.column, op.offset),
                                            binOp, std::move(expr), std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::parseFactor() {
    auto expr = parseUnary();
    
    while (match(TokenType::STAR) || match(TokenType::SLASH) || match(TokenType::PERCENT)) {
        Token op = previous_;
        BinaryExpr::Op binOp;
        switch (op.type) {
            case TokenType::STAR: binOp = BinaryExpr::Op::Mul; break;
            case TokenType::SLASH: binOp = BinaryExpr::Op::Div; break;
            case TokenType::PERCENT: binOp = BinaryExpr::Op::Mod; break;
            default: binOp = BinaryExpr::Op::Mul;
        }
        auto right = parseUnary();
        expr = std::make_unique<BinaryExpr>(SourceLocation(op.line, op.column, op.offset),
                                            binOp, std::move(expr), std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::parseUnary() {
    if (match(TokenType::NOT) || match(TokenType::MINUS) || 
        match(TokenType::BIT_NOT) || match(TokenType::INC) || 
        match(TokenType::DEC) || match(TokenType::STAR) ||
        match(TokenType::BIT_AND)) {
        
        Token op = previous_;
        UnaryExpr::Op unaryOp;
        switch (op.type) {
            case TokenType::NOT: unaryOp = UnaryExpr::Op::Not; break;
            case TokenType::MINUS: unaryOp = UnaryExpr::Op::Neg; break;
            case TokenType::BIT_NOT: unaryOp = UnaryExpr::Op::BitNot; break;
            case TokenType::INC: unaryOp = UnaryExpr::Op::Inc; break;
            case TokenType::DEC: unaryOp = UnaryExpr::Op::Dec; break;
            case TokenType::STAR: unaryOp = UnaryExpr::Op::Deref; break;
            case TokenType::BIT_AND: unaryOp = UnaryExpr::Op::Ref; break;
            default: unaryOp = UnaryExpr::Op::Neg;
        }
        
        auto operand = parseUnary();
        return std::make_unique<UnaryExpr>(SourceLocation(op.line, op.column, op.offset),
                                           unaryOp, std::move(operand));
    }
    
    return parsePostfix();
}

std::unique_ptr<Expr> Parser::parsePostfix() {
    auto expr = parsePrimary();
    
    while (true) {
        if (match(TokenType::LPAREN)) {
            expr = parseCall(std::move(expr));
        } else if (match(TokenType::LBRACKET)) {
            expr = parseIndex(std::move(expr));
        } else if (match(TokenType::DOT) || match(TokenType::ARROW)) {
            expr = parseMember(std::move(expr));
        } else if (match(TokenType::INC) || match(TokenType::DEC)) {
            // Postfix increment/decrement - convert to unary with special flag
            Token op = previous_;
            UnaryExpr::Op unaryOp = (op.type == TokenType::INC) ? UnaryExpr::Op::Inc : UnaryExpr::Op::Dec;
            expr = std::make_unique<UnaryExpr>(SourceLocation(op.line, op.column, op.offset),
                                               unaryOp, std::move(expr));
        } else {
            break;
        }
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::parseCall(std::unique_ptr<Expr> callee) {
    SourceLocation loc(previous_.line, previous_.column, previous_.offset);
    auto args = parseArguments();
    consume(TokenType::RPAREN, "Expected ')' after arguments");
    
    return std::make_unique<CallExpr>(loc, std::move(callee), std::move(args));
}

std::vector<std::unique_ptr<Expr>> Parser::parseArguments() {
    std::vector<std::unique_ptr<Expr>> args;
    
    if (!check(TokenType::RPAREN)) {
        do {
            args.push_back(parseExpression());
        } while (match(TokenType::COMMA));
    }
    
    return args;
}

std::unique_ptr<Expr> Parser::parseIndex(std::unique_ptr<Expr> base) {
    SourceLocation loc(previous_.line, previous_.column, previous_.offset);
    auto index = parseExpression();
    consume(TokenType::RBRACKET, "Expected ']' after index");
    
    return std::make_unique<IndexExpr>(loc, std::move(base), std::move(index));
}

std::unique_ptr<Expr> Parser::parseMember(std::unique_ptr<Expr> object) {
    Token op = previous_;
    bool isArrow = (op.type == TokenType::ARROW);
    
    Token member = consume(TokenType::IDENTIFIER, "Expected member name after '.' or '->'");
    SourceLocation loc(op.line, op.column, op.offset);
    
    return std::make_unique<MemberExpr>(loc, std::move(object), member.lexeme, isArrow);
}

std::unique_ptr<Expr> Parser::parsePrimary() {
    if (match(TokenType::INTEGER_LITERAL)) {
        return std::make_unique<LiteralExpr>(SourceLocation(previous_.line, previous_.column, previous_.offset),
                                             previous_.intValue);
    }
    
    if (match(TokenType::FLOAT_LITERAL)) {
        return std::make_unique<LiteralExpr>(SourceLocation(previous_.line, previous_.column, previous_.offset),
                                             previous_.floatValue);
    }
    
    if (match(TokenType::STRING_LITERAL)) {
        return std::make_unique<LiteralExpr>(SourceLocation(previous_.line, previous_.column, previous_.offset),
                                             previous_.stringValue);
    }
    
    if (match(TokenType::KW_TRUE)) {
        return std::make_unique<LiteralExpr>(SourceLocation(previous_.line, previous_.column, previous_.offset),
                                             true);
    }
    
    if (match(TokenType::KW_FALSE)) {
        return std::make_unique<LiteralExpr>(SourceLocation(previous_.line, previous_.column, previous_.offset),
                                             false);
    }
    
    if (match(TokenType::KW_NULL)) {
        return std::make_unique<LiteralExpr>(SourceLocation(previous_.line, previous_.column, previous_.offset));
    }
    
    if (match(TokenType::IDENTIFIER)) {
        return std::make_unique<IdentifierExpr>(SourceLocation(previous_.line, previous_.column, previous_.offset),
                                                previous_.lexeme);
    }
    
    if (match(TokenType::LPAREN)) {
        auto expr = parseExpression();
        consume(TokenType::RPAREN, "Expected ')' after expression");
        return expr;
    }
    
    if (match(TokenType::LBRACKET)) {
        SourceLocation loc(previous_.line, previous_.column, previous_.offset);
        std::vector<std::unique_ptr<Expr>> elements;
        
        if (!check(TokenType::RBRACKET)) {
            do {
                elements.push_back(parseExpression());
            } while (match(TokenType::COMMA));
        }
        
        consume(TokenType::RBRACKET, "Expected ']' after array elements");
        return std::make_unique<ArrayExpr>(loc, std::move(elements));
    }
    
    error("Expected expression");
    return std::make_unique<LiteralExpr>(SourceLocation(current_.line, current_.column, current_.offset), 0);
}

std::unique_ptr<Type> Parser::parseType() {
    if (match(TokenType::LBRACKET)) {
        return parseArrayType();
    }
    
    if (match(TokenType::BIT_AND) || match(TokenType::STAR)) {
        return parsePointerType();
    }
    
    if (match(TokenType::KW_FN)) {
        return parseFunctionType();
    }
    
    return parsePrimitiveType();
}

std::unique_ptr<Type> Parser::parsePrimitiveType() {
    Token name = consume(TokenType::IDENTIFIER, "Expected type name");
    SourceLocation loc(name.line, name.column, name.offset);
    
    static std::unordered_map<std::string, PrimitiveType::TypeCode> typeMap = {
        {"void", PrimitiveType::TypeCode::Void},
        {"bool", PrimitiveType::TypeCode::Bool},
        {"i8", PrimitiveType::TypeCode::I8},
        {"i16", PrimitiveType::TypeCode::I16},
        {"i32", PrimitiveType::TypeCode::I32},
        {"i64", PrimitiveType::TypeCode::I64},
        {"u8", PrimitiveType::TypeCode::U8},
        {"u16", PrimitiveType::TypeCode::U16},
        {"u32", PrimitiveType::TypeCode::U32},
        {"u64", PrimitiveType::TypeCode::U64},
        {"f32", PrimitiveType::TypeCode::F32},
        {"f64", PrimitiveType::TypeCode::F64},
        {"char", PrimitiveType::TypeCode::Char},
        {"string", PrimitiveType::TypeCode::String}
    };
    
    auto it = typeMap.find(name.lexeme);
    if (it != typeMap.end()) {
        return std::make_unique<PrimitiveType>(loc, it->second);
    }
    
    // Named type (struct, enum, or typedef)
    return std::make_unique<NamedType>(loc, name.lexeme);
}

std::unique_ptr<Type> Parser::parseArrayType() {
    SourceLocation loc(previous_.line, previous_.column, previous_.offset);
    auto elemType = parseType();
    
    size_t size = 0;
    if (match(TokenType::SEMICOLON)) {
        Token sizeToken = consume(TokenType::INTEGER_LITERAL, "Expected array size");
        size = static_cast<size_t>(sizeToken.intValue);
    }
    
    consume(TokenType::RBRACKET, "Expected ']' after array type");
    
    return std::make_unique<ArrayType>(loc, std::move(elemType), size);
}

std::unique_ptr<Type> Parser::parsePointerType() {
    SourceLocation loc(previous_.line, previous_.column, previous_.offset);
    bool isMut = match(TokenType::KW_MUT);
    auto pointee = parseType();
    
    return std::make_unique<PointerType>(loc, std::move(pointee), isMut);
}

std::unique_ptr<Type> Parser::parseFunctionType() {
    SourceLocation loc(previous_.line, previous_.column, previous_.offset);
    
    consume(TokenType::LPAREN, "Expected '(' after 'fn'");
    std::vector<std::unique_ptr<Type>> params;
    
    if (!check(TokenType::RPAREN)) {
        do {
            params.push_back(parseType());
        } while (match(TokenType::COMMA));
    }
    
    consume(TokenType::RPAREN, "Expected ')' after parameter types");
    
    std::unique_ptr<Type> retType = nullptr;
    if (match(TokenType::ARROW)) {
        retType = parseType();
    } else {
        retType = std::make_unique<PrimitiveType>(loc, PrimitiveType::TypeCode::Void);
    }
    
    return std::make_unique<FunctionType>(loc, std::move(params), std::move(retType));
}

} // namespace compiler
