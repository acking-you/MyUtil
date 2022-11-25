//
// Created by Alone on 2022-7-20.
//

#ifndef MYUTIL_INIFILE_H
#define MYUTIL_INIFILE_H
#include <variant>
using std::variant;

#include<string>
using std::string;

#include<sstream>
using std::stringstream;

#include<string_view>
using std::string_view;

#include<map>
using std::map;

class Value{
public:
    Value() = default;
    using value_t = string;

    Value(const char* value):m_value(value){}
    Value(string value):m_value(std::move(value)){}
    //parse int
    operator int();
    //parse bool
    operator bool();
    //parse string
    operator string();
    //parse double
    operator double();

    //input to Value
    template<class T>
    Value& operator=(T value){
        stringstream ss;
        ss<<value;
        ss>>m_value;
        return *this;
    }
private:
    value_t m_value;
};


class IniFile {
public:
    using Section = map<string,Value>;

    IniFile() = default;
    explicit IniFile(string_view filename);

    bool Load(string_view filename);
    bool Save(string_view filename);
    void Show();
    void Clear();

    Section& operator[](string_view name){
        return m_inifile[name.data()];
    }

private:
    static string trim(string src);

private:
    string m_filename;
    map<string,Section> m_inifile;
};


#endif //MYUTIL_INIFILE_H
