//
// Created by Alone on 2022-7-21.
//

#ifndef MYUTIL_POSTFORM_H
#define MYUTIL_POSTFORM_H

#include "common.h"
#include <map>
#include <string>
#include <string_view>
#include <vector>


namespace http
{
    using std::map;
    using std::string;
    using std::vector;
    using std::string_view;

    class PostForm
    {
    public:
        using post_form_t = map<string, vector<string>>;

        static PostForm FromData(string_view content, ACCEPT_CONTENT_TYPE type);

        void ParseQueryUrlData(string_view content);

        void ParseMultFormData(string_view content);

        string Query(string const &key);

        post_form_t &From()
        {
            return m_postform;
        }

    private:
        http::ACCEPT_CONTENT_TYPE m_post_type;

        post_form_t m_postform;
    };
}

#endif // MYUTIL_POSTFORM_H
