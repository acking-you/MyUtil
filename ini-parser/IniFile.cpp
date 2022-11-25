//
// Created by Alone on 2022-7-20.
//

#include <stdexcept>
#include <algorithm>
#include <fstream>
#include <iostream>
#include "IniFile.h"

Value::operator int() {
    return std::stoi(m_value);
}

Value::operator bool() {
    if(m_value == "true"){
        return true;
    }else if(m_value=="false"){
        return false;
    }
    throw std::logic_error("value type error in bool()");
}

Value::operator double() {
    double ret;
    std::stringstream ss;
    ss<<m_value;
    ss>>ret;
    return ret;
}

Value::operator string() {
    return m_value;
}


IniFile::IniFile(string_view filename) {
    Load(filename);
}

bool IniFile::Load(string_view filename) {
    m_filename = filename;
    m_inifile.clear();

    string name;
    string line;
    std::ifstream fin(filename.data());
    if(!fin){
        printf("loading file failed: %s is not found.\n",filename.data());
        fflush(stdout);
        return false;
    }
    while(std::getline(fin,line)){
        line = trim(line);
        auto view = string_view(line);
        if(view.empty())continue;
        //it is a section
        if(view[0] == '['){
            //start parse section
            auto pos = view.find_first_of(']');
            if(pos!=-1){
                name = trim(string(view.substr(1,pos-1)));
                m_inifile[name];
            }
        }else if(view[0] == '#') //it is a comment
            continue;
        else{   //it is key-value
            auto pos = view.find_first_of('=');
            if(pos!=-1){
                string key = trim(string(view.substr(0,pos)));
                string value = trim(string(view.substr(pos+1,view.size()-pos-1)));
                auto it = m_inifile.find(name);
                if(it == m_inifile.end()){
                    printf("parsing error: section=%s key=%s\n", name.c_str(), key.c_str());
                    fflush(stdout);
                    return false;
                }
                m_inifile[name][key] = value;
            }
        }
    }

    return true;
}

bool IniFile::Save(string_view filename) {
    std::ofstream fout(filename.data());
    if(!fout){
        fprintf(stderr,"saving file failed %s is not open\n",filename.data());
        return false;
    }
    for(auto&&[name,section]:m_inifile){
        //write section
        fout<<'['<<name<<']'<<"\r\n";
        for(auto&&[k,v]:section){
            //write key-value
            fout<<k<<'='<<string(v)<<"\r\n";
        }
        fout<<std::endl;
    }
    return true;
}


void IniFile::Show() {
    for(auto&&[name,section]:m_inifile){
        std::cout<<'['<<name<<']'<<"\r\n";
        for(auto&&[k,v]:section){
            std::cout<<k<<'='<<string(v)<<"\r\n";
        }
        std::cout<<std::endl;
    }
}

void IniFile::Clear() {
    m_inifile.clear();
}

string IniFile::trim(string src) {
    //删除前半段空格
    src.erase(src.begin(), std::find_if(src.begin(), src.end(),[](char ch){
        return !isspace(ch);
    }));
    //删除后半段空格
    src.erase(std::find_if(src.rbegin(), src.rend(),[](char ch){
        return !std::isspace(ch);
    }).base(),src.end());

    return std::move(src);
}