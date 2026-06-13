#ifndef RESPPARSER_H
#define RESPPARSER_H

#include <string>
#include <vector>

using namespace std;
class RESPParser{

public:
    vector<string> parse(const string& raw_input);
    string serializeSimpleString(const string& str);
    string serializeError(const string& err);
    string serializeInteger(const int val);
};

#endif