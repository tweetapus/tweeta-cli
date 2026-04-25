#include "tweeta.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
  JSON_NULL,
  JSON_BOOL,
  JSON_NUMBER,
  JSON_STRING,
  JSON_ARRAY,
  JSON_OBJECT
} JsonKind;

typedef struct JsonValue JsonValue;

typedef struct {
  char *key;
  JsonValue *value;
} JsonPair;

struct JsonValue {
  JsonKind kind;
  union {
    bool boolean;
    char *text;
    struct {
      JsonValue **items;
      size_t len;
    } array;
    struct {
      JsonPair *pairs;
      size_t len;
    } object;
  } as;
};

typedef struct {
  const char *p;
} JsonParser;

static void skip_ws(JsonParser *p) {
  while (isspace((unsigned char)*p->p)) p->p++;
}

static bool match(JsonParser *p, const char *literal) {
  size_t n = strlen(literal);
  if (strncmp(p->p, literal, n) != 0) return false;
  p->p += n;
  return true;
}

static JsonValue *json_new(JsonKind kind) {
  JsonValue *v = calloc(1, sizeof(*v));
  if (!v) die("out of memory");
  v->kind = kind;
  return v;
}

static int hex_value(char ch) {
  if (ch >= '0' && ch <= '9') return ch - '0';
  if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
  if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
  return -1;
}

static void append_utf8(Buffer *b, unsigned code) {
  unsigned char tmp[4];
  size_t n = 0;
  if (code <= 0x7f) {
    tmp[n++] = (unsigned char)code;
  } else if (code <= 0x7ff) {
    tmp[n++] = (unsigned char)(0xc0 | (code >> 6));
    tmp[n++] = (unsigned char)(0x80 | (code & 0x3f));
  } else {
    tmp[n++] = (unsigned char)(0xe0 | (code >> 12));
    tmp[n++] = (unsigned char)(0x80 | ((code >> 6) & 0x3f));
    tmp[n++] = (unsigned char)(0x80 | (code & 0x3f));
  }
  b->data = xrealloc(b->data, b->len + n + 1);
  memcpy(b->data + b->len, tmp, n);
  b->len += n;
  b->data[b->len] = 0;
}

static char *parse_string(JsonParser *p) {
  if (*p->p != '"') return NULL;
  p->p++;
  Buffer b = {0};
  b.data = xstrdup("");
  while (*p->p && *p->p != '"') {
    unsigned char ch = (unsigned char)*p->p++;
    if (ch == '\\') {
      ch = (unsigned char)*p->p++;
      if (ch == '"') ch = '"';
      else if (ch == '\\') ch = '\\';
      else if (ch == '/') ch = '/';
      else if (ch == 'b') ch = '\b';
      else if (ch == 'f') ch = '\f';
      else if (ch == 'n') ch = '\n';
      else if (ch == 'r') ch = '\r';
      else if (ch == 't') ch = '\t';
      else if (ch == 'u') {
        unsigned code = 0;
        for (int i = 0; i < 4; i++) {
          int hv = hex_value(*p->p++);
          if (hv < 0) {
            free(b.data);
            return NULL;
          }
          code = (code << 4) | (unsigned)hv;
        }
        append_utf8(&b, code);
        continue;
      } else {
        free(b.data);
        return NULL;
      }
    } else if (ch < 32) {
      free(b.data);
      return NULL;
    }
    b.data = xrealloc(b.data, b.len + 2);
    b.data[b.len++] = (char)ch;
    b.data[b.len] = 0;
  }
  if (*p->p != '"') {
    free(b.data);
    return NULL;
  }
  p->p++;
  return b.data;
}

static JsonValue *parse_value(JsonParser *p);
static void json_free(JsonValue *v);

static JsonValue *parse_array(JsonParser *p) {
  if (*p->p != '[') return NULL;
  p->p++;
  JsonValue *v = json_new(JSON_ARRAY);
  skip_ws(p);
  if (*p->p == ']') {
    p->p++;
    return v;
  }
  while (*p->p) {
    JsonValue *item = parse_value(p);
    if (!item) {
      json_free(v);
      return NULL;
    }
    v->as.array.items = xrealloc(v->as.array.items, sizeof(*v->as.array.items) * (v->as.array.len + 1));
    v->as.array.items[v->as.array.len++] = item;
    skip_ws(p);
    if (*p->p == ']') {
      p->p++;
      return v;
    }
    if (*p->p != ',') {
      json_free(v);
      return NULL;
    }
    p->p++;
    skip_ws(p);
  }
  json_free(v);
  return NULL;
}

static JsonValue *parse_object(JsonParser *p) {
  if (*p->p != '{') return NULL;
  p->p++;
  JsonValue *v = json_new(JSON_OBJECT);
  skip_ws(p);
  if (*p->p == '}') {
    p->p++;
    return v;
  }
  while (*p->p) {
    char *key = parse_string(p);
    if (!key) {
      json_free(v);
      return NULL;
    }
    skip_ws(p);
    if (*p->p != ':') {
      free(key);
      json_free(v);
      return NULL;
    }
    p->p++;
    JsonValue *value = parse_value(p);
    if (!value) {
      free(key);
      json_free(v);
      return NULL;
    }
    v->as.object.pairs = xrealloc(v->as.object.pairs, sizeof(*v->as.object.pairs) * (v->as.object.len + 1));
    v->as.object.pairs[v->as.object.len].key = key;
    v->as.object.pairs[v->as.object.len].value = value;
    v->as.object.len++;
    skip_ws(p);
    if (*p->p == '}') {
      p->p++;
      return v;
    }
    if (*p->p != ',') {
      json_free(v);
      return NULL;
    }
    p->p++;
    skip_ws(p);
  }
  json_free(v);
  return NULL;
}

static JsonValue *parse_number(JsonParser *p) {
  const char *start = p->p;
  if (*p->p == '-') p->p++;
  if (*p->p == '0') p->p++;
  else {
    if (!isdigit((unsigned char)*p->p)) return NULL;
    while (isdigit((unsigned char)*p->p)) p->p++;
  }
  if (*p->p == '.') {
    p->p++;
    if (!isdigit((unsigned char)*p->p)) return NULL;
    while (isdigit((unsigned char)*p->p)) p->p++;
  }
  if (*p->p == 'e' || *p->p == 'E') {
    p->p++;
    if (*p->p == '+' || *p->p == '-') p->p++;
    if (!isdigit((unsigned char)*p->p)) return NULL;
    while (isdigit((unsigned char)*p->p)) p->p++;
  }
  size_t n = (size_t)(p->p - start);
  JsonValue *v = json_new(JSON_NUMBER);
  v->as.text = malloc(n + 1);
  if (!v->as.text) die("out of memory");
  memcpy(v->as.text, start, n);
  v->as.text[n] = 0;
  return v;
}

static JsonValue *parse_value(JsonParser *p) {
  skip_ws(p);
  if (*p->p == '"') {
    JsonValue *v = json_new(JSON_STRING);
    v->as.text = parse_string(p);
    if (!v->as.text) {
      free(v);
      return NULL;
    }
    return v;
  }
  if (*p->p == '[') return parse_array(p);
  if (*p->p == '{') return parse_object(p);
  if (*p->p == 't' && match(p, "true")) {
    JsonValue *v = json_new(JSON_BOOL);
    v->as.boolean = true;
    return v;
  }
  if (*p->p == 'f' && match(p, "false")) {
    JsonValue *v = json_new(JSON_BOOL);
    v->as.boolean = false;
    return v;
  }
  if (*p->p == 'n' && match(p, "null")) return json_new(JSON_NULL);
  if (*p->p == '-' || isdigit((unsigned char)*p->p)) return parse_number(p);
  return NULL;
}

static void json_free(JsonValue *v) {
  if (!v) return;
  if (v->kind == JSON_STRING || v->kind == JSON_NUMBER) {
    free(v->as.text);
  } else if (v->kind == JSON_ARRAY) {
    for (size_t i = 0; i < v->as.array.len; i++) json_free(v->as.array.items[i]);
    free(v->as.array.items);
  } else if (v->kind == JSON_OBJECT) {
    for (size_t i = 0; i < v->as.object.len; i++) {
      free(v->as.object.pairs[i].key);
      json_free(v->as.object.pairs[i].value);
    }
    free(v->as.object.pairs);
  }
  free(v);
}

static bool is_scalar(const JsonValue *v) {
  return v->kind == JSON_NULL || v->kind == JSON_BOOL || v->kind == JSON_NUMBER || v->kind == JSON_STRING;
}

static void print_indent(int indent) {
  for (int i = 0; i < indent; i++) fputc(' ', stdout);
}

static void print_string_value(const char *s) {
  bool simple = *s != 0;
  for (const unsigned char *p = (const unsigned char *)s; *p; p++) {
    if (*p == '\n' || *p == '\r' || *p == '\t' || *p < 32) simple = false;
  }
  if (simple) {
    fputs(s, stdout);
    return;
  }
  fputc('"', stdout);
  for (const unsigned char *p = (const unsigned char *)s; *p; p++) {
    if (*p == '\n') fputs("\\n", stdout);
    else if (*p == '\r') fputs("\\r", stdout);
    else if (*p == '\t') fputs("\\t", stdout);
    else if (*p == '"') fputs("\\\"", stdout);
    else if (*p == '\\') fputs("\\\\", stdout);
    else fputc(*p, stdout);
  }
  fputc('"', stdout);
}

static void print_scalar(const JsonValue *v) {
  if (v->kind == JSON_NULL) fputs("null", stdout);
  else if (v->kind == JSON_BOOL) fputs(v->as.boolean ? "true" : "false", stdout);
  else if (v->kind == JSON_NUMBER) fputs(v->as.text, stdout);
  else if (v->kind == JSON_STRING) print_string_value(v->as.text);
}

static void print_value(const JsonValue *v, int indent);

static void print_object(const JsonValue *v, int indent) {
  if (v->as.object.len == 0) {
    print_indent(indent);
    fputs("{}", stdout);
    fputc('\n', stdout);
    return;
  }
  for (size_t i = 0; i < v->as.object.len; i++) {
    const JsonPair *pair = &v->as.object.pairs[i];
    print_indent(indent);
    printf("%s:", pair->key);
    if (is_scalar(pair->value)) {
      fputc(' ', stdout);
      print_scalar(pair->value);
      fputc('\n', stdout);
    } else {
      fputc('\n', stdout);
      print_value(pair->value, indent + 2);
    }
  }
}

static void print_array(const JsonValue *v, int indent) {
  if (v->as.array.len == 0) {
    print_indent(indent);
    fputs("[]\n", stdout);
    return;
  }
  for (size_t i = 0; i < v->as.array.len; i++) {
    const JsonValue *item = v->as.array.items[i];
    print_indent(indent);
    if (is_scalar(item)) {
      fputs("- ", stdout);
      print_scalar(item);
      fputc('\n', stdout);
    } else if (item->kind == JSON_OBJECT && item->as.object.len == 0) {
      fputs("- ", stdout);
      fputs("{}\n", stdout);
    } else if (item->kind == JSON_ARRAY && item->as.array.len == 0) {
      fputs("- ", stdout);
      fputs("[]\n", stdout);
    } else {
      fputs("-\n", stdout);
      print_value(item, indent + 2);
    }
  }
}

static void print_value(const JsonValue *v, int indent) {
  if (is_scalar(v)) {
    print_indent(indent);
    print_scalar(v);
    fputc('\n', stdout);
  } else if (v->kind == JSON_ARRAY) {
    print_array(v, indent);
  } else {
    print_object(v, indent);
  }
}

bool print_json_readable(const char *body) {
  JsonParser parser = {body};
  skip_ws(&parser);
  JsonValue *root = parse_value(&parser);
  if (!root) {
    if (getenv("TWEETA_JSON_DEBUG")) {
      fprintf(stderr, "tweeta: json parse failed at byte %zu near '%.32s'\n", (size_t)(parser.p - body), parser.p);
    }
    return false;
  }
  skip_ws(&parser);
  if (*parser.p != 0) {
    if (getenv("TWEETA_JSON_DEBUG")) {
      fprintf(stderr, "tweeta: trailing data after json at byte %zu near '%.32s'\n", (size_t)(parser.p - body), parser.p);
    }
    json_free(root);
    return false;
  }
  print_value(root, 0);
  json_free(root);
  return true;
}
