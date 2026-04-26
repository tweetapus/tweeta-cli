#include "tweeta.h"
#include "cJSON.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool g_json_output_all = false;

void set_json_output_all(bool all) {
  g_json_output_all = all;
}

static cJSON *parse_json_body(const char *body) {
  const char *end = NULL;
  cJSON *root = cJSON_ParseWithOpts(body, &end, 1);
  if (!root && getenv("TWEETA_JSON_DEBUG")) {
    const char *err = cJSON_GetErrorPtr();
    fprintf(stderr, "tweeta: json parse failed near '%.32s'\n", err ? err : body);
  }
  return root;
}

static bool is_scalar(const cJSON *v) {
  return cJSON_IsNull(v) || cJSON_IsBool(v) || cJSON_IsNumber(v) || cJSON_IsString(v);
}

static void print_indent(int indent) {
  for (int i = 0; i < indent; i++) fputc(' ', stdout);
}

static void print_string_value(const char *s) {
  bool simple = s && *s;
  for (const unsigned char *p = (const unsigned char *)(s ? s : ""); *p; p++) {
    if (*p == '\n' || *p == '\r' || *p == '\t' || *p < 32) simple = false;
  }
  if (simple) {
    fputs(s, stdout);
    return;
  }
  fputc('"', stdout);
  for (const unsigned char *p = (const unsigned char *)(s ? s : ""); *p; p++) {
    if (*p == '\n') fputs("\\n", stdout);
    else if (*p == '\r') fputs("\\r", stdout);
    else if (*p == '\t') fputs("\\t", stdout);
    else if (*p == '"') fputs("\\\"", stdout);
    else if (*p == '\\') fputs("\\\\", stdout);
    else fputc(*p, stdout);
  }
  fputc('"', stdout);
}

static void print_scalar(const cJSON *v) {
  if (cJSON_IsNull(v)) {
    fputs("null", stdout);
  } else if (cJSON_IsBool(v)) {
    fputs(cJSON_IsTrue(v) ? "true" : "false", stdout);
  } else if (cJSON_IsString(v)) {
    print_string_value(cJSON_GetStringValue(v));
  } else if (cJSON_IsNumber(v)) {
    char *s = cJSON_PrintUnformatted(v);
    if (!s) die("out of memory");
    fputs(s, stdout);
    cJSON_free(s);
  }
}

static size_t array_len(const cJSON *arr) {
  size_t n = 0;
  const cJSON *item = NULL;
  cJSON_ArrayForEach(item, arr) n++;
  return n;
}

static size_t object_len(const cJSON *obj) {
  size_t n = 0;
  const cJSON *item = NULL;
  cJSON_ArrayForEach(item, obj) n++;
  return n;
}

static void print_value(const cJSON *v, int indent);

static void print_object(const cJSON *v, int indent) {
  if (!v->child) {
    print_indent(indent);
    fputs("{}\n", stdout);
    return;
  }
  const cJSON *pair = NULL;
  cJSON_ArrayForEach(pair, v) {
    print_indent(indent);
    printf("%s:", pair->string ? pair->string : "");
    if (is_scalar(pair)) {
      fputc(' ', stdout);
      print_scalar(pair);
      fputc('\n', stdout);
    } else {
      fputc('\n', stdout);
      print_value(pair, indent + 2);
    }
  }
}

static void print_array(const cJSON *v, int indent) {
  if (!v->child) {
    print_indent(indent);
    fputs("[]\n", stdout);
    return;
  }
  const cJSON *item = NULL;
  cJSON_ArrayForEach(item, v) {
    print_indent(indent);
    if (is_scalar(item)) {
      fputs("- ", stdout);
      print_scalar(item);
      fputc('\n', stdout);
    } else if (cJSON_IsObject(item) && !item->child) {
      fputs("- {}\n", stdout);
    } else if (cJSON_IsArray(item) && !item->child) {
      fputs("- []\n", stdout);
    } else {
      fputs("-\n", stdout);
      print_value(item, indent + 2);
    }
  }
}

static void print_value(const cJSON *v, int indent) {
  if (is_scalar(v)) {
    print_indent(indent);
    print_scalar(v);
    fputc('\n', stdout);
  } else if (cJSON_IsArray(v)) {
    print_array(v, indent);
  } else if (cJSON_IsObject(v)) {
    print_object(v, indent);
  }
}

bool print_json_readable(const char *body) {
  cJSON *root = parse_json_body(body);
  if (!root) return false;
  print_value(root, 0);
  cJSON_Delete(root);
  return true;
}

static const cJSON *object_get(const cJSON *obj, const char *key) {
  if (!cJSON_IsObject(obj)) return NULL;
  return cJSON_GetObjectItemCaseSensitive(obj, key);
}

static const char *json_text(const cJSON *v) {
  enum { SLOTS = 16, SLOT_LEN = 64 };
  static char slots[SLOTS][SLOT_LEN];
  static int slot = 0;

  if (!v || cJSON_IsNull(v)) return NULL;
  if (cJSON_IsString(v)) return cJSON_GetStringValue(v);
  if (cJSON_IsBool(v)) return cJSON_IsTrue(v) ? "true" : "false";
  if (!cJSON_IsNumber(v)) return NULL;

  char *s = cJSON_PrintUnformatted(v);
  if (!s) die("out of memory");
  slot = (slot + 1) % SLOTS;
  snprintf(slots[slot], SLOT_LEN, "%s", s);
  cJSON_free(s);
  return slots[slot];
}

static bool json_truthy(const cJSON *v) {
  const char *s = json_text(v);
  if (!s) return false;
  if (cJSON_IsBool(v)) return cJSON_IsTrue(v);
  if (cJSON_IsNumber(v)) return strcmp(s, "0") != 0;
  if (cJSON_IsString(v)) return s[0] && strcmp(s, "0") != 0 && strcmp(s, "false") != 0;
  return false;
}

static bool text_present(const char *s) {
  return s && *s;
}

static void print_field_if_present(const char *label, const cJSON *v) {
  const char *s = json_text(v);
  if (!text_present(s)) return;
  printf("%s: ", label);
  print_string_value(s);
  fputc('\n', stdout);
}

static void print_flag_list(const cJSON *profile) {
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

static void print_count_triplet(const cJSON *profile) {
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

static void print_profile_ref(const char *label, const cJSON *v) {
  if (!cJSON_IsObject(v)) return;
  const char *username = json_text(object_get(v, "username"));
  const char *name = json_text(object_get(v, "name"));
  if (!text_present(username) && !text_present(name)) return;
  printf("%s: ", label);
  if (text_present(name)) print_string_value(name);
  if (text_present(username)) printf("%s@%s", text_present(name) ? " " : "", username);
  fputc('\n', stdout);
}

static void print_community_tag(const cJSON *profile) {
  const cJSON *tag = object_get(profile, "community_tag");
  if (!cJSON_IsObject(tag)) return;
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

static void print_relationships(const cJSON *root) {
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

static void print_recent_posts(const cJSON *root, size_t max) {
  const cJSON *posts = object_get(root, "posts");
  if (!cJSON_IsArray(posts) || !posts->child) return;
  size_t total = array_len(posts);
  size_t limit = total < max ? total : max;
  printf("\nrecent posts (%zu of %zu):\n", limit, total);
  const cJSON *post = NULL;
  size_t i = 0;
  cJSON_ArrayForEach(post, posts) {
    if (i++ >= limit) break;
    if (!cJSON_IsObject(post)) continue;
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

static bool print_profile_summary_value(const cJSON *root) {
  const cJSON *profile = object_get(root, "profile");
  if (!cJSON_IsObject(profile)) return false;

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
  cJSON *root = parse_json_body(body);
  if (!root) return false;
  bool ok = print_profile_summary_value(root);
  cJSON_Delete(root);
  return ok;
}

static const cJSON *first_present(const cJSON *obj, const char **keys, size_t count) {
  for (size_t i = 0; i < count; i++) {
    const cJSON *v = object_get(obj, keys[i]);
    if (v) return v;
  }
  return NULL;
}

static const char *first_text(const cJSON *obj, const char **keys, size_t count) {
  return json_text(first_present(obj, keys, count));
}

static bool looks_like_tweet(const cJSON *obj) {
  return cJSON_IsObject(obj) && object_get(obj, "content") &&
         (object_get(obj, "id") || object_get(obj, "created_at") || object_get(obj, "author"));
}

static bool looks_like_user(const cJSON *obj) {
  return cJSON_IsObject(obj) && object_get(obj, "username") &&
         (object_get(obj, "name") || object_get(obj, "bio") || object_get(obj, "follower_count") || object_get(obj, "id"));
}

static void print_author_inline(const cJSON *tweet) {
  const cJSON *author = object_get(tweet, "author");
  const char *name = NULL;
  const char *username = NULL;
  if (cJSON_IsObject(author)) {
    name = json_text(object_get(author, "name"));
    username = json_text(object_get(author, "username"));
  }
  if (!username) username = json_text(object_get(tweet, "username"));
  if (!name) name = json_text(object_get(tweet, "name"));
  if (text_present(name)) print_string_value(name);
  if (text_present(username)) printf("%s@%s", text_present(name) ? " " : "", username);
}

static void print_tweet_summary(const cJSON *tweet, int indent) {
  const char *id_keys[] = {"id", "post_id"};
  const char *date_keys[] = {"created_at", "sort_date", "retweet_created_at"};
  const char *id = first_text(tweet, id_keys, sizeof(id_keys) / sizeof(id_keys[0]));
  const char *date = first_text(tweet, date_keys, sizeof(date_keys) / sizeof(date_keys[0]));
  const char *content = json_text(object_get(tweet, "content"));
  const char *likes = json_text(object_get(tweet, "like_count"));
  const char *replies = json_text(object_get(tweet, "reply_count"));
  const char *retweets = json_text(object_get(tweet, "retweet_count"));
  const cJSON *attachments = object_get(tweet, "attachments");

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
  if (cJSON_IsArray(attachments) && attachments->child) {
    size_t total = array_len(attachments);
    printf(", %zu attachment%s", total, total == 1 ? "" : "s");
  }
  fputc('\n', stdout);
}

static void print_user_summary(const cJSON *user, int indent) {
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
    if (key && strcmp(key, skip[i]) == 0) return false;
  }
  return true;
}

static void print_top_scalars(const cJSON *obj) {
  if (!cJSON_IsObject(obj)) return;
  size_t printed = 0;
  const cJSON *pair = NULL;
  cJSON_ArrayForEach(pair, obj) {
    if (printed >= 12) break;
    if (!should_print_top_scalar(pair->string)) continue;
    if (is_scalar(pair)) {
      printf("%s: ", pair->string ? pair->string : "");
      print_scalar(pair);
      fputc('\n', stdout);
      printed++;
    }
  }
}

static void print_generic_object_summary(const cJSON *obj, int indent) {
  if (looks_like_tweet(obj)) {
    print_tweet_summary(obj, indent);
  } else if (looks_like_user(obj)) {
    print_user_summary(obj, indent);
  } else {
    print_indent(indent);
    fputs("{", stdout);
    size_t printed = 0;
    const cJSON *pair = NULL;
    cJSON_ArrayForEach(pair, obj) {
      if (printed >= 6) break;
      if (!is_scalar(pair)) continue;
      if (printed) fputs(", ", stdout);
      printf("%s: ", pair->string ? pair->string : "");
      print_scalar(pair);
      printed++;
    }
    if (object_len(obj) > printed) fputs(printed ? ", ..." : "...", stdout);
    fputs("}\n", stdout);
  }
}

static bool print_array_summary(const char *label, const cJSON *arr, size_t max) {
  if (!cJSON_IsArray(arr)) return false;
  size_t total = array_len(arr);
  printf("%s (%zu%s):\n", label, total < max ? total : max, total > max ? " shown" : "");
  size_t i = 0;
  const cJSON *item = NULL;
  cJSON_ArrayForEach(item, arr) {
    if (i++ >= max) break;
    if (cJSON_IsObject(item)) {
      fputc('-', stdout);
      fputc('\n', stdout);
      if (looks_like_tweet(item)) print_tweet_summary(item, 2);
      else if (looks_like_user(item)) print_user_summary(item, 2);
      else print_generic_object_summary(item, 2);
    } else {
      fputs("- ", stdout);
      print_scalar(item);
      fputc('\n', stdout);
    }
  }
  if (total > max) printf("... %zu more\n", total - max);
  return true;
}

static const char *summary_array_key(const cJSON *root, const cJSON **out) {
  const char *keys[] = {"posts", "tweets", "timeline", "users", "notifications", "items", "results", "data", "replies"};
  if (!cJSON_IsObject(root)) return NULL;
  for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); i++) {
    const cJSON *v = object_get(root, keys[i]);
    if (cJSON_IsArray(v)) {
      *out = v;
      return keys[i];
    }
  }
  return NULL;
}

static bool print_json_summary_value(const cJSON *root) {
  if (cJSON_IsObject(root)) {
    if (print_profile_summary_value(root)) return true;
    if (looks_like_tweet(root)) {
      print_tweet_summary(root, 0);
      return true;
    }
    const cJSON *arr = NULL;
    const char *key = summary_array_key(root, &arr);
    if (key) {
      print_top_scalars(root);
      if (arr->child) {
        if (object_len(root) > 1) fputc('\n', stdout);
        print_array_summary(key, arr, 10);
      } else {
        printf("%s: []\n", key);
      }
      return true;
    }
  } else if (cJSON_IsArray(root)) {
    return print_array_summary("items", root, 10);
  }
  return false;
}

bool print_json_smart(const char *body) {
  cJSON *root = parse_json_body(body);
  if (!root) return false;
  if (g_json_output_all || !print_json_summary_value(root)) print_value(root, 0);
  cJSON_Delete(root);
  return true;
}
