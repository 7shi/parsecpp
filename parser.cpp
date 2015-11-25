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

template<typename T> struct Closure {
    virtual ~Closure() {}
    virtual Closure *clone() const = 0;
    virtual T operator()(Source *s) const = 0;
};

template<typename T> class Parser {
    Closure<T> *f;
public:
    Closure<T> &get() const { return *f; }
    Parser(const Closure<T> &f) : f(f.clone()) {}
    ~Parser() { delete f; }
    T operator()(Source *s) const { return (*f)(s); }
};

template<typename T> void parseTest(const Parser<T> &p, const char *s) {
    Source src = s;
    try {
        std::cout << p(&src) << std::endl;
    } catch (const std::string &e) {
        std::cout << e << std::endl;
    }
}

struct AnyChar : public Closure<char> {
    virtual Closure *clone() const { return new AnyChar; }
    virtual char operator()(Source *s) const {
        char ch = s->peek();
        s->next();
        return ch;
    };
};
Parser<char> anyChar = AnyChar();

class Char1 : public Closure<char> {
    char ch;

public:
    Char1(char ch) : ch(ch) {}
    virtual Closure *clone() const { return new Char1(ch); }
    virtual char operator()(Source *s) const {
        char ch = s->peek();
        if (this->ch != ch) {
            throw s->ex(std::string("char '") + this->ch + "': '" + ch + "'");
        }
        s->next();
        return ch;
    };
};
Parser<char> char1(char ch) { return Char1(ch); }

class Satisfy : public Closure<char> {
    bool (*f)(char);
    std::string err;

public:
    Satisfy(bool (*f)(char), const std::string &err) : f(f), err(err) {}
    virtual Closure *clone() const { return new Satisfy(f, err); }
    virtual char operator()(Source *s) const {
        char ch = s->peek();
        if (!f(ch)) throw s->ex("not " + err + ": '" + ch + "'");
        s->next();
        return ch;
    };
};
Parser<char> satisfy(bool (*f)(char), const std::string &err = "???") {
    return Satisfy(f, err);
}

bool isDigit   (char ch) { return std::isdigit(ch); }
bool isUpper   (char ch) { return std::isupper(ch); }
bool isLower   (char ch) { return std::islower(ch); }
bool isAlpha   (char ch) { return std::isalpha(ch); }
bool isAlphaNum(char ch) { return isalpha(ch) || isdigit(ch); }
bool isLetter  (char ch) { return isalpha(ch) || ch == '_';   }

Parser<char> digit    = satisfy(isDigit   , "digit"   );
Parser<char> upper    = satisfy(isUpper   , "upper"   );
Parser<char> lower    = satisfy(isLower   , "lower"   );
Parser<char> alpha    = satisfy(isAlpha   , "alpha"   );
Parser<char> alphaNum = satisfy(isAlphaNum, "alphaNum");
Parser<char> letter   = satisfy(isLetter  , "letter"  );

template<typename T, typename T1>
class UnaryOperator : public Closure<T> {
protected:
    Closure<T1> *p;

public:
    UnaryOperator(const Closure<T1> &p) : p(p.clone()) {}
    virtual ~UnaryOperator() { delete p; }
};

template<typename T, typename T1, typename T2>
class BinaryOperator : public Closure<T> {
protected:
    Closure<T1> *p1;
    Closure<T2> *p2;

public:
    BinaryOperator(const Closure<T1> &p1, const Closure<T2> &p2) :
        p1(p1.clone()), p2(p2.clone()) {}
    virtual ~BinaryOperator() { delete p1; delete p2; }
};

struct CharToString : public UnaryOperator<std::string, char> {
    CharToString(const Closure<char> &p) : UnaryOperator(p) {}
    virtual Closure *clone() const { return new CharToString(*p); }

    virtual std::string operator()(Source *s) const {
        return std::string(1, (*p)(s));
    }
};

struct Many : public UnaryOperator<std::string, std::string> {
    Many(const Closure<std::string> &p) : UnaryOperator(p) {}
    virtual Closure *clone() const { return new Many(*p); }

    virtual std::string operator()(Source *s) const {
        std::string ret;
        try {
            for (;;) ret += (*p)(s);
        } catch (const std::string &e) {}
        return ret;
    }
};
Parser<std::string> many(const Parser<char> &p) {
    return Many(CharToString(p.get()));
}
Parser<std::string> many(const Parser<std::string> &p) {
    return Many(p.get());
}

struct Sequence : public BinaryOperator<std::string, std::string, std::string> {
    Sequence(const Closure<std::string> &p1, const Closure<std::string> &p2) :
        BinaryOperator(p1, p2) {}
    virtual Closure *clone() const {
        return new Sequence(*p1, *p2);
    }

    virtual std::string operator()(Source *s) const {
        std::string ret;
        ret += (*p1)(s);
        ret += (*p2)(s);
        return ret;
    }
};
Sequence operator+(const Parser<char> &p1, const Parser<char> &p2) {
    return Sequence(CharToString(p1.get()), CharToString(p2.get()));
}
Sequence operator+(const Parser<char> &p1, const Parser<std::string> &p2) {
    return Sequence(CharToString(p1.get()), p2.get());
}
Sequence operator+(const Parser<std::string> &p1, const Parser<char> &p2) {
    return Sequence(p1.get(), CharToString(p2.get()));
}
Sequence operator+(const Parser<std::string> &p1, const Parser<std::string> &p2) {
    return Sequence(p1.get(), p2.get());
}

template<typename T> struct Or : public BinaryOperator<T, T, T> {
    Or(const Closure<T> &p1, const Closure<T> &p2) :
        BinaryOperator<T, T, T>(p1, p2) {}
    virtual Closure<T> *clone() const { return new Or(*this->p1, *this->p2); }

    virtual T operator()(Source *s) const {
        T ret;
        try {
            ret = (*this->p1)(s);
        } catch (const std::string &e) {
            ret = (*this->p2)(s);
        }
        return ret;
    }
};
template<typename T> Parser<T> operator||(const Parser<T> &p1, const Parser<T> &p2) {
    return Or<T>(p1.get(), p2.get());
}

Parser<std::string> test1 = anyChar + anyChar;
Parser<std::string> test2 = test1 + anyChar;
Parser<std::string> test3 = letter + digit + digit;
Parser<char> test4 = letter || digit;
Parser<std::string> test7 = many(letter);
Parser<std::string> test8 = many(letter || digit);

int main() {
    parseTest(anyChar   , "abc"   );
    parseTest(test1     , "abc"   );
    parseTest(test2     , "abc"   );
    parseTest(test2     , "12"    );  // NG
    parseTest(test2     , "123"   );
    parseTest(char1('a'), "abc"   );
    parseTest(char1('a'), "123"   );  // NG
    parseTest(digit     , "abc"   );  // NG
    parseTest(digit     , "123"   );
    parseTest(letter    , "abc"   );
    parseTest(letter    , "123"   );  // NG
    parseTest(test3     , "abc"   );  // NG
    parseTest(test3     , "123"   );  // NG
    parseTest(test3     , "a23"   );
    parseTest(test3     , "a234"  );
    parseTest(test4     , "a"     );
    parseTest(test4     , "1"     );
    parseTest(test4     , "!"     );  // NG
    parseTest(test7     , "abc123");
    parseTest(test7     , "123abc");
    parseTest(test8     , "abc123");
    parseTest(test8     , "123abc");
}
