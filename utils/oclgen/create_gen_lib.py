
# Requires python 2.7 or later.

# Disable generation of byte-code files.
import sys
sys.dont_write_bytecode = True

import re

from argparse import ArgumentParser, FileType
from contextlib import contextmanager
from string import Template

#
# Exceptions.
#

class DSLException(Exception):
  def __init__(self, msg):
    self.msg = msg

  def __str__(self):
    return self.msg

class ParseException(DSLException):
  def __init__(self, token, to_end, stream):
    self.token = token
    self.to_end = to_end
    self.stream = stream

  def __str__(self):
    msg = "cannot parse " + self.token + ": " \
        +  self.stream[:-self.to_end] + "@" + self.stream[-self.to_end:]

    return msg

#
# OpenCL C pseudo-AST components.
#

class Arg(object):
  def __init__(self, ty, identifier):
    self.ty = ty
    self.identifier = identifier

  def __str__(self):
    return str(self.ty) + " " + str(self.identifier)

class TypeId(object):
  def __init__(self, name, vector_lenght = 0):
    self.name = name
    self.vector_lenght = vector_lenght

  def __str__(self):
    if self.vector_lenght != 0:
      ext = str(self.vector_lenght)
    else:
      ext = ""

    return self.name + ext

  def get_cty(self):
    if self.name == "float":
      return "f"
    else:
      return ""

  def is_gentype(self):
    return self.name == "gentype"

  def is_base(self):
    return self.vector_lenght == 0

class Identifier(object):
  def __init__(self, name):
    self.name = name

  def __str__(self):
    return self.name

class Prototype(object):
  def __init__(self, ret_ty, name, args):
    self.ret_ty = ret_ty
    self.name = name
    self.args = args

  def __str__(self):
    buf = str(self.ret_ty) + " " + str(self.name)

    buf = buf + "("
    for i in range(0, len(self.args)):
      if i != 0:
        buf = buf + ", "
      buf = buf + str(self.args[i])
    buf = buf + ")"

    return buf

#
# Parsers. Grammar is:
#
# prototype  -> type_id identifier lpar arg_list rpar
# arg_list   -> arg+
# arg        -> type_id identifier
# type_id    -> gentype
# identifier -> [a-zA-Z_][a-zA-Z0-9_]*
# lpar       -> '('
# rpar       -> ')'

class Parser(object):
  def __init__(self, stream):
    self.stream = stream
    self.rest = stream

  def parse_prototype(self):
    ret_ty = self.parse_type_id()
    name = self.parse_identifier()
    self.parse_lpar()
    args = self.parse_arg_list()
    self.parse_rpar()

    return Prototype(ret_ty, name, args)

  def parse_arg_list(self):
    args = []

    while True:
      arg = self.parse_arg()

      args.append(arg)

      match = re.match("\s*(,)", self.rest)

      if not match:
        break

      self.rest = self.rest[match.end(1):]

    return args

  def parse_arg(self):
    ty = self.parse_type_id()
    identifier = self.parse_identifier()

    return Arg(ty, identifier)

  def parse_type_id(self):
    match = re.match("\s*((gentype)|(float))", self.rest)

    # Mandatory known type.
    if not match:
      raise ParseException("type_id", len(self.rest), self.stream)

    ty_name = match.group(1)
    self.rest = self.rest[match.end(1):]

    # Optional vector length.
    match = re.match("((2)|(3)|(4)|(8)|(16))", self.rest)

    if match:
      vector_lenght = int(match.group(1))
      self.rest = self.rest[match.end(1):]
    else:
      vector_lenght = 0

    return TypeId(ty_name, vector_lenght)

  def parse_identifier(self):
    match = re.match("\s*([^\W\d]\w*)", self.rest)

    if not match:
      raise ParseException("identifier", len(self.rest), self.stream)

    identifier = Identifier(match.group(1))
    self.rest = self.rest[match.end(1):]

    return identifier

  def parse_lpar(self):
    match = re.match("\s*(\()", self.rest)

    if not match:
      raise ParseException("lpar", len(self.rest), self.stream)

    lpar = match.group(1)
    self.rest = self.rest[match.end(1):]

    return lpar

  def parse_rpar(self):
    match = re.match("\s*(\))", self.rest)

    if not match:
      raise ParseException("rpar", len(self.rest), self.stream)

    rpar = match.group(1)
    self.rest = self.rest[match.end(1):]

    return rpar

  def parse_end(self):
    match = re.match("\s*", self.rest)

    if len(self.rest) != match.end():
      raise ParseException("end", len(self.rest), self.stream)

    self.rest = self.rest[match.end():]

def parse_prototype(stream):
  parser = Parser(stream)

  proto = parser.parse_prototype()
  parser.parse_end()

  return proto

def parse_type_id(stream):
  parser = Parser(stream)

  type_id = parser.parse_type_id()
  parser.parse_end()

  return type_id

#
# DSL primitives.
#

class Function(object):
  def __init__(self, proto):
    self.proto = parse_prototype(proto)
    self.pure = True
    self.inline = True

  def __setattr__(self, attr, value):
    if attr == "proto":
      object.__setattr__(self, "proto", value)

    elif attr == "body":
      object.__setattr__(self, "body", Template(value))

    elif attr == "types":
      object.__setattr__(self, "types", [parse_type_id(ty) for ty in value])

    elif attr in ["pure", "inline"]:
      object.__setattr__(self, attr, value)

    else:
      raise DSLException("Unknown property: " + attr)

  def emit_decls(self, file):
    if not self.types:
      return

    file.write("\n/* Declarations of " + str(self.proto) + " */\n")
    for ty in self.types:
      self.emit_header(ty, file)
      file.write(";\n")

  def emit_impls(self, file):
    if not self.types:
      return

    file.write("\n/* Implementations of " + str(self.proto) + " */\n")
    for ty in self.types:
      file.write("\n")
      self.emit_header(ty, file)
      file.write(" {\n")

      if ty.is_base():
        self.emit_base_body(ty, file)
      else:
        self.emit_vect_body(ty, file)

      file.write("}\n")

  def emit_header(self, ty, file):
    proto = self.proto

    self.emit_attributes(file)
    file.write("\n")
    self.emit_type_id(proto.ret_ty, ty, file)
    file.write(" " + str(proto.name))

    file.write("(")
    for i in range(0, len(proto.args)):
      proto_ty = proto.args[i].ty
      identifier = proto.args[i].identifier

      if i != 0:
        file.write(", ")
      self.emit_type_id(proto_ty, ty, file)
      file.write(" " + str(identifier))
    file.write(")")

  def emit_base_body(self, ty, file):
    body = self.body

    file.write(body.substitute(cty = ty.get_cty()))
    file.write("\n")

  def emit_vect_body(self, ty, file):
    proto = self.proto
    ret_ty = proto.ret_ty

    if ret_ty.is_gentype():
      cur_ty = ty
    else:
      cur_ty = ret_ty
    file.write(str(cur_ty) + " Res;\n")

    for i in range(0, ty.vector_lenght):
      if ret_ty.is_gentype():
        file.write("Res[" + str(i) + "] = ")
      else:
        file.write("Res = ")

      file.write(str(proto.name));

      file.write("(")
      for j in range(0, len(proto.args)):
        cur_ty = proto.args[j].ty
        cur_var = proto.args[j].identifier

        if j != 0:
          file.write(", ")

        if cur_ty.is_gentype():
          file.write(str(cur_var) + "[" + str(i) + "]")
        else:
          file.write(str(cur_var))
      file.write(");\n")

    file.write("return Res;\n")

  def emit_attributes(self, file):
    file.write("__attribute__((overloadable))")
    if self.pure:
      file.write("\n__attribute__((pure))")
    if self.inline:
      file.write("\n__attribute__((always_inline))")

  def emit_type_id(self, proto_ty, ty, file):
    if proto_ty.is_gentype():
      cur_ty = ty
    else:
      cur_ty = proto_ty
    file.write(str(cur_ty))

@contextmanager
def gentype_func(proto):
  try:
    func = Function(proto)
    yield func

    if config.gen_decl:
      func.emit_decls(config.gen_decl)

    if config.gen_impl:
      func.emit_impls(config.gen_impl)
  except DSLException as ex:
    print ex

def vectors(base_ty, sizes = [2, 3, 4, 8, 16]):
  return [base_ty + str(size) for size in sizes]

#
# Module initialization.
#

def parse_command_line():
  parser = ArgumentParser(description = "OpenCL C generic library generator")

  parser.add_argument( "-d", "--gen-decl"
                     , metavar = "F"
                     , type = FileType("w")
                     , help = "write function declarations in file F"
                     )
  parser.add_argument( "-i", "--gen-impl"
                     , metavar = "F"
                     , type = FileType("w")
                     , help = "generate implementations in file F"
                     )

  return parser.parse_args()

config = parse_command_line()
