#ifndef types_h
#define types_h

#ifdef __clang__
   #define constexpr_t
#else
   #define constexpr_t constexpr
#endif // __clang__

#if (CUAP_IS_BIG_ENDIAN == 1)
   #define htobe16_mod htobe16
   #define htobe32_mod htobe32
#else
   #define htobe16_mod
   #define htobe32_mod
#endif // USE_BIG_ENDIAN

#include "buffer.h"

namespace cuap
{
   namespace pdu {
      constexpr short HEADER_LEN = 20;

      constexpr static uint8_t Ox00 = 0x00;
      constexpr static uint8_t OxFF = 0xFF;
      constexpr static uint32_t OxFFFFFFFF = 0xFFFFFFFF;
      constexpr static uint32_t Ox00000000 = 0x00000000;

      enum CommandIDs : uint32_t
      {
         Bind   = 0x00000065, UnBind = 0x00000066, BindResp = 0x00000067, UnBindResp= 0x00000068,
         Begin  = 0x0000006f, Continue = 0x00000070, End = 0x71, Abort = 0x72,
         Switch = 0x00000074, SwitchBegin = 0x00000077,
         ChargeInd = 0x00000075, ChargeIndResp = 0x00000076,
         Shake  = 0x00000083, ShakeResp = 0x00000084, Error = 0x00
      };

      enum Header : uint8_t
      { CommandLength = 0, CommandID = 4, CommandStatus = 8, SenderID = 12, ReceiverID = 16};

      enum BindBody : uint8_t
      { System_ID  = 20, Password = 31, System_Type = 40, Interface_Version = 53 };

      enum BeginBody : uint8_t
      { Ussd_Version = 20, Ussd_Op_Type = 21, MsIsdn = 22, Service_Code = 43, Code_Scheme = 64, Ussd_Content = 65 };

      enum class Switch : uint8_t
      { SwitchMode = 20, MsIsdn = 21, Org_Service_Code = 42, Dest_Service_Code = 63, Ussd_Content = 84 };

      enum class SwitchBegin : uint8_t
      { Ussd_Version = 20, Ussd_Op_Type = 21, MsIsdn = 22, Org_Service_Code = 43, Dest_Service_Code = 64,
        Code_Scheme = 85, Ussd_Content = 86 };

      enum class ChargeInd : uint8_t
      { ChargeRatio = 20, ChargeType = 24, ChargeSource = 28, ChargeLocation = 39, };

      enum UssdVersion
      {
         PHASEI = 0x10, PHASEII = 0x20, PHASEII_plus = 0x25
      };

      /** USSD Operation types*/
      enum USSDOperationTypes : uint8_t
      {
         /**unstructured supplementary services request (USSR) message sent from an SP to the USSDC.*/
         USSR = 0x01,
         /** process unstructured supplementary service data request (PSSR) */
         PSSR = 0x01,
         /**unstructured supplementary service data notify (USSN) message sent from an SP to the USSDC.*/
         USSN = 0x02,
         /**response sent from the USSDC to an SP*/
         USSDC_Resp = 0x03,
         /**BEGIN message sent from an SP to release a session*/
         Release_Req = 0x04
      };

      using USSDOpTypes = USSDOperationTypes;

      /**The value is 0x44 in a message sent from the USSDC to an SP. <br>
         In a message sent from an SP to the USSDC, the field indicates the coding scheme that the USSDC
         must use for USSD strings to be sent to MSs. Common coding schemes are as
         follows:
      */
      enum CodeScheme : uint8_t
      {
         /**7-bit coding scheme*/
         Ox0F = 0x0F,
         /**8-bit coding scheme*/
         Ox44 = 0x44,
         /** 0x11 and 0x48, 16-bit coding scheme, */
         Ox11 = 0x11,
         /** 0x11 and 0x48, 16-bit coding scheme, */
         Ox48 = 0x48
      };

      using CHARGEIND_RESP = Header;

      template <class T = uint8_t, uint16_t SZ = 20>
      struct basic_header : public misc::static_buffer<T, SZ>
      {
         using misc::static_buffer<T, SZ>::static_buffer;

         void set_command_len()                { this->assign((uint32_t)this->size(), Header::CommandLength); }
         void set_command_len(uint32_t val)    { this->assign(val, Header::CommandLength); }
         void set_command_id(uint32_t val)     { this->assign(val, Header::CommandID); }
         void set_command_status(uint32_t val) { this->assign(val, Header::CommandStatus); }
         void set_sender_id(uint32_t val)      { this->assign(val, Header::SenderID); }
         void set_receiver_id(uint32_t val)    { this->assign(val, Header::ReceiverID); }

         const uint32_t command_len()  const  { return this->get_u32(Header::CommandLength); }
         uint32_t command_id()  const   { return this->get_u32(Header::CommandID); }
         uint32_t command_status() const { return this->get_u32(Header::CommandStatus); }
         uint32_t sender_id()   const   { return this->get_u32(Header::SenderID); }
         uint32_t receiver_id() const   { return this->get_u32(Header::ReceiverID); }

         // Methods to test fot the command type/id

         bool is_Bind()       { return command_id() == 0x65; }
         bool is_UnBind()     { return command_id() == 0x66; }

         bool is_BindResp()   { return command_id() == 0x67; }
         bool is_UnBindResp() { return command_id() == 0x68; }

         bool is_Begin()      { return command_id() == 0x6f; }
         bool is_Continue()   { return command_id() == 0x70; }

         bool is_End()        { return command_id() == 0x71; }
         bool is_Abort()      { return command_id() == 0x72; }

         bool is_Switch()     { return command_id() == 0x74; }
         bool is_SwitchBegin(){ return command_id() == 0x77; }

         bool is_ChargeInd()  { return command_id() == 0x75; }
         bool is_ChargeIndResp()  { return command_id() == 0x76; }

         bool is_Shake()      { return command_id() == 0x83; }
         bool is_ShakeResp()  { return command_id() == 0x84; }

         void encode_header()
         {
            set_command_len(htobe32(command_len()));
            set_command_id(htobe32(command_id()));
            set_command_status(htobe32(command_status()));
            set_sender_id(htobe32(sender_id()));
            set_receiver_id(htobe32(receiver_id()));
         }

         void decode_header()
         {
            encode_header();
         }
      };

      using header_msg_t = basic_header<uint8_t, 20>;
      using abort_msg_t  = basic_header<uint8_t, 20>;
   }
}

namespace cuap::pdu
{
   //using namespace pdu;
}

#endif//types_h
