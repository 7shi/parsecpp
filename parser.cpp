#include <iostream>
#include <sstream>
#include <string>
#include <list>
#include <cctype>
#include <cstdarg>

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

template<typename T> struct Parser {
    virtual ~Parser() {}
    virtual Parser *clone() const = 0;
    virtual T operator()(Source *s) const = 0;
};

template<typename T> void parseTest(const Parser<T> &p, const char *s) {
    Source src = s;
    try {
        std::cout << p(&src) << std::endl;
    } catch (const std::string &e) {
        std::cout << e << std::endl;
    }
}

class AnyChar : public Parser<char> {
protected:
    virtual void check(Source *, char) const {}

public:
    virtual Parser *clone() const { return new AnyChar; }
    virtual char operator()(Source *s) const {
        char ch = s->peek();
        check(s, ch);
        s->next();
        return ch;
    };
} anyChar;

class char1 : public AnyChar {
    char ch;

public:
    char1(char ch) : ch(ch) {}
    virtual Parser *clone() const { return new char1(ch); }

protected:
    virtual void check(Source *s, char ch) const {
        if (this->ch != ch) {
            throw s->ex(std::string("char '") + this->ch + "': '" + ch + "'");
        }
    }
};

class satisfy : public AnyChar {
    bool (*f)(char);
    std::string err;

public:
    satisfy(bool (*f)(char), const std::string &err) : f(f), err(err) {}
    virtual Parser *clone() const { return new satisfy(f, err); }

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

struct Test1 : public Parser<std::string> {
    virtual Parser *clone() const { return new Test1; }
    virtual std::string operator()(Source *s) const {
        char x1 = anyChar(s);
        char x2 = anyChar(s);
        char ret[] = { x1, x2, 0 };
        return ret;
    }
} test1;

struct Test2 : public Parser<std::string> {
    virtual Parser *clone() const { return new Test2; }
    virtual std::string operator()(Source *s) const {
        std::string x1 = test1(s);
        char x2 = anyChar(s);
        return x1 + x2;
    }
} test2;

struct Test3 : public Parser<std::string> {
    virtual Parser *clone() const { return new Test3; }
    virtual std::string operator()(Source *s) const {
        char x1 = letter(s);
        char x2 = digit(s);
        char x3 = digit(s);
        char ret[] = { x1, x2, x3, 0 };
        return ret;
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
