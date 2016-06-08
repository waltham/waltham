# !/usr/bin/env python
# -*- coding: utf-8 -*-

# Copyright © 2013-2014 Collabora, Ltd.
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

# dictionary for variable-size params, e.g. strings
# the key format is 'funcname:paramname'
# the value is the code to determine the size of the data to send, and
# can reference other params. It must declare 'int $(param)_sz;',
# e.g. "int pixels_sz = width * height * 4;", for a `pixels' parameter
# where width and height are other params in the function.
variable_size_attributes = dict()

# dictionary for variable-size output params
# the key format is 'funcname:paramname'
# the value is the code to determine the size of the required buffer.
# It must set the integer pointed by $(param)_sz,
# e.g. "*buffer_sz = *n * sizeof (uint32_t);", for a `buffer' parameter
# where n is another param in the function.
output_variable_size_attributes = dict()
