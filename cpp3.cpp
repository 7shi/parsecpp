#include "parsecpp.cpp"

/*
number = do
    x <- many1 digit
    return (read x :: Int)
*/
Parser<int> number = [](Source *s) {
    std::string x = many1(digit)(s);
    int ret;
    std::istringstream(x) >> ret;
    return ret;
};

/*
eval m fs = foldl (\x f -> f x) <$> m <*> fs
*/
Parser<int> eval(
        const Parser<int> &m,
        const Parser<std::list<std::function<int (int)>>> &fs) {
    return [=](Source *s) {
        int x = m(s);
        auto xs = fs(s);
        for (auto it = xs.begin(); it != xs.end(); ++it) {
            x = (*it)(x);
        }
        return x;
    };
}

/*
apply f m = flip f <$> m
*/
Parser<std::function<int (int)>> apply(
        const std::function<int (int, int)> &f,
        Parser<int> m) {
    return [=](Source *s) {
        int y = m(s);
        return [=](int x) {
            return f(x, y);
        };
    };
}

// a forward declaration and a wrapper
extern Parser<int> factor_;
Parser<int> factor = [](Source *s) { return factor_(s); };

/*
-- term = factor, {("*", factor) | ("/", factor)}
term = eval factor $ many $
        char '*' *> apply (*) factor
    <|> char '/' *> apply div factor
*/
auto term = eval(factor, many(
       char1('*') >> apply([](int x, int y) { return x * y; }, factor)
    || char1('/') >> apply([](int x, int y) { return x / y; }, factor)
));

/*
-- expr = term, {("+", term) | ("-", term)}
expr = eval term $ many $
        char '+' *> apply (+) term
    <|> char '-' *> apply (-) term
*/
auto expr = eval(term, many(
       char1('+') >> apply([](int x, int y) { return x + y; }, term)
    || char1('-') >> apply([](int x, int y) { return x - y; }, term)
));

/*
-- factor = [spaces], ("(", expr, ")") | number, [spaces]
factor = spaces
      *> (char '(' *> expr <* char ')' <|> number)
     <*  spaces
*/
Parser<int> factor_ = spaces
                   >> (char1('(') >> expr << char1(')') || number)
                   << spaces;

/*
main = do
    parseTest number "123"
    parseTest expr   "1 + 2"
    parseTest expr   "123"
    parseTest expr   "1 + 2 + 3"
    parseTest expr   "1 - 2 - 3"
    parseTest expr   "1 - 2 + 3"
    parseTest expr   "2 * 3 + 4"
    parseTest expr   "2 + 3 * 4"
    parseTest expr   "100 / 10 / 2"
    parseTest expr   "( 2 + 3 ) * 4"
*/
int main() {
    parseTest(number, "123");
    parseTest(expr  , "1 + 2");
    parseTest(expr  , "123");
    parseTest(expr  , "1 + 2 + 3");
    parseTest(expr  , "1 - 2 - 3");
    parseTest(expr  , "1 - 2 + 3");
    parseTest(expr  , "2 * 3 + 4");
    parseTest(expr  , "2 + 3 * 4");
    parseTest(expr  , "100 / 10 / 2");
    parseTest(expr  , "( 2 + 3 ) * 4");
}
