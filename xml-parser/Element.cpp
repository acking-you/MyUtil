//
// Created by Alone on 2022-8-8.
//

#include "Element.h"

using namespace xml;

#include<sstream>

using std::ostringstream;

string Element::_to_string()
{
    if (m_name == "")
    {
        return "";
    }
    std::ostringstream os;
    os << "<" << m_name;
    for (auto &&[key, value]: m_attrs)
    {
        os << " " << key << "=\"" << value << "\"";
    }
    if (m_text.empty() && m_children.empty()) //判断是否为单元素
    {
        os << "/>";
        return os.str();
    }
    os << ">";
    os << m_text;
    if (!m_children.empty())
    {
        for (auto &&child: m_children)
        {
            os << child._to_string();
        }
    }

    os << "</" << m_name << ">";
    return os.str();
}
