#include <iostream>
#include <sstream>
#include <string>
#include <memory>

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
    std::shared_ptr<Closure<T>> p;
public:
    Closure<T> &get() const { return *p; }
    Parser(const Closure<T> &p) : p(p.clone()) {}
    T operator()(Source *s) const { return (*p)(s); }
};

/*
parseTest p s = case evalStateT p s of
    Right r     -> print r
    Left (e, _) -> putStrLn e
*/
template <typename T>
void parseTest(const Parser<T> &p, const char *s) {
    Source src = s;
    try {
        std::cout << p(&src) << std::endl;
    } catch (const std::string &e) {
        std::cout << e << std::endl;
    }
}

/*
anyChar = StateT $ anyChar where
    anyChar (x:xs) = Right (x, xs)
    anyChar    xs  = Left ("too short", xs)
*/
struct AnyChar : public Closure<char> {
    virtual Closure *clone() const { return new AnyChar; }
    virtual char operator()(Source *s) const {
        char ch = s->peek();
        s->next();
        return ch;
    }
};
Parser<char> anyChar = AnyChar();

/*
char c = satisfy (== c) <|> left ("not char " ++ show c)
*/
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

/*
satisfy f = StateT $ satisfy where
    satisfy (x:xs) | not $ f x = Left (": " ++ show x, x:xs)
    satisfy    xs              = runStateT anyChar xs
*/
class Satisfy : public Closure<char> {
    bool (*f)(char);
public:
    Satisfy(bool (*f)(char)) : f(f) {}
    virtual Closure *clone() const { return new Satisfy(f); }
    virtual char operator()(Source *s) const {
        char ch = s->peek();
        if (!f(ch)) throw s->ex(std::string("error: '") + ch + "'");
        s->next();
        return ch;
    }
};
Parser<char> satisfy(bool (*f)(char)) {
    return Satisfy(f);
}

/*
left e = StateT $ \s -> Left (e, s)
*/
template <typename T>
class Left : public Closure<T> {
    std::string msg;
public:
    Left(const std::string &msg) : msg(msg) {}
    virtual Closure<T> *clone() const { return new Left(msg); }
    virtual T operator()(Source *s) const {
        char ch = s->peek();
        throw s->ex(msg + ": '" + ch + "'");
    }
};
Parser<char> left(const std::string &msg) {
    return Left<char>(msg);
}
template <typename T>
Parser<T> left(const std::string &msg) {
    return Left<T>(msg);
}

/**/
template <typename T, typename T1>
class UnaryOperator : public Closure<T> {
protected:
    std::shared_ptr<Closure<T1>> p;
public:
    UnaryOperator(const Closure<T1> &p) : p(p.clone()) {}
};

/**/
template <typename T, typename T1, typename T2>
class BinaryOperator : public Closure<T> {
protected:
    std::shared_ptr<Closure<T1>> p1;
    std::shared_ptr<Closure<T2>> p2;
public:
    BinaryOperator(const Closure<T1> &p1, const Closure<T2> &p2) :
        p1(p1.clone()), p2(p2.clone()) {}
};

/*
many p = ((:) <$> p <*> many p) <|> return []
*/
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

/* sequence */
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

/*
replicate n _ | n < 1 = []
replicate n x         = x : replicate (n - 1) x
*/
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

/*
(StateT a) <|> (StateT b) = StateT f where
    f s0 =   (a  s0) <|> (b  s0) where
        Left (a, s1) <|> _ | s0 /= s1 = Left (     a, s1)
        Left (a, _ ) <|> Left (b, s2) = Left (b ++ a, s2)
        Left _       <|> b            = b
        a            <|> _            = a
*/
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

/*
try (StateT p) = StateT $ \s -> case p s of
    Left (e, _) -> Left (e, s)
    r           -> r 
*/
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

/*
string s = sequence [char x | x <- s]
*/
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

/*
import Data.Char
*/
#include <cctype>
bool isDigit   (char ch) { return std::isdigit(ch); }
bool isUpper   (char ch) { return std::isupper(ch); }
bool isLower   (char ch) { return std::islower(ch); }
bool isAlpha   (char ch) { return std::isalpha(ch); }
bool isAlphaNum(char ch) { return isalpha(ch) || isdigit(ch); }
bool isLetter  (char ch) { return isalpha(ch) || ch == '_';   }

/*
digit    = satisfy isDigit    <|> left "not digit"
upper    = satisfy isUpper    <|> left "not upper"
lower    = satisfy isLower    <|> left "not lower"
alpha    = satisfy isAlpha    <|> left "not alpha"
alphaNum = satisfy isAlphaNum <|> left "not alphaNum"
letter   = satisfy isLetter   <|> left "not letter"
*/
auto digit    = satisfy(isDigit   ) || left("not digit"   );
auto upper    = satisfy(isUpper   ) || left("not upper"   );
auto lower    = satisfy(isLower   ) || left("not lower"   );
auto alpha    = satisfy(isAlpha   ) || left("not alpha"   );
auto alphaNum = satisfy(isAlphaNum) || left("not alphaNum");
auto letter   = satisfy(isLetter  ) || left("not letter"  );

/*
test1  = sequence [anyChar, anyChar]
test2  = (++) <$> test1 <*> sequence [anyChar]
test3  = sequence [letter, digit, digit]
test4  = letter <|> digit
test5  = sequence [letter, digit, digit, digit]
test6  = sequence $ letter : replicate 3 digit
test7  = many letter
test8  = many (letter <|> digit)
test9  =      sequence [char 'a', char 'b']
         <|>  sequence [char 'a', char 'c']
test10 = try (sequence [char 'a', char 'b'])
         <|>  sequence [char 'a', char 'c']
test11 =      string "ab"  <|> string "ac"
test12 = try (string "ab") <|> string "ac"
test13 = sequence [char 'a', char 'b' <|> char 'c']
*/
auto test1  = anyChar + anyChar;
auto test2  = test1 + anyChar;
auto test3  = letter + digit + digit;
auto test4  = letter || digit;
auto test5  = letter + digit + digit + digit;
auto test6  = letter + 3 * digit;
auto test7  = many(letter);
auto test8  = many(letter || digit);
auto test9  =      char1('a') + char1('b')  || char1('a') + char1('c');
auto test10 = tryp(char1('a') + char1('b')) || char1('a') + char1('c');
auto test11 =      string("ab")  || string("ac");
auto test12 = tryp(string("ab")) || string("ac");
auto test13 = char1('a') + (char1('b') || char1('c'));

/*
main = do
    parseTest anyChar    "abc"
    parseTest test1      "abc"
    parseTest test2      "abc"
    parseTest test2      "12"      -- NG
    parseTest test2      "123"
    parseTest (char 'a') "abc"
    parseTest (char 'a') "123"     -- NG
    parseTest digit      "abc"     -- NG
    parseTest digit      "123"
    parseTest letter     "abc"
    parseTest letter     "123"     -- NG
    parseTest test3      "abc"     -- NG
    parseTest test3      "123"     -- NG
    parseTest test3      "a23"
    parseTest test3      "a234"
    parseTest test4      "a"
    parseTest test4      "1"
    parseTest test4      "!"       -- NG
    parseTest test5      "a123"
    parseTest test5      "ab123"   -- NG
    parseTest test6      "a123"
    parseTest test6      "ab123"   -- NG
    parseTest test7      "abc123"
    parseTest test7      "123abc"
    parseTest test8      "abc123"
    parseTest test8      "123abc"
    parseTest test9      "ab"
    parseTest test9      "ac"      -- NG
    parseTest test10     "ab"
    parseTest test10     "ac"
    parseTest test11     "ab"
    parseTest test11     "ac"      -- NG
    parseTest test12     "ab"
    parseTest test12     "ac"
    parseTest test13     "ab"
    parseTest test13     "ac"
*/
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
