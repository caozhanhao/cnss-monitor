// MIT License
//
// Copyright (c) 2024 caozhanhao
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
#ifndef BUSIN_EMAIL_HPP
#define BUSIN_EMAIL_HPP
#pragma once

#include <string>

#include "nlohmann/json.hpp"

namespace czh
{
  struct email_upload_status
  {
    size_t bytes_read;
    const std::string *const payload_text;
  };
  struct Email
  {
    std::string from_addr;
    std::string from_name;
    std::string to_addr;
    std::string to_name;
    std::string subject;
    std::string body;
  };
  
  struct EmailSender
  {
    std::string username;
    std::string passwd;
    std::string server;
    explicit EmailSender(const nlohmann::ordered_json& path);

    int send(const Email &email) const;
  };
}
#endif