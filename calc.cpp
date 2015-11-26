#include <iostream>
#include <sstream>
#include <string>
#include <list>
#include <numeric>

template <typename T>
std::string toString(const std::list<T> &list) {
    std::stringstream ss;
    ss << "[";
    for (typename std::list<T>::const_iterator it = list.begin();
            it != list.end(); ++it) {
        if (it != list.begin()) ss << ",";
        ss << *it;
    }
    ss << "]";
    return ss.str();
}
template <typename T>
std::ostream &operator<<(std::ostream &cout, const std::list<T> &list) {
    return cout << toString(list);
}

/* : */
template <typename T>
std::list<T> operator+(T x, const std::list<T> &list) {
    std::list<T> ret = list;
    ret.push_front(x);
    return ret;
}

/* sum */
template <typename T>
T sum(const std::list<T> &list) {
    return std::accumulate(list.begin(), list.end(), 0);
}

class Source {
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
    Closure<T1> *p;
public:
    UnaryOperator(const Closure<T1> &p) : p(p.clone()) {}
    virtual ~UnaryOperator() { delete p; }
};

/**/
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
Parser<std::string> many(const Parser<char> &p) {
    return Many<char>(p.get());
}
Parser<std::string> many(const Parser<std::string> &p) {
    return Many<std::string>(p.get());
}
template <typename T>
struct ManyList : public UnaryOperator<std::list<T>, T> {
    ManyList(const Closure<T> &p) : UnaryOperator<std::list<T>, T>(p) {}
    virtual Closure< std::list<T> > *clone() const { return new ManyList<T>(*this->p); }
    virtual std::list<T> operator()(Source *s) const {
        std::list<T> ret;
        try {
            for (;;) ret.push_back((*this->p)(s));
        } catch (const std::string &e) {}
        return ret;
    }
};
template <typename T>
Parser< std::list<T> > many(const Parser<T> &p) {
    return ManyList<T>(p.get());
}

/*
many1 p = (:) <$> p <*> many p
*/
template <typename T>
struct Many1 : public UnaryOperator<std::list<T>, T> {
    Many1(const Closure<T> &p) : UnaryOperator<std::list<T>, T>(p) {}
    virtual Closure< std::list<T> > *clone() const { return new Many1<T>(*this->p); }
    virtual std::list<T> operator()(Source *s) const {
        std::list<T> ret;
        ret.push_back((*this->p)(s));
        try {
            for (;;) ret.push_back(*this->p)(s);
        } catch (const std::string &e) {}
        return ret;
    }
};
Parser<std::string> many1(const Parser<char> &p) {
    return p + many(p);
}
Parser<std::string> many1(const Parser<std::string> &p) {
    return p + many(p);
}
template <typename T>
Parser< std::list<T> > many1(const Parser<T> &p) {
    return Many1<T>(p.get());
}

/* >>, *> */
template <typename T1, typename T2>
struct ReturnRight : public BinaryOperator<T2, T1, T2> {
    ReturnRight(const Closure<T1> &p1, const Closure<T2> &p2) :
        BinaryOperator<T2, T1, T2>(p1, p2) {}
    virtual Closure<T2> *clone() const {
        return new ReturnRight(*this->p1, *this->p2);
    }
    virtual T2 operator()(Source *s) const {
        (*this->p1)(s);
        return (*this->p2)(s);
    }
};
template <typename T1, typename T2>
Parser<T2> operator>>(const Parser<T1> &p1, const Parser<T2> &p2) {
    return ReturnRight<T1, T2>(p1.get(), p2.get());
}

/* >>= */
template <typename T1, typename T2>
class Bind : public UnaryOperator<T2, T1> {
    T2 (*f)(T1);
public:
    Bind(const Closure<T1> &p, T2 (*f)(T1)) : UnaryOperator<T2, T1>(p), f(f) {}
    virtual Closure<T2> *clone() const { return new Bind<T1, T2>(*this->p, f); }
    virtual T2 operator()(Source *s) const { return f((*this->p)(s)); }
};
template <typename T1, typename T2>
Parser<T2> operator>=(const Parser<T1> &p, T2 (*f)(T1)) {
    return Bind<T1, T2>(p.get(), f);
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
Parser<char> digit    = satisfy(isDigit   ) || left("not digit"   );
Parser<char> upper    = satisfy(isUpper   ) || left("not upper"   );
Parser<char> lower    = satisfy(isLower   ) || left("not lower"   );
Parser<char> alpha    = satisfy(isAlpha   ) || left("not alpha"   );
Parser<char> alphaNum = satisfy(isAlphaNum) || left("not alphaNum");
Parser<char> letter   = satisfy(isLetter  ) || left("not letter"  );

/*
number = do
    x <- many1 digit
    return (read x :: Int)  -- •ÏŠ·
*/
struct Number : public Closure<int> {
    virtual Closure *clone() const { return new Number; }
    virtual int operator()(Source *s) const {
        std::string x = many1(digit)(s);
        int ret;
        std::istringstream(x) >> ret;
        return ret;
    }
};
Parser<int> number = Number();

/* add x = (+ x) -- etc */
enum IntOp { Add, Sub, Mul, Div };
class Section {
    IntOp op;
    int y;
public:
    Section() {}
    Section(IntOp op, int y) : op(op), y(y) {}
    int operator()(int x) const {
        switch (op) {
        case Add: return x + y;
        case Sub: return x - y;
        case Mul: return x * y;
        case Div: return x / y;
        }
        throw std::string("invalid operator");
    }
};
Section add(int y) { return Section(Add, y); }
Section sub(int y) { return Section(Sub, y); }
Section mul(int y) { return Section(Mul, y); }
Section div(int y) { return Section(Div, y); }
int foldl(int x, const std::list<Section> &xs) {
    for (std::list<Section>::const_iterator it = xs.begin(); it != xs.end(); ++it) {
        x = (*it)(x);
    }
    return x;
}

/*
term = do
    x  <- number
    fs <- many $ do
            char '*'
            y <- number
            return (* y)
        <|> do
            char '/'
            y <- number
            return (`div` y)
    return $ foldl (\x f -> f x) x fs
*/
struct Term : public Closure<int> {
    virtual Closure *clone() const { return new Term; }
    virtual int operator()(Source *s) const {
        int x = number(s);
        std::list<Section> xs = many(
               char1('*') >> number >= mul
            || char1('/') >> number >= div
        )(s);
        return foldl(x, xs);
    }
};
Parser<int> term = Term();

/*
expr = do
    x  <- term
    fs <- many $ do
            char '+'
            y <- term
            return (+ y)
        <|> do
            char '-'
            y <- term
            return $ subtract y
    return $ foldl (\x f -> f x) x fs
*/
struct Expr : public Closure<int> {
    virtual Closure *clone() const { return new Expr; }
    virtual int operator()(Source *s) const {
        int x = term(s);
        std::list<Section> xs = many(
               char1('+') >> term >= add
            || char1('-') >> term >= sub
        )(s);
        return foldl(x, xs);
    }
};
Parser<int> expr = Expr();

/*
main = do
    parseTest number "123"
    parseTest expr   "1+2"
    parseTest expr   "123"
    parseTest expr   "1+2+3"
    parseTest expr   "1-2-3"
    parseTest expr   "1-2+3"
    parseTest expr   "2*3+4"
    parseTest expr   "2+3*4"
    parseTest expr   "100/10/2"
*/
int main() {
    parseTest(number, "123");
    parseTest(expr  , "1+2");
    parseTest(expr  , "123");
    parseTest(expr  , "1+2+3");
    parseTest(expr  , "1-2-3");
    parseTest(expr  , "1-2+3");
    parseTest(expr  , "2*3+4");
    parseTest(expr  , "2+3*4");
    parseTest(expr  , "100/10/2");
}
