#include "tweeta.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void die(const char *msg) {
  fprintf(stderr, "tweeta: %s\n", msg);
  exit(1);
}

char *xstrdup(const char *s) {
  char *p = strdup(s ? s : "");
  if (!p) die("out of memory");
  return p;
}

void *xrealloc(void *p, size_t n) {
  void *q = realloc(p, n);
  if (!q) die("out of memory");
  return q;
}

size_t write_cb(char *ptr, size_t size, size_t nmemb, void *userdata) {
  Buffer *b = (Buffer *)userdata;
  size_t n = size * nmemb;
  b->data = xrealloc(b->data, b->len + n + 1);
  memcpy(b->data + b->len, ptr, n);
  b->len += n;
  b->data[b->len] = 0;
  return n;
}

void trim_newline(char *s) {
  size_t n = strlen(s);
  while (n && (s[n - 1] == '\n' || s[n - 1] == '\r')) s[--n] = 0;
}

char *read_file(const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f) {
    perror(path);
    exit(1);
  }
  if (fseek(f, 0, SEEK_END) != 0) die("failed to seek file");
  long n = ftell(f);
  if (n < 0) die("failed to read file length");
  rewind(f);
  char *buf = malloc((size_t)n + 1);
  if (!buf) die("out of memory");
  if (fread(buf, 1, (size_t)n, f) != (size_t)n) die("failed to read file");
  buf[n] = 0;
  fclose(f);
  return buf;
}

char *urlenc(CURL *curl, const char *s) {
  char *e = curl_easy_escape(curl, s ? s : "", 0);
  if (!e) die("url encode failed");
  char *out = xstrdup(e);
  curl_free(e);
  return out;
}

char *join_query(CURL *curl, int argc, char **argv, int start) {
  Buffer b = {0};
  for (int i = start; i + 1 < argc; i += 2) {
    if (strncmp(argv[i], "--", 2) != 0) continue;
    if (strcmp(argv[i], "--all") == 0 || strcmp(argv[i], "--short") == 0) continue;
    char *k = urlenc(curl, argv[i] + 2);
    char *v = urlenc(curl, argv[i + 1]);
    size_t need = b.len + strlen(k) + strlen(v) + 3;
    b.data = xrealloc(b.data, need);
    b.len += (size_t)snprintf(b.data + b.len, need - b.len, "%c%s=%s", b.len ? '&' : '?', k, v);
    free(k);
    free(v);
  }
  if (!b.data) return xstrdup("");
  return b.data;
}

char *json_escape(const char *s) {
  Buffer b = {0};
  b.data = xstrdup("\"");
  b.len = 1;
  for (const unsigned char *p = (const unsigned char *)s; *p; p++) {
    const char *rep = NULL;
    char tmp[8];
    if (*p == '"') rep = "\\\"";
    else if (*p == '\\') rep = "\\\\";
    else if (*p == '\n') rep = "\\n";
    else if (*p == '\r') rep = "\\r";
    else if (*p == '\t') rep = "\\t";
    else if (*p < 32) {
      snprintf(tmp, sizeof(tmp), "\\u%04x", *p);
      rep = tmp;
    }
    if (rep) {
      size_t n = strlen(rep);
      b.data = xrealloc(b.data, b.len + n + 2);
      memcpy(b.data + b.len, rep, n);
      b.len += n;
    } else {
      b.data = xrealloc(b.data, b.len + 3);
      b.data[b.len++] = (char)*p;
    }
  }
  b.data[b.len++] = '"';
  b.data[b.len] = 0;
  return b.data;
}

char *json_object_from_pairs(int argc, char **argv, int start) {
  Buffer b = {0};
  b.data = xstrdup("{");
  b.len = 1;
  bool first = true;
  for (int i = start; i + 1 < argc; i += 2) {
    if (strncmp(argv[i], "--", 2) != 0) continue;
    char *k = json_escape(argv[i] + 2);
    char *v = json_escape(argv[i + 1]);
    size_t n = strlen(k) + strlen(v) + 4;
    b.data = xrealloc(b.data, b.len + n + 1);
    b.len += (size_t)snprintf(b.data + b.len, n + 1, "%s%s:%s", first ? "" : ",", k, v);
    first = false;
    free(k);
    free(v);
  }
  b.data = xrealloc(b.data, b.len + 2);
  b.data[b.len++] = '}';
  b.data[b.len] = 0;
  return b.data;
}

bool is_number_literal(const char *s) {
  if (!s || !*s) return false;
  char *end = NULL;
  errno = 0;
  strtod(s, &end);
  return errno == 0 && end && *end == 0;
}

char *json_value_auto(const char *s) {
  if (!s) return xstrdup("true");
  if (strcmp(s, "true") == 0 || strcmp(s, "false") == 0 || strcmp(s, "null") == 0 || is_number_literal(s)) {
    return xstrdup(s);
  }
  if ((*s == '{' || *s == '[') && strlen(s) > 1) return xstrdup(s);
  return json_escape(s);
}

char *json_object_from_options(int argc, char **argv, int start) {
  Buffer b = {0};
  b.data = xstrdup("{");
  b.len = 1;
  bool first = true;
  for (int i = start; i < argc; i++) {
    if (strncmp(argv[i], "--", 2) != 0) continue;
    const char *key = argv[i] + 2;
    const char *val = NULL;
    if (i + 1 < argc && strncmp(argv[i + 1], "--", 2) != 0) val = argv[++i];
    if (strcmp(key, "all") == 0 || strcmp(key, "short") == 0) continue;
    if (strcmp(key, "file") == 0) continue;
    char *k = json_escape(key);
    char *v = json_value_auto(val);
    size_t n = strlen(k) + strlen(v) + 4;
    b.data = xrealloc(b.data, b.len + n + 1);
    b.len += (size_t)snprintf(b.data + b.len, n + 1, "%s%s:%s", first ? "" : ",", k, v);
    first = false;
    free(k);
    free(v);
  }
  b.data = xrealloc(b.data, b.len + 2);
  b.data[b.len++] = '}';
  b.data[b.len] = 0;
  return b.data;
}

char *query_from_options(CURL *curl, int argc, char **argv, int start) {
  Buffer b = {0};
  for (int i = start; i < argc; i++) {
    if (strncmp(argv[i], "--", 2) != 0) continue;
    const char *key = argv[i] + 2;
    const char *val = "true";
    if (i + 1 < argc && strncmp(argv[i + 1], "--", 2) != 0) val = argv[++i];
    if (strcmp(key, "all") == 0 || strcmp(key, "short") == 0) continue;
    if (strcmp(key, "file") == 0) continue;
    char *k = urlenc(curl, key);
    char *v = urlenc(curl, val);
    size_t need = b.len + strlen(k) + strlen(v) + 3;
    b.data = xrealloc(b.data, need);
    b.len += (size_t)snprintf(b.data + b.len, need - b.len, "%c%s=%s", b.len ? '&' : '?', k, v);
    free(k);
    free(v);
  }
  return b.data ? b.data : xstrdup("");
}

const char *opt_value(int argc, char **argv, const char *name, const char *def) {
  for (int i = 0; i + 1 < argc; i++) {
    if (strcmp(argv[i], name) == 0) return argv[i + 1];
  }
  return def;
}
