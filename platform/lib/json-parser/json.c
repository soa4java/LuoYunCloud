
/* vim: set et ts=3 sw=3 ft=c:
 *
 * Copyright (C) 2012 James McLaughlin et al.  All rights reserved.
 * https://github.com/udp/json-parser
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "json.h"

#ifdef _MSC_VER
   #ifndef _CRT_SECURE_NO_WARNINGS
      #define _CRT_SECURE_NO_WARNINGS
   #endif
#endif

#ifdef __cplusplus
   const struct _json_value json_value_none; /* zero-d by ctor */
#else
   const struct _json_value json_value_none = { 0 };
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

typedef unsigned short json_uchar;

static unsigned char hex_value (json_char c)
{
   if (c >= 'A' && c <= 'F')
      return (c - 'A') + 10;

   if (c >= 'a' && c <= 'f')
      return (c - 'a') + 10;

   if (c >= '0' && c <= '9')
      return c - '0';

   return 0xFF;
}

typedef struct
{
   json_settings settings;
   int first_pass;

   unsigned long used_memory;

   unsigned int uint_max;
   unsigned long ulong_max;

} json_state;

static void * json_alloc (json_state * state, unsigned long size, int zero)
{
   void * mem;

   if ((state->ulong_max - state->used_memory) < size)
      return 0;

   if (state->settings.max_memory
         && (state->used_memory += size) > state->settings.max_memory)
   {
      return 0;
   }

   if (! (mem = zero ? calloc (size, 1) : malloc (size)))
      return 0;

   return mem;
}

static int new_value
   (json_state * state, json_value ** top, json_value ** root, json_value ** alloc, json_type type)
{
   json_value * value;
   int values_size;

   if (!state->first_pass)
   {
      value = *top = *alloc;
      *alloc = (*alloc)->_reserved.next_alloc;

      if (!*root)
         *root = value;

      switch (value->type)
      {
         case json_array:

            if (! (value->u.array.values = (json_value **) json_alloc
               (state, value->u.array.length * sizeof (json_value *), 0)) )
            {
               return 0;
            }

            break;

         case json_object:

            values_size = sizeof (*value->u.object.values) * value->u.object.length;

            if (! ((*(void **) &value->u.object.values) = json_alloc
                  (state, values_size + ((unsigned long) value->u.object.values), 0)) )
            {
               return 0;
            }

            value->_reserved.object_mem = (*(char **) &value->u.object.values) + values_size;

            break;

         case json_string:

            if (! (value->u.string.ptr = (json_char *) json_alloc
               (state, (value->u.string.length + 1) * sizeof (json_char), 0)) )
            {
               return 0;
            }

            break;

         default:
            break;
      };

      value->u.array.length = 0;

      return 1;
   }

   value = (json_value *) json_alloc (state, sizeof (json_value), 1);

   if (!value)
      return 0;

   if (!*root)
      *root = value;

   value->type = type;
   value->parent = *top;

   if (*alloc)
      (*alloc)->_reserved.next_alloc = value;

   *alloc = *top = value;

   return 1;
}

#define e_off \
   ((int) (i - cur_line_begin))

#define whitespace \
   case '\n': ++ cur_line;  cur_line_begin = i; \
   case ' ': case '\t': case '\r'

#define string_add(b)  \
   do { if (!state.first_pass) string [string_length] = b;  ++ string_length; } while (0);

const static int
   flag_next = 1, flag_reproc = 2, flag_need_comma = 4, flag_seek_value = 8, flag_exponent = 16,
   flag_got_exponent_sign = 32, flag_escaped = 64, flag_string = 128, flag_need_colon = 256,
   flag_done = 512;

json_value * json_parse_ex (json_settings * settings, const json_char * json, char * error_buf)
{
   json_char error [128];
   unsigned int cur_line;
   const json_char * cur_line_begin, * i;
   json_value * top, * root, * alloc = 0;
   json_state state;
   int flags;

   error[0] = '\0';

   memset (&state, 0, sizeof (json_state));
   memcpy (&state.settings, settings, sizeof (json_settings));

   memset (&state.uint_max, 0xFF, sizeof (state.uint_max));
   memset (&state.ulong_max, 0xFF, sizeof (state.ulong_max));

   state.uint_max -= 8; /* limit of how much can be added before next check */
   state.ulong_max -= 8;

   for (state.first_pass = 1; state.first_pass >= 0; -- state.first_pass)
   {
      json_uchar uchar;
      unsigned char uc_b1, uc_b2, uc_b3, uc_b4;
      json_char * string = NULL;
      unsigned int string_length = 0;

      top = root = 0;
      flags = flag_seek_value;

      cur_line = 1;
      cur_line_begin = json;

      for (i = json ;; ++ i)
      {
         json_char b = *i;

         if (flags & flag_done)
         {
            if (!b)
               break;

            switch (b)
            {
               whitespace:
                  continue;

               default:
                  sprintf (error, "%d:%d: Trailing garbage: `%c`", cur_line, e_off, b);
                  goto e_failed;
            };
         }

         if (flags & flag_string)
         {
            if (!b)
            {  sprintf (error, "(%s:%d) Unexpected EOF in string (at %d:%d)",  __FILE__, __LINE__, cur_line, e_off);
               goto e_failed;
            }

            if (string_length > state.uint_max)
               goto e_overflow;

            if (flags & flag_escaped)
            {
               flags &= ~ flag_escaped;

               switch (b)
               {
                  case 'b':  string_add ('\b');  break;
                  case 'f':  string_add ('\f');  break;
                  case 'n':  string_add ('\n');  break;
                  case 'r':  string_add ('\r');  break;
                  case 't':  string_add ('\t');  break;
                  case 'u':

                    if ((uc_b1 = hex_value (*++ i)) == 0xFF || (uc_b2 = hex_value (*++ i)) == 0xFF
                          || (uc_b3 = hex_value (*++ i)) == 0xFF || (uc_b4 = hex_value (*++ i)) == 0xFF)
                    {
                        sprintf (error, "Invalid character value `%c` (at %d:%d)", b, cur_line, e_off);
                        goto e_failed;
                    }

                    uc_b1 = uc_b1 * 16 + uc_b2;
                    uc_b2 = uc_b3 * 16 + uc_b4;

                    uchar = ((json_char) uc_b1) * 256 + uc_b2;

                    if (sizeof (json_char) >= sizeof (json_uchar) || (uc_b1 == 0 && uc_b2 <= 0x7F))
                    {
                       string_add ((json_char) uchar);
                       break;
                    }

                    if (uchar <= 0x7FF)
                    {
                        if (state.first_pass)
                           string_length += 2;
                        else
                        {  string [string_length ++] = 0xC0 | ((uc_b2 & 0xC0) >> 6) | ((uc_b1 & 0x3) << 3);
                           string [string_length ++] = 0x80 | (uc_b2 & 0x3F);
                        }

                        break;
                    }

                    if (state.first_pass)
                       string_length += 3;
                    else
                    {  string [string_length ++] = 0xE0 | ((uc_b1 & 0xF0) >> 4);
                       string [string_length ++] = 0x80 | ((uc_b1 & 0xF) << 2) | ((uc_b2 & 0xC0) >> 6);
                       string [string_length ++] = 0x80 | (uc_b2 & 0x3F);
                    }

                    break;

                  default:
                     string_add (b);
               };

               continue;
            }

            if (b == '\\')
            {
               flags |= flag_escaped;
               continue;
            }

            if (b == '"')
            {
               if (!state.first_pass)
                  string [string_length] = 0;

               flags &= ~ flag_string;
               string = 0;

               switch (top->type)
               {
                  case json_string:

                     top->u.string.length = string_length;
                     flags |= flag_next;

                     break;

                  case json_object:

                     if (state.first_pass)
                        (*(json_char **) &top->u.object.values) += string_length + 1;
                     else
                     {  
                        top->u.object.values [top->u.object.length].name
                           = (json_char *) top->_reserved.object_mem;

                        (*(json_char **) &top->_reserved.object_mem) += string_length + 1;
                     }

                     flags |= flag_seek_value | flag_need_colon;
                     continue;

                  default:
                     break;
               };
            }
            else
            {
               string_add (b);
               continue;
            }
         }

         if (flags & flag_seek_value)
         {
            switch (b)
            {
               whitespace:
                  continue;

               case ']':

                  if (top->type == json_array)
                     flags = (flags & ~ (flag_need_comma | flag_seek_value)) | flag_next;
                  else if (!state.settings.settings & json_relaxed_commas)
                  {  sprintf (error, "(%s:%d) %d:%d: Unexpected ]", __FILE__, __LINE__, cur_line, e_off);
                     goto e_failed;
                  }

                  break;

               default:

                  if (flags & flag_need_comma)
                  {
                     if (b == ',')
                     {  flags &= ~ flag_need_comma;
                        continue;
                     }
                     else
                     {  sprintf (error, "%d:%d: Expected , before %c", cur_line, e_off, b);
                        goto e_failed;
                     }
                  }

                  if (flags & flag_need_colon)
                  {
                     if (b == ':')
                     {  flags &= ~ flag_need_colon;
                        continue;
                     }
                     else
                     {  sprintf (error, "%d:%d: Expected : before %c", cur_line, e_off, b);
                        goto e_failed;
                     }
                  }

                  flags &= ~ flag_seek_value;

                  switch (b)
                  {
                     case '{':

                        if (!new_value (&state, &top, &root, &alloc, json_object))
                           goto e_alloc_failure;

                        continue;

                     case '[':

                        if (!new_value (&state, &top, &root, &alloc, json_array))
                           goto e_alloc_failure;

                        flags |= flag_seek_value;
                        continue;

                     case '"':

                        if (!new_value (&state, &top, &root, &alloc, json_string))
                           goto e_alloc_failure;

                        flags |= flag_string;

                        string = top->u.string.ptr;
                        string_length = 0;

                        continue;

                     case 't':

                        if (*(++ i) != 'r' || *(++ i) != 'u' || *(++ i) != 'e')
                           goto e_unknown_value;

                        if (!new_value (&state, &top, &root, &alloc, json_boolean))
                           goto e_alloc_failure;

                        top->u.boolean = 1;

                        flags |= flag_next;
                        break;

                     case 'f':

                        if (*(++ i) != 'a' || *(++ i) != 'l' || *(++ i) != 's' || *(++ i) != 'e')
                           goto e_unknown_value;

                        if (!new_value (&state, &top, &root, &alloc, json_boolean))
                           goto e_alloc_failure;

                        flags |= flag_next;
                        break;

                     case 'n':

                        if (*(++ i) != 'u' || *(++ i) != 'l' || *(++ i) != 'l')
                           goto e_unknown_value;

                        if (!new_value (&state, &top, &root, &alloc, json_null))
                           goto e_alloc_failure;

                        flags |= flag_next;
                        break;

                     default:

                        if (isdigit (b) || b == '-')
                        {
                           if (!new_value (&state, &top, &root, &alloc, json_integer))
                              goto e_alloc_failure;

                           flags &= ~ (flag_exponent | flag_got_exponent_sign);

                           if (state.first_pass)
                              continue;

                           if (top->type == json_double)
                              top->u.dbl = strtod (i, (json_char **) &i);
                           else
                              top->u.integer = strtol (i, (json_char **) &i, 10);

                           flags |= flag_next | flag_reproc;
                        }
                        else
                        {  sprintf (error, "(%s:%d) %d:%d: Unexpected %c when seeking value",  __FILE__, __LINE__, cur_line, e_off, b);
                           goto e_failed;
                        }
                  };
            };
         }
         else
         {
            switch (top->type)
            {
            case json_object:
               
               switch (b)
               {
                  whitespace:
                     continue;

                  case '"':

                     if (flags & flag_need_comma && (!state.settings.settings & json_relaxed_commas))
                     {
                        sprintf (error, "%d:%d: Expected , before \"", cur_line, e_off);
                        goto e_failed;
                     }

                     flags |= flag_string;

                     string = (json_char *) top->_reserved.object_mem;
                     string_length = 0;

                     break;
                  
                  case '}':

                     flags = (flags & ~ flag_need_comma) | flag_next;
                     break;

                  case ',':

                     if (flags & flag_need_comma)
                     {
                        flags &= ~ flag_need_comma;
                        break;
                     }

                  default:

                     sprintf (error, "(%s:%d) %d:%d: Unexpected `%c` in object",  __FILE__, __LINE__, cur_line, e_off, b);
                     goto e_failed;
               };

               break;

            case json_integer:
            case json_double:

               if (isdigit (b))
                  continue;

               if (b == 'e' || b == 'E')
               {
                  if (!(flags & flag_exponent))
                  {
                     flags |= flag_exponent;
                     top->type = json_double;

                     continue;
                  }
               }
               else if (b == '+' || b == '-')
               {
                  if (flags & flag_exponent && !(flags & flag_got_exponent_sign))
                  {
                     flags |= flag_got_exponent_sign;
                     continue;
                  }
               }
               else if (b == '.' && top->type == json_integer)
               {
                  top->type = json_double;
                  continue;
               }

               flags |= flag_next | flag_reproc;
               break;

            default:
               break;
            };
         }

         if (flags & flag_reproc)
         {
            flags &= ~ flag_reproc;
            -- i;
         }

         if (flags & flag_next)
         {
            flags = (flags & ~ flag_next) | flag_need_comma;

            if (!top->parent)
            {
               /* root value done */

               flags |= flag_done;
               continue;
            }

            if (top->parent->type == json_array)
               flags |= flag_seek_value;
               
            if (!state.first_pass)
            {
               json_value * parent = top->parent;

               switch (parent->type)
               {
                  case json_object:

                     parent->u.object.values
                        [parent->u.object.length].value = top;

                     break;

                  case json_array:

                     parent->u.array.values
                           [parent->u.array.length] = top;

                     break;

                  default:
                     break;
               };
            }

            if ( (++ top->parent->u.array.length) > state.uint_max)
               goto e_overflow;

            top = top->parent;

            continue;
         }
      }

      alloc = root;
   }

   return root;

e_unknown_value:

   sprintf (error, "%d:%d: Unknown value", cur_line, e_off);
   goto e_failed;

e_alloc_failure:

   strcpy (error, "Memory allocation failure");
   goto e_failed;

e_overflow:

   sprintf (error, "%d:%d: Too long (caught overflow)", cur_line, e_off);
   goto e_failed;

e_failed:

   if (error_buf)
   {
      if (*error)
         strcpy (error_buf, error);
      else
         strcpy (error_buf, "Unknown error");
   }

   if (state.first_pass)
      alloc = root;

   while (alloc)
   {
      top = alloc->_reserved.next_alloc;
      free (alloc);
      alloc = top;
   }

   if (!state.first_pass)
      json_value_free (root);

   return 0;
}

json_value * json_parse (const json_char * json)
{
   json_settings settings;
   memset (&settings, 0, sizeof (json_settings));

   return json_parse_ex (&settings, json, 0);
}

void json_value_free (json_value * value)
{
   json_value * cur_value;

   if (!value)
      return;

   value->parent = 0;

   while (value)
   {
      switch (value->type)
      {
         case json_array:

            if (!value->u.array.length)
            {
               free (value->u.array.values);
               break;
            }

            value = value->u.array.values [-- value->u.array.length];
            continue;

         case json_object:

            if (!value->u.object.length)
            {
               free (value->u.object.values);
               break;
            }

            value = value->u.object.values [-- value->u.object.length].value;
            continue;

         case json_string:

            free (value->u.string.ptr);
            break;

         default:
            break;
      };

      cur_value = value;
      value = value->parent;
      free (cur_value);
   }
}


