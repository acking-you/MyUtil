//
// Created by Alone on 2022-7-21.
//

#include <sstream>
#include "Request.h"
#include "Response.h"

using namespace http;

METHOD Request::get_method_from_text(string_view text)
{
    if (text == "GET")
        return GET;
    if (text == "POST")
        return POST;
    return static_cast<METHOD>(0);
}

void Request::init_special_fields()
{
    auto iter = m_head.find("Content-Length");
    if (iter != m_head.end())
    {
        m_contentLen = stoi(iter->second);
    }
    iter = m_head.find("Host");
    if (iter != m_head.end())
    {
        m_host = iter->second;
    }
    iter = m_head.find("Connection");
    if (iter != m_head.end())
    {
        m_keep_alive = iter->second == "keep-alive";
    }
    iter = m_head.find("Content-type");
    if (iter != m_head.end())
    {
        m_contentType = Response::get_type_from_text(iter->second);
    }
    //初始化url数据
    m_urlData = Url::FromData(m_url);
    //初始化post数据
    m_postForm = PostForm::FromData(m_body, (ACCEPT_CONTENT_TYPE) m_contentType);
}

string Request::to_string()
{

    auto &req = *this;
    std::stringstream ss;
    int method = req.method();
    if (method != GET && method != POST)
        throw std::logic_error("missing method parameter");
    ss << (method == GET ? "GET" : "POST") << ' ';
    ss << req.url() << ' ';
    ss << "HTTP/1.1"
       << "\r\n";

    //特殊head字段填充
    if (req.content_length() != 0)
    {
        req.head()["Content-Length"] = std::to_string(req.content_length());
    }

    for (auto &&[k, v]: req.head())
    {
        ss << k << ':' << v << "\r\n";
    }
    ss << "\r\n";
    ss << req.body();

    return ss.str();
}