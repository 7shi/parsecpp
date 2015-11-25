#include <iostream>
#include <sstream>
#include <string>
#include <cctype>

class Source {
private:
    const char *p;
    int line, col;

public:
    Source(const char *p) : p(p), line(1), col(1) {}

    char peek() {
        if (!*p) throw ex("too short");
        return *p;
    }

    void next() {
        if (!*p) throw ex("at last");
        if (*p == '\n') {
            ++line;
            col = 0;
        }
        ++p;
        ++col;
    }

    std::string ex(const std::string &msg) {
        std::stringstream ss;
        ss << "[line " << line << ", col " << col << "] " << msg;
        return ss.str();
    }
};

class Parser {
protected:
    virtual void check(Source *, char) const {}

public:
    virtual std::string operator()(Source *s) const {
        char ch = s->peek();
        check(s, ch);
        s->next();
        return std::string(1, ch);
    };
} anyChar;

void parseTest(const Parser &p, const char *s) {
    Source src = s;
    try {
        std::cout << p(&src) << std::endl;
    } catch (std::string e) {
        std::cout << e << std::endl;
    }
}

class char1 : public Parser {
    char ch;

public:
    char1(char ch) : ch(ch) {}

protected:
    virtual void check(Source *s, char ch) const {
        if (this->ch != ch) {
            throw s->ex(std::string("char '") + this->ch + "': '" + ch + "'");
        }
    }
};

class satisfy : public Parser {
    bool (*f)(char);
    std::string err;

public:
    satisfy(bool (*f)(char), const std::string &err) : f(f), err(err) {}

protected:
    virtual void check(Source *s, char ch) const {
        if (!f(ch)) throw s->ex("not " + err + ": '" + ch + "'");
    }
};

bool isDigit   (char ch) { return std::isdigit(ch); }
bool isUpper   (char ch) { return std::isupper(ch); }
bool isLower   (char ch) { return std::islower(ch); }
bool isAlpha   (char ch) { return std::isalpha(ch); }
bool isAlphaNum(char ch) { return isalpha(ch) || isdigit(ch); }
bool isLetter  (char ch) { return isalpha(ch) || ch == '_';   }

satisfy digit   (isDigit   , "digit"   );
satisfy upper   (isUpper   , "upper"   );
satisfy lower   (isLower   , "lower"   );
satisfy alpha   (isAlpha   , "alpha"   );
satisfy alphaNum(isAlphaNum, "alphaNum");
satisfy letter  (isLetter  , "letter"  );

struct Test1 : public Parser {
    virtual std::string operator()(Source *s) const {
        std::string x1 = anyChar(s);
        std::string x2 = anyChar(s);
        return x1 + x2;
    }
} test1;

struct Test2 : public Parser {
    virtual std::string operator()(Source *s) const {
        std::string x1 = test1(s);
        std::string x2 = anyChar(s);
        return x1 + x2;
    }
} test2;

struct Test3 : public Parser {
    virtual std::string operator()(Source *s) const {
        std::string x1 = letter(s);
        std::string x2 = digit(s);
        std::string x3 = digit(s);
        return x1 + x2 + x3;
    }
} test3;

int main() {
    parseTest(anyChar   , "abc" );
    parseTest(test1     , "abc" );
    parseTest(test2     , "abc" );
    parseTest(test2     , "12"  );  // NG
    parseTest(test2     , "123" );
    parseTest(char1('a'), "abc" );
    parseTest(char1('a'), "123" );  // NG
    parseTest(digit     , "abc" );  // NG
    parseTest(digit     , "123" );
    parseTest(letter    , "abc" );
    parseTest(letter    , "123" );  // NG
    parseTest(test3     , "abc" );  // NG
    parseTest(test3     , "123" );  // NG
    parseTest(test3     , "a23" );
    parseTest(test3     , "a234");
}
