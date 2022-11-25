//
// Created by Alone on 2022-8-8.
//
#include<iostream>
#include <fstream>
#include<string>
#include"Parser.h"
#include <Timer.hpp>

using namespace std;

class test_move
{
public:

};

class test
{
public:


private:
    test_move m_data;
};

void test_xml_parser()
{
    ifstream ifs("../../test_source/test.xml");
    if (!ifs)
    {
        std::cerr << "ifs read error";
        exit(1);
    }
    string text((istreambuf_iterator<char>(ifs)), istreambuf_iterator<char>());
    xml::Element element;
    {
        Timer t;
        element = std::move(xml::Parser::FromString(text));
    }
    ofstream ofs("../../test_source/test_out.xml");
    ofs << element.to_string();
}

void test_move_construct()
{
    test t{};
    test tt(t);

    test_move q;

}

int main()
{
    test_move_construct();
}