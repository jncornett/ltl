#ifndef LUA_EXECPTION_H
#define LUA_EXECPTION_H

#include <string>
#include <sstream>

namespace Ltl
{

class Exception
{
public:
    virtual std::string what() = 0;
};

class TypeError : public Exception
{
public:
    TypeError(std::string expected, std::string actual = "") :
        expected { expected }, actual { actual } { }

    virtual std::string what() override
    {
        std::ostringstream oss;
        oss << "TypeError: expected " << expected << ", got " << expected;
        return oss.str();
    }

    std::string expected;
    std::string actual;
};

}

#endif
