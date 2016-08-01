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

from varsize import variable_size_attributes
from varsize import output_variable_size_attributes

infunc = 0
infuncelementname = 0
interface = ''

outstr = ''
header_structs = ''
header_funcs = ''
header_interface = ''

funcdef = dict()
paramcnt = 0
target = ''
prefix = ''
opcode = '0'

grayley_generated_funcs = dict()

util_funcs = dict()
generated_funcs = dict()
handwritten_funcs = dict()

max_opcode = 0

preamble_files = []
input_files = []
output_file = "-"
typegen = "gael"
mode = "client"

accepted_enums = dict()


ignorelist = dict({
  "gael":
    {
    },
  "grayley":
    {
    },
  "util":
    {
    },
  "header":
    {
    },
})

native_types = {
  "int":      "int32_t",
  "uint":     "uint32_t",
  "fixed":    "wl_fixed_t",
  "string":   "char *",
  "array":    "struct wl_array *",
  "data":     "void *",
}

type_formats = {
  "int32_t":           "%d",
  "uint32_t":          "%u",
  "char *":            "%s",
}


def gael_generator(funcdef, opcode):
   # func return type
   if 'rettype' in funcdef:
      outstr = funcdef.get('rettype') + '\n'
   else:
      outstr = 'void\n'

   funcname = funcdef.get('name')

   # func name
   outstr += prefix + funcname + ' ('

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
         if ('output' not in funcdef.get(searchstr)) and not (funcname + ':' + params.get('val') in variable_size_attributes) and (params.get('type') in type_formats):
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
      outstr += '   ' + funcdef.get('rettype') + ' ret = (' + funcdef.get('rettype') + ') wth_object_new ();\n'
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
         if ('output' not in params) \
           and funcname + ':' + params.get('val') not in variable_size_attributes:
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

   outstr += '   LOCK();\n'

   #serialize message header
   outstr += '   START_MESSAGE( "' + funcname + '", sz, '
   outstr += 'API_WAYLAND'
   outstr += ', ' + str(opcode) + ' );\n'

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
            assert 'output' not in params, \
               "variable-size params not compatible with output params"

            if funcname + ':' + params.get('val') in variable_size_attributes:
               var_attr_size += variable_size_attributes.get(funcname + ':' + params.get('val'))
               var_attr_size += '   sz += sizeof (unsigned int) + PADDED(' + params.get('val') + '_sz);\n'

            # we use SERIALIZE_DATA instead of SERIALIZE_PARAM for variable-size params
            outstr += '   SERIALIZE_DATA( (void *)' + params.get('val') + ', ' + params.get('val') + '_sz);\n'

         elif ('output' not in funcdef.get(searchstr)):
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

   outstr += '   END_MESSAGE();\n'

   if var_attr_size != "":
      outstr = outstr.replace('VAR_ATTR_SIZE', var_attr_size + '\n')
   else:
      outstr = outstr.replace('VAR_ATTR_SIZE', '')

   #wait for outputs, if applicable
   receive_reply = ""
   haveparams = 1
   paramitr = 0
   while haveparams:
      searchstr = ('param' + str(paramitr))
      haveparams = searchstr in funcdef
      if haveparams:
         if ('output' in funcdef.get(searchstr)):
            params = funcdef.get(searchstr)

            assert params['type'][-1:] == '*', \
              "Type '" + params['type'] + "' must end with a '*'"
            dereferenced_type = params['type'][:-1].strip()

            param_size = 'sizeof(' + dereferenced_type + ')'

            if funcname + ':' + params.get('val') in output_variable_size_attributes:
               receive_reply += '   DESERIALIZE_DATA( (void *)' + params['val'] + ');\n'
            else:
               receive_reply += '   DESERIALIZE_OPTIONAL_PARAM( (void *)' + params['val'] + ', ' + param_size + ' );\n'

         paramitr += 1
      else:
         break
   if 'rettype' in funcdef:
      param_size = 'sizeof(' + funcdef.get('rettype') + ')'
      receive_reply = (receive_reply + '   DESERIALIZE_PARAM( (void *)&ret, ' + param_size + ' );\n')

   outstr += '   UNLOCK();\n'

   outstr += '   END_TIMING(' + fmt_ret + ');\n'

   #return val, if applicable
   if 'rettype' in funcdef:
      outstr += '   return ret;\n'

   #end of function
   outstr += '}\n\n'

   return outstr


def grayley_generator(funcdef, opcode):
   global grayley_generated_funcs

   apifuncname = funcdef.get('name')
   funcname = 'function_' + opcode
   grayley_generated_funcs[int(opcode)] = funcdef.get('name')

   code = "/* Function: " + apifuncname + "\n"
   code += " * Opcode " + str(opcode) + \
           " (hexadecimal: " + hex(int(opcode)) + ")\n"
   code += " */\n"
   code += "static gboolean " + funcname + "(const hdr_t *header, \n" + \
           "        const char *body, hdr_t *header_reply, char *body_reply)\n"
   code += "{\n"

   haveparams = 1
   paramitr = 0
   offset_string = ''
   size_output = ''
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
              code += '  struct wl_array ' + params.get('val') + \
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

           code += '  ' + objtype + params.get('val') + ' = (' + objtype + ') wth_object_new_with_id (*(uint32_t *)(body' + offset_string + '));\n'
           offset_string += ' + PADDED (sizeof (' + type_ + '))'
           params_call += params.get('val')

           fmt_string += '%u'
           fmt_params += ', ((wth_object *)' + params.get('val') + ')->id'

         elif params.get('object'):
           type_ = 'uint32_t'
           objtype = params.get('type')

           code += '  ' + objtype + params.get('val') + ' = g_hash_table_lookup (hash, GUINT_TO_POINTER (*(uint32_t *)(body' + offset_string + ')));\n'
           offset_string += ' + PADDED (sizeof (' + type_ + '))'
           params_call += params.get('val')

           fmt_string += '%u'
           fmt_params += ', ((wth_object *)' + params.get('val') + ')->id'

         elif ('output' not in funcdef.get(searchstr)) and \
           var_id not in output_variable_size_attributes:
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
         else:
           # output parameters: local variable initialized to point to the
           # right offset in the reply message
           assert params.get('type')[-1:] == '*', \
             "Type '" + params.get('type') + "' must end with a '*'"

           fmt_string += '[unprintable output type ' + params.get('type') + ']'
           if var_id in output_variable_size_attributes:
             # variable-size out parameter
             # define a param_sz variable in the reply output...
             code += '  int *' + params.get('val') + '_sz = (void*)(body_reply' + size_output + ');\n'
             # ...and initialize it to the actual size of the out param
             code += output_variable_size_attributes.get(var_id)
             size_output += ' + sizeof (int)'
             # now define the output buffer
             code += '  ' + params.get('type') + ' ' + params.get('val') + \
                     ' = (void*)(body_reply' + size_output + ');\n'
             size_output += ' + PADDED (*' + params.get('val') + '_sz)'
             params_call += params.get('val')
           else:
             # fixed-size out parameter
             dereferenced_type = params.get('type')[:-1].strip()
             code += '  ' + dereferenced_type + ' *' + params.get('val') + \
                     ' = (void*)(body_reply' + size_output + ');\n'
             size_output += ' + PADDED (sizeof (' + dereferenced_type + '))'
             params_call += params.get('val')

         paramitr += 1
      else:
         break

   if paramitr != 0:
      code += '\n'

   code += '  log_debug ("' + apifuncname + '(' + fmt_string + ') (opcode ' \
           + str(opcode) + ') called."' + fmt_params + ');\n'

   objname = funcdef['param0']['val']
   listener = 'struct {}_{} *'.format(objname, "listener" if mode == "client" else "interface")
   vfunc = funcdef['origname']

   code += '  START_TIMING();\n'
   code += '  ((' + listener + ')(((struct wth_object *)' + objname + ')->vfunc))->' + vfunc + '\n'
   code += '    (' + params_call + ');\n'
   code += '  END_TIMING("' + apifuncname +'");\n'

   if size_output != '':
      code += "\n"
      code += "  header_reply->sz = sizeof (*header)" + size_output + ";\n"
      code += "\n"
      code += "  return TRUE;\n"
   else:
      code += "\n"
      code += "  return FALSE;\n"

   code += "}\n"
   code += "\n"

   return code

def util_generator(funcdef, opcode):
   global generated_funcs
   global util_funcs

   expect_reply = 0

   if 'rettype' in funcdef:
      expect_reply = 1

   haveparams = 1
   paramitr = 0
   while haveparams:
      searchstr = ('param' + str(paramitr))
      haveparams = searchstr in funcdef
      if haveparams:
         if ('output' in funcdef.get(searchstr)):
            expect_reply = 1
         paramitr += 1

   generated_funcs[int(opcode)] = funcdef.get('name')
   util_funcs[int(opcode)] = expect_reply

   return ""

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
   global header_interface
   global interface

   if (mode == "client" and type_ == "event") \
      or (mode == "server" and type_ == "request"):
      if interface != header_interface:
         if header_interface != "":
            header_structs += "};\n\n"
            # wthp_foo_set_listener func
            header_structs += "static inline void\n"
            header_structs += "{0}_set_{1}(struct {0} *self, const struct {0}_{1} *funcs, void *user_data)\n".format(header_interface, "listener" if mode == "client" else "interface")
            header_structs += "{\n"
            header_structs += "  wth_object_set_listener((struct wth_object *)self, (void (**)(void)) funcs, user_data);\n"
            header_structs += "}\n\n"

         # new interface. declare the struct
         header_interface = interface
         header_structs += "struct {}_{} {{\n".format(interface, "listener" if mode == "client" else "interface")

      header_structs += "  void (*" + funcdef.get('origname') + ") " + get_func_params(funcdef) + ";\n"
      return ""
   elif mode == "server" and type_ == "request":
      return ""

   header_funcs += get_func_prototype (funcdef) + ' APICALL;\n\n'
   return ""

def generate_func(funcdef, elemtype):
   global typegen

   if mode == "client":
      if typegen == "gael" and elemtype == "event":
         return False
      elif typegen == "grayley" and elemtype == "request":
         return False
   elif mode == "server":
      if typegen == "gael" and elemtype == "request":
         return False
      elif typegen == "grayley" and elemtype == "event":
         return False

   return True

def sanitize_params(funcdef):
   global accepted_enums
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
      param['clean_type'] = param['type'].replace("*", "").replace("const", "").replace(" ", "")
      if param.get('count') == "1":
         param['is_out_var'] = True
      elif param.get('type') == "struct wl_array *":
         param['is_array'] = True
      if "void *" in param.get('type'):
         param['is_data'] = True
      if param.get('is_data') and param.get('type').count('*') == 2:
         param['is_2d_data'] = True
      if param.get('type') == "char *":
         param['is_string'] = True
      if param.get('is_string') and param.get('output'):
         param['is_out_string'] = True

   funcdef["params"] = params


def start_element(elementname, attrs):
   global infunc
   global infuncelementname
   global interface
   global funcdef
   global paramcnt
   global api
   global opcode
   global ignorelist
   global typegen
   global max_opcode
   global parameter_size
   global accepted_enums

   if (elementname == "request" or elementname == "event") and attrs.get('name') not in ignorelist[typegen]:
      #infunc indicates we are inside an xml block for a request/event
      infunc = 1
      infuncelementname = elementname
      if elementname == "event":
          funcdef['name'] = interface + '_send_' + attrs.get('name')
      else:
          funcdef['name'] = interface + '_' + attrs.get('name')
      funcdef['origname'] = attrs.get('name')
      opcode = str(int(opcode) + 1)
      if typegen == 'grayley' or typegen == 'gael':
         while (int(opcode) in handwritten_funcs) and funcdef['name'] != handwritten_funcs[int(opcode)][0]:
            opcode = str(int(opcode) + 1)
      if (int(opcode) > max_opcode):
         max_opcode = int(opcode)

   # arguments
   if infunc == 1 and (elementname == "arg" or elementname == "request" or elementname == "event"):
      paramentry = 'param' + str(paramcnt)
      if elementname == "request" or elementname == "event":
         funcdef[paramentry] = dict([('type', 'struct ' + interface + ' *'), ('val', interface), ('object', True)])
      else:
         funcdef[paramentry] = dict([('type', attrs.get('type')), ('val', attrs.get('name'))])
      # FIXME we can probably get rid of some of these options
      if attrs.get('output') == 'true':
         funcdef[paramentry]['output'] = True
      if attrs.get('count') is not None:
         funcdef[paramentry]['count'] = attrs.get('count')
      if attrs.get('counter') == 'true':
         funcdef[paramentry]['is_counter'] = True
      if attrs.get('pairs') == 'true':
         funcdef[paramentry]['pairs'] = True
      if attrs.get('type') in native_types:
         funcdef[paramentry]['type'] = native_types[attrs.get('type')]
      if attrs.get('type') == "object":
         funcdef[paramentry]['object'] = True
         if attrs.get('interface'):
            funcdef[paramentry]['type'] = 'struct ' + attrs.get('interface') + ' *'
         else:
            funcdef[paramentry]['type'] = 'struct wth_object *'
      if attrs.get('type') == "new_id":
         funcdef[paramentry]['type'] = 'uint32_t'
         funcdef[paramentry]['new_id'] = True
         if attrs.get('interface'):
            funcdef['rettype'] = 'struct ' + attrs.get('interface') + ' *'
         else:
            funcdef['rettype'] = 'struct wth_object *'
         funcdef[paramentry]['objtype'] = funcdef['rettype']

      paramcnt += 1

   if elementname == "interface":
      interface = attrs.get('name')

def end_element(elementname):
   global infunc
   global infuncelementname
   global outstr
   global funcdef
   global paramcnt
   global api
   global opcode
   global typegen

   if infunc == 1 and elementname == infuncelementname:
      #print(funcdef)
      infunc = 0
      paramcnt = 0

      sanitize_params(funcdef)

      outstr = ''
      if generate_func(funcdef, elementname):
         if typegen == "gael":
            outstr = gael_generator(funcdef, opcode)
         elif typegen == "grayley":
            outstr = grayley_generator(funcdef, opcode)
         elif typegen == "util":
            outstr = util_generator(funcdef, opcode)
         elif typegen == "header":
            outstr = header_generator(funcdef, elementname)
         else:
            outstr = ''

      out.write(str(outstr))
      funcdef.clear()


try:
   opts, args = getopt.getopt(sys.argv[1:],"hp:i:o:t:m:x:",
                              ["preamble=","input=","output=","type=","mode=","prefix="])
except getopt.GetoptError:
   print 'gen.py -p <preamblefile> -i <inputfile> -o <outputfile> -t <type> -m <mode> [-x <prefix>]'
   sys.exit(2)
for opt, arg in opts:
   if opt == '-h':
      print 'gen.py -p <preamblefile> -i <inputfile> -o <outputfile> -t <type> -m <mode> [-x <prefix>]'
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
   elif opt in ("-x", "--prefix") and arg:
      prefix = arg + '_'

out = open( output_file, 'w' )

for x in preamble_files:
   preamblefile = open( x, 'r' )
   out.write(preamblefile.read())
   preamblefile.close()

for x in input_files:
   inputfile = open( x, 'rb' )

   p1 = xml.parsers.expat.ParserCreate()
   p1.StartElementHandler = start_element
   p1.EndElementHandler = end_element

   p1.ParseFile(inputfile)
   inputfile.close()

if typegen == 'grayley':
   # check the max opcode of handwritten functions
   if handwritten_funcs:
      max_handwritten_opcode = max(handwritten_funcs.keys())
      if max_handwritten_opcode > max_opcode:
         max_opcode = max_handwritten_opcode

   # add constants
   out.write("const int grayley_max_opcode __attribute__ ((visibility (\"default\"))) = " \
             + str(max_opcode) + ";\n")

   # add array of function pointers
   out.write("const grayley_helper_function_t grayley_functions" + \
             "[" + str(max_opcode + 1) + "] \n")
   out.write("__attribute__ ((visibility (\"default\"))) = {\n")
   for x in range(0, max_opcode + 1):
      funcname = 'function_' + str(x)
      if x in grayley_generated_funcs or x in handwritten_funcs:
         out.write("  " + funcname + ",\n")
      else:
         out.write("  NULL,\n")
   out.write("};\n")

if typegen == 'header':
   if header_structs != "":
      # close the last struct
      header_structs += "};\n\n"

   # header guard
   out.write('#ifndef __WALTHAM_{0}__\n#define __WALTHAM_{0}__\n\n'.format(mode.upper()))

   out.write(header_structs)
   out.write(header_funcs)

   # header guard
   out.write('#endif')

if typegen == 'util':
   out.write("#include <string.h>\n\n")

   # add constants
   out.write("const int util_max_opcode __attribute__ ((visibility (\"default\"))) = " + str(max_opcode) + ";\n")

   # add array of function names
   out.write("const char *util_function_names" + \
             "[" + str(max_opcode + 1) + \
             "] __attribute__ ((visibility (\"default\"))) =\n")
   out.write("{\n")
   for x in range(0, max_opcode + 1):
      funcname = 'function_' + str(x)
      if x in generated_funcs:
         out.write('  "' + generated_funcs.get(x) + '",\n')
      else:
         out.write("  NULL,\n")
   out.write("};\n")

   # add array of functions expecting a reply
   out.write("const char function_expects_reply" + \
             "[" + str(max_opcode + 1) + "] =\n")
   out.write("{\n")
   for x in range(0, max_opcode + 1):
      funcname = 'function_' + str(x)
      if x in generated_funcs:
         out.write("  " + str(util_funcs.get(x)) + ",\n")
      elif x in handwritten_funcs:
         out.write("  " + str(handwritten_funcs[x][1]) + ",\n")
      else:
         out.write("  0,\n")
   out.write("};\n")

out.close()
