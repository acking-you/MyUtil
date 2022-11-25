#include <iostream>
#include <fstream>
#include "../BenchMark/Timer.h"
#include "ini-parser/IniFile.h"
#include "http-parser/Parser.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

void error_die(const char *msg)
{
    std::cerr << msg << "\n";
    exit(1);
}

int main()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_port = htons(3000);
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_family = AF_INET;
    if (bind(fd, (sockaddr *) &addr, sizeof(addr)) < 0)
    {
        error_die("bind error");
    }
    if (listen(fd, 5) < 0)
    {
        error_die("listen error");
    }
    sockaddr_in addr_client{};
    socklen_t client_len = sizeof(addr_client);

    while (1)
    {
        int cfd = accept(fd, (sockaddr *) &addr_client, &client_len);
        if (cfd < 0)
        {
            error_die("accept error");
        }
        char buffer[1024]{};
        if (recv(cfd, (void *) buffer, 1024, 0) < 0)
        {
            error_die("recv error");
        }
        http::Parser parser;
        auto req = parser.ToRequest(buffer);
        std::cout << req.to_string(); //根据request内容获取对应的http报文
        req.head()["dfasf"] = "fda"; //随意设置request的header
        req.body() = "fdsafsadf"; //设置request的body部分
        //request的特殊字段（GET的FORM和POST的form
        auto v = req.Query("test"); //获取第一个值
        auto v1 = req.PostQuery("test"); //获取post表单里的第一个query值
        req.PostMultiPart("test"); //返回form-data的键值（可以传入文件

        http::Response response;
        response.SetStatus(http::OK);
        response.SetContentType(http::ACCEPT_CONTENT_TYPE::T_JSON);
        response.SetConnection(false);
        response.body() = R"({"hello world!":2323})";
        auto response_text = response.to_string();
        if (send(cfd, response_text.data(), response_text.length(), 0) < 0)
        {
            error_die("send error");
        }
        close(cfd);
    }
}
