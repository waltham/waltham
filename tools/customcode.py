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

# dictionaries for custom extra code needed in gael/grayley functions.
# This avoids hand-writing whole functions just for a few lines of extra
# code that don't touch serialization.
# the key format is 'funcname'
# the value is the extra code to add to the function.

# For gael_custom_code, the code is added right before the LOCK() call.
gael_custom_code = dict()

# For gael_post_call_custom_code, the code is added right after the
# UNLOCK() call.
gael_post_call_custom_code = dict()

# For grayley_pre_call_custom_code, the code is added right before the
# respective handler call.
grayley_pre_call_custom_code = dict()

# For grayley_post_call_custom_code, the code is added right after the
# respective handler call.
grayley_post_call_custom_code = dict()
