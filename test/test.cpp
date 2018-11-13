#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <fstream>
#include "doctest.h"
#include "test.h"

const std::string TestDir(TEST_DIR);

char *load(std::string const& name, int& len)
{
    std::ifstream is;
    is.open(TestDir + '/' + name, std::ios::binary);
    is.seekg(0, std::ios::end);
    len = is.tellg();
    is.seekg(0, std::ios::beg);
    char *buf = new char[len];
    is.read(&buf[0], len);
    is.close();
    return buf;
}

