//
// Created by Alone on 2022-7-21.
//

#ifndef MYUTIL_RESPONSE_H
#define MYUTIL_RESPONSE_H

#include <string>
#include <string_view>
#include <optional>
#include "Request.h"

namespace http
{
    using std::optional;
    using std::string;
    using std::string_view;

    class Response
    {
    public:
        using head_t = Request::head_t;

        static optional<string> get_status_text(int status);

        static optional<string> get_body_type_text(int type);

        static int get_type_from_text(string const &text);

        VERSION &version()
        {
            return m_httpVersion;
        }

        STATUS_CODE &status()
        {
            return m_status;
        }

        string &status_description()
        {
            return m_status_description;
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

        Response &SetStatus(STATUS_CODE status)
        {
            m_status = status;
            return *this;
        }

        Response &SetContentType(ACCEPT_CONTENT_TYPE contentType)
        {
            m_contentType = contentType;
            return *this;
        }

        Response &SetConnection(bool status)
        {
            m_keep_alive = status;
            return *this;
        }

        Response &SetContentLength(int length)
        {
            m_contentLen = length;
            return *this;
        }

        void init_special_fields();

        string to_string();

    private:
        VERSION m_httpVersion = VERSION1_1;
        STATUS_CODE m_status = static_cast<STATUS_CODE>(0);
        int m_contentType = -1;
        long m_contentLen{};
        bool m_keep_alive{};
        string m_host;
        string m_status_description;
        string m_body;
        head_t m_head;
    };
}

#endif // MYUTIL_RESPONSE_H
