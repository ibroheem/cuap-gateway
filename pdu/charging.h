#ifndef charging_h
#define charging_h

#include "types.h"

//! Charging Message

namespace cuap::pdu
{
   using namespace misc;
   using cuap::ChargeInd;

   struct basic_charge_ind : public cuap::pdu::basic_header<uint8_t, 50>
   {
         using basic_header<uint8_t, 50>::basic_header;
      public:

         uint32_t charge_ratio() { return get_u32(static_cast<uint8_t>(ChargeInd::ChargeRatio)); }
         void     set_charge_ratio(uint32_t cr)
         {
            assign(cr, static_cast<uint8_t>(ChargeInd::ChargeRatio));
         }

         uint32_t charge_type() { return get_u32(static_cast<uint8_t>(ChargeInd::ChargeType)); }
         void     set_charge_type(uint32_t cr)
         {
            assign(cr, static_cast<uint8_t>(ChargeInd::ChargeType));
         }

         template <uint32_t N>
         void charge_src(uint8_t(&dest)[N])
         {
            misc::set_null(dest);
            memcpy(dest, &buffer[static_cast<uint8_t>(ChargeInd::ChargeSource)], 21);
            /** ChargeSource is 21 byte octet string */
         }

         /// Charge Source
         /// Charging source ID,  which contains an SP's enterprise ID and service code
         void set_charge_src(const uint8_t* chsrc, size_t sz)
         {
            assign_n(chsrc, sz,
                     static_cast<uint8_t>(ChargeInd::ChargeLocation),
                     static_cast<uint8_t>(ChargeInd::ChargeSource));
         }

         inline  uint8_t charge_loc() { return buffer[static_cast<uint8_t>(ChargeInd::ChargeLocation)]; }
         /// Charging address
         /**  \param chloc <br>
          **  0x00: Both the USSDC and the USSDGateway generate charging bills. <br>
              0x01: Only the USSDC generates charging bills.<br>
              0x02: Only the USSDGateway generates charging bills<br>
         */
         void    set_charge_loc(uint8_t chloc)
         {
            buffer[static_cast<uint8_t>(ChargeInd::ChargeLocation)] = chloc;
         }
   };
}

namespace cuap::pdu
{
   using namespace pdu;
   using chargeind_t      = basic_charge_ind;
   using chargeind_resp_t = basic_header<uint8_t, 20>;
}

#endif//charging_h
