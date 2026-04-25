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

static bool g_json_output_all = false;

void set_json_output_all(bool all) {
  g_json_output_all = all;
}

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

static JsonValue *parse_json_body(const char *body) {
  JsonParser parser = {body};
  skip_ws(&parser);
  JsonValue *root = parse_value(&parser);
  if (!root) {
    if (getenv("TWEETA_JSON_DEBUG")) {
      fprintf(stderr, "tweeta: json parse failed at byte %zu near '%.32s'\n", (size_t)(parser.p - body), parser.p);
    }
    return NULL;
  }
  skip_ws(&parser);
  if (*parser.p != 0) {
    if (getenv("TWEETA_JSON_DEBUG")) {
      fprintf(stderr, "tweeta: trailing data after json at byte %zu near '%.32s'\n", (size_t)(parser.p - body), parser.p);
    }
    json_free(root);
    return NULL;
  }
  return root;
}

bool print_json_readable(const char *body) {
  JsonValue *root = parse_json_body(body);
  if (!root) return false;
  print_value(root, 0);
  json_free(root);
  return true;
}

static const JsonValue *object_get(const JsonValue *obj, const char *key) {
  if (!obj || obj->kind != JSON_OBJECT) return NULL;
  for (size_t i = 0; i < obj->as.object.len; i++) {
    if (strcmp(obj->as.object.pairs[i].key, key) == 0) return obj->as.object.pairs[i].value;
  }
  return NULL;
}

static const char *json_text(const JsonValue *v) {
  if (!v || v->kind == JSON_NULL) return NULL;
  if (v->kind == JSON_STRING || v->kind == JSON_NUMBER) return v->as.text;
  if (v->kind == JSON_BOOL) return v->as.boolean ? "true" : "false";
  return NULL;
}

static bool json_truthy(const JsonValue *v) {
  if (!v || v->kind == JSON_NULL) return false;
  if (v->kind == JSON_BOOL) return v->as.boolean;
  if (v->kind == JSON_NUMBER) return strcmp(v->as.text, "0") != 0;
  if (v->kind == JSON_STRING) return v->as.text[0] && strcmp(v->as.text, "0") != 0 && strcmp(v->as.text, "false") != 0;
  return false;
}

static bool text_present(const char *s) {
  return s && *s;
}

static void print_field_if_present(const char *label, const JsonValue *v) {
  const char *s = json_text(v);
  if (!text_present(s)) return;
  printf("%s: ", label);
  print_string_value(s);
  fputc('\n', stdout);
}

static void print_flag_list(const JsonValue *profile) {
  const char *labels[] = {"verified", "gold", "gray", "private", "suspended", "restricted", "shadowbanned", "affiliate"};
  bool first = true;
  for (size_t i = 0; i < sizeof(labels) / sizeof(labels[0]); i++) {
    if (!json_truthy(object_get(profile, labels[i]))) continue;
    if (first) {
      fputs("flags: ", stdout);
      first = false;
    } else {
      fputs(", ", stdout);
    }
    fputs(labels[i], stdout);
  }
  if (!first) fputc('\n', stdout);
}

static void print_count_triplet(const JsonValue *profile) {
  const char *followers = json_text(object_get(profile, "follower_count"));
  const char *following = json_text(object_get(profile, "following_count"));
  const char *posts = json_text(object_get(profile, "post_count"));
  if (followers || following || posts) {
    printf("stats: %s followers, %s following, %s posts\n",
           followers ? followers : "0",
           following ? following : "0",
           posts ? posts : "0");
  }
}

static void print_profile_ref(const char *label, const JsonValue *v) {
  if (!v || v->kind != JSON_OBJECT) return;
  const char *username = json_text(object_get(v, "username"));
  const char *name = json_text(object_get(v, "name"));
  if (!text_present(username) && !text_present(name)) return;
  printf("%s: ", label);
  if (text_present(name)) print_string_value(name);
  if (text_present(username)) printf("%s@%s", text_present(name) ? " " : "", username);
  fputc('\n', stdout);
}

static void print_community_tag(const JsonValue *profile) {
  const JsonValue *tag = object_get(profile, "community_tag");
  if (!tag || tag->kind != JSON_OBJECT) return;
  const char *emoji = json_text(object_get(tag, "emoji"));
  const char *text = json_text(object_get(tag, "text"));
  const char *name = json_text(object_get(tag, "community_name"));
  if (!text_present(emoji) && !text_present(text) && !text_present(name)) return;
  fputs("community tag: ", stdout);
  if (text_present(emoji)) printf("%s ", emoji);
  if (text_present(text)) print_string_value(text);
  if (text_present(name)) {
    fputs(" in ", stdout);
    print_string_value(name);
  }
  fputc('\n', stdout);
}

static void print_relationships(const JsonValue *root) {
  bool any = false;
  if (json_truthy(object_get(root, "isFollowing"))) {
    fputs("relationship: following", stdout);
    any = true;
  }
  if (json_truthy(object_get(root, "followsMe"))) {
    fputs(any ? ", follows you" : "relationship: follows you", stdout);
    any = true;
  }
  if (json_truthy(object_get(root, "isOwnProfile"))) {
    fputs(any ? ", own profile" : "relationship: own profile", stdout);
    any = true;
  }
  const char *request = json_text(object_get(root, "followRequestStatus"));
  if (text_present(request)) {
    printf(any ? ", request: %s" : "relationship: request: %s", request);
    any = true;
  }
  if (any) fputc('\n', stdout);
}

static void print_one_line_text(const char *s, size_t max) {
  if (!s) return;
  size_t n = 0;
  for (const char *p = s; *p && n < max; p++, n++) {
    if (*p == '\n' || *p == '\r' || *p == '\t') fputc(' ', stdout);
    else fputc(*p, stdout);
  }
  if (strlen(s) > max) fputs("...", stdout);
}

static void print_recent_posts(const JsonValue *root, size_t max) {
  const JsonValue *posts = object_get(root, "posts");
  if (!posts || posts->kind != JSON_ARRAY || posts->as.array.len == 0) return;
  size_t limit = posts->as.array.len < max ? posts->as.array.len : max;
  printf("\nrecent posts (%zu of %zu):\n", limit, posts->as.array.len);
  for (size_t i = 0; i < limit; i++) {
    const JsonValue *post = posts->as.array.items[i];
    if (!post || post->kind != JSON_OBJECT) continue;
    const char *id = json_text(object_get(post, "id"));
    const char *created = json_text(object_get(post, "created_at"));
    const char *content = json_text(object_get(post, "content"));
    const char *likes = json_text(object_get(post, "like_count"));
    const char *replies = json_text(object_get(post, "reply_count"));
    const char *retweets = json_text(object_get(post, "retweet_count"));
    printf("- %s", id ? id : "(no id)");
    if (created) printf(" at %s", created);
    fputc('\n', stdout);
    if (content) {
      fputs("  ", stdout);
      print_one_line_text(content, 120);
      fputc('\n', stdout);
    }
    printf("  engagement: %s likes, %s replies, %s retweets\n",
           likes ? likes : "0", replies ? replies : "0", retweets ? retweets : "0");
  }
}

static bool print_profile_summary_value(const JsonValue *root) {
  const JsonValue *profile = object_get(root, "profile");
  if (!profile || profile->kind != JSON_OBJECT) return false;

  const char *username = json_text(object_get(profile, "username"));
  const char *name = json_text(object_get(profile, "name"));
  if (text_present(name)) print_string_value(name);
  if (text_present(username)) printf("%s@%s", text_present(name) ? " " : "", username);
  if (text_present(name) || text_present(username)) fputc('\n', stdout);

  print_field_if_present("id", object_get(profile, "id"));
  print_field_if_present("bio", object_get(profile, "bio"));
  print_field_if_present("pronouns", object_get(profile, "pronouns"));
  print_field_if_present("location", object_get(profile, "location"));
  print_field_if_present("website", object_get(profile, "website"));
  print_field_if_present("joined", object_get(profile, "created_at"));
  print_count_triplet(profile);
  print_flag_list(profile);
  print_profile_ref("affiliated with", object_get(profile, "affiliate_with_profile"));
  print_community_tag(profile);
  print_relationships(root);
  print_recent_posts(root, 5);

  return true;
}

bool print_profile_summary(const char *body) {
  JsonValue *root = parse_json_body(body);
  if (!root) return false;
  bool ok = print_profile_summary_value(root);
  json_free(root);
  return ok;
}

static const JsonValue *first_present(const JsonValue *obj, const char **keys, size_t count) {
  for (size_t i = 0; i < count; i++) {
    const JsonValue *v = object_get(obj, keys[i]);
    if (v) return v;
  }
  return NULL;
}

static const char *first_text(const JsonValue *obj, const char **keys, size_t count) {
  return json_text(first_present(obj, keys, count));
}

static bool looks_like_tweet(const JsonValue *obj) {
  return obj && obj->kind == JSON_OBJECT && object_get(obj, "content") &&
         (object_get(obj, "id") || object_get(obj, "created_at") || object_get(obj, "author"));
}

static bool looks_like_user(const JsonValue *obj) {
  return obj && obj->kind == JSON_OBJECT && object_get(obj, "username") &&
         (object_get(obj, "name") || object_get(obj, "bio") || object_get(obj, "follower_count") || object_get(obj, "id"));
}

static void print_author_inline(const JsonValue *tweet) {
  const JsonValue *author = object_get(tweet, "author");
  const char *name = NULL;
  const char *username = NULL;
  if (author && author->kind == JSON_OBJECT) {
    name = json_text(object_get(author, "name"));
    username = json_text(object_get(author, "username"));
  }
  if (!username) username = json_text(object_get(tweet, "username"));
  if (!name) name = json_text(object_get(tweet, "name"));
  if (text_present(name)) print_string_value(name);
  if (text_present(username)) printf("%s@%s", text_present(name) ? " " : "", username);
}

static void print_tweet_summary(const JsonValue *tweet, int indent) {
  const char *id_keys[] = {"id", "post_id"};
  const char *date_keys[] = {"created_at", "sort_date", "retweet_created_at"};
  const char *id = first_text(tweet, id_keys, sizeof(id_keys) / sizeof(id_keys[0]));
  const char *date = first_text(tweet, date_keys, sizeof(date_keys) / sizeof(date_keys[0]));
  const char *content = json_text(object_get(tweet, "content"));
  const char *likes = json_text(object_get(tweet, "like_count"));
  const char *replies = json_text(object_get(tweet, "reply_count"));
  const char *retweets = json_text(object_get(tweet, "retweet_count"));
  const JsonValue *attachments = object_get(tweet, "attachments");

  print_indent(indent);
  if (id) printf("%s", id);
  else fputs("(no id)", stdout);
  if (date) printf(" at %s", date);
  if (object_get(tweet, "content_type")) printf(" [%s]", json_text(object_get(tweet, "content_type")));
  fputc('\n', stdout);

  print_indent(indent + 2);
  print_author_inline(tweet);
  fputc('\n', stdout);

  if (content) {
    print_indent(indent + 2);
    print_one_line_text(content, 160);
    fputc('\n', stdout);
  }
  print_indent(indent + 2);
  printf("engagement: %s likes, %s replies, %s retweets",
         likes ? likes : "0", replies ? replies : "0", retweets ? retweets : "0");
  if (attachments && attachments->kind == JSON_ARRAY && attachments->as.array.len) {
    printf(", %zu attachment%s", attachments->as.array.len, attachments->as.array.len == 1 ? "" : "s");
  }
  fputc('\n', stdout);
}

static void print_user_summary(const JsonValue *user, int indent) {
  const char *username = json_text(object_get(user, "username"));
  const char *name = json_text(object_get(user, "name"));
  const char *bio = json_text(object_get(user, "bio"));
  print_indent(indent);
  if (text_present(name)) print_string_value(name);
  if (text_present(username)) printf("%s@%s", text_present(name) ? " " : "", username);
  if (!text_present(name) && !text_present(username)) fputs("(unknown user)", stdout);
  fputc('\n', stdout);
  if (bio) {
    print_indent(indent + 2);
    print_one_line_text(bio, 120);
    fputc('\n', stdout);
  }
  const char *followers = json_text(object_get(user, "follower_count"));
  const char *following = json_text(object_get(user, "following_count"));
  const char *posts = json_text(object_get(user, "post_count"));
  if (followers || following || posts) {
    print_indent(indent + 2);
    printf("%s followers, %s following, %s posts\n",
           followers ? followers : "0", following ? following : "0", posts ? posts : "0");
  }
}

static bool should_print_top_scalar(const char *key) {
  const char *skip[] = {"profile", "posts", "tweets", "timeline", "users", "notifications", "items", "results", "data"};
  for (size_t i = 0; i < sizeof(skip) / sizeof(skip[0]); i++) {
    if (strcmp(key, skip[i]) == 0) return false;
  }
  return true;
}

static void print_top_scalars(const JsonValue *obj) {
  if (!obj || obj->kind != JSON_OBJECT) return;
  size_t printed = 0;
  for (size_t i = 0; i < obj->as.object.len && printed < 12; i++) {
    const JsonPair *pair = &obj->as.object.pairs[i];
    if (!should_print_top_scalar(pair->key)) continue;
    if (is_scalar(pair->value)) {
      printf("%s: ", pair->key);
      print_scalar(pair->value);
      fputc('\n', stdout);
      printed++;
    }
  }
}

static void print_generic_object_summary(const JsonValue *obj, int indent) {
  if (looks_like_tweet(obj)) {
    print_tweet_summary(obj, indent);
  } else if (looks_like_user(obj)) {
    print_user_summary(obj, indent);
  } else {
    print_indent(indent);
    fputs("{", stdout);
    size_t printed = 0;
    for (size_t i = 0; i < obj->as.object.len && printed < 6; i++) {
      const JsonPair *pair = &obj->as.object.pairs[i];
      if (!is_scalar(pair->value)) continue;
      if (printed) fputs(", ", stdout);
      printf("%s: ", pair->key);
      print_scalar(pair->value);
      printed++;
    }
    if (obj->as.object.len > printed) fputs(printed ? ", ..." : "...", stdout);
    fputs("}\n", stdout);
  }
}

static bool print_array_summary(const char *label, const JsonValue *arr, size_t max) {
  if (!arr || arr->kind != JSON_ARRAY) return false;
  printf("%s (%zu%s):\n", label, arr->as.array.len < max ? arr->as.array.len : max,
         arr->as.array.len > max ? " shown" : "");
  size_t limit = arr->as.array.len < max ? arr->as.array.len : max;
  for (size_t i = 0; i < limit; i++) {
    const JsonValue *item = arr->as.array.items[i];
    if (item->kind == JSON_OBJECT) {
      fputc('-', stdout);
      if (looks_like_tweet(item)) {
        fputc('\n', stdout);
        print_tweet_summary(item, 2);
      } else if (looks_like_user(item)) {
        fputc('\n', stdout);
        print_user_summary(item, 2);
      } else {
        fputc('\n', stdout);
        print_generic_object_summary(item, 2);
      }
    } else {
      fputs("- ", stdout);
      print_scalar(item);
      fputc('\n', stdout);
    }
  }
  if (arr->as.array.len > max) printf("... %zu more\n", arr->as.array.len - max);
  return true;
}

static const char *summary_array_key(const JsonValue *root, const JsonValue **out) {
  const char *keys[] = {"posts", "tweets", "timeline", "users", "notifications", "items", "results", "data", "replies"};
  if (!root || root->kind != JSON_OBJECT) return NULL;
  for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); i++) {
    const JsonValue *v = object_get(root, keys[i]);
    if (v && v->kind == JSON_ARRAY) {
      *out = v;
      return keys[i];
    }
  }
  return NULL;
}

static bool print_json_summary_value(const JsonValue *root) {
  if (root->kind == JSON_OBJECT) {
    if (print_profile_summary_value(root)) return true;
    if (looks_like_tweet(root)) {
      print_tweet_summary(root, 0);
      return true;
    }
    const JsonValue *arr = NULL;
    const char *key = summary_array_key(root, &arr);
    if (key) {
      print_top_scalars(root);
      if (arr->as.array.len) {
        if (root->as.object.len > 1) fputc('\n', stdout);
        print_array_summary(key, arr, 10);
      } else {
        printf("%s: []\n", key);
      }
      return true;
    }
  } else if (root->kind == JSON_ARRAY) {
    return print_array_summary("items", root, 10);
  }
  return false;
}

bool print_json_smart(const char *body) {
  JsonValue *root = parse_json_body(body);
  if (!root) return false;
  if (g_json_output_all || !print_json_summary_value(root)) print_value(root, 0);
  json_free(root);
  return true;
}
