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
    bool operator==(const Source &s) {
        return p == s.p && line == s.line && col == s.col;
    }
    bool operator!=(const Source &s) {
        return !(*this == s);
    }
};

template <typename T>
struct Closure {
    virtual ~Closure() {}
    virtual Closure *clone() const = 0;
    virtual T operator()(Source *s) const = 0;
};

template <typename T>
class Parser {
    Closure<T> *p;
public:
    Closure<T> &get() const { return *p; }
    Parser(const Closure<T> &p) : p(p.clone()) {}
    ~Parser() { delete p; }
    T operator()(Source *s) const { return (*p)(s); }
};

template <typename T>
void parseTest(const Parser<T> &p, const char *s) {
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
    }
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
            throw s->ex(std::string("not char '") + this->ch + "': '" + ch + "'");
        }
        s->next();
        return ch;
    }
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
    }
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

template <typename T, typename T1>
class UnaryOperator : public Closure<T> {
protected:
    Closure<T1> *p;
public:
    UnaryOperator(const Closure<T1> &p) : p(p.clone()) {}
    virtual ~UnaryOperator() { delete p; }
};

template <typename T, typename T1, typename T2>
class BinaryOperator : public Closure<T> {
protected:
    Closure<T1> *p1;
    Closure<T2> *p2;

public:
    BinaryOperator(const Closure<T1> &p1, const Closure<T2> &p2) :
        p1(p1.clone()), p2(p2.clone()) {}
    virtual ~BinaryOperator() { delete p1; delete p2; }
};

template <typename T>
struct Many : public UnaryOperator<std::string, T> {
    Many(const Closure<T> &p) : UnaryOperator<std::string, T>(p) {}
    virtual Closure<std::string> *clone() const { return new Many<T>(*this->p); }
    virtual std::string operator()(Source *s) const {
        std::string ret;
        try {
            for (;;) ret += (*this->p)(s);
        } catch (const std::string &e) {}
        return ret;
    }
};
template <typename T>
Parser<std::string> many(const Parser<T> &p) {
    return Many<T>(p.get());
}

template <typename T1, typename T2>
struct Sequence : public BinaryOperator<std::string, T1, T2> {
    Sequence(const Closure<T1> &p1, const Closure<T2> &p2) :
        BinaryOperator<std::string, T1, T2>(p1, p2) {}
    virtual Closure<std::string> *clone() const {
        return new Sequence(*this->p1, *this->p2);
    }
    virtual std::string operator()(Source *s) const {
        std::string ret;
        ret += (*this->p1)(s);
        ret += (*this->p2)(s);
        return ret;
    }
};
template <typename T1, typename T2>
Parser<std::string> operator+(const Parser<T1> &p1, const Parser<T2> &p2) {
    return Sequence<T1, T2>(p1.get(), p2.get());
}

template <typename T>
class Replicate : public UnaryOperator<std::string, T> {
    int n;
public:
    Replicate(int n, const Closure<T> &p) :
        UnaryOperator<std::string, T>(p), n(n) {}
    virtual Closure<std::string> *clone() const {
        return new Replicate(n, *this->p);
    }
    virtual std::string operator()(Source *s) const {
        std::string ret;
        for (int i = 0; i < n; ++i) ret += (*this->p)(s);
        return ret;
    }
};
template <typename T>
Parser<std::string> operator*(int n, const Parser<T> &p) {
    return Replicate<T>(n, p.get());
}
template <typename T>
Parser<std::string> operator*(const Parser<T> &p, int n) {
    return Replicate<T>(n, p.get());
}

template <typename T>
struct Or : public BinaryOperator<T, T, T> {
    Or(const Closure<T> &p1, const Closure<T> &p2) :
        BinaryOperator<T, T, T>(p1, p2) {}
    virtual Closure<T> *clone() const { return new Or(*this->p1, *this->p2); }
    virtual T operator()(Source *s) const {
        T ret;
        Source ss = *s;
        try {
            ret = (*this->p1)(s);
        } catch (const std::string &e) {
            if (*s != ss) throw;
            ret = (*this->p2)(s);
        }
        return ret;
    }
};
template <typename T>
Parser<T> operator||(const Parser<T> &p1, const Parser<T> &p2) {
    return Or<T>(p1.get(), p2.get());
}

template <typename T>
struct Try : public UnaryOperator<T, T> {
    Try(const Closure<T> &p) : UnaryOperator<T, T>(p) {}
    virtual Closure<T> *clone() const { return new Try<T>(*this->p); }
    virtual T operator()(Source *s) const {
        T ret;
        Source ss = *s;
        try {
            ret = (*this->p)(s);
        } catch (const std::string &e) {
            *s = ss;
            throw;
        }
        return ret;
    }
};
template <typename T>
Parser<T> tryp(const Parser<T> &p) {
    return Try<T>(p.get());
}

class String : public Closure<std::string> {
    std::string str;
public:
    String(const std::string &str) : str(str) {}
    virtual Closure *clone() const { return new String(str); }
    virtual std::string operator()(Source *s) const {
        for (int i = 0; i < str.length(); ++i) {
            char ch = s->peek();
            if (ch != str[i]) {
                throw s->ex(std::string("not string \"") + str + "\": '" + ch + "'");
            }
            s->next();
        }
        return str;
    }
};
Parser<std::string> string(const std::string &str) {
    return String(str);
}

Parser<std::string> test1  = anyChar + anyChar;
Parser<std::string> test2  = test1 + anyChar;
Parser<std::string> test3  = letter + digit + digit;
Parser<char>        test4  = letter || digit;
Parser<std::string> test5  = letter + digit + digit + digit;
Parser<std::string> test6  = letter + 3 * digit;
Parser<std::string> test7  = many(letter);
Parser<std::string> test8  = many(letter || digit);
Parser<std::string> test9  =      char1('a') + char1('b')  || char1('a') + char1('c');
Parser<std::string> test10 = tryp(char1('a') + char1('b')) || char1('a') + char1('c');
Parser<std::string> test11 =      string("ab")  || string("ac");
Parser<std::string> test12 = tryp(string("ab")) || string("ac");
Parser<std::string> test13 = char1('a') + (char1('b') || char1('c'));

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
    parseTest(test5     , "a123"  );
    parseTest(test5     , "ab123" );  // NG
    parseTest(test6     , "a123"  );
    parseTest(test6     , "ab123" );  // NG
    parseTest(test7     , "abc123");
    parseTest(test7     , "123abc");
    parseTest(test8     , "abc123");
    parseTest(test8     , "123abc");
    parseTest(test9     , "ab"    );
    parseTest(test9     , "ac"    );  // NG
    parseTest(test10    , "ab"    );
    parseTest(test10    , "ac"    );
    parseTest(test11    , "ab"    );
    parseTest(test11    , "ac"    );  // NG
    parseTest(test12    , "ab"    );
    parseTest(test12    , "ac"    );
    parseTest(test13    , "ab"    );
    parseTest(test13    , "ac"    );
}
