//
// Created by Alone on 2022-7-25.
//

#ifndef MYUTIL_PARSER_H
#define MYUTIL_PARSER_H

#include<string>
#include<string_view>
#include<sstream>

namespace json
{
#define FUNC_TO_NAME _to_json
#define FUNC_FROM_NAME _from_json

#define START_TO_JSON void FUNC_TO_NAME(json::JObject & obj) const{
#define to(key) obj[key]
    //push一个自定义类型的成员
#define to_struct(key, struct_member) json::JObject tmp((json::dict_t())); struct_member.FUNC_TO_NAME(tmp); obj[key] = tmp
#define END_TO_JSON  }

#define START_FROM_JSON void FUNC_FROM_NAME(json::JObject& obj) {
#define from(key, type) obj[key].Value<type>()
#define from_struct(key, struct_member) struct_member.FUNC_FROM_NAME(obj[key])
#define END_FROM_JSON }
    using std::string;
    using std::string_view;
    using std::stringstream;

    class JObject;

    class Parser
    {
    public:
        Parser() = default;

        static JObject FromString(string_view content);

        template<class T>
        static string ToJSON(T const &src)
        {
            //如果是基本类型
            if constexpr(IS_TYPE(T, int_t))
            {
                JObject object(src);
                return object.to_string();
            } else if constexpr(IS_TYPE(T, bool_t))
            {
                JObject object(src);
                return object.to_string();
            } else if constexpr(IS_TYPE(T, double_t))
            {
                JObject object(src);
                return object.to_string();
            } else if constexpr(IS_TYPE(T, str_t))
            {
                JObject object(src);
                return object.to_string();
            }
            //如果是自定义类型调用方法完成dict的赋值，然后to_string即可
            json::JObject obj((json::dict_t()));
            src.FUNC_TO_NAME(obj);
            return obj.to_string();
        }

        template<class T>
        static T FromJson(string_view src)
        {
            JObject object = FromString(src);
            //如果是基本类型
            if constexpr(is_basic_type<T>())
            {
                return object.template Value<T>();
            }

            //调用T类型对应的成岩函数
            if (object.Type() != T_DICT)throw std::logic_error("not dict type fromjson");
            T ret;
            ret.FUNC_FROM_NAME(object);
            return ret;
        }

        void init(string_view src);

        void trim_right();

        void skip_comment();

        bool is_esc_consume(size_t pos);

        char get_next_token();

        JObject parse();

        JObject parse_null();

        JObject parse_number();

        bool parse_bool();

        string parse_string();

        JObject parse_list();

        JObject parse_dict();

    private:
        string m_str;
        size_t m_idx{};
    };
}


#endif //MYUTIL_PARSER_H
