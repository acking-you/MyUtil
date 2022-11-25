## http协议的组成

> http请求报文如下：
> ![image.png](https://p3-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/937fc897e6ce4d70bbdd58e30123fe3c~tplv-k3u1fbpfcp-watermark.image?)
> ![image.png](https://p9-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/24f806c32db44631866e6df0381d1c5a~tplv-k3u1fbpfcp-watermark.image?)

> http响应报文如下：
> ![image.png](https://p9-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/6ee6d2f97fb944099fa7bad0d9366d40~tplv-k3u1fbpfcp-watermark.image?)
> ![image.png](https://p6-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/869ed006c08d48719a7c2bf6a48b0a9b~tplv-k3u1fbpfcp-watermark.image?)

## 状态机设计

> 请求报文解析
> ![image.png](https://p3-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/cd0472a8371a442584b37783b9a5b52f~tplv-k3u1fbpfcp-watermark.image?)

> 响应报文解析
> ![image.png](https://p1-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/70f56c79e0c14df3a439c2e56f4a5cd7~tplv-k3u1fbpfcp-watermark.image?)

## 代码结构设计

* 基础结构类：Response和Request，其中都包含一个Url类，用于解析得到路径和Query参数。
* 工具类：HttpParserr，HttpParser用于解析纯http报文然后得到对应的Response或Request，如果要将Request或Response组合成对应的http报文直接调用它们的to_string()方法即可。

![http__Parser.png](https://p6-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/276b51bccb68441abb2f7dd0cc3f3831~tplv-k3u1fbpfcp-watermark.image?)

代码结构：
![image.png](https://p6-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/0667947a3dfd42249fba94e703781b72~tplv-k3u1fbpfcp-watermark.image?)

## 简单使用

```cpp
void simple(){
        http::Parser parser;
        auto req = parser.ToRequest(buffer);
        std::cout << req.to_string(); //根据request内容获取对应的http报文
        req.m_head()["dfasf"] = "fda"; //随意设置request的header
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
}
```