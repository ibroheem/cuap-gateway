#ifndef bind_family_h
#define bind_family_h

#include "types.h"
#include "functions.h"

namespace cuap::pdu
{
   using namespace misc;
   using namespace concepts;

   struct basic_bind_resp : public basic_header<uint8_t, 64>
   {
      using basic_header<uint8_t, 64>::basic_header;

      template <class T, uint16_t N>
      void  set_system_id(T(&id)[N])
      {
         assign_n(id, N, BindBody::System_ID, BindBody::Password);
      }

      void  set_system_id(string_view_t id)
      {
      #if ENABLE_DEBUG
         fmt::print("System ID: {}\n", id);
      #endif // ENABLE_DEBUG
         assign_n(id.data(), id.size(), BindBody::System_ID, BindBody::Password);
      }

      void  set_system_id(const uchar_t id, size_t sz)
      {
         assign_n(id, sz, BindBody::System_ID, BindBody::Password);
      }

      /// @brief Gets System_ID from Message
      /// sets @id to null, then assign the target string to it.
      template <class T2, uint N>
      void system_id(T2(&id)[N])
      {
         misc::set_null(id);
         functions::get_body_str(*this, id, BindBody::System_ID, 11);
      }

      std::string system_id()
      {
         string ret;
         for (int i = BindBody::System_ID; i < (BindBody::System_ID + 11) and i < capacity(); ++i)
         {
            if (buffer[i] == '\0') { break; }
            ret += buffer[i];
         }
         return ret;
      }
   };

   struct basic_bind_msg : public basic_bind_resp
   {
      using basic_bind_resp::basic_bind_resp;

      std::string system_id()
      {
         string ret;
         for (int i = BindBody::System_ID; i < BindBody::Password and i < capacity(); ++i)
         {
            if (buffer[i] == '\0') { break; }
            ret += buffer[i];
         }
         return ret;
      }

      std::string password()
      {
         string ret;
         for (int i = BindBody::Password; i < BindBody::System_Type and i < capacity(); ++i)
         {
            if (buffer[i] == '\0') { break; }
            ret += buffer[i];
         }
         return ret;
      }

      void  set_system_id(string_view_t id)
      {
         assign_n(id.data(), id.size(), BindBody::System_ID, BindBody::Password);
      }

      /// @brief Gets Password from Message
      /// sets @pass to null, then assign the target string to it.
      template <typename T, size_t N>
      void password(T(&pass)[N])
      {
         set_null(pass);
         functions::get_body_str(*this, pass, BindBody::Password, 11);
      }

      void set_password(uchar_t pass, size_t sz)
      {
         assign_n(pass, sz, BindBody::Password, BindBody::System_Type);
      }

      void set_password(string_view_t pass)
      {
         assign_n(pass.data(), pass.size(), BindBody::Password, BindBody::System_Type);
      }

      template <typename T, size_t N>
      void system_type(T(&sys_type)[N])
      {
         set_null(sys_type);
         functions::get_body_str(*this, sys_type, BindBody::System_Type, 11);
      }

      void set_system_type(string_view_t sys_type)
      {
         assign_n(sys_type.data(), sys_type.size(), BindBody::System_Type , BindBody::Interface_Version);
      }

      void set_system_type(uchar_t tp, size_t sz)
      {
         this->assign_n(tp, sz, BindBody::System_Type, BindBody::Interface_Version);
      }

      uint32_t interface_ver() { return get_u32(BindBody::Interface_Version); }
      void set_interface_ver(uint32_t iv)
      {
         memcpy(&buffer[BindBody::Interface_Version], &iv, sizeof(uint32_t));
      }

   };

}

namespace cuap
{
   namespace pdu {
      using bind_msg_t  = basic_bind_msg;
      using bind_resp_t = basic_bind_resp;

      /// These don't have body, hence it's just alias of basic_header
      template <pdu::CommandIDs CMD_ID, typename T = uint8_t>
      struct generic_header_t : public basic_header<T, 20>
      {
         using basic_header<T, 20>::basic_header;

         generic_header_t() {
            this->set_command_len(20);
            this->set_command_id(CMD_ID);
            this->set_command_status(0x00);
            this->set_receiver_id(pdu::OxFFFFFFFF);
            this->set_sender_id(pdu::OxFFFFFFFF);
            this->encode_header();
         }
      };

      using shake_msg_t   = generic_header_t<pdu::CommandIDs::Shake>;
      using shake_resp_t  = generic_header_t<pdu::CommandIDs::ShakeResp>;
      using unbind_msg_t  = generic_header_t<pdu::CommandIDs::UnBind>;
      using unbind_resp_t = generic_header_t<pdu::CommandIDs::UnBindResp>;

   }
}

#endif//bind_family_h
