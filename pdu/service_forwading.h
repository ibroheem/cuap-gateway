#ifndef service_forwading_h
#define service_forwading_h

#include "session_sa.h"

//! Service forwarding

/*! SF
*/

namespace cuap::pdu
{
   struct basic_switch_msg : public basic_header<uint8_t, 268>
   {
         using basic_header<uint8_t, 268>::basic_header;
      public:
         inline uint8_t switch_mode() { return buffer[static_cast<uint8_t>(Switch::SwitchMode)]; }
         void   set_switch_mode(uint8_t swtch_mode)
         {
            buffer[static_cast<uint8_t>(Switch::SwitchMode)] = swtch_mode;
         }

         template <uint32_t N>
         void msisdn(uint8_t(&dest)[N])
         {
            misc::set_null(dest);
            memcpy(dest, &buffer[static_cast<uint8_t>(Switch::MsIsdn)], 21); /** MSISDN is 21  byte octet string */
         }

         void set_msisdn(const uint8_t* _msisdn, size_t sz)
         {
            assign_n(_msisdn, sz,
                     static_cast<uint8_t>(Switch::MsIsdn),
                     static_cast<uint8_t>(Switch::Org_Service_Code));
         }

         template <uint32_t N>
         void originating_SC(uint8_t(&dest)[N])
         {
            misc::set_null(dest);
            memcpy(dest, &buffer[static_cast<uint8_t>(Switch::Org_Service_Code)], 21);
            /** Org_Service_Code is 21  byte octet string */
         }

         void set_originating_SC(const uint8_t* _orig_sc, size_t sz)
         {
            assign_n(_orig_sc, sz,
                     static_cast<uint8_t>(Switch::Org_Service_Code),
                     static_cast<uint8_t>(Switch::Dest_Service_Code));
         }

         template <uint32_t N>
         void destination_SC(uint8_t(&dest)[N])
         {
            misc::set_null(dest);
            memcpy(dest, &buffer[static_cast<uint8_t>(Switch::Dest_Service_Code)], 21);
            /** Org_Service_Code is 21  byte octet string */
         }

         void set_destination_SC(const uint8_t* _dest_sc, size_t sz)
         {
            assign_n(_dest_sc, sz,
                     static_cast<uint8_t>(Switch::Dest_Service_Code),
                     static_cast<uint8_t>(Switch::Ussd_Content));
         }

         void ussd_content (uint8_t* dest, size_t sz)
         {
            uint8_t content_offset = static_cast<uint8_t>(Switch::Ussd_Content);
            misc::set_null(dest, sz);
            memcpy(dest,
                   &buffer[content_offset],
                   command_len() - content_offset);
         }

         void set_ussd_content(const uint8_t* srv_code, size_t sz)
         {
            uint8_t content_offset = static_cast<uint8_t>(Switch::Ussd_Content);
            assign_n(srv_code, sz, content_offset, content_offset + 182);
         }

   };

   struct basic_switch_begin_msg : public pdu::basic_header<uint8_t, 268>
   {
         using basic_header<uint8_t, 268>::basic_header;

      public:

         inline uint8_t ussd_ver() { return buffer[static_cast<uint8_t>(SwitchBegin::Ussd_Version)]; }
         void   set_ussd_ver(uint8_t ussd)
         {
            buffer[static_cast<uint8_t>(SwitchBegin::Ussd_Version)] = ussd;
         }

         inline uint8_t ussd_op_type() { return buffer[static_cast<uint8_t>(SwitchBegin::Ussd_Op_Type)]; }
         void   set_ussd_op_type(uint8_t op_type)
         {
            buffer[static_cast<uint8_t>(SwitchBegin::Ussd_Op_Type)] = op_type;
         }

         inline uint8_t code_scheme() { return buffer[static_cast<uint8_t>(SwitchBegin::Code_Scheme)]; }
         void   set_code_scheme(uint8_t csch)
         {
            buffer[static_cast<uint8_t>(SwitchBegin::Code_Scheme)] = csch;
         }

         template <uint32_t N>
         void msisdn(uint8_t(&dest)[N])
         {
            misc::set_null(dest);
            memcpy(dest, &buffer[static_cast<uint8_t>(SwitchBegin::MsIsdn)], 21); /** MSISDN is 21  byte octet string */
         }

         void set_msisdn(const uint8_t* _msisdn, size_t sz)
         {
            assign_n(_msisdn, sz,
                     static_cast<uint8_t>(SwitchBegin::MsIsdn),
                     static_cast<uint8_t>(SwitchBegin::Org_Service_Code));
         }

         template <uint32_t N>
         void originating_SC(uint8_t(&dest)[N])
         {
            misc::set_null(dest);
            memcpy(dest, &buffer[static_cast<uint8_t>(SwitchBegin::Org_Service_Code)], 21);
            /** Org_Service_Code is 21  byte octet string */
         }

         void set_originating_SC(const uint8_t* _orig_sc, size_t sz)
         {
            assign_n(_orig_sc, sz,
                     static_cast<uint8_t>(SwitchBegin::Org_Service_Code),
                     static_cast<uint8_t>(SwitchBegin::Dest_Service_Code));
         }

         template <uint32_t N>
         void destination_SC(uint8_t(&dest)[N])
         {
            misc::set_null(dest);
            memcpy(dest, &buffer[static_cast<uint8_t>(SwitchBegin::Dest_Service_Code)], 21);
            /** Org_Service_Code is 21  byte octet string */
         }

         void set_destination_SC(const uint8_t* _dest_sc, size_t sz)
         {
            assign_n(_dest_sc, sz,
                     static_cast<uint8_t>(SwitchBegin::Dest_Service_Code),
                     static_cast<uint8_t>(SwitchBegin::Ussd_Content));
         }

         void ussd_content (uint8_t* dest, size_t sz)
         {
            uint8_t content_offset = static_cast<uint8_t>(SwitchBegin::Ussd_Content);
            misc::set_null(dest, sz);
            memcpy(dest,
                   &buffer[content_offset],
                   command_len() - content_offset);
         }

         void set_ussd_content(const uint8_t* srv_code, size_t sz)
         {
            uint8_t content_offset = static_cast<uint8_t>(SwitchBegin::Ussd_Content);
            assign_n(srv_code, sz, content_offset, content_offset + 182);
         }
   };
}

namespace cuap::pdu
{
   /// Represents a SWITCH message
   using switch_msg_t = basic_switch_msg;

   /// Represents a  SWITCH_BEGIN message
   using switch_begin_msg_t = basic_switch_begin_msg;
}


#endif//service_forwading_h
