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

#include <iostream>
#include <iomanip>

#include "nlohmann/json.hpp"

#include "busin/cnss.hpp"
#include "busin/utils.hpp"
#include "busin/email.hpp"

namespace czh
{
  std::string pretty_typename(const std::string& raw)
  {
    if (raw == "re") return "RE";
    if (raw == "blockchain") return "BlockChain";
    if (raw == "sa") return "SA";
    return static_cast<char>(std::toupper(raw[0])) + raw.substr(1);
  }

  std::ostream& operator<<(std::ostream& os, const Rank& rank)
  {
    for (const auto& type : rank.rank)
    {
      os << pretty_typename(type.first) << ": \n";
      for (size_t i = 0; i < type.second.size(); ++i)
      {
        const auto& item = type.second[i];
        os << "  " << std::setw(2) << std::setfill('0')
            << i + 1 << "  " << item.name << " " << item.score << "\n";
      }
    }
    return os;
  }

  void print_subrank(std::ostream& os, const std::string& type, const std::vector<RankItem>& subrank)
  {
    os << pretty_typename(type) << ": \n";
    for (size_t i = 0; i < subrank.size(); ++i)
    {
      os << "  " << std::setw(2) << std::setfill('0')
          << i + 1 << "  " << subrank[i].name << " " << subrank[i].score << "\n";
    }
    os << "----------------------------------------\n";
  }

  struct Changes
  {
    std::vector<RankItem> in;
    std::vector<RankItem> out;

    bool empty() const { return in.empty() && out.empty(); }
  };

  Changes get_changes(std::vector<RankItem> curr, std::vector<RankItem> last)
  {
    auto comp = [](auto&& r1, auto&& r2) { return r1.id < r2.id; };
    std::sort(curr.begin(), curr.end(), comp);
    std::sort(last.begin(), last.end(), comp);
    std::vector<RankItem> in;
    std::set_difference(curr.cbegin(), curr.cend(), last.cbegin(), last.cend(),
                        std::back_inserter(in), comp);
    std::vector<RankItem> out;
    std::set_difference(last.cbegin(), last.cend(), curr.cbegin(), curr.cend(),
                        std::back_inserter(out), comp);
    return {in, out};
  }

  void print_changes(std::ostream& os, const Changes& changes)
  {
    const auto& [in, out] = changes;
    if (!in.empty())
    {
      std::string in_str;
      for (auto& r : in)
      {
        in_str += r.name + ", ";
      }
      in_str.pop_back();
      in_str.pop_back();
      os << in_str << " 进入了前十名。\n";
    }

    if (!out.empty())
    {
      std::string out_str;
      for (auto& r : out)
      {
        out_str += r.name + ", ";
      }
      out_str.pop_back();
      out_str.pop_back();
      os << out_str << " 掉出了前十名。\n";
    }

    if (!in.empty() || !out.empty())
      os << "----------------------------------------\n";
  }

  void email_notify(const nlohmann::ordered_json& smtp, const EmailSender& email_sender, Email e)
  {
    for (auto& r : smtp["receiver_emails"])
    {
      if (!r.is_string())
      {
        std::cerr << "Invalid email: '" << r << "'.";
      }
      e.to_addr = r.get<std::string>();
      e.to_name = e.to_addr;
      auto ret = email_sender.send(e);
      if (ret != 0)
      {
        std::cerr << "Failed to send email to '" << e.to_addr << "'.\n";
      }
      else
      {
        std::cout << "An Email was sent to '" << e.to_addr << "'.\n";
      }
    }
  }

  void notify_rank(const nlohmann::ordered_json& smtp,
                   const EmailSender& email_sender,
                   const std::string& type,
                   const std::vector<RankItem>& subrank,
                   const Changes& changes)
  {
    std::stringstream ss;
    print_subrank(ss, type, subrank);
    print_changes(ss, changes);

    std::string content = ss.str();
    std::cout << content;
    Email e;
    e.from_addr = smtp["sender_email"].get<std::string>();
    e.from_name = "CNSS Recruit Monitor";
    e.subject = "CNSS Recruit Monitor - Update";
    e.body = "尊敬的用户，您好：\nCNSS Rank有如下变化：\n" + content + "\n" + "祝好，\nCNSS Rank Monitor\n";
    email_notify(smtp, email_sender, e);
  }

  void notify_task(const nlohmann::ordered_json& smtp,
                   const EmailSender& email_sender,
                   const std::string& who,
                   const std::string& task_name)
  {
    std::string content = "passer: " + who + ", task: " + task_name + ", time " + get_time() + "\n";
    std::cout << content;
    Email e;
    e.from_addr = smtp["sender_email"].get<std::string>();
    e.from_name = "CNSS Recruit Monitor";
    e.subject = "CNSS Recruit Monitor - Update";
    e.body = "尊敬的用户，您好：\nCNSS Task有如下变化：\n" + content + "\n" + "祝好，\nCNSS Rank Monitor\n";
    email_notify(smtp, email_sender, e);
  }


  CNSS::CNSS(std::string path)
    : interval(2000), config_path(std::move(path)), listen_port(80)
  {
    std::ifstream config_file(config_path);
    if (!config_file.is_open())
      throw std::runtime_error("Failed to open config file.");
    config = nlohmann::ordered_json::parse(config_file);
    config_file.close();

    if (!config.contains("monitor"))
      throw std::runtime_error("Missing monitor in the config file.");

    auto monitor = config["monitor"];

    if (monitor.contains("interval_in_ms"))
      interval = std::chrono::milliseconds(monitor["interval_in_ms"].get<int>());

    if (!monitor.contains("server"))
      throw std::runtime_error("Missing server in the config file.");
    monitor["server"].get_to(server);
    cli = new httplib::Client(server);

    if (!monitor.contains("types"))
      throw std::runtime_error("Missing types in the config file.");
    monitor["types"].get_to(types);

    if (!monitor.contains("tasks"))
      throw std::runtime_error("Missing tasks in the config file.");
    monitor["tasks"].get_to(tasks);

    if (!monitor.contains("token"))
      throw std::runtime_error("Missing token in the config file.");
    monitor["token"].get_to(token);


    if (!config.contains("server"))
      throw std::runtime_error("Missing server in the config file.");

    auto server = config["server"];

    if (!server.contains("addr"))
      throw std::runtime_error("Missing addr in the config file.");
    server["addr"].get_to(listen_addr);

    if (!server.contains("port"))
      throw std::runtime_error("Missing port in the config file.");
    server["port"].get_to(listen_port);

    if (!server.contains("admin_password"))
      throw std::runtime_error("Missing admin_password in the config file.");
    server["admin_password"].get_to(admin_password);

    if (!server.contains("resource_path"))
      throw std::runtime_error("Missing resource_path in the config file.");
    server["resource_path"].get_to(res_path);
    svr = new httplib::Server();
  }

  CNSS::~CNSS()
  {
    if (cli != nullptr)
    {
      delete cli;
      cli = nullptr;
    }
    if (svr != nullptr)
    {
      delete svr;
      svr = nullptr;
    }
  }

  [[nodiscard]] std::vector<Task> CNSS::get_tasks() const
  {
    assert(cli != nullptr);
    auto res = cli->Get("/v1/tasks/494", httplib::Headers{{"token", token}});
    if (!res || res->status != 200)
      return {};

    std::vector<Task> ret;
    try
    {
      nlohmann::json tasks_json = nlohmann::ordered_json::parse(res->body);
      for (const auto& type : types)
      {
        for (const auto& task : tasks_json[type])
        {
          ret.emplace_back(Task{
            .id = task["id"].get<size_t>(),
            .name = task["title"].get<std::string>(),
            .pass_number = task["pass_number"].get<size_t>(),
            .type = task["category"].get<std::string>(),
            .score = task["score"].get<size_t>(),
            .full_score = task["full_score"].get<size_t>()
          });
        }
      }
    }
    catch (...)
    {
      return {};
    }

    return ret;
  }

  [[nodiscard]] std::optional<Rank> CNSS::get_rank() const
  {
    assert(cli != nullptr);
    auto res = cli->Get("/v1/fullrank", httplib::Headers{{"token", token}});
    if (!res || res->status != 200)
      return std::nullopt;

    Rank ret;
    try
    {
      nlohmann::json rank_json = nlohmann::ordered_json::parse(res->body);
      for (const auto& type : types)
      {
        ret.rank.emplace_back(type, std::vector<RankItem>{});

        for (const auto& item : rank_json[type])
        {
          ret.rank.back().second.emplace_back(RankItem{
            .id = item["ID"].get<int>(),
            .name = item["Name"].get<std::string>(),
            .score = item["Score"].get<int>(),
            .identity = item["Identity"].get<int>()
          });
        }
      }
    }
    catch (...)
    {
      return std::nullopt;
    }

    return ret;
  }

  void CNSS::auth_do(const httplib::Request& req, httplib::Response& res,
                     const std::function<nlohmann::json(const httplib::Request&)>& func) const
  {
    if (req.has_param("admin_password") && req.get_param_value("admin_password") == admin_password)
    {
      auto res_json = func(req);
      res.set_content(res_json.dump(), "application/json");
    }
    else
    {
      if (req.has_param("admin_password"))
      {
        res.set_content(nlohmann::json
                        {
                          {"status", "failed"},
                          {"message", "Incorrect password"}
                        }.dump(), "application/json");
      }
      else
      {
        res.set_content(nlohmann::json
                        {
                          {"status", "failed"},
                          {"message", "Permission denied"}
                        }.dump(), "application/json");
      }
    }
  }

  std::vector<std::string> handle_arrary(const std::string& raw)
  {
    std::vector<std::string> ret{""};
    for (auto& ch : raw)
    {
      if (ch == ',')
        ret.emplace_back("");
      else
        ret.back() += ch;
    }
    return ret;
  }

  std::vector<size_t> handle_num_arrary(const std::string& raw)
  {
    std::vector<size_t> ret;
    auto str = handle_arrary(raw);
    for (auto& r : str)
      ret.emplace_back(std::stoull(r));
    return ret;
  }

  void CNSS::start()
  {
    assert(svr != nullptr);
    EmailSender email_sender{config};
    std::thread monitor{
      [this, &email_sender]
      {
        std::map<size_t, Task> last_tasks;
        auto all_tasks = this->get_tasks();

        for (auto& r : all_tasks)
        {
          if (auto it = std::find(tasks.cbegin(), tasks.cend(), r.id); it != tasks.cend())
            last_tasks.insert(std::make_pair(r.id, r));
        }

        Rank last_rank, last_rank10;
        auto opt = this->get_rank();
        if (opt.has_value())
        {
          last_rank = *opt;
          last_rank10 = last_rank.trunc(10);
          std::cout << last_rank10 << "----------------------------------------\n";
        }
        else
        {
          std::cerr << "Failed to get rank.\n";
          return -1;
        }

        while (true)
        {
          Rank curr_rank;
          auto rank_opt = this->get_rank();
          if (rank_opt.has_value())
            curr_rank = *rank_opt;
          else
          {
            std::cerr << "Failed to get rank.\n";
            return -1;
          }

          // Check task
          std::map<size_t, Task> curr_tasks;
          auto all_tasks = this->get_tasks();
          for (auto& r : all_tasks)
          {
            if (auto it = std::find(tasks.cbegin(), tasks.cend(), r.id); it != tasks.cend())
              curr_tasks.insert(std::make_pair(r.id, r));
          }

          for (auto& curr : curr_tasks)
          {
            auto last = last_tasks.find(curr.first);
            if (last != last_tasks.end())
            {
              if (curr.second.pass_number != last->second.pass_number) // Someone has passed the task
              {
                std::vector<std::pair<int, RankItem>> score_ups;
                RankItem who_passed;
                auto comp = [&curr](auto&& x) { return x.first == curr.second.type; };
                auto curr_subrank =
                    std::find_if(curr_rank.rank.cbegin(), curr_rank.rank.cend(), comp);
                auto last_subrank =
                    std::find_if(last_rank.rank.cbegin(), last_rank.rank.cend(), comp);
                if (curr_subrank != curr_rank.rank.cend() && last_subrank != last_rank.rank.cend())
                {
                  for (const auto& curr_user : curr_subrank->second)
                  {
                    auto last_user = std::find_if(last_subrank->second.cbegin(), last_subrank->second.cend(),
                                            [&curr_user](auto&& u) { return u.id == curr_user.id; });
                    if (last_user != last_subrank->second.cend())
                    {
                      int offset = curr_user.score - last_user->score;
                      if (offset == curr.second.score)
                      {
                        who_passed = curr_user;
                        break;
                      }
                      if(offset > 0)
                      {
                        score_ups.emplace_back(offset, curr_user);
                      }
                    }
                  }
                }
                if (!who_passed.name.empty())
                {
                  notify_task(config["notification"]["smtp"], email_sender, who_passed.name, curr.second.name);
                }
                else if(!score_ups.empty())
                {
                  const auto* closest = &score_ups[0];
                  for(const auto& r : score_ups)
                  {
                    if(std::abs(r.first - static_cast<int>(curr.second.score))
                      < std::abs(closest->first - static_cast<int>(curr.second.score)))
                    {
                      closest = &r;
                    }
                  }
                  notify_task(config["notification"]["smtp"], email_sender,
                    closest->second.name, curr.second.name);
                }
              }
            }
          }

          // Check rank
          auto curr_rank10 = curr_rank.trunc(10);
          for (size_t i = 0; i < curr_rank10.rank.size(); ++i)
          {
            auto changes = get_changes(curr_rank10.rank[i].second, last_rank10.rank[i].second);
            if (!changes.empty())
            {
              notify_rank(config["notification"]["smtp"], email_sender,
                          curr_rank10.rank[i].first, curr_rank10.rank[i].second, changes);
            }
          }

          last_rank = std::move(curr_rank);
          last_rank10 = std::move(curr_rank10);
          last_tasks = std::move(curr_tasks);

          std::this_thread::sleep_for(interval);
        }
      }
    };
    svr->set_mount_point("/", res_path + "/html");
    svr->set_mount_point("/", res_path + "/js");
    svr->set_mount_point("/", res_path + "/icon");

    svr->Get("/api/v1/login", [this](const httplib::Request& req, httplib::Response& res)
    {
      this->auth_do(req, res, [this](const httplib::Request&) -> nlohmann::json
      {
        return {{"status", "success"}, {"message", ""}};
      });
    });

    svr->Get("/api/v1/get_config", [this](const httplib::Request& req, httplib::Response& res)
    {
      this->auth_do(req, res, [this](const httplib::Request&) -> nlohmann::json
      {
        return {
          {"status", "success"},
          {"config", config}
        };
      });
    });

    svr->Get("/api/v1/update_config", [this, &email_sender](const httplib::Request& req, httplib::Response& res)
    {
      this->auth_do(req, res, [this, &email_sender](const httplib::Request& req) -> nlohmann::json
      {
        std::string message = "{";
        // crash
        // if (req.has_param("monitor_server") && config["monitor"]["server"] != req.get_param_value("monitor_server"))
        // {
        //   auto val = req.get_param_value("monitor_server");
        //   this->server = val;
        //   this->config["monitor"]["server"] = val;
        //   delete cli;
        //   cli = new httplib::Client(val);
        //   message += "monitor_server, ";
        // }

        if (req.has_param("monitor_token") && config["monitor"]["token"] != req.get_param_value("monitor_token"))
        {
          auto val = req.get_param_value("monitor_token");
          this->token = val;
          this->config["monitor"]["token"] = val;
          message += "monitor_token, ";
        }

        if (req.has_param("monitor_types"))
        {
          auto val = handle_arrary(req.get_param_value("monitor_types"));
          if (this->types != val)
          {
            this->types = val;
            this->config["monitor"]["types"] = val;
            message += "monitor_types, ";
          }
        }

        if (req.has_param("monitor_tasks"))
        {
          auto val = handle_num_arrary(req.get_param_value("monitor_tasks"));
          if (this->tasks != val)
          {
            this->tasks = val;
            this->config["monitor"]["tasks"] = val;
            message += "monitor_tasks, ";
          }
        }

        if (req.has_param("monitor_interval_in_ms") &&
            std::to_string(config["monitor"]["interval_in_ms"].get<int>()) != req.get_param_value(
              "monitor_interval_in_ms"))
        {
          auto val = std::stoi(req.get_param_value("monitor_interval_in_ms"));
          this->interval = std::chrono::milliseconds(val);
          this->config["monitor"]["interval_in_ms"] = val;
          message += "monitor_interval_in_ms, ";
        }

        if (req.has_param("notification_smtp_server") && config["notification"]["smtp"]["server"] != req.
            get_param_value("notification_smtp_server"))
        {
          auto val = req.get_param_value("notification_smtp_server");
          email_sender.server = val;
          this->config["notification"]["smtp"]["server"] = val;
          message += "notification_smtp_server, ";
        }
        if (req.has_param("notification_smtp_username") && config["notification"]["smtp"]["username"] != req.
            get_param_value("notification_smtp_username"))
        {
          auto val = req.get_param_value("notification_smtp_username");
          email_sender.username = val;
          this->config["notification"]["smtp"]["username"] = val;
          message += "notification_smtp_username, ";
        }
        if (req.has_param("notification_smtp_password") && config["notification"]["smtp"]["password"] != req.
            get_param_value("notification_smtp_password"))
        {
          auto val = req.get_param_value("notification_smtp_password");
          email_sender.passwd = val;
          this->config["notification"]["smtp"]["password"] = val;
          message += "notification_smtp_password, ";
        }
        if (req.has_param("notification_smtp_sender_email") && config["notification"]["smtp"]["sender_email"] != req.
            get_param_value("notification_smtp_sender_email"))
        {
          auto val = req.get_param_value("notification_smtp_sender_email");
          this->config["notification"]["smtp"]["sender_email"] = val;
          message += "notification_smtp_sender_email, ";
        }

        if (req.has_param("notification_smtp_receiver_emails"))
        {
          auto val = handle_arrary(req.get_param_value("notification_smtp_receiver_emails"));
          if (config["notification"]["smtp"]["receiver_emails"] != val)
          {
            this->config["notification"]["smtp"]["receiver_emails"] = val;
            message += "notification_smtp_receiver_emails, ";
          }
        }

        if (req.has_param("new_admin_password") &&
            config["server"]["admin_password"] != req.get_param_value("new_admin_password"))
        {
          auto val = req.get_param_value("new_admin_password");
          this->admin_password = val;
          this->config["server"]["admin_password"] = val;
          message += "admin_password, ";
        }

        if (message != "{")
        {
          std::ofstream config_file(config_path);
          config_file << config.dump();
          config_file.flush();
          config_file.close();
          message.pop_back();
          message.pop_back();
          message += "} 已修改";
        }
        else
        {
          message = "配置没有改动";
        }
        return {
          {"status", "success"},
          {"message", message}
        };
      });
    });
    svr->listen(listen_addr, listen_port);
  }

  Rank Rank::trunc(size_t num) const
  {
    auto ret = rank;
    for (auto& [type, subrank] : ret)
    {
      while (subrank.size() > num)
        subrank.pop_back();
    }
    return {ret};
  }
}
