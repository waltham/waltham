#!/usr/bin/env python2
# -*- coding: utf-8 -*-

# Copyright © 2013-2014 Collabora, Ltd.
# Copyright © 2016 DENSO CORPORATION
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice (including the next
# paragraph) shall be included in all copies or substantial portions of the
# Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.

import sys, getopt
import xml.parsers.expat

infunc = 0
infuncelementname = 0
interface = ''

outstr = ''
header_structs = ''
header_funcs = ''
listener_interface = ''
freefunc_interface = ''

funcdef = dict()
opcode = '0'

demarshaller_generated_funcs = dict()

max_opcode = 0

preamble_files = []
input_files = []
output_file = "-"
typegen = "marshaller"
mode = "client"

native_types = {
  "int":      "int32_t",
  "uint":     "uint32_t",
  "fixed":    "wth_fixed_t",
  "string":   "const char *",
  "array":    "struct wth_array *",
  "data":     "void *",
}

type_formats = {
  "int32_t":           "%d",
  "uint32_t":          "%u",
  "const char *":      "%s",
}

# dictionary for variable-size params, e.g. strings
# the key format is 'funcname:paramname'
# the value is the code to determine the size of the data to send, and
# can reference other params. It must declare 'int $(param)_sz;',
# e.g. "int pixels_sz = width * height * 4;", for a `pixels' parameter
# where width and height are other params in the function.
variable_size_attributes = dict()


def marshaller_generator(funcdef, opcode):
   # func return type
   if 'rettype' in funcdef:
      outstr = funcdef.get('rettype') + '\n'
   else:
      outstr = 'void\n'

   funcname = funcdef.get('name')

   # func name
   outstr += funcname + ' ('

   # func params, retains param order from parsing
   haveparams = 1
   paramitr = 0
   fmt_string = ''
   fmt_params = ''
   comma = ''
   while haveparams:
      searchstr = ('param' + str(paramitr))
      paramitr += 1
      haveparams = searchstr in funcdef
      if haveparams:
         params = funcdef.get(searchstr)
         if params.get('new_id'):
            continue
         outstr += comma + params.get('type') + ' ' + params.get('val')
         comma = ', '
         if not fmt_string == '':
            fmt_string += ', '
         if not funcname + ':' + params.get('val') in variable_size_attributes and params.get('type') in type_formats:
            fmt = type_formats.get(params.get('type'))
            if fmt == '%b':
               fmt = '0x%x'
            fmt_string += fmt
            fmt_params += ', ' + params.get('val')
         else:
            fmt_string += '[return/variable/unprintable type ' + params.get('type') + ']'
      else:
         break
   outstr += ')\n{\n'

   #declare ret var, if we have one
   fmt_ret = '""'
   if 'rettype' in funcdef:
      outstr += '   ' + funcdef.get('rettype') + ' ret = (' + funcdef.get('rettype') + ') wth_object_new (((struct wth_object *)' + funcdef.get('param0').get('val') + ')->connection);\n'
      if funcdef.get('rettype') in type_formats:
         fmt_ret = '"' + type_formats.get(funcdef.get('rettype')) + ' ", ret'

   #sz local variable
   outstr += '   int sz = sizeof(hdr_t)'
   haveparams = 1
   paramitr = 0
   while haveparams:
      searchstr = ('param' + str(paramitr))
      haveparams = searchstr in funcdef
      if haveparams:
         params = funcdef.get(searchstr)
         if funcname + ':' + params.get('val') not in variable_size_attributes:
            if params.get('object') or params.get('new_id'):
               outstr += ' + PADDED(sizeof(uint32_t))'
            elif params.get('is_string'):
               # Don't add anything here. It gets added later through variable_size_attributes
               pass
            elif params.get('is_data'):
               outstr += ' + PADDED(' + params.get('val') + '_sz)'
            else:
               outstr += ' + PADDED(sizeof(' + params.get('type') + '))'
         paramitr += 1
      else:
         break
   outstr += ';\n\n'
   outstr += 'VAR_ATTR_SIZE'

   outstr += '   START_TIMING("' + funcname + '", "' + fmt_string + '"' + fmt_params + ');\n'

   #serialize message header
   outstr += '   START_MESSAGE( "' + funcname
   outstr += '", wth_connection_get_next_message_id(((struct wth_object *)' + funcdef.get('param0').get('val') + ')->connection), sz, '
   outstr += str(opcode) + ' );\n'

   #serialize params
   haveparams = 1
   paramitr = 0
   var_attr_size = ""
   while haveparams:
      searchstr = ('param' + str(paramitr))
      haveparams = searchstr in funcdef
      if haveparams:
         params = funcdef.get(searchstr)

         if params.get('is_string'):
            code = "   int " + params.get('val') + "_sz = strlen (" + params.get('val') +  ") + 1;\n"
            variable_size_attributes[funcname + ':' + params.get('val')] = code

         if funcname + ':' + params.get('val') in variable_size_attributes or params.get('is_data'):
            # param with a variable size, we have custom code to determine it
            if funcname + ':' + params.get('val') in variable_size_attributes:
               var_attr_size += variable_size_attributes.get(funcname + ':' + params.get('val'))
               var_attr_size += '   sz += sizeof (unsigned int) + PADDED(' + params.get('val') + '_sz);\n'

            # we use SERIALIZE_DATA instead of SERIALIZE_PARAM for variable-size params
            outstr += '   SERIALIZE_DATA( (void *)' + params.get('val') + ', ' + params.get('val') + '_sz);\n'

         else:
            if params.get('is_array'):
               outstr += '   SERIALIZE_ARRAY( ' + params.get('val') + ' );\n'
            else:
               if params.get('new_id'):
                  outstr += '   SERIALIZE_PARAM( (void *)&((struct wth_object *)ret)->id, sizeof(uint32_t) );\n'
               elif params.get('object'):
                  outstr += '   SERIALIZE_PARAM( (void *)&((struct wth_object *)' + params.get('val') + ')->id, sizeof(uint32_t) );\n'
               elif params.get('is_counter'):
                  # Don't serialize the size here, it gets sent through SERIALIZE_DATA
                  pass
               else:
                  outstr += '   SERIALIZE_PARAM( (void *)&' + params.get('val') + ', sizeof(' + params.get('val') + ') );\n'
         paramitr += 1
      else:
         break

   outstr += '   END_MESSAGE(((struct wth_object *){})->connection);\n'.format(funcdef.get('param0').get('val'))

   if var_attr_size != "":
      outstr = outstr.replace('VAR_ATTR_SIZE', var_attr_size + '\n')
   else:
      outstr = outstr.replace('VAR_ATTR_SIZE', '')

   if 'destructor' in funcdef:
      outstr += '   {0}_free({0});\n'.format(interface)

   outstr += '   END_TIMING(' + fmt_ret + ');\n'

   #return val, if applicable
   if 'rettype' in funcdef:
      outstr += '   return ret;\n'

   #end of function
   outstr += '}\n\n'

   return outstr


def demarshaller_generator(funcdef, opcode):
   global demarshaller_generated_funcs

   apifuncname = funcdef.get('name')
   funcname = 'function_' + opcode
   demarshaller_generated_funcs[int(opcode)] = funcdef.get('name')

   code = "/* Function: " + apifuncname + "\n"
   code += " * Opcode " + str(opcode) + \
           " (hexadecimal: " + hex(int(opcode)) + ")\n"
   code += " */\n"
   code += "static void " + funcname + "(struct wth_connection *conn,\n"
   code += "    const hdr_t *header, const char *body)\n"
   code += "{\n"

   haveparams = 1
   paramitr = 0
   offset_string = ''
   params_call = ''
   fmt_string = ''
   fmt_params = ''
   while haveparams:
      searchstr = ('param' + str(paramitr))
      haveparams = searchstr in funcdef
      if haveparams:
         if (params_call != ''):
           params_call += ', '
         if (fmt_string != ''):
           fmt_string += ', '

         params = funcdef.get(searchstr)
         var_id = apifuncname + ':' + params.get('val')

         if params.get('is_counter'):
           # don't deserialise anything, the value is embedded with the 'data' arg
           params_call += params.get('val')
           fmt_string += '[variable type ' + params.get('type') + ']'

         elif var_id in variable_size_attributes or params.get('is_string') or params.get('is_array') or params.get('is_data'):
           # variable size param, first comes the number of bytes and then the data
           # size of the input parameter, needed to determine where the next parameter
           # starts
           code += '  unsigned int ' + params.get('val') + '_sz __attribute__((unused))' \
                   ' = *(unsigned int *)(body' + offset_string + ');\n'
           # input parameter: local variable initialized to point to the
           # right offset in the received message
           if params.get('is_array'):
              code += '  struct wth_array ' + params.get('val') + \
                      ' = { ' + params.get('val') + '_sz, ' + params.get('val') +  '_sz, (void*)(body' + offset_string + ' + sizeof (unsigned int)) };\n'
              params_call += '&'
           else:
              code += '  ' + params.get('type') + params.get('val') + \
                      ' = (void*)(body' + offset_string + ' + sizeof (unsigned int));\n'
           offset_string += ' + sizeof (unsigned int) + PADDED (' + params.get('val') + '_sz) '
           params_call += params.get('val')
           fmt_string += '[variable type ' + params.get('type') + ']'

         elif params.get('new_id'):
           type_ = params.get('type')
           objtype = params.get('objtype')

           code += '  ' + objtype + params.get('val') + ' = (' + objtype + ') wth_object_new_with_id (((struct wth_object *)' + funcdef.get('param0').get('val') + ')->connection, *(uint32_t *)(body' + offset_string + '));\n'
           offset_string += ' + PADDED (sizeof (' + type_ + '))'
           params_call += params.get('val')

           fmt_string += '%u'
           fmt_params += ', ((struct wth_object *)' + params.get('val') + ')->id'

         elif params.get('object'):
           type_ = 'uint32_t'
           objtype = params.get('type')

           code += '  ' + objtype + params.get('val') + ' = (void *) wth_connection_get_object (conn, *(uint32_t *)(body' + offset_string + '));\n'
           offset_string += ' + PADDED (sizeof (' + type_ + '))'
           params_call += params.get('val')

           fmt_string += '%u'
           fmt_params += ', ((struct wth_object *)' + params.get('val') + ')->id'

         else:
           # input parameters: local variable initialized to point to the
           # right offset in the received message
           type_ = params.get('type')
           code += '  ' + type_ + ' *'
           code += params.get('val') + ' = (void*)(body' + offset_string + ');\n'
           offset_string += ' + PADDED (sizeof (' + type_ + '))'
           params_call += '*' + params.get('val')
           if type_ in type_formats:
              fmt = type_formats.get(type_)
              if fmt == '%b':
                 fmt = '0x%x'
              fmt_string += fmt
              fmt_params += ', *' + params.get('val')
           else:
              fmt_string += '[unprintable type ' + type_ + ']'

         paramitr += 1
      else:
         break

   if paramitr != 0:
      code += '\n'

   code += '  g_debug ("' + apifuncname + '(' + fmt_string + ') (opcode ' \
           + str(opcode) + ') called."' + fmt_params + ');\n'

   objname = funcdef['param0']['val']
   listener = 'struct {}_{} *'.format(objname, "listener" if mode == "client" else "interface")
   vfunc = funcdef['origname']

   code += '  START_TIMING();\n'
   code += '  ((' + listener + ')(((struct wth_object *)' + objname + ')->vfunc))->' + vfunc + '\n'
   code += '    (' + params_call + ');\n'
   code += '  END_TIMING("' + apifuncname +'");\n'
   code += "}\n"
   code += "\n"

   return code

def get_func_params(funcdef):
   outstr = ''

   outstr += '('

   # func params, retains param order from parsing
   comma = ''
   paramitr = 0
   while True:
      searchstr = ('param' + str(paramitr))
      paramitr += 1
      if not searchstr in funcdef:
         break

      param = funcdef.get(searchstr)
      type_ = param.get('type')
      if param.get('new_id'):
         if mode == "client":
            continue
         else:
            type_ = param.get('objtype')

      outstr += comma + type_ + ' ' + param.get('val')
      comma = ', '

   outstr += ')'

   return outstr

def get_func_prototype(funcdef):
   outstr = ""

   if 'rettype' in funcdef:
      outstr += funcdef.get('rettype') + '\n'
   else:
      outstr += 'void\n'

   # func name
   outstr += funcdef.get('name') + ' '
   outstr += get_func_params(funcdef)

   return outstr

def header_generator(funcdef, type_):
   global header_structs
   global header_funcs
   global listener_interface
   global freefunc_interface
   global interface

   if interface != freefunc_interface:
      # wthp_foo_free func
      header_funcs += "static inline void\n"
      header_funcs += "{0}_free(struct {0} *self)\n".format(interface)
      header_funcs += "{\n"
      header_funcs += "  wth_object_delete((struct wth_object *)self);\n"
      header_funcs += "}\n\n"

      freefunc_interface = interface

   if (mode == "client" and type_ == "event") \
      or (mode == "server" and type_ == "request"):
      if interface != listener_interface:
         if listener_interface != "":
            header_structs += "};\n\n"
            # wthp_foo_set_listener func
            header_structs += "static inline void\n"
            header_structs += "{0}_set_{1}(struct {0} *self, const struct {0}_{1} *funcs, void *user_data)\n".format(listener_interface, "listener" if mode == "client" else "interface")
            header_structs += "{\n"
            header_structs += "  wth_object_set_listener((struct wth_object *)self, (void (**)(void)) funcs, user_data);\n"
            header_structs += "}\n\n"

         # new interface. declare the struct
         listener_interface = interface
         header_structs += "struct {}_{} {{\n".format(interface, "listener" if mode == "client" else "interface")

      header_structs += "  void (*" + funcdef.get('origname') + ") " + get_func_params(funcdef) + ";\n"
      return ""

   header_funcs += get_func_prototype (funcdef) + ' APICALL;\n\n'
   return ""

def generate_func(funcdef, elemtype):
   global typegen

   if mode == "client":
      if typegen == "marshaller" and elemtype == "event":
         return False
      elif typegen == "demarshaller" and elemtype == "request":
         return False
   elif mode == "server":
      if typegen == "marshaller" and elemtype == "request":
         return False
      elif typegen == "demarshaller" and elemtype == "event":
         return False

   return True

def sanitize_params(funcdef):
   global function_definitions

   func_name = funcdef.get("name")
   funcdef["func_name"] = func_name

   params = list()
   paramitr = 0

   while 1:
      searchstr = ('param' + str(paramitr))
      if searchstr not in funcdef:
         break
      paramitr += 1
      param = funcdef.get(searchstr)
      params.append(param)
      if param.get('type') == "struct wth_array *":
         param['is_array'] = True
      if param.get('type') == "void *":
         param['is_data'] = True
      if param.get('type') == "const char *":
         param['is_string'] = True

   funcdef["params"] = params

def add_new_argument(funcdef, attrs, new_param):
   paramcnt = funcdef['paramcnt']
   entry = "param{}".format(paramcnt)
   funcdef['paramcnt'] = paramcnt + 1

   funcdef[entry] = new_param

   if funcdef[entry]['type'] in native_types:
      funcdef[entry]['type'] = native_types[funcdef[entry]['type']]
   if attrs.get('type') == "object":
      funcdef[entry]['object'] = True
      if attrs.get('interface'):
         funcdef[entry]['type'] = 'struct ' + attrs.get('interface') + ' *'
      else:
         funcdef[entry]['type'] = 'struct wth_object *'
   if attrs.get('type') == "new_id":
      funcdef[entry]['type'] = 'uint32_t'
      funcdef[entry]['new_id'] = True
      if attrs.get('interface'):
         funcdef['rettype'] = 'struct ' + attrs.get('interface') + ' *'
      else:
         funcdef['rettype'] = 'struct wth_object *'
         add_new_argument(funcdef, {}, dict([('type', 'string'), ('val', 'interface')]))
         add_new_argument(funcdef, {}, dict([('type', 'uint'), ('val', 'version')]))
      funcdef[entry]['objtype'] = funcdef['rettype']

def start_element(elementname, attrs):
   global infunc
   global infuncelementname
   global interface
   global funcdef
   global opcode
   global typegen
   global max_opcode
   global parameter_size

   if elementname == "request" or elementname == "event":
      #infunc indicates we are inside an xml block for a request/event
      infunc = 1
      infuncelementname = elementname
      if elementname == "event":
          funcdef['name'] = interface + '_send_' + attrs.get('name')
      else:
          funcdef['name'] = interface + '_' + attrs.get('name')
      funcdef['origname'] = attrs.get('name')
      funcdef['paramcnt'] = 0
      if attrs.get('type') == 'destructor':
         funcdef['destructor'] = True
      opcode = str(int(opcode) + 1)
      if (int(opcode) > max_opcode):
         max_opcode = int(opcode)

   # arguments
   if infunc == 1 and (elementname == "arg" or elementname == "request" or elementname == "event"):
      if elementname == "request" or elementname == "event":
         add_new_argument(funcdef, attrs, dict([('type', 'struct ' + interface + ' *'), ('val', interface), ('object', True)]))
      else:
         if attrs.get('type') == "data":
            # data type argument. autogenerate a _sz argument
            add_new_argument(funcdef, {}, dict([('type', 'uint'), ('val', '{}_sz'.format(attrs.get('name'))), ('is_counter', True)]))

         add_new_argument(funcdef, attrs, dict([('type', attrs.get('type')), ('val', attrs.get('name'))]))

   if elementname == "interface":
      interface = attrs.get('name')

def end_element(elementname):
   global infunc
   global infuncelementname
   global outstr
   global funcdef
   global opcode
   global typegen

   if infunc == 1 and elementname == infuncelementname:
      #print(funcdef)
      infunc = 0

      sanitize_params(funcdef)

      outstr = ''
      if generate_func(funcdef, elementname):
         if typegen == "marshaller":
            outstr = marshaller_generator(funcdef, opcode)
         elif typegen == "demarshaller":
            outstr = demarshaller_generator(funcdef, opcode)
         elif typegen == "header":
            outstr = header_generator(funcdef, elementname)
         else:
            outstr = ''

      out.write(str(outstr))
      funcdef.clear()


try:
   opts, args = getopt.getopt(sys.argv[1:],"hp:i:o:t:m:",
                              ["preamble=","input=","output=","type=","mode="])
except getopt.GetoptError:
   print 'gen.py -p <preamblefile> -i <inputfile> -o <outputfile> -t <type> -m <mode>'
   sys.exit(2)
for opt, arg in opts:
   if opt == '-h':
      print 'gen.py -p <preamblefile> -i <inputfile> -o <outputfile> -t <type> -m <mode>'
      sys.exit()
   elif opt in ("-p", "--preamble"):
      preamble_files.append(arg)
   elif opt in ("-i", "--input"):
      input_files.append(arg)
   elif opt in ("-o", "--output"):
      output_file = arg
   elif opt in ("-t", "--type"):
      typegen = arg
   elif opt in ("-m", "--mode"):
      mode = arg

out = open( output_file, 'w' )

for x in preamble_files:
   preamblefile = open( x, 'r' )
   out.write(preamblefile.read())
   preamblefile.close()

if typegen == 'marshaller' or typegen == 'demarshaller':
   if mode == "client":
      out.write('#include "waltham-client.h"\n\n')
   else:
      out.write('#include "waltham-server.h"\n\n')

for x in input_files:
   inputfile = open( x, 'rb' )

   p1 = xml.parsers.expat.ParserCreate()
   p1.StartElementHandler = start_element
   p1.EndElementHandler = end_element

   p1.ParseFile(inputfile)
   inputfile.close()

if typegen == 'demarshaller':
   # add array of function pointers
   out.write("const int {0}_max_opcode = {1};\n"
             "const demarshaller_helper_function_t {2}_demarshaller_functions[] = {{"
             "\n".format("demarshaller" if mode == "client" else "demarshaller_", max_opcode,
                         "event" if mode == "client" else "request"))
   for x in range(0, max_opcode + 1):
      funcname = 'function_' + str(x)
      if x in demarshaller_generated_funcs:
         out.write("  " + funcname + ",\n")
      else:
         out.write("  NULL,\n")
   out.write("};\n")

if typegen == 'header':
   if header_structs != "":
      # close the last struct
      header_structs += "};\n\n"

   # header guard
   # FIXME this should be before the includes
   out.write('#ifndef WALTHAM_{0}_H\n#define WALTHAM_{0}_H\n\n'.format(mode.upper()))

   out.write(header_structs)
   out.write(header_funcs)

   # C++ guard
   out.write('#ifdef  __cplusplus\n'
             '}\n'
             '#endif\n\n')

   # header guard
   out.write('#endif')

out.close()
