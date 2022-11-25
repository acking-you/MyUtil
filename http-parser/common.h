//
// Created by Alone on 2022-7-23.
//

#ifndef MYUTIL_COMMON_H
#define MYUTIL_COMMON_H
namespace http
{

    enum PARSER_REQUEST_STATE
    {
        REQUEST_LINE,
        REQUEST_HEAD,
        REQUEST_BODY,
        REQUEST_END
    };
    enum PARSER_RESPONSE_STATE
    {
        RESPONSE_LINE,
        RESPONSE_HEAD,
        RESPONSE_BODY,
        RESPONSE_END
    };

    enum METHOD
    {
        GET = 1,
        POST
    };
    enum VERSION
    {
        VERSION1_1
    };

    enum STATUS_CODE
    {
        Continue = 100,
        Switching = 101,
        OK = 200,
        Created = 201,
        NotFound = 404,
        RequestTimeout = 408,
        InternalError = 500,
        GatewayTimeout = 504,
        NotSupportedVersion = 505
    };


    enum ACCEPT_CONTENT_TYPE
    {
        T_JSON,
        T_POST_URL,
        T_POST_FROM_DATA,
        T_ANY
    };
}
#endif //MYUTIL_COMMON_H
