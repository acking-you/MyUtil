//
// Created by Alone on 2022-7-21.
//

#ifndef MYUTIL_REQUEST_H
#define MYUTIL_REQUEST_H

#include <map>
#include <string>
#include <vector>
#include <string_view>
#include "Url.h"
#include "PostForm.h"

namespace http
{
    class PostForm;

    using std::map;
    using std::string;
    using std::string_view;
    using std::vector;

    class Request
    {

    public:
        using head_t = map<string, string>;              //请求头的key-value对
        using post_form_t = map<string, vector<string>>; // post请求的表单数据

        VERSION &version()
        {
            return m_version;
        }

        METHOD &method()
        {
            return m_method;
        }

        string &url()
        {
            return m_url;
        }

        head_t &head()
        {
            return m_head;
        }

        string &body()
        {
            return m_body;
        }

        long &content_length()
        {
            return m_contentLen;
        }

        static METHOD get_method_from_text(string_view text);

        void init_special_fields();

        string to_string();

        string Query(string const &key)
        {
            return m_urlData.Query(key);
        }

        vector<string> &QueryList(string const &key)
        {
            return m_urlData.From()[key];
        }

        // TODO
        string PostQuery(string const &key)
        {
            return m_postForm.Query(key);
        }

        vector<string> &PostQueryList(string const &key)
        {
            return m_postForm.From()[key];
        }

        string PostMultiPart(string const &key)
        {
        }

    private:
        VERSION m_version = VERSION1_1;
        METHOD m_method;
        int m_contentType = -1;
        long m_contentLen{};
        bool m_keep_alive;
        string m_host;
        string m_url;
        head_t m_head;
        string m_body;
        Url m_urlData;       // url对应的数据（包含Query数据
        PostForm m_postForm; // post对应的数据，主要是postForm的数据解析
    };

}

#endif // MYUTIL_REQUEST_H
