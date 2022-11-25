//
// Created by Alone on 2022-8-8.
//

#include "Parser.h"

using namespace xml;

#include<fstream>

using std::ifstream;

#include<sstream>

using std::ostringstream;

#include<filesystem>


void xml::Parser::LoadFile(const string &filename)
{
    ifstream ifs(filename);
    if (!ifs)
    {

        ostringstream info;
        info << "load file error:file path not exist! path:" << filename;
        info << " current path:" << std::filesystem::current_path();
        throw std::runtime_error(info.str());
    }
    m_str = std::move(string(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>()));
    m_idx = 0;
    _trim_right();
}

void xml::Parser::LoadString(string_view content)
{
    m_str = content;
    m_idx = 0;
    //去除左右边的多余空白字符
    _trim_right();
}

Element xml::Parser::FromFile(const string &filename)
{
    static Parser instance;
    instance.LoadFile(filename);
    return instance.Parse();
}

Element xml::Parser::FromString(string_view content)
{
    static Parser instance;
    instance.LoadString(content);
    return instance.Parse();
}

//用于返回较为详细的错误信息，方便错误追踪
#define THROW_ERROR(error_info, error_detail) \
    do{                                    \
    string info = "parse error in ";              \
    string file_pos = __FILE__;                          \
    file_pos.append(":");                                \
    file_pos.append(std::to_string(__LINE__));\
    info += file_pos;                                  \
    info += ", ";                          \
    info += (error_info);                    \
    info += "\ndetail:";                          \
    info += (error_detail);\
    throw std::logic_error(info); \
}while(false)


char xml::Parser::_get_next_token()
{
    while (m_idx < m_str.size() && isspace(m_str[m_idx]))
    {
        m_idx++;
    }
    if (m_idx >= m_str.size())
        THROW_ERROR("invalid format", "out of length");
    return m_str[m_idx];
}

void xml::Parser::_trim_right()
{
    m_str.erase(std::find_if(m_str.rbegin(), m_str.rend(), [](char x)
    {
        return !isspace(x);
    }).base(), m_str.end());
    if (m_str.back() != '>') //格式的验证
        THROW_ERROR("format error,expected '>'", m_str.substr(m_str.size() - detail_len));
}


Element xml::Parser::Parse()
{
    while (true)
    {
        char t = _get_next_token();

        if (t != '<')
        {
            THROW_ERROR("invalid format", m_str.substr(m_idx, detail_len));
        }

        //解析版本号
        if (m_idx + 4 < m_str.size() && m_str.compare(m_idx, 5, "<?xml") == 0)
        {
            if (!_parse_version())
            {
                THROW_ERROR("version parse error", m_str.substr(m_idx, detail_len));
            }
            continue;
        }

        //解析注释
        if (m_idx + 3 < m_str.size() && m_str.compare(m_idx, 4, "<!--") == 0)
        {
            if (!_parse_comment())
            {
                THROW_ERROR("comment parse error", m_str.substr(m_idx, detail_len));
            }
            continue;
        }

        //解析element
        if (m_idx + 1 < m_str.size() && (isalpha(m_str[m_idx + 1]) || m_str[m_idx + 1] == '_'))
        {
            return _parse_element();
        }

        //出现未定义情况直接抛出异常
        THROW_ERROR("error format", m_str.substr(m_idx, detail_len));
    }
}

bool xml::Parser::_parse_version()
{
    auto pos = m_str.find("?>", m_idx);
    if (pos == string::npos)
    {
        return false;
    }
    m_idx = pos + 2;
    return true;
}


bool xml::Parser::_parse_comment()
{
    auto pos = m_str.find("-->", m_idx);
    if (pos == string::npos)
    {
        return false;
    }
    m_idx = pos + 3;
    return true;
}


Element xml::Parser::_parse_element()
{
    Element element;
    auto pre_pos = ++m_idx; //过掉<
    //判断name首字符合法性
    if (!(m_idx < m_str.size() && (std::isalpha(m_str[m_idx]) || m_str[m_idx] == '_')))
    {
        THROW_ERROR("error occur in parse name", m_str.substr(m_idx, detail_len));
    }

    //解析name
    while (m_idx < m_str.size() && (isalpha(m_str[m_idx]) || m_str[m_idx] == ':' ||
                                    m_str[m_idx] == '-' || m_str[m_idx] == '_' || m_str[m_idx] == '.'))
    {
        m_idx++;
    }

    if (m_idx >= m_str.size())
        THROW_ERROR("error occur in parse name", m_str.substr(m_idx, detail_len));

    element.Name() = m_str.substr(pre_pos, m_idx - pre_pos);

    //正式解析内部
    while (m_idx < m_str.size())
    {
        char token = _get_next_token();
        if (token == '/') //1.单元素，直接解析后结束
        {
            if (m_str[m_idx + 1] == '>')
            {
                m_idx += 2;
                return element;
            } else
            {
                THROW_ERROR("parse single_element failed", m_str.substr(m_idx, detail_len));
            }
        }

        if (token == '<')//2.对应三种情况：结束符、注释、下个子节点
        {
            //结束符
            if (m_str[m_idx + 1] == '/')
            {
                if (m_str.compare(m_idx + 2, element.Name().size(), element.Name()) != 0)
                {
                    THROW_ERROR("parse end tag error", m_str.substr(m_idx, detail_len));
                }

                m_idx += 2 + element.Name().size();
                char x = _get_next_token();
                if (x != '>')
                {
                    THROW_ERROR("parse end tag error", m_str.substr(m_idx, detail_len));
                }
                m_idx++; //千万注意把 '>' 过掉，防止下次解析被识别为初始的tag结束，实际上这个element已经解析完成
                return element;
            }
            //是注释的情况
            if (m_idx + 3 < m_str.size() && m_str.compare(m_idx, 4, "<!--") == 0)
            {
                if (!_parse_comment())
                {
                    THROW_ERROR("parse comment error", m_str.substr(m_idx, detail_len));
                }
                continue;
            }
            //其余情况可能是注释或子元素，直接调用parse进行解析得到即可
            element.push_back(Parse());
            continue;
        }
        if (token == '>') //3.对应两种情况：该标签的text内容，下个标签的开始或者注释（直接continue跳到到下次循环即可
        {
            m_idx++;
            //判断下个token是否为text，如果不是则continue
            char x = _get_next_token();
            if (x == '<')//不可能是结束符，因为xml元素不能为空body，如果直接出现这种情况也有可能是中间夹杂了注释
            {
                continue;
            }
            //解析text再解析child
            auto pos = m_str.find('<', m_idx);
            if (pos == string::npos)
                THROW_ERROR("parse text error", m_str.substr(m_idx, detail_len));
            element.Text() = m_str.substr(m_idx, pos - m_idx);
            m_idx = pos;
            //注意：有可能直接碰上结束符，所以需要continue，让element里的逻辑来进行判断
            continue;
        }

        //4.其余情况都为属性的解析

        auto key = _parse_attr_key();
        auto x = _get_next_token();
        if (x != '=')
        {
            THROW_ERROR("parse attrs error", m_str.substr(m_idx, detail_len));
        }
        m_idx++;
        auto value = _parse_attr_value();
        element[key] = value;
    }

    THROW_ERROR("parse element error", m_str.substr(m_idx, detail_len));
}


string xml::Parser::_parse_attr_key()
{
    char token = _get_next_token();
    auto start_pos = m_idx;
    if (isalpha(token) || token == '_')
    {
        while (m_idx < m_str.size() &&
               (isalpha(m_str[m_idx]) || isdigit(m_str[m_idx]) ||
                m_str[m_idx] == '_' || m_str[m_idx] == '-' || m_str[m_idx] == ':')
               && m_str[m_idx] != '=')
        {
            m_idx++;
        }
        return m_str.substr(start_pos, m_idx - start_pos);
    }
    THROW_ERROR("parse attr key error", m_str.substr(m_idx, detail_len));
}

string xml::Parser::_parse_attr_value()
{
    char token = _get_next_token();
    if (token == '"')
    {
        auto start_pos = ++m_idx;
        auto pos = m_str.find_first_of('"', m_idx);
        if (pos == string::npos)
        {
            THROW_ERROR("parse attr value error", m_str.substr(m_idx, detail_len));
        }
        m_idx = pos + 1;
        return m_str.substr(start_pos, pos - start_pos);
    }
    THROW_ERROR("parse attr key error", m_str.substr(m_idx, detail_len));
}
