﻿#ifndef GATEWAY_T_H
#define GATEWAY_T_H

#include <trantor/net/TcpClient.h>
#include <trantor/net/TcpServer.h>
#include <trantor/net/EventLoop.h>

#include <drogon/HttpAppFramework.h>
#include <drogon/HttpClient.h>

#include "pdu/pdu.h"

#include "misc.h"
#include "config.h"

using namespace trantor;
using namespace drogon;

using cchar_t = const char*;
using namespace cuap;

namespace gateway
{
   using tcp_client_t    = std::shared_ptr<trantor::TcpClient>;
   using tcp_conn_t      = const TcpConnectionPtr&;
   using msg_buffer_t    = MsgBuffer*;
   using http_request_t  = const HttpRequestPtr&;
   using http_response_t = const HttpResponsePtr&;

   void setup_bind(config::config_t& cfg, pdu::bind_msg_t& bindmsg)
   {
      bindmsg.set_system_id(cfg.gateway.system_id);
      bindmsg.set_password(cfg.gateway.password);
      bindmsg.set_system_type(cfg.gateway.system_type);
      bindmsg.set_command_id(pdu::CommandIDs::Bind);
      bindmsg.set_command_status(0);
      bindmsg.set_sender_id(0xFFFFFFFF);
      bindmsg.set_receiver_id(0xFFFFFFFF);
      bindmsg.set_command_len(64);
      bindmsg.encode_header();
   }

   template <typename T>
   concept bool pdu_type =  requires (T x)
   {
      { x.command_len() }; { x.command_id() };
      T::buffer;
   };

   namespace send
   {
      namespace samples
      {
         uint8_t unbind[] =
         {
            0x14, 0x00, 0x00, 0x00, 0x66, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff
         };

         uint8_t shake[] =
         {
            0x14, 0x00, 0x00, 0x00, 0x83, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff
         };
      }

      void shake(tcp_client_t client, tcp_conn_t conn, msg_buffer_t msg)
      {
         pdu::shake_msg_t shake (samples::shake, sizeof(samples::shake));
         shake.encode_header();
         conn->send(shake, shake.capacity());
      #ifdef ENABLE_PDU_LOG
         fmt::print_cyan("\n{}. [ {}::shake info ]: Sent Shake Message to: {}\n",
            misc::current_time(), client->name(), conn->peerAddr().toIpPort()
         );
         misc::print_pdu(samples::shake, sizeof(samples::shake));
      #endif
         std::this_thread::sleep_for (std::chrono::milliseconds(250));
      }

      void unbind(tcp_client_t client, tcp_conn_t conn, msg_buffer_t msg)
      {
         pdu::unbind_msg_t unbind (samples::unbind, sizeof(samples::unbind));
         unbind.encode_header();
         conn->send(unbind, unbind.capacity());
         fmt::print_cyan("\n{}. [{}::on_message]: Sent UnBind Message to: {}\n",
            misc::current_time(), client->name(), conn->peerAddr().toIpPort()
         );
         #ifdef ENABLE_PDU_LOG
            misc::print_pdu(unbind);
         #endif
      }

      void db_request(HttpClientPtr& http, string_view_t body)
      {
         HttpRequestPtr req = HttpRequest::newHttpRequest();
         req->setBody(body.data());
         http->sendRequest(req, [ & ](ReqResult result, const HttpResponsePtr& response)
         {
            if (result == ReqResult::Ok && response)
            {
               fmt::print_green("{}. [ send::db_request info ]: Data submitted to dB handler\n", misc::current_time());
            }
            else
            {
               fmt::print_yellow("{}. [ send::db_request info ]: Data not submitted to dB handler, saving...\n", misc::current_time());
            }
            fmt::print(std::flush(std::cout), "");
         });
      }
   }

   struct gateway_t
   {
      enum class data_transfer_mode_t { json, xml };

      gateway_t(misc::cli_config_t& config);

      void build_whitelist();

      void build_abort(pdu_type&,     auto&&);
      void build_begin(pdu_type&,     pdu_type&, auto&&);
      void build_continue(pdu_type&,  pdu_type&, auto&&);

      template <command_id request_type = command_id::begin>
      auto build_http_request(pdu_type& packet);
      void send_http_request(HttpRequestPtr& req);

      void init();

      void on_connect(tcp_conn_t conn);
      void on_conn_error();
      void on_message(tcp_conn_t conn, msg_buffer_t msg);

      void setup_config();
      void setup_data_transfer_mode();

      void run();

      misc::cli_config_t   cli_cfg;
      config::config_t     cfg;

      EventLoopThread      evloop_tcp  = EventLoopThread{"eventloop.thread.tcp"};
      EventLoopThread      evloop_http = EventLoopThread{"eventloop.thread.http"};
      InetAddress          addr;
      tcp_client_t         tcp_client;
      HttpClientPtr        http_client;

      pdu::bind_msg_t      bindmsg;
      pdu::unbind_msg_t    unbindmsg;

      std::set<string>     white_list;
      data_transfer_mode_t data_transfer_mode = data_transfer_mode_t::json;
   };

   gateway_t::gateway_t(misc::cli_config_t& config) : cli_cfg(config)
   {
   }

   void gateway_t::build_whitelist()
   {
      auto& config = this->cfg;
      string tmp, file = config["gateway"]["white-list"].asString();
      fstream ifs (file, fstream::in);
      if (!ifs.is_open())
      {
         fmt::print_yellow("{}. [ gateway_t::build_whitelist info ]: '{}' not found.\n", misc::current_time(), file);
         return;
      }
      while (getline(ifs, tmp))
      {
         white_list.insert(tmp);
         #ifdef ENABLE_PDU_LOG
            fmt::print("{}\n", tmp);
         #endif
      }
      fmt::print_green("{}. [ gateway_t::build_whitelist info ]: Whitelist built from '{}'\n", misc::current_time(), file);
   }

   void gateway_t::build_abort(pdu_type& pdu_req, auto&& fn)
   {
      static char fn_name[] = "build_abort";

      pdu_req.decode_header();

      auto sender_id   = pdu_req.sender_id();
      auto receiver_id = pdu_req.receiver_id();

      fmt::print("{}. [ gateway::build_abort info ]: request: {}", misc::current_time(),
         fmt::format(fmt_req_begin, sender_id, receiver_id, "", "", "")
      );

      HttpRequestPtr req = build_http_request<command_id::abort>(pdu_req);
      http_client->sendRequest(req, [&/*, tconn = std::move(conn)*/](ReqResult result, const HttpResponsePtr& response)
      {
         if (result == ReqResult::Ok && response)
         {
            Json::Value json;
            if (misc::parse_json(json, response->getBody()) and misc::check_json(json))
            {
               fmt::print_green("{}. [ gateway::build_abort info ]: response: {}\n", misc::current_time(), response->getBody());
            }
            else
            {
               fmt::print_red("{}. [ gateway::build_abort error ]: Unable to parse JSON response: {}\n", misc::current_time(), response->body());
            }
         }
         else
         {
            fmt::print_red("{}. [ gateway::build_abort error ]: request to {} failed\n",
               misc::current_time(), cfg.gateway.client.url
            );
         }

         #ifdef ENABLE_PDU_LOG
            misc::print_pdu(pdu_req, pdu_req.command_len());
         #endif
         fn();
      });

   }

   void gateway_t::build_begin(pdu_type& pdu, pdu_type& pdu_req, auto&& fn)
   {
      static char fn_name[] = "build_begin";

      pdu_req.decode_header();
      string msisdn = pdu_req.msisdn();
      if (!white_list.empty() and !white_list.contains(msisdn))
      {
         fmt::print_yellow("{}. [ gateway_t::build_begin warn ]: '{}' not found in white-list, not serving.\n", misc::current_time(), msisdn);
         return ;
      }

      auto sender_id   = pdu_req.sender_id();
      auto receiver_id = pdu_req.receiver_id();
      auto op          = pdu_req.ussd_op_type();

      string service_code = pdu_req.service_code();
      string content      = pdu_req.ussd_content();

      fmt::print("{}. [ gateway::build_begin info ]: request: {}", misc::current_time(),
         fmt::format(fmt_req_begin, sender_id, receiver_id, content, op_name(op), msisdn)
      );

      pdu.set_command_status(0);
      pdu.set_receiver_id(sender_id);
      pdu.set_ussd_ver(pdu::UssdVersion::PHASEII);
      pdu.set_msisdn(msisdn);
      pdu.set_service_code(service_code);
      pdu.set_code_scheme(pdu::CodeScheme::Ox0F);

      HttpRequestPtr req = build_http_request<command_id::begin>(pdu_req);
      http_client->sendRequest(req, [&](ReqResult result, const HttpResponsePtr& response)
      {
         if (result == ReqResult::Ok && response)
         {
            Json::Value json;
            if (misc::parse_json(json, response->getBody()) and misc::check_json(json))
            {
               try
               {
                  fmt::print_green("{}. [ gateway::build_begin info ]: response: {}\n", misc::current_time(), response->getBody());
                  pdu.set_ussd_content(json["content"].asCString());
                  pdu.set_ussd_op_type(json["op_type"].asUInt());
                  pdu.set_command_id(json["command"].asUInt());
               }
               catch(std::exception& e)
               {
                  fmt::print_red("{}. [ gateway::build_begin exception ]: {}\n", misc::current_time(), e.what());
                  pdu.set_command_id(pdu::CommandIDs::End);
               }
            }
            else
            {
               fmt::print_red("{}. [ gateway::build_begin error ]: Unable to parse JSON response: {}\n",
                  misc::current_time(), response->body()
               );
               fmt::print_red(fmt_data_error, misc::current_time(), fn_name, sender_id, cfg.gateway.client.error.invalid_data);
               pdu.set_command_id(pdu::CommandIDs::End);
               pdu.set_ussd_op_type(pdu::USSDOperationTypes::USSN);
               pdu.set_ussd_content(cfg.http.error.invalid_data);
            }
         }
         else
         {
            fmt::print_red(fmt_req_error, misc::current_time(), fn_name, cfg.gateway.client.url, sender_id,
                cfg.gateway.client.error.request_failed
            );
            pdu.set_ussd_op_type(pdu::USSDOperationTypes::USSN);
            pdu.set_command_id(pdu::CommandIDs::End);
            pdu.set_ussd_content(cfg.gateway.client.error.request_failed);
         }

         pdu.set_command_len();
         pdu.encode_header();

         #ifdef ENABLE_PDU_LOG
            misc::print_pdu(pdu, be32toh(pdu.command_len()));
         #endif
         fn();
      });

   }

   void gateway_t::build_continue(pdu_type& pdu, pdu_type& pdu_req, auto&& fn)
   {
      static char fn_name[] = "build_continue";

      pdu_req.decode_header();

      auto sender_id   = pdu_req.sender_id();
      auto receiver_id = pdu_req.receiver_id();
      auto op          = pdu_req.ussd_op_type();

      string msisdn       = pdu_req.msisdn();
      string service_code = pdu_req.service_code();
      string ussd_content = pdu_req.ussd_content();

      fmt::print("{}. [ gateway::build_continue info ]: request: {}", misc::current_time(),
         fmt::format(fmt_req_begin, sender_id, receiver_id, service_code, op_name(op), msisdn)
      );

      pdu.set_command_status(0);
      pdu.set_receiver_id(sender_id);
      pdu.set_ussd_ver(pdu::UssdVersion::PHASEII);
      pdu.set_msisdn(msisdn);
      pdu.set_service_code(service_code);
      pdu.set_code_scheme(pdu::CodeScheme::Ox0F);

      HttpRequestPtr req = build_http_request<command_id::continue_>(pdu_req);
      http_client->sendRequest(req, [&](ReqResult result, const HttpResponsePtr& response)
      {
         if (result == ReqResult::Ok && response)
         {
            Json::Value json;
            if (misc::parse_json(json, response->getBody()) and misc::check_json(json))
            {
               try
               {
                  fmt::print_green("{}. [ gateway::build_continue info ]: response: {}\n", misc::current_time(), response->getBody());
                  pdu.set_ussd_content(json["content"].asString());
                  pdu.set_ussd_op_type(json["op_type"].asUInt());
                  pdu.set_command_id(json["command"].asUInt());
               }
               catch(std::exception& e)
               {
                  fmt::print_red("{}. [ gateway::build_continue exception ]: {}\n", misc::current_time(), e.what());
                  pdu.set_command_id(pdu::CommandIDs::End);
               }
            }
            else
            {
               fmt::print_red("{}. [ gateway::build_continue error ]: Unable to parse JSON response: {}\n", misc::current_time(), response->body());
               fmt::print_red(fmt_data_error, misc::current_time(), fn_name, sender_id, cfg.gateway.client.error.invalid_data);
               pdu.set_command_id(pdu::CommandIDs::End);
               pdu.set_ussd_op_type(pdu::USSDOperationTypes::USSN);
               pdu.set_ussd_content(cfg.gateway.client.error.invalid_data);
            }
         }
         else
         {
            fmt::print_red(fmt_req_error, misc::current_time(), fn_name, cfg.gateway.client.url, sender_id,
               cfg.gateway.client.error.could_not_fetch
            );
            pdu.set_ussd_op_type(pdu::USSDOperationTypes::USSN);
            pdu.set_command_id(pdu::CommandIDs::End);
            pdu.set_ussd_content(cfg.gateway.client.error.could_not_fetch);
         }

         pdu.set_command_len();
         pdu.encode_header();

         #ifdef ENABLE_PDU_LOG
            misc::print_pdu(pdu, be32toh(pdu.command_len()));
         #endif
         fn();
      });

   }

   template <command_id request_type = command_id::begin>
   auto gateway_t::build_http_request(pdu_type& packet)
   {
      static char frmt_begin[] = R"({{ "command": {}, "sid": "0x{:08x}", "length": {}, "msisdn": "{}", "content": "{}" }})""\n";

      HttpRequestPtr req = HttpRequest::newHttpRequest();
      req->setMethod(drogon::Get);
      req->setPath("/");

      if constexpr (request_type == command_id::begin)
      {
         // When command_id = Begin, content is service code. Other times content stays content
         req->setBody(fmt::format(frmt_begin,
               command_id::begin, packet.sender_id(), packet.command_len(), packet.msisdn(), packet.ussd_content()
            )
         );
      }
      else if constexpr (request_type == command_id::continue_)
      {
         req->setBody(fmt::format(frmt_begin,
               command_id::continue_, packet.sender_id(), packet.command_len(), packet.msisdn(), packet.ussd_content()
            )
         );
      }
      else if constexpr (request_type == command_id::abort)
      {
         static char frmt_abort[] = R"({{ "command": {}, "sid": "0x{:08x}", "length": {} }})""\n";
         req->setBody(fmt::format(frmt_abort,
               command_id::abort, packet.sender_id(), packet.command_len()
            )
         );
      }
      else if constexpr (request_type == command_id::bind)
      {
         req->setBody(fmt::format(R"({{ "command": {}, "pdu-status": {}, "length": {}, "system_id": "{}" }})""\n",
               command_id::bind, packet.command_status(), packet.command_len(), packet.system_id()
            )
         );
      }
      fmt::print("{}. [ gateway::build_http_request info ]: request: {}", misc::current_time(), req->body());
      return req;
   }

   void gateway_t::init()
   {
      tcp_client  = std::make_shared<trantor::TcpClient>(evloop_tcp.getLoop(), addr, "gateway");
      tcp_client->setConnectionCallback([&] (tcp_conn_t conn)  { on_connect(conn); });
      tcp_client->setConnectionErrorCallback([&] { on_conn_error(); });
      tcp_client->setMessageCallback([&] (tcp_conn_t conn, msg_buffer_t msg) { on_message(conn, msg); } );
      tcp_client->connect();
   }

   void gateway_t::send_http_request(HttpRequestPtr& req)
   {
      http_client->sendRequest(req, [&](ReqResult result, const HttpResponsePtr& response)
      {
         if (result == ReqResult::Ok && response)
         {
            fmt::print_green("{}. [ gateway::send_http_request info ]: response: {}\n", misc::current_time(), response->getBody());
         }
         else
         {
            fmt::print_red("{}. [ gateway::send_http_request error ]: request to {} failed\n",
               misc::current_time(), cfg.gateway.client.url
            );
            /// TODO: Error to be displayed to mobile be defined in config file
         }
      });
   }

   void gateway_t::on_connect(tcp_conn_t conn)
   {
      if (conn->connected())
      {
         std::string raddr = conn->peerAddr().toIpPort();
         fmt::print_cyan("{}. [ {}::on_connection info ]: Connected to: {}\n", misc::current_time(), tcp_client->name(), raddr);
         setup_bind(cfg, bindmsg);

         #ifdef ENABLE_PDU_LOG
            misc::print_pdu(bindmsg);
         #endif

         conn->send(bindmsg, bindmsg.capacity());
         fmt::print_green("{}. [ {}::on_connection info ]: Sent Bind Message to: {}\n", misc::current_time(), tcp_client->name(), raddr);
      }
      else
      {
         fmt::print_red("{}. [ gateway::on_connection error ]: Disconnected.\n", misc::current_time());
         init();
      }
      fmt::print(std::flush(std::cout), "");
   }

   void gateway_t::on_conn_error()
   {
      using namespace std::literals;

      fmt::print_red("{}. [ gateway_t::on_conn_error error ]: Connection Lost.\n", current_time());
      fmt::print(std::flush(std::cout), "");
      std::this_thread::sleep_for(10s);
      init();
   }

   void gateway_t::on_message(tcp_conn_t conn, msg_buffer_t msg)
   {
      static std::atomic_int id = 0;
      if (conn->connected())
      {
         string_view_t data { msg->peek(), msg->readableBytes() };
         auto cmd = htobe32(header::command_id(data));

      #ifdef ENABLE_PDU_LOG
         //fmt::print_green(fmt_cmdid, cmd, pdu_name(cmd));
         misc::print_pdu(data.data(), data.size());
      #endif

         switch (cmd)
         {
            case CommandIDs::BindResp:
            {
               pdu::bind_resp_t bindresp(msg->peek(), msg->readableBytes() );
               bindresp.decode_header();
               if (bindresp.command_status() == 0)
               {
                  fmt::print_green("{}. [ {}::on_message info ]: Bind Successful!\n", misc::current_time(), tcp_client->name());
               }
               else
               {
                  fmt::print_red("{}. [ {}::on_message error ]: Bind Failed!\n", misc::current_time(), tcp_client->name());
               }
               fmt::print(std::flush(std::cout), "");
               HttpRequestPtr req = build_http_request<command_id::bind>(bindresp);
               send_http_request(req);
               msg->retrieveAll();
            }
            break;

            case CommandIDs::ShakeResp:
               send::shake(tcp_client, conn, msg);
               msg->retrieveAll();
            break;

            /// Listens to Begin from USSDC
            case CommandIDs::Begin:
            {
               continue_msg_t pdu, pdu_req { msg->peek(), msg->readableBytes() };
               pdu.set_sender_id(++id);
               build_begin(pdu, pdu_req, [&, tconn = std::move(conn)] {
                  tconn->send(pdu, pdu.capacity());
               });
            }
            break;

            /// Listens to Continue from USSDC
            case CommandIDs::Continue:
            {
               continue_msg_t pdu, pdu_req { msg->peek(), msg->readableBytes() };
               pdu.set_sender_id(id);
               build_continue(pdu, pdu_req, [&, tconn = std::move(conn)] {
                  tconn->send(pdu, pdu.capacity());
               });
            } break;

            case CommandIDs::End:
            break;

            case CommandIDs::Abort:
            {
               abort_msg_t pdu_req { msg->peek(), msg->readableBytes() };
               build_abort(pdu_req, [&] {
                  //
               });
            } break;

            /// UssdBindResp can be sent only by the USSDC to the service application.
            case CommandIDs::UnBindResp:
            {
               msg->retrieveAll();
               fmt::print_green("{}. [ {}::on_message info ]: UnBind Successful!\n", misc::current_time(), tcp_client->name());
            } break;

            default:
            {
               fmt::print_red("{}. [ {}::on_message info ]: Unknown Command\n", misc::current_time(), tcp_client->name());
               msg->retrieveAll();
            }
         }
      }
      else
      {
         fmt::print_red("{}. [ gateway_t::on_message error ]: Not connected\n", current_time());
      }
      fmt::print(std::flush(std::cout), "");
      msg->retrieveAll();
   }

   void gateway_t::setup_config()
   {
      if (!cli_cfg.config.empty())
      {
         cfg.read_then_parse<config::config_type::json>(cli_cfg.config);
         addr        = InetAddress(cfg.gateway.host, cfg.gateway.port);
         http_client = HttpClient::newHttpClient(cfg.gateway.client.url, evloop_http.getLoop());
         cli_cfg.rurl= cfg.gateway.client.url;
      }
      else
      {
         addr = InetAddress(cli_cfg.chost, cli_cfg.cport);
         http_client = HttpClient::newHttpClient(cli_cfg.rurl, evloop_http.getLoop());
      }
   }

   void gateway_t::setup_data_transfer_mode()
   {
      string md = cfg["data-transfer-mode"].asString();
      if (md == "xml")
      {
         data_transfer_mode = data_transfer_mode_t::xml;
         format::req_cont = R"(<continue>
   <sid>{}</sid>
   <rid>{}</rid>
   <service_code>{}</service_code>
   <operation>{}</operation>
   <msisdn>{}</msisdn>
   <content>{}</content>
</continue>)";
      }

   }

   void gateway_t::run()
   {
      Logger::setLogLevel(Logger::LogLevel::kError);
      setup_config();
      setup_bind(cfg, bindmsg);
      build_whitelist();
      init();

      evloop_tcp.run();
      evloop_http.run();
      evloop_tcp.wait();
   }

   void main(int argc,  char* argv[])
   {
      if (misc::setup_cli(misc::args, argc, argv))
      {
         misc::cli_config_t cfg;
         misc::setup_config(cfg);
         gateway_t gateway(cfg);
         gateway.run();
      }
   }
}


#endif // GATEWAY_T_H
