import sys
import operator
from functools import reduce

import re

_MODIFIER = re.compile(r"^(shr|minus)\(\d+\)$")

def parse(path, start_element, end_element):
    start_element("blxml", {})
    open_tag = None

    with open(path) as handle:
        for number, raw in enumerate(handle, 1):
            line = raw.split("//", 1)[0].rstrip()
            if not line.strip():
                continue

            indented = line[0].isspace()
            line = line.strip()

            if not indented:
                if open_tag:
                    end_element(open_tag)
                    open_tag = None

                if line.startswith("enum "):
                    body = line[len("enum "):]
                    attrs = {}
                    if " : " in body:
                        body, trailing = (part.strip() for part in body.rsplit(" : ", 1))
                        if trailing == "bitmask":
                            attrs["bitmask"] = "true"
                        else:
                            raise SyntaxError("%s:%d: unknown enum trait %r"
                                              % (path, number, trailing))
                    attrs["name"] = body.strip()
                    start_element("enum", attrs)
                    open_tag = "enum"
                elif line.startswith("struct "):
                    body = line[len("struct "):]
                    if " : " not in body:
                        raise SyntaxError("%s:%d: a struct needs `: size <bytes>`"
                                          % (path, number))
                    name, trailing = (part.strip() for part in body.rsplit(" : ", 1))
                    words = trailing.split()
                    if len(words) != 2 or words[0] != "size":
                        raise SyntaxError("%s:%d: expected `size <bytes>`, got %r"
                                          % (path, number, trailing))
                    start_element("struct", {"name": name, "size": words[1]})
                    open_tag = "struct"
                else:
                    raise SyntaxError("%s:%d: expected `enum` or `struct`, got %r"
                                      % (path, number, line))
                continue

            if open_tag is None:
                raise SyntaxError("%s:%d: indented line outside any enum or struct"
                                  % (path, number))

            if open_tag == "enum":
                name, _, value = (part.strip() for part in line.partition("="))
                if not value:
                    raise SyntaxError("%s:%d: an enum value needs `= <number>`"
                                      % (path, number))
                start_element("value", {"name": name, "value": value})
                end_element("value")
                continue

            name, _, rest = (part.strip() for part in line.rpartition(" : "))
            if not rest:
                raise SyntaxError("%s:%d: a field needs `: <start> <size> <type>`"
                                  % (path, number))

            words = rest.split()
            if len(words) < 3:
                raise SyntaxError("%s:%d: a field needs a start, a size and a type"
                                  % (path, number))

            attrs = {"name": name, "start": words[0], "size": words[1]}
            words = words[2:]

            if _MODIFIER.match(words[-1]):
                attrs["modifier"] = words.pop()

            if "=" in words:
                cut = words.index("=")
                attrs["default"] = " ".join(words[cut + 1:])
                if not attrs["default"]:
                    raise SyntaxError("%s:%d: `=` with no default after it"
                                      % (path, number))
                words = words[:cut]

            if not words:
                raise SyntaxError("%s:%d: a field needs a type" % (path, number))
            attrs["type"] = " ".join(words)

            start_element("field", attrs)
            end_element("field")

    if open_tag:
        end_element(open_tag)
    end_element("blxml")

def load(path):
    structs = {}
    enums = {}
    current = [None]

    def start(tag, attrs):
        if tag == "struct":
            current[0] = structs.setdefault(
                attrs["name"], {"size": attrs.get("size"), "fields": []})
        elif tag == "enum":
            current[0] = enums.setdefault(
                attrs["name"], {"bitmask": attrs.get("bitmask") == "true", "values": []})
        elif tag == "field":
            current[0]["fields"].append(attrs)
        elif tag == "value":
            current[0]["values"].append(attrs)

    parse(path, start, lambda tag: None)
    return structs, enums

global_prefix = "agx"

pack_prelude_head = """
/* Generated from lib/cmdbuf.def by lib/gencmd.py. Do not edit by hand.
 *
 * One typedef per command stream record, with the pack and unpack functions
 * that move it between the packed words the GPU reads and the plain struct
 * this project writes. The printers and byte maps only the decoder wants are
 * generated separately into agx_cmd_decode.h, so a translation unit that only
 * builds command buffers does not carry them.
 */

#ifndef AGX_PACK_H
#define AGX_PACK_H

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/* Bit casts between a float and its IEEE-754 encoding. The command stream
 * stores floats as raw 32 bit words, so packing one means reinterpreting its
 * bits rather than converting its value. They are spelled out here so that this
 * generated header stands on its own. */
static inline uint32_t
agx_float_bits(float f)
{
  uint32_t u = 0;
  memcpy(&u, &f, 4);
  return u;
}

static inline float
agx_bits_float(uint32_t u)
{
  float f = 0;
  memcpy(&f, &u, 4);
  return f;
}

"""

pack_helper_align = """#define agx_align_pot(v, pot) (((v) + ((pot) - 1)) & ~((pot) - 1))

/* Place a value in bits [start, end] of the word it is being packed into. */
static inline uint64_t
agx_cmd_uint(uint64_t v, uint32_t start, uint32_t end)
{
#ifndef NDEBUG
  const uint32_t width = end - start + 1;
  if (width < 64)
  {
    const uint64_t max = (1ull << width) - 1;
    assert(v <= max);
  }
#endif

  return v << start;
}

"""

pack_helper_sint = """static inline uint32_t
agx_cmd_sint(int32_t v, uint32_t start, uint32_t end)
{
#ifndef NDEBUG
  const uint32_t width = end - start + 1;
  if (width < 64)
  {
    const int64_t max = (1ll << (width - 1)) - 1;
    const int64_t min = -(1ll << (width - 1));
    assert(min <= v && v <= max);
  }
#endif

  return (((uint32_t)v) << start) & ((2ll << end) - 1);
}

static inline uint64_t
agx_unpack_uint(const uint8_t* restrict cl, uint32_t start, uint32_t end)
{
  const uint32_t width = end - start + 1;
  const uint64_t mask = (width == 64 ? ~0ull : (1ull << width) - 1);
  uint64_t       val  = 0;

  for (uint32_t byte = start / 8; byte <= end / 8; byte++)
    val |= ((uint64_t)cl[byte]) << ((byte - start / 8) * 8);

  return (val >> (start % 8)) & mask;
}

"""

pack_helper_unpack_sint = """static inline int64_t
agx_unpack_sint(const uint8_t* restrict cl, uint32_t start, uint32_t end)
{
  const uint32_t width = end - start + 1;
  const int64_t  val   = (int64_t)agx_unpack_uint(cl, start, end);

  /* Sign extend from the field's own width. */
  return (val << (64 - width)) >> (64 - width);
}

static inline float
agx_unpack_float(const uint8_t* restrict cl, uint32_t start, uint32_t end)
{
  return agx_bits_float((uint32_t)agx_unpack_uint(cl, start, end));
}

/* Fill a record and pack it. The block sets the fields it cares about on
 * `name`, which starts from the record's defaults, and the record is packed
 * into `dst` when the block ends:
 *
 *     agx_cmd(cursor, ppp_header, cfg) { cfg.viewport_count = 1; }
 *
 * `T` is the record named the way its functions are, lower case and without the
 * agx_ prefix, because the preprocessor cannot change a token's case and the
 * macro has to build `agx_ppp_header_pack` out of it. */
#define agx_cmd(dst, T, name)                                                 \\
  for (__typeof__(agx_##T##_default()) name = agx_##T##_default(),             \\
                                       *_agx_cmd_once = (void*)&name;         \\
       _agx_cmd_once;                                                         \\
       (agx_##T##_pack((uint32_t*)(dst), &name), _agx_cmd_once = NULL))

/* Fill a record, pack it at a cursor, and advance the cursor past it. The
 * length comes from the record itself, since agx_*_pack returns the bytes it
 * wrote, so a record can never be emitted with one length and stepped over with
 * another -- the failure mode of a hand written
 *
 *     memcpy(base + off, packed, AGX_DRAW_LENGTH);
 *     off += AGX_DRAW_LENGTH;
 *
 * where the two lengths are free to disagree and a conditionally emitted record
 * leaves every offset after it wrong. */
#define agx_push(ptr, T, name)                                                 \\
  for (__typeof__(agx_##T##_default()) name = agx_##T##_default(),             \\
                                       *_agx_push_once = (void*)&name;         \\
       _agx_push_once;                                                         \\
       ((ptr) = (void*)((uintptr_t)(ptr) +                                     \\
                        agx_##T##_pack((uint32_t*)(ptr), &name)),              \\
        _agx_push_once = NULL))

/* Declare a local of the record's type and unpack `src` into it. */
#define agx_unpack(src, T, name)                                               \\
  __typeof__(agx_##T##_default()) name;                                        \\
  agx_##T##_unpack((const uint8_t*)(src), &name)

"""

pack_prelude = (pack_prelude_head + pack_helper_align + pack_helper_sint +
                pack_helper_unpack_sint)

decode_prelude = """
/* Generated from lib/cmdbuf.def by lib/gencmd.py. Do not edit by hand.
 *
 * The decode side of the record definitions: one printer per record, one string
 * table per enum, and a byte map naming which field owns which bytes. Only the
 * decoder wants these, so they are kept out of agx_cmd.h.
 */

#ifndef AGX_PACK_DECODE_H
#define AGX_PACK_DECODE_H

#include <agx_cmd.h>
#include <inttypes.h>
#include <stdio.h>

/* One field's byte range within a record, so a decoder can report which bytes
 * of a capture are accounted for and, more usefully, which bytes a capture sets
 * that no field names. */
typedef struct Agx_Field_Span
{
  uint16_t    offset;
  uint16_t    bytes;
  const char* name;
} Agx_Field_Span;

/* Print an unpacked record: agx_print(fp, ppp_header, cfg, indent). */
#define agx_print(fp, T, var, indent) agx_##T##_print((fp), &(var), (indent))

"""

def to_alphanum(name):
    substitutions = {
        ' ': '_',
        '/': '_',
        '[': '',
        ']': '',
        '(': '',
        ')': '',
        '-': '_',
        ':': '',
        '.': '',
        ',': '',
        '=': '',
        '>': '',
        '#': '',
        '&': '',
        '*': '',
        '"': '',
        '+': '',
        '\'': '',
    }

    for i, j in substitutions.items():
        name = name.replace(i, j)

    return name

def safe_name(name):
    name = to_alphanum(name)
    if not name[0].isalpha():
        name = '_' + name

    return name

def prefixed_upper_name(prefix, name):
    if prefix:
        name = prefix + "_" + name
    return safe_name(name).upper()

def enum_name(name):
    return "{}_{}".format(global_prefix, safe_name(name)).lower()

def enum_type_name(name):
    return "_".join(part.capitalize() for part in enum_name(name).split("_"))

def struct_type_name(name):
    return "_".join(part.capitalize() for part in struct_func_name(name).split("_"))

def struct_func_name(name):
    return "{}_{}".format(global_prefix, safe_name(name)).lower()

def struct_macro_name(name):
    return struct_func_name(name).upper()

def num_from_str(num_str):
    if num_str.lower().startswith('0x'):
        return int(num_str, base=16)
    else:
        assert(not num_str.startswith('0') and 'octals numbers not allowed')
        return int(num_str)

MODIFIERS = ["shr", "minus", "align", "log2"]

def parse_modifier(modifier):
    if modifier is None:
        return None

    for mod in MODIFIERS:
        if modifier[0:len(mod)] == mod:
            if mod == "log2":
                assert(len(mod) == len(modifier))
                return [mod]

            if modifier[len(mod)] == '(' and modifier[-1] == ')':
                ret = [mod, int(modifier[(len(mod) + 1):-1])]
                if ret[0] == 'align':
                    align = ret[1]

                    assert(align > 0 and not(align & (align - 1)));

                return ret

    print("Invalid modifier")
    assert(False)

class Field(object):
    def __init__(self, parser, attrs):
        self.parser = parser
        if "name" in attrs:
            self.name = safe_name(attrs["name"]).lower()
            self.human_name = attrs["name"]

        if ":" in str(attrs["start"]):
            (word, bit) = attrs["start"].split(":")
            self.start = (int(word) * 32) + int(bit)
        else:
            self.start = int(attrs["start"])

        self.end = self.start + int(attrs["size"]) - 1
        self.type = attrs["type"]

        if self.type == 'bool' and self.start != self.end:
            print("#error Field {} has bool type but more than one bit of size".format(self.name));

        width = self.end - self.start + 1
        if width > 64:
            raise ValueError(
                "field '{}' is {} bits wide; a field cannot exceed 64 bits "
                "because every value is packed through a uint64_t. Split it at "
                "64-bit boundaries.".format(getattr(self, 'human_name', self.name), width))

        if "prefix" in attrs:
            self.prefix = safe_name(attrs["prefix"]).upper()
        else:
            self.prefix = None

        if "exact" in attrs:
            self.exact = int(attrs["exact"])
        else:
            self.exact = None

        self.default = attrs.get("default")

        if self.type in self.parser.enums and self.default is not None:
            self.default = safe_name('{}_{}_{}'.format(global_prefix, self.type, self.default)).upper()

        self.modifier  = parse_modifier(attrs.get("modifier"))

    def emit_template_struct(self, dim):
        if self.type == 'address':
            type = 'uint64_t'
        elif self.type == 'bool':
            type = 'bool'
        elif self.type == 'float':
            type = 'float'
        elif self.type in ['uint', 'hex'] and self.end - self.start > 32:
            type = 'uint64_t'
        elif self.type == 'int':
            type = 'int32_t'
        elif self.type in ['uint', 'uint/float', 'hex']:
            type = 'uint32_t'
        elif self.type in self.parser.structs:
            type = struct_type_name(self.type)
        elif self.type in self.parser.enums:
            type = enum_type_name(self.type)
        else:
            print("#error unhandled type: %s" % self.type)
            type = "uint32_t"

        print("  %s %s%s;" % (type, self.name, dim))

        for value in self.values:
            name = prefixed_upper_name(self.prefix, value.name)
            print("#define %s %d" % (name, value.value))

    def overlaps(self, field):
        return self != field and max(self.start, field.start) <= min(self.end, field.end)

class Group(object):
    def __init__(self, parser, parent, start, count, label):
        self.parser = parser
        self.parent = parent
        self.start = start
        self.count = count
        self.label = label
        self.size = 0
        self.length = 0
        self.fields = []

    def get_length(self):

        calculated = max(field.end // 8 for field in self.fields) + 1 if len(self.fields) > 0 else 0
        if self.length > 0:
            assert(self.length >= calculated)
        else:
            self.length = calculated
        return self.length

    def emit_template_struct(self, dim):
        if self.count == 0:
            print("  /* variable length fields follow */")
        else:
            if self.count > 1:
                dim = "%s[%d]" % (dim, self.count)

            if len(self.fields) == 0:
                print("  uint32_t dummy;")

            for field in self.fields:
                if field.exact is not None:
                    continue

                field.emit_template_struct(dim)

    class Word:
        def __init__(self):
            self.size = 32
            self.contributors = []

    class FieldRef:
        def __init__(self, field, path, start, end):
            self.field = field
            self.path = path
            self.start = start
            self.end = end

    def collect_fields(self, fields, offset, path, all_fields):
        for field in fields:
            field_path = '{}{}'.format(path, field.name)
            field_offset = offset + field.start

            if field.type in self.parser.structs:
                sub_struct = self.parser.structs[field.type]
                self.collect_fields(sub_struct.fields, field_offset, field_path + '.', all_fields)
                continue

            start = field_offset
            end = offset + field.end
            all_fields.append(self.FieldRef(field, field_path, start, end))

    def collect_words(self, fields, offset, path, words):
        for field in fields:
            field_path = '{}{}'.format(path, field.name)
            start = offset + field.start

            if field.type in self.parser.structs:
                sub_fields = self.parser.structs[field.type].fields
                self.collect_words(sub_fields, start, field_path + '.', words)
                continue

            end = offset + field.end
            contributor = self.FieldRef(field, field_path, start, end)
            first_word = contributor.start // 32
            last_word = contributor.end // 32
            for b in range(first_word, last_word + 1):
                if not b in words:
                    words[b] = self.Word()
                words[b].contributors.append(contributor)

    def emit_pack_function(self):
        self.get_length()

        words = {}
        self.collect_words(self.fields, 0, '', words)

        for field in self.fields:
            if field.modifier is None:
                continue

            assert(field.exact is None)

            if field.modifier[0] == "shr":
                shift = field.modifier[1]
                mask = hex((1 << shift) - 1)
                print("  assert((values->{} & {}) == 0);".format(field.name, mask))
            elif field.modifier[0] == "minus":
                print("  assert(values->{} >= {});".format(field.name, field.modifier[1]))
            elif field.modifier[0] == "log2":
                print("  assert(agx_is_power_of_two(values->{}));".format(field.name))

        for index in range((self.length + 3) // 4):

            if not index in words:
                print("  cl[%2d] = 0;" % index)
                continue

            word = words[index]

            word_start = index * 32

            v = None
            prefix = "  cl[%2d] =" % index

            for contributor in word.contributors:
                field = contributor.field
                name = field.name
                start = contributor.start
                end = contributor.end
                contrib_word_start = (start // 32) * 32
                start -= contrib_word_start
                end -= contrib_word_start

                value = str(field.exact) if field.exact is not None else "values->{}".format(contributor.path)
                if field.modifier is not None:
                    if field.modifier[0] == "shr":
                        value = "{} >> {}".format(value, field.modifier[1])
                    elif field.modifier[0] == "minus":
                        value = "{} - {}".format(value, field.modifier[1])
                    elif field.modifier[0] == "align":
                        value = "agx_align_pot({}, {})".format(value, field.modifier[1])
                    elif field.modifier[0] == "log2":
                        value = "util_logbase2({})".format(value)

                if field.type in ["uint", "hex", "address"]:
                    s = "agx_cmd_uint(%s, %d, %d)" % \
                        (value, start, end)
                elif field.type in self.parser.enums:
                    s = "agx_cmd_uint(%s, %d, %d)" % \
                        (value, start, end)
                elif field.type == "int":
                    s = "agx_cmd_sint(%s, %d, %d)" % \
                        (value, start, end)
                elif field.type == "bool":
                    s = "agx_cmd_uint(%s, %d, %d)" % \
                        (value, start, end)
                elif field.type == "float":
                    assert(start == 0 and end == 31)
                    s = "agx_cmd_uint(agx_float_bits({}), 0, 32)".format(value)
                else:
                    s = "#error unhandled field {}, type {}".format(contributor.path, field.type)

                if not s == None:
                    shift = word_start - contrib_word_start
                    if shift:
                        s = "%s >> %d" % (s, shift)

                    if contributor == word.contributors[-1]:
                        print("%s %s;" % (prefix, s))
                    else:
                        print("%s %s |" % (prefix, s))
                    prefix = "          "

            continue

    def emit_unpack_function(self):
        fieldrefs = []
        self.collect_fields(self.fields, 0, '', fieldrefs)
        for fieldref in fieldrefs:
            field = fieldref.field
            convert = None

            args = []
            args.append('cl')
            args.append(str(fieldref.start))
            args.append(str(fieldref.end))

            if field.type in set(["uint", "uint/float", "address", "hex"]) | self.parser.enums:
                convert = "agx_unpack_uint"
            elif field.type == "int":
                convert = "agx_unpack_sint"
            elif field.type == "bool":
                convert = "agx_unpack_uint"
            elif field.type == "float":
                convert = "agx_unpack_float"
            else:
                s = "/* unhandled field %s, type %s */\n" % (field.name, field.type)

            suffix = ""
            prefix = ""
            if field.modifier:
                if field.modifier[0] == "minus":
                    suffix = " + {}".format(field.modifier[1])
                elif field.modifier[0] == "shr":
                    suffix = " << {}".format(field.modifier[1])
                if field.modifier[0] == "log2":
                    prefix = "1 << "

            decoded = '{}{}({}){}'.format(prefix, convert, ', '.join(args), suffix)

            cast = ''
            if field.type in self.parser.enums:
                cast = '({})'.format(enum_type_name(field.type))
            print('  values->{} = {}{};'.format(fieldref.path, cast, decoded))
            if field.modifier and field.modifier[0] == "align":
                mask = hex(field.modifier[1] - 1)
                print('  assert(!(values->{} & {}));'.format(fieldref.path, mask))

    def emit_print_function(self):
        for field in self.fields:
            convert = None
            name, val = field.human_name, 'values->{}'.format(field.name)

            if field.type in self.parser.structs:
                print('  fprintf(fp, "%*s{}:\\n", (int)indent, "");'.format(field.human_name))
                print("  {}_print(fp, &values->{}, indent + 2);".format(struct_func_name(field.type), field.name))
            elif field.type == "address":

                print('  fprintf(fp, "%*s{}: 0x%" PRIx64 "\\n", (int)indent, "", {});'.format(name, val))
            elif field.type in self.parser.enums:
                print('  fprintf(fp, "%*s{}: %s\\n", (int)indent, "", {}_as_str({}));'.format(name, enum_name(field.type), val))
            elif field.type == "int":
                print('  fprintf(fp, "%*s{}: %d\\n", (int)indent, "", {});'.format(name, val))
            elif field.type == "bool":
                print('  fprintf(fp, "%*s{}: %s\\n", (int)indent, "", {} ? "true" : "false");'.format(name, val))
            elif field.type == "float":
                print('  fprintf(fp, "%*s{}: %f\\n", (int)indent, "", {});'.format(name, val))
            elif field.type in ["uint", "hex"] and (field.end - field.start) >= 32:
                print('  fprintf(fp, "%*s{}: 0x%" PRIx64 "\\n", (int)indent, "", {});'.format(name, val))
            elif field.type == "hex":
                print('  fprintf(fp, "%*s{}: 0x%" PRIx32 "\\n", (int)indent, "", {});'.format(name, val))
            elif field.type == "uint/float":
                print('  fprintf(fp, "%*s{}: 0x%X (%f)\\n", (int)indent, "", {}, agx_bits_float({}));'.format(name, val, val))
            else:
                print('  fprintf(fp, "%*s{}: %u\\n", (int)indent, "", {});'.format(name, val))

class Value(object):
    def __init__(self, attrs):
        self.name = attrs["name"]
        self.value = int(attrs["value"], 0)

class Parser(object):
    def __init__(self, decode):
        self.decode = decode

        self.struct = None
        self.structs = {}

        self.enums = set()

    def start_element(self, name, attrs):
        if name == "blxml":
            print(decode_prelude if self.decode else pack_prelude)
        elif name == "struct":
            name = attrs["name"]
            self.no_direct_packing = attrs.get("no-direct-packing", False)
            self.struct = name

            self.group = Group(self, None, 0, 1, name)
            if "size" in attrs:
                self.group.length = int(attrs["size"])
            self.group.align = int(attrs["align"]) if "align" in attrs else None
            self.structs[attrs["name"]] = self.group
        elif name == "field":
            self.group.fields.append(Field(self, attrs))
            self.values = []
        elif name == "enum":
            self.values = []
            self.enum = safe_name(attrs["name"])
            self.enums.add(attrs["name"])

            self.enum_bitmask = attrs.get("bitmask") == "true"
            if "prefix" in attrs:
                self.prefix = attrs["prefix"]
            else:
                self.prefix= None
        elif name == "value":
            self.values.append(Value(attrs))

    def end_element(self, name):
        if name == "struct":
            self.emit_struct()
            self.struct = None
            self.group = None
        elif name  == "field":
            self.group.fields[-1].values = self.values
        elif name  == "enum":
            self.emit_enum()
            self.enum = None
        elif name == "blxml":
            print('#endif')

    def emit_default_function(self, name):
        type_name = struct_type_name(name)
        default_fields = []
        for field in self.group.fields:
            if not type(field) is Field:
                continue
            if field.default is not None:
                default_fields.append("    .{} = {}".format(field.name, field.default))
            elif field.type in self.structs:
                default_fields.append("    .{} = {}_default()".format(field.name, struct_func_name(field.type)))

        print("static inline {}".format(type_name))
        print("{}_default(void)".format(struct_func_name(name)))
        print("{")
        if default_fields:
            print("  return ({}){{".format(type_name))
            print(",\n".join(default_fields) + ",")
            print("  };")
        else:
            print("  return ({}){{ 0 }};".format(type_name))
        print("}\n")

    def emit_template_struct(self, name, group):
        type_name = struct_type_name(name)
        print("typedef struct %s" % type_name)
        print("{")
        group.emit_template_struct("")
        print("} %s;\n" % type_name)

        print('#define {} {}'.format(struct_macro_name(name) + "_LENGTH", group.get_length()))
        if group.align != None:
            print('#define {} {}'.format(struct_macro_name(name) + "_ALIGN", group.align))
        print('')

    def emit_pack_function(self, name, group):
        func = struct_func_name(name)
        type_name = struct_type_name(name)
        print("static inline uint32_t")
        print("%s_pack(uint32_t* restrict cl, const %s* restrict values)" % (func, type_name))
        print("{")

        group.emit_pack_function()

        print("")
        print("  return %s_LENGTH;" % struct_macro_name(name))
        print("}\n")

    def emit_unpack_function(self, name, group):
        func = struct_func_name(name)
        type_name = struct_type_name(name)
        print("static inline void")
        print("%s_unpack(const uint8_t* restrict cl, %s* restrict values)" % (func, type_name))
        print("{")

        group.emit_unpack_function()

        print("}\n")

    def emit_field_map(self, name, group):
        rows = []
        for field in group.fields:
            if field.name in ("Reserved", "Unknown"):
                continue
            lo, hi = field.start // 8, field.end // 8
            rows.append((lo, hi - lo + 1, field.name))
        if not rows:
            return
        print("static const Agx_Field_Span {}_fields[] = {{".format(struct_func_name(name)))
        for lo, n, fname in sorted(rows):
            print('  { 0x%04x, %2d, "%s" },' % (lo, n, fname))
        print("};\n")

    def emit_print_function(self, name, group):
        print("static inline void")
        print("{}_print(FILE* fp, const {}* values, uint32_t indent)".format(
            struct_func_name(name), struct_type_name(name)))
        print("{")

        group.emit_print_function()

        print("}\n")

    def emit_struct(self):
        name = self.struct

        if self.decode:
            self.emit_print_function(name, self.group)
            self.emit_field_map(name, self.group)
            return

        self.emit_template_struct(name, self.group)
        self.emit_default_function(name)
        if self.no_direct_packing == False:
            self.emit_pack_function(name, self.group)
            self.emit_unpack_function(name, self.group)

    def enum_prefix(self, name):
        return

    def emit_enum(self):
        e_name = enum_name(self.enum)
        t_name = enum_type_name(self.enum)
        prefix = e_name if self.enum != 'Format' else global_prefix
        names = []
        for value in self.values:
            names.append((safe_name('{}_{}'.format(prefix, value.name)).upper(), value))

        if not self.decode:
            print('typedef enum {}'.format(t_name))
            print('{')
            for name, value in names:
                print('  %s = %d,' % (name, value.value))
            print('}} {};\n'.format(t_name))
            return

        if self.enum_bitmask:
            print("/* The value is a mask, so this expands it: \"Blit|Vertex\", with any bit")
            print(" * the enum does not name appended as hex. The buffer is static, so one")
            print(" * call's result is live until the next. */")
            print("static inline const char*")
            print("{}_as_str({} imm)".format(e_name, t_name))
            print("{")
            print("  static char text[256];")
            print("  uint32_t    rest = (uint32_t)imm;")
            print("  size_t      used = 0;")
            print("")
            print("  text[0] = '\\0';")
            for name, value in names:
                print("  if (rest & %s)" % name)
                print("  {")
                print('    used += (size_t)snprintf(text + used, sizeof(text) - used, "%s%s",')
                print('                            used ? "|" : "", "{}");'.format(value.name))
                print("    rest &= ~(uint32_t)%s;" % name)
                print("  }")
            print("  if (rest)")
            print("  {")
            print('    snprintf(text + used, sizeof(text) - used, "%s0x%x", used ? "|" : "", rest);')
            print("  }")
            print('  return text[0] ? text : "none";')
            print("}\n")
            return

        print("static inline const char*")
        print("{}_as_str({} imm)".format(e_name, t_name))
        print("{")
        print("  switch (imm)")
        print("  {")
        for name, value in names:
            print('  case {}: return "{}";'.format(name, value.name))
        print('  default: return "invalid";')
        print("  }")
        print("}\n")

    def parse(self, filename):
        parse(filename, self.start_element, self.end_element)

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("usage: gencmd.py cmdbuf.def [--decode]")
        sys.exit(1)

    input_file = sys.argv[1]
    decode = "--decode" in sys.argv[2:]

    p = Parser(decode)
    p.parse(input_file)
