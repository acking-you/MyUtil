//
// Created by Alone on 2022-7-21.
//

#ifndef MYUTIL_PARSER_H
#define MYUTIL_PARSER_H

#include <string>
#include <string_view>
#include <sstream>
#include "Request.h"
#include "Response.h"
#include "common.h"

namespace http
{
    using std::string;
    using std::string_view;
    using std::stringstream;

    class StringBuffer : public std::stringstream
    {
    public:
        /**
         * Create an empty stringstream
         */
        StringBuffer() : stringstream()
        {}

        /**
         * Create a string stream with initial contents, underlying
         * stringstream is set to append mode
         *
         * @param initial contents
         */
        explicit StringBuffer(const char *initial)
                : stringstream(initial, ios_base::ate | ios_base::in | ios_base::out)
        {
            // Using GCC the ios_base::ate flag does not seem to have the desired effect
            // As a backup seek the output pointer to the end of buffer
            seekp(0, std::ios::end);
        }

        /**
         * @return the length of a str held in the underlying stringstream
         */
        long length()
        {
            long length = tellp();

            if (length < 0)
                length = 0;

            return length;
        }

        long available()
        {
            auto read_pos = tellg();
            if (read_pos == -1)
                return 0;

            long avail = length() - tellg();

            if (avail < 0)
                return 0;

            return avail;
        }
    };

    class Parser
    {

    public:
        Request ToRequest(string_view content);

        Response ToResponse(string_view content);

    private:
        bool parse_requestline(string_view line, Request &request);

        bool parse_responseline(string_view line, Response &response);

        bool parse_head(string_view line, Request &request);

        bool parse_head(string_view line, Response &response);

        bool parse_body(StringBuffer &buffer, Request &request);

        bool parse_body(StringBuffer &buffer, Response &response);

        static bool is_end_flag(string_view src);

        string trim(string src);

    private:
        PARSER_REQUEST_STATE m_parserRequestState = REQUEST_LINE;
        PARSER_RESPONSE_STATE m_parserResponseState = RESPONSE_LINE;
    };
}

#endif // MYUTIL_PARSER_H
