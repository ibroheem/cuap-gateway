#ifndef misc_h
#define misc_h

#include <chrono>
#include <cstdlib>

#include "argparser/argparser.hpp"
#include "fmt-5.h"

using std::is_same_v;
using std::is_integral_v;
using std::string;

#define NL printf("\n");
#define DL printf("\n\n");

char fmt_cmdid []    = "Command ID  : 0x{:08x}, {}\n";
char fmt_req_cont[]  = R"({{ "sid": "0x{:08x}", "rid": "0x{:08x}", "service_code": "{}", "operation": "{}", "msisdn": "{}", "content": "{}" }})""\n";
char fmt_resp_ok[]   = R"({{ "status": {}, "sid": "0x{:08x}", "rid": "0x{:08x}", "service_code": "{}", "operation": "{}", "msisdn": "{}", "content": "{}" }})""\n";

char fmt_req_begin[]  = R"({{ "sid": "0x{:08x}", "rid": "0x{:08x}", "service_code": "{}", "operation": "{}", "msisdn": "{}" }})""\n";
char fmt_req_error[]  = R"({}. [ gateway::{} error ]: request to {} failed: {{ "sid": "0x{:08x}", "message": "{}" }})""\n";
char fmt_data_error[] = R"({}. [ gateway::{} error ]: {{ "sid": "0x{:08x}", "message": "{}" }})""\n";

namespace format
{
   string cmd_id    = "Command ID  : 0x{:08x}, {}\n";
   string req_cont  = R"({{ "sid": "0x{:08x}", "rid": "0x{:08x}", "service_code": "{}", "operation": "{}", "msisdn": "{}", "content": "{}" }})""\n";
}

namespace misc
{
   enum class time_format { default_, logfile };

   ap::argmap args;

   template <time_format tm = time_format::default_>
   std::string current_time()
   {
      std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
      char s[20] {0};
      if constexpr (tm == time_format::default_)
      {
         std::strftime(&s[0], sizeof(s), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
      }
      else if constexpr (tm == time_format::logfile)
      {
         std::strftime(&s[0], sizeof(s), "%Y-%m-%d", std::localtime(&now));
      }
      return s;
   }

   void print_pdu(const auto pdu, int size)
   {
      constexpr int div = 16;
      int sz  = size / div, j = 0, k = 0;

      for (int i = 0, l = 1; i < sz; ++i, ++l)
      {
         fmt::print("\t");
         for (; j < (div * l); ++j)
         {
            fmt::print("{:02x} ", pdu[j] >= 255 or pdu[j] < 0 ? 0xFF : pdu[j]);
         }
         j = div * l;
         fmt::print("\t");
         for (; k < (div * l); ++k)
         {
            fmt::print("{}", pdu[k] == 0 or pdu[i] == 255 ? '.' : char(pdu[k]));
         }
         k = j;
         fmt::print("\n");
      }

      fmt::print("\t");
      for (int i = k; i < size; ++i)
      {
         fmt::print("{:02x} ", pdu[j] >= 255 or pdu[j] < 0 ? 0xFF : pdu[j]);
      }

      for (int i = 0; i < 8; ++i)
         fmt::print(" ");

      for (int i = k; i < size; ++i)
      {
         fmt::print("{}", pdu[i] == 0 or pdu[i] == 255 ? '.' : char(pdu[i]));
      }
      fmt::print("\n");
   }

   inline void print_pdu(const auto& pdu)
   {
      print_pdu(pdu.data(), pdu.capacity());
   }

   bool setup_cli(ap::argmap& args, int argc, char* argv[]);
}

namespace misc
{
   using namespace cuap;

   template <typename T>
   cchar* pdu_name(T val)
   {
      using namespace cuap;

      uint32_t cmd_id;

      if constexpr (!is_same_v<T, CommandIDs> and !is_integral_v<T>)
         cmd_id = header::command_id(val);
      else if constexpr (is_same_v<T, CommandIDs> or is_same_v<T, uint32_t> or is_integral_v<T>)
         cmd_id = val;

      switch (cmd_id)
      {
         case CommandIDs::Bind:
            return "Bind";
            break;

         case CommandIDs::BindResp:
            return "BindResp";
            break;

         case CommandIDs::UnBind:
            return "UnBind";
            break;

         case CommandIDs::UnBindResp:
            return "UnBindResp";
            break;

         case CommandIDs::Shake:
            return "Shake";
            break;

         case CommandIDs::ShakeResp:
            return "ShakeResp";
            break;

         case CommandIDs::Abort:
            return "Abort";
            break;

         case CommandIDs::Begin:
            return "Begin";
            break;

         case CommandIDs::End:
            return "End";
            break;

         case CommandIDs::Continue:
            return "Continue";
            break;

         case CommandIDs::Switch:
            return "Switch";
            break;

         case CommandIDs::SwitchBegin:
            return "SwitchBegin";
            break;

         case CommandIDs::ChargeInd:
            return "ChargeInd";
            break;

         case CommandIDs::ChargeIndResp:
            return "ChargeIndResp";
            break;

         default:
            //
            break;
      }

      return "Unknown Command";
   }

   cchar* op_name(pdu::USSDOperationTypes val)
   {
      using pdu::USSDOperationTypes;
      switch (val)
      {
         //case USSDOperationTypes::PSSR:
         case USSDOperationTypes::USSR:
            return "USSR";
            break;

         case USSDOperationTypes::Release_Req:
            return "Release_Req";
            break;

         case USSDOperationTypes::USSN:
            return "USSN";
            break;

         case USSDOperationTypes::USSDC_Resp:
            return "USSDC_Resp";
            break;

         default:
            return "";
            break;
      }
   }

   cchar* op_name(uint8_t val)
   {
      return op_name(static_cast<pdu::USSDOperationTypes>(val));
   }

   void test_login()
   {
      pdu::unbind_msg_t ub;
      ub.decode_header();
      fmt::print_green("Command: {} Message\n", pdu_name(ub));
      misc::print_pdu(ub);
      DL;

      pdu::shake_msg_t shk;
      shk.decode_header();
      fmt::print_green("Command: {} Message\n", pdu_name(shk));
      misc::print_pdu(shk);
      DL;

      pdu::generic_header_t<pdu::CommandIDs::Shake> shkresp;
      shkresp.decode_header();
      fmt::print_green("Command: {} Message\n", pdu_name(shkresp));
      misc::print_pdu(shkresp);
   }

}

namespace misc
{
   struct cli_config_t
   {
      string host = "0.0.0.0", chost, rurl, rhost, config;
      int port     = 9900 /** Issue Tracker takes 9900 - 9904 */, cport, rport= 0/** remote url port */;
      int num_conns = 10, threads = 4, timeout = 5, report_every = 10; // timeout and report_every are in secs
   };

   enum command_id
   {
      bind   = 0x00000065, unbind = 0x00000066, bindresp = 0x00000067, unbindresp= 0x00000068,
      begin  = 0x0000006f, continue_ = 0x00000070, end = 0x71, abort = 0x72,
      switch_ = 0x00000074, switchbegin = 0x00000077,
      chargeind = 0x00000075, chargeindresp = 0x00000076,
      shake  = 0x00000083, shakeresp = 0x00000084, error = 0x00
   };

   bool check_json(Json::Value& body)
   {
      if (body.isMember("command"))
      {
         decltype(auto) cmd = body["command"].asUInt();
         if (cmd == command_id::begin or cmd == command_id::continue_ or cmd == command_id::end)
         {
            return body.isMember("msisdn")  and body.isMember("content");
         }
         else if (body["command"].asUInt() == command_id::bind)
         {
            return body.isMember("system_id");
         }
      }
      return false;
   }

   auto parse_json(Json::Value& json, string_view_t text)
   {
      Json::Reader reader;
      return reader.parse(text.data(), json);
   }

   bool setup_cli(ap::argmap& args, int argc, char* argv[])
   {
      ap::parser p;
      p.init(argc, argv);
      p.set_caption("A CUAP gateway that allows data transfer via HTTP interface.");
      p.add("-c", "--config",    "config file",     ap::mode::OPTIONAL);

      args = p.parse();

      if (args.parsed_successfully())
      {
         return true;
      }

      fmt::print(std::clog, "{}. [ misc::setup_cli error ]: Error Parsing Command Line Arguements\n", misc::current_time());
      return false;
   }

   void setup_config(cli_config_t& cfg)
   {
      auto fn = [&] <typename T> (std::string arg, T val)
      {
         bool empty = args[arg].empty();
         if constexpr (std::is_integral_v<T>)
         {
            return !empty ? std::stoi(args[arg]) : val;
         }
         else
         {
            return !empty ? args[arg] : val;
         }
      };

      cfg.config   = fn("--config",  cfg.config);
   }
}

#endif//misc_h

