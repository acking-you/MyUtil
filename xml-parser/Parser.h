//
// Created by Alone on 2022-8-8.
//

#ifndef MYUTIL_PARSER_H
#define MYUTIL_PARSER_H

static const int detail_len = 60;

#include "Element.h"

namespace xml
{
    class Parser
    {
    public:
        void LoadFile(const string &filename);

        void LoadString(string_view content);

        Element Parse();

        static Element FromFile(const string &filename);

        static Element FromString(string_view content);

    private:
        char _get_next_token();

        void _trim_right();

        bool _parse_version();

        bool _parse_comment();

        Element _parse_element();

        string _parse_attr_key();

        string _parse_attr_value();

    private:
        string m_str;
        size_t m_idx{};
    };
}


#endif //MYUTIL_PARSER_H