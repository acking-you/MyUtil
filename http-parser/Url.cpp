//
// Created by Alone on 2022-7-21.
//

#include "Url.h"

using namespace http;

Url Url::FromData(string_view src)
{
    Url url;
    auto pos = src.find("://");
    if (pos != string::npos)
    { // parse scheme
        url.m_scheme = src.substr(0, pos);
        src = src.substr(pos + 3);
    }
    pos = src.find('/');
    if (pos != string::npos)
    { // parse domain
        url.m_domain = src.substr(0, pos);
    }
    pos = src.find('?');
    if (pos != string::npos)
    { // parse path
        url.m_path = src.substr(0, pos);
        // parse parameters
        ParseQueryForm(src.substr(pos + 1), url.m_form);
        return url;
    }
    url.m_path = src;
}

void Url::ParseQueryForm(string_view src, form_t &form)
{
    long pre_pos = -1;
    size_t cur_pos;
    string key;
    string value;
    auto find_next_end = [](string_view const &src)
    {
        return src.find('&');
    };
    while ((cur_pos = src.find('=', pre_pos + 1)) != string::npos)
    {
        key = string(src.substr(pre_pos + 1, cur_pos));
        //根据剩余的切片进行操作得到value
        auto remain = src.substr(cur_pos + 1);
        pre_pos = find_next_end(remain);
        value = pre_pos == -1 ? remain : remain.substr(0, pre_pos);
        form[key].push_back(value);
        pre_pos = cur_pos;
    }
}

string Url::to_string()
{
    string ret;
    if (!m_scheme.empty())
    {
        ret.append(m_scheme);
        ret.append("://");
    }
    ret.append(m_domain);
    ret.append(m_path);
    ret.push_back('?');
    for (auto &&[k, v]: m_form)
    {
        for (auto &&vv: v)
        {
            ret.append(k);
            ret.push_back('=');
            ret.append(vv);
            ret.push_back('&');
        }
    }
    ret.pop_back();
    return std::move(ret);
}