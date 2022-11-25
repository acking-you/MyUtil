//
// Created by Alone on 2022-7-21.
//
#include <stdexcept>
#include <algorithm>
#include "Parser.h"

using namespace http;
using namespace std;

string Parser::trim(string src)
{
    src.erase(src.begin(), find_if(src.begin(), src.end(), [](char ch)
    { return !isspace(ch); }));
    src.erase(find_if(src.rbegin(), src.rend(), [](char ch)
              { return !isspace(ch); })
                      .base(),
              src.end());

    return std::move(src);
}

bool Parser::is_end_flag(string_view src)
{
    if (src.empty())
        return true;

    return std::all_of(src.begin(), src.end(), [](char ch)
    { return ch == '\r' || ch == '\n' || ch == ' '; });
}

bool Parser::parse_requestline(string_view line, Request &request)
{
    //解析请求方法
    stringstream ss;
    ss << line;
    string item;
    ss >> item;
    int method = Request::get_method_from_text(item);
    if (method == 0)
        return false;
    request.method() = static_cast<METHOD>(method);

    ss >> item;
    auto url = item;
    request.url() = url;
    //这里需要再parse url

    ss >> item;
    if (item != "HTTP/1.1")
        return false;
    request.version() = VERSION1_1;

    return true;
}

bool Parser::parse_responseline(string_view line, Response &response)
{
    stringstream ss;
    ss << line;
    string item;
    ss >> item;
    if (item != "HTTP/1.1")
        return false;
    int status_code;
    ss >> status_code;
    response.status() = static_cast<STATUS_CODE>(status_code);
    auto option = Response::get_status_text(status_code);
    if (!option)
        return false;

    ss >> item;
    response.status_description() = item;
    return true;
}

bool Parser::parse_head(string_view line, Request &request)
{
    auto pos = line.find_first_of(':');
    if (pos != -1)
    {
        auto key = trim(string(line.substr(0, pos)));
        auto value = trim(string(line.substr(pos + 1)));
        request.head()[key] = value;
        return true;
    }
    return false;
}

bool Parser::parse_head(string_view line, Response &response)
{
    auto pos = line.find_first_of(':');
    if (pos != -1)
    {
        auto key = trim(string(line.substr(0, pos)));
        auto value = trim(string(line.substr(pos + 1)));
        response.head()[key] = value;
        return true;
    }
    return false;
}

bool Parser::parse_body(StringBuffer &buffer, Request &request)
{
    long avail = buffer.available();
    string tmp_data((std::istreambuf_iterator<char>(buffer)), std::istreambuf_iterator<char>());
    request.body().append(tmp_data);

    //状态判断与变更(如果有content_length字段则根据这个进行判断是否读完，以及是否进入到下一个阶段，若此次请求的数据并未解析完，则Request的content_len不为0
    if (request.content_length() > avail)
    {
        request.content_length() -= avail;
        return false;
    }
    request.content_length() = 0;
    m_parserRequestState = REQUEST_END;
    return true;
}

bool Parser::parse_body(StringBuffer &buffer, Response &response)
{
    long avail = buffer.available();
    string tmp_data((std::istreambuf_iterator<char>(buffer)), std::istreambuf_iterator<char>());
    response.body().append(tmp_data);

    //状态判断与变更(如果有content_length字段则根据这个进行判断是否读完，以及是否进入到下一个阶段，若此次请求的数据并未解析完，则Request的content_len不为0
    if (response.content_length() > avail)
    {
        response.content_length() -= avail;
        return false;
    }
    response.content_length() = 0;
    m_parserResponseState = RESPONSE_END;
    return true;
}

Request Parser::ToRequest(string_view content)
{

    StringBuffer ss;
    ss << content;
    Request request;
    //开始解析
    string line;
    while (getline(ss, line))
    { //解析以行为界的状态
        if (m_parserRequestState == REQUEST_LINE)
        {
            if (!parse_requestline(line, request))
                throw logic_error("parse error request_line");
            m_parserRequestState = REQUEST_HEAD;
        } else if (m_parserRequestState == REQUEST_HEAD)
        {
            if (is_end_flag(line))
            {                                  //到了body状态跳出，跳出前更新关键head信息到数据
                request.init_special_fields(); //初始化特殊字段到内存
                m_parserRequestState = REQUEST_BODY;
                break;
            }
            if (!parse_head(line, request))
                throw logic_error("parse error request_head");
        } else
        {
            throw std::logic_error("error state in parse Request");
        }
    }
    parse_body(ss, request);

    return request;
}

Response Parser::ToResponse(string_view content)
{
    StringBuffer ss;
    ss << content;
    Response response;
    string line;
    while (getline(ss, line))
    { //解析以行为界的状态
        if (m_parserResponseState == RESPONSE_LINE)
        {
            if (!parse_responseline(line, response))
                throw logic_error("parse error response_line");
            m_parserResponseState = RESPONSE_HEAD;
        } else if (m_parserResponseState == RESPONSE_HEAD)
        {
            if (is_end_flag(line))
            {                                   //到了body状态跳出，跳出前更新关键head信息到数据
                response.init_special_fields(); //初始化特殊字段到内存
                m_parserResponseState = RESPONSE_BODY;
                break;
            }
            if (!parse_head(line, response))
                throw logic_error("parse error request_head");
        } else
        {
            throw std::logic_error("error state in parse Response");
        }
    }
    parse_body(ss, response);

    return response;
}
