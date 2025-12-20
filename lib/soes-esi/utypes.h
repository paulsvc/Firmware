#ifndef __UTYPES_H__
#define __UTYPES_H__

#include "cc.h"

/* Object dictionary storage */

typedef struct
{
   /* Identity */

   uint32_t serial;

   /* Inputs */

   int32_t Counter;

   /* Outputs */

   struct
   {
      uint8_t New_record_subitem;
   } New_Record;

} _Objects;

extern _Objects Obj;

#endif /* __UTYPES_H__ */
