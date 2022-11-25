//
// Created by Alone on 2022-7-21.
//

#include <stdexcept>
#include <sstream>
#include "Response.h"

using namespace http;

optional<string> Response::get_status_text(int status)
{
    switch (status)
    {
        case 100:
            return "Continue";
        case 101:
            return "Switching Protocols";
        case 200:
            return "OK";
        case 201:
            return "Created";
        case 202:
            return "Accepted";
        case 203:
            return "Non-Authoritative Information";
        case 204:
            return "No Content";
        case 205:
            return "Reset Content";
        case 206:
            return "Partial Content";
        case 300:
            return "Multiple Choices";
        case 301:
            return "Moved Permanently";
        case 302:
            return "Found";
        case 303:
            return "See Other";
        case 304:
            return "Not Modified";
        case 305:
            return "Use Proxy";
            // case 306: return "(reserved)";
        case 307:
            return "Temporary Redirect";
        case 400:
            return "Bad Request";
        case 401:
            return "Unauthorized";
        case 402:
            return "Payment Required";
        case 403:
            return "Forbidden";
        case 404:
            return "Not Found";
        case 405:
            return "Method Not Allowed";
        case 406:
            return "Not Acceptable";
        case 407:
            return "Proxy Authentication Required";
        case 408:
            return "Request Timeout";
        case 409:
            return "Conflict";
        case 410:
            return "Gone";
        case 411:
            return "Length Required";
        case 412:
            return "Precondition Failed";
        case 413:
            return "Request Entity Too Large";
        case 414:
            return "Request-URI Too Long";
        case 415:
            return "Unsupported Media Type";
        case 416:
            return "Requested Range Not Satisfiable";
        case 417:
            return "Expectation Failed";
        case 500:
            return "Internal Server Error";
        case 501:
            return "Not Implemented";
        case 502:
            return "Bad Gateway";
        case 503:
            return "Service Unavailable";
        case 504:
            return "Gateway Timeout";
        case 505:
            return "HTTP Version Not Supported";
        default:
            return {};
    }
}

optional<string> Response::get_body_type_text(int type)
{
    switch (type)
    {
        case http::T_JSON:
            return "application/json";
        case http::T_POST_URL:
            return "application/x-www-form-urlencoded";
        case http::T_POST_FROM_DATA:
            return "multipart/form-data";
        case http::T_ANY:
            return "*/*";
        default:
            return {};
    }
}

int Response::get_type_from_text(const string &text)
{
    if (text.find("application/json") != -1)
        return T_JSON;
    if (text.find("application/x-www-form-urlencoded") != -1)
        return T_POST_URL;
    if (text.find("multipart/form-data") != -1)
        return T_POST_FROM_DATA;

    return -1;
}

void Response::init_special_fields()
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
        m_contentType = get_type_from_text(iter->second);
    }
}

string Response::to_string()
{
    auto &rep = *this;
    std::stringstream ss;
    ss << "HTTP/1.1 ";
    int status = rep.m_status;
    if (status == 0)
        throw std::runtime_error("status error in Composer");
    ss << status << ' ';
    auto description = Response::get_status_text(status);
    ss << description.value() << "\r\n";

    //特殊head字段填充
    if (rep.content_length() != 0)
    {
        rep.head()["Content-Length"] = std::to_string(rep.content_length());
    }

    if (rep.m_keep_alive)
    {
        rep.m_head["Connection"] = "keep-alive";
    } else
    {
        rep.m_head["Connection"] = "close";
    }

    if (rep.m_contentType != -1)
    {
        rep.m_head["Content-Type"] = get_body_type_text(rep.m_contentType).value();
    }

    for (auto &&[k, v]: rep.head())
    {
        ss << k << ':' << v << "\r\n";
    }

    ss << "\r\n";
    ss << rep.body();

    return ss.str();
}