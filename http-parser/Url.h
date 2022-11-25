//
// Created by Alone on 2022-7-21.
//

#ifndef MYUTIL_URL_H
#define MYUTIL_URL_H

#include <string>
#include <string_view>
#include <vector>
#include <map>

namespace http
{
    using std::map;
    using std::string;
    using std::string_view;
    using std::vector;

    class Url
    {
    public:
        using form_t = map<string, vector<string>>; // query数据

        Url() = default;

        static Url FromData(string_view src);

        static void ParseQueryForm(string_view src, form_t &form);

        form_t &From()
        {
            return m_form;
        }

        string Query(string const &key)
        {
            auto iter = m_form.find(key);
            if (iter == m_form.end())
            {
                return "";
            }
            return iter->second.empty() ? "" : iter->second.back();
        }

        string &Path()
        {
            return m_path;
        }

        string &Domain()
        {
            return m_domain;
        }

        string to_string();

    private:
        string m_scheme;
        string m_domain;
        string m_path;
        form_t m_form;
    };
}

#endif // MYUTIL_URL_H
