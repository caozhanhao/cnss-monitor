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
#ifndef BUSIN_CNSS_HPP
#define BUSIN_CNSS_HPP
#pragma once

#include <string>
#include <optional>
#include <vector>

#include "nlohmann/json.hpp"

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "cpp-httplib/httplib.h"

namespace czh
{
  struct RankItem
  {
    int id;
    std::string name;
    int score;
    int identity;
  };

  struct Rank
  {
    std::vector<std::pair<std::string, std::vector<RankItem> > > rank;

    Rank trunc(size_t num) const;
  };

  std::ostream& operator<<(std::ostream& os, const Rank& rank);

  struct Task
  {
    size_t id;
    std::string name;
    size_t pass_number;
    std::string type;
    size_t score;
    size_t full_score;
  };

  class CNSS
  {
  private:
    std::string server;
    std::string token;
    std::vector<std::string> types;
    std::vector<size_t> tasks;
    std::chrono::milliseconds interval{};
    std::string res_path;
    std::string listen_addr;
    int listen_port;
    nlohmann::ordered_json config;
    std::string config_path;
    std::string admin_password;
    httplib::Client* cli;
    httplib::Server* svr;

  public:
    explicit CNSS(std::string path);

    ~CNSS();

    [[nodiscard]] std::optional<Rank> get_rank() const;

    [[nodiscard]] std::vector<Task> get_tasks() const;

    void auth_do(const httplib::Request& req, httplib::Response& res,
                 const std::function<nlohmann::json(const httplib::Request&)>& func) const;

    void start();
  };
}
#endif
