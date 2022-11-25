//
// Created by Alone on 2022-7-21.
//

#include "PostForm.h"
#include "Url.h"
#include <stdexcept>

using namespace http;

PostForm PostForm::FromData(string_view content, ACCEPT_CONTENT_TYPE type)
{
    PostForm postForm;
    if (type == T_POST_URL)
    {
        postForm.m_post_type = T_POST_URL;
        postForm.ParseQueryUrlData(content);
        return postForm;
    }
    if (type == T_POST_FROM_DATA)
    {
        throw std::runtime_error("not supported type");
    }
}

void PostForm::ParseQueryUrlData(string_view content)
{
    Url::ParseQueryForm(content, m_postform);
}

void PostForm::ParseMultFormData(string_view content)
{
}

string PostForm::Query(string const &key)
{
    if (m_postform[key].empty())
        return "";
    return m_postform[key][0];
}