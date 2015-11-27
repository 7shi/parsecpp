#include <iostream>
#include <sstream>
#include <string>
#include <list>
#include <numeric>
#include <functional>

template <typename T>
std::string toString(const std::list<T> &list) {
    std::stringstream ss;
    ss << "[";
    for (auto it = list.begin();
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
using Parser = std::function<T (Source *)>;

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
Parser<char> anyChar = [](Source *s) {
    char ch = s->peek();
    s->next();
    return ch;
};

/*
char c = satisfy (== c) <|> left ("not char " ++ show c)
*/
Parser<char> char1(char c) {
    return [=](Source *s) {
        char ch = s->peek();
        if (c != ch) {
            throw s->ex(std::string("not char '") + c + "': '" + ch + "'");
        }
        s->next();
        return ch;
    };
}

/*
satisfy f = StateT $ satisfy where
    satisfy (x:xs) | not $ f x = Left (": " ++ show x, x:xs)
    satisfy    xs              = runStateT anyChar xs
*/
Parser<char> satisfy(const std::function<bool (char)> &f) {
    return [=](Source *s) {
        char ch = s->peek();
        if (!f(ch)) throw s->ex(std::string("error: '") + ch + "'");
        s->next();
        return ch;
    };
}

/* right */
template <typename T>
Parser<T> right(const T &r) {
    return [=](Source *) {
        return r;
    };
}

/*
left e = StateT $ \s -> Left (e, s)
*/
template <typename T>
Parser<T> left(const std::string &msg) {
    return [=](Source *s) -> T {
        char ch = s->peek();
        throw s->ex(msg + ": '" + ch + "'");
    };
}
Parser<char> left(const std::string &msg) {
    return left<char>(msg);
}

/* >>, *> */
template <typename T1, typename T2>
Parser<T2> operator>>(const Parser<T1> &p1, const Parser<T2> &p2) {
    return [=](Source *s) {
        p1(s);
        return p2(s);
    };
}

/* <* */
template <typename T1, typename T2>
Parser<T1> operator<<(const Parser<T1> &p1, const Parser<T2> &p2) {
    return [=](Source *s) {
        T1 ret = p1(s);
        p2(s);
        return ret;
    };
}

/* sequence */
template <typename T1, typename T2>
Parser<std::string> operator+(const Parser<T1> &p1, const Parser<T2> &p2) {
    return [=](Source *s) {
        std::string ret;
        ret += p1(s);
        ret += p2(s);
        return ret;
    };
}

/*
replicate n _ | n < 1 = []
replicate n x         = x : replicate (n - 1) x
*/
template <typename T>
Parser<std::string> operator*(int n, const Parser<T> &p) {
    return [=](Source *s) {
        std::string ret;
        for (int i = 0; i < n; ++i) ret += p(s);
        return ret;
    };
}
template <typename T>
Parser<std::string> operator*(const Parser<T> &p, int n) {
    return n * p;
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
const Parser<T> operator||(const Parser<T> &p1, const Parser<T> &p2) {
    return [=](Source *s) {
        T ret;
        Source ss = *s;
        try {
            ret = p1(s);
        } catch (const std::string &e) {
            if (*s != ss) throw;
            ret = p2(s);
        }
        return ret;
    };
}

/*
try (StateT p) = StateT $ \s -> case p s of
    Left (e, _) -> Left (e, s)
    r           -> r 
*/
template <typename T>
Parser<T> tryp(const Parser<T> &p) {
    return [=](Source *s) {
        T ret;
        Source ss = *s;
        try {
            ret = p(s);
        } catch (const std::string &e) {
            *s = ss;
            throw;
        }
        return ret;
    };
}

/*
string s = sequence [char x | x <- s]
*/
Parser<std::string> string(const std::string &str) {
    return [=](Source *s) {
        for (int i = 0; i < str.length(); ++i) {
            char ch = s->peek();
            if (ch != str[i]) {
                throw s->ex(std::string("not string \"") + str + "\": '" + ch + "'");
            }
            s->next();
        }
        return str;
    };
}

/*
many p = ((:) <$> p <*> many p) <|> return []
*/
template <typename T>
Parser<std::string> many_(const Parser<T> &p) {
    return [=](Source *s) {
        std::string ret;
        try {
            for (;;) ret += p(s);
        } catch (const std::string &e) {}
        return ret;
    };
}
Parser<std::string> many(const Parser<char> &p) {
    return many_(p);
}
Parser<std::string> many(const Parser<std::string> &p) {
    return many_(p);
}
template <typename T>
Parser<std::list<T>> many(const Parser<T> &p) {
    return [=](Source *s) {
        std::list<T> ret;
        try {
            for (;;) ret.push_back(p(s));
        } catch (const std::string &e) {}
        return ret;
    };
}

/*
many1 p = (:) <$> p <*> many p
*/
Parser<std::string> many1(const Parser<char> &p) {
    return p + many(p);
}
Parser<std::string> many1(const Parser<std::string> &p) {
    return p + many(p);
}
template <typename T>
Parser<std::list<T>> many1(const Parser<T> &p) {
    return [=](Source *s) {
        std::list<T> ret;
        ret.push_back(p(s));
        try {
            for (;;) ret.push_back(p(s));
        } catch (const std::string &e) {}
        return ret;
    };
}

/*
skipMany p = many p *> return ()
*/
template <typename T>
Parser<std::string> skipMany(const Parser<T> &p) {
    return many(p) >> right<std::string>("");
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
bool isSpace   (char ch) { return ch == '\t'  || ch == ' ';   }

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
auto space    = satisfy(isSpace   ) || left("not space"   );

/*
spaces = skipMany space
*/
auto spaces = skipMany(space);
