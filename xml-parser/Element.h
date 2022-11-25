//
// Created by Alone on 2022-8-8.
//

#ifndef MYUTIL_ELEMENT_H
#define MYUTIL_ELEMENT_H

#include<vector>
#include<map>
#include<string>
#include<string_view>

namespace xml
{
    using std::vector;
    using std::map;
    using std::string_view;

    using std::string;

    class Element
    {

    public:
        using children_t = vector<Element>;
        using attrs_t = map<string, string>;
        using iterator = vector<Element>::iterator;
        using const_iterator = vector<Element>::const_iterator;


        string &Name()
        {
            return m_name;
        }

        string &Text()
        {
            return m_text;
        }

        iterator begin()
        {
            return m_children.begin();
        }

        [[nodiscard]] const_iterator begin() const
        {
            return m_children.begin();
        }

        iterator end()
        {
            return m_children.end();
        }

        [[nodiscard]] const_iterator end() const
        {
            return m_children.end();
        }

        void push_back(Element const &element)
        {
            m_children.push_back(element);
        }

        void push_back(Element &&element)
        {
            m_children.push_back(std::move(element));
        }

        string &operator[](string const &key)
        {
            return m_attrs[key];
        }

        string to_string()
        {
            return _to_string();
        }

    private:
        string _to_string();

    private:
        string m_name;
        string m_text;
        children_t m_children;
        attrs_t m_attrs;
    };
}


#endif //MYUTIL_ELEMENT_H
