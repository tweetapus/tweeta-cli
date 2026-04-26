#include "tweeta.h"
#include "cJSON.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

void usage(void) {
  puts(
      "tweeta " VERSION "\n"
      "Usage:\n"
      "  tweeta config set-base URL | set-token TOKEN | show\n"
      "  tweeta auth login USER PASS | register USER PASS [--challenge-token TOKEN] | me\n"
      "  tweeta GROUP ACTION [ARGS...] [--field VALUE...] [--file PATH]\n"
      "  tweeta upload get --id POST_ID > attachment.bin\n"
      "  tweeta upload get --id POST_ID --all\n"
      "  tweeta upload get --id POST_ID --really-all > first-attachment.bin\n"
      "  tweeta routes\n"
      "  tweeta request METHOD PATH [--json JSON | --data TEXT | --data-file FILE | --upload FIELD FILE]\n"
      "  tweeta get|post|patch|put|delete PATH [--json JSON]\n"
      "  tweeta me\n"
      "  tweeta timeline [--limit N] [--before ID]\n"
      "  tweeta tweet create --content TEXT [--reply-to ID] [--quote-tweet-id ID]\n"
      "  tweeta tweet get ID | like ID | retweet ID | delete ID | reactions ID\n"
      "  tweeta profile get USER | follow USER | unfollow USER | followers USER | following USER\n"
      "  tweeta upload media FILE\n"
      "  tweeta admin stats | users [--search Q] | user ID_OR_NAME\n"
      "  tweeta admin suspend USER_ID --reason TEXT [--action suspend|restrict|shadowban|warn] [--duration MINUTES]\n"
      "  tweeta admin unsuspend USER_ID\n"
      "  tweeta endpoints\n\n"
      "Run `tweeta routes` for the full named command table. Universal request remains available for debugging.");
}

int cmd_request(Config *cfg, int argc, char **argv) {
  if (argc < 3) {
    usage();
    return 2;
  }
  const char *method = argv[1], *path = argv[2], *json = NULL, *data = NULL, *upload_field = NULL, *upload_file = NULL;
  for (int i = 3; i < argc; i++) {
    if (strcmp(argv[i], "--json") == 0 && i + 1 < argc) json = argv[++i];
    else if (strcmp(argv[i], "--data") == 0 && i + 1 < argc) data = argv[++i];
    else if (strcmp(argv[i], "--data-file") == 0 && i + 1 < argc) data = read_file(argv[++i]);
    else if (strcmp(argv[i], "--upload") == 0 && i + 2 < argc) {
      upload_field = argv[++i];
      upload_file = argv[++i];
    } else if (strcmp(argv[i], "--all") == 0 || strcmp(argv[i], "--short") == 0) {
      continue;
    } else {
      fprintf(stderr, "unknown request option: %s\n", argv[i]);
      return 2;
    }
  }
  const char *body = json ? json : data;
  return http_request(cfg, method, path, body, body ? (json ? "application/json" : "text/plain") : NULL, upload_field, upload_file);
}

static const char *find_string_field_recursive(const cJSON *item, const char **keys, size_t key_count) {
  if (!item) return NULL;
  if (cJSON_IsObject(item)) {
    const cJSON *child = NULL;
    cJSON_ArrayForEach(child, item) {
      for (size_t i = 0; i < key_count; i++) {
        if (child->string && strcmp(child->string, keys[i]) == 0 && cJSON_IsString(child)) {
          return cJSON_GetStringValue(child);
        }
      }
      const char *found = find_string_field_recursive(child, keys, key_count);
      if (found) return found;
    }
  } else if (cJSON_IsArray(item)) {
    const cJSON *child = NULL;
    cJSON_ArrayForEach(child, item) {
      const char *found = find_string_field_recursive(child, keys, key_count);
      if (found) return found;
    }
  }
  return NULL;
}

void store_token_from_login(Config *cfg, const char *body) {
  const char *keys[] = {"token", "authToken", "jwt"};
  cJSON *root = cJSON_Parse(body);
  if (!root) return;
  const char *token = find_string_field_recursive(root, keys, sizeof(keys) / sizeof(keys[0]));
  if (token) {
    snprintf(cfg->token, sizeof(cfg->token), "%s", token);
    save_config(cfg);
    fprintf(stderr, "Saved token to %s\n", config_path());
  }
  cJSON_Delete(root);
}

int login_request(Config *cfg, const char *path, const char *json) {
  CURL *curl = curl_easy_init();
  if (!curl) die("curl init failed");
  Buffer resp = {0};
  char url[4096];
  snprintf(url, sizeof(url), "%s%s", cfg->base_url, path);
  struct curl_slist *headers = build_headers(cfg, "POST", path, "application/json");
  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp);
  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json);
  CURLcode rc = curl_easy_perform(curl);
  long status = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
  if (rc != CURLE_OK) fprintf(stderr, "tweeta: curl: %s\n", curl_easy_strerror(rc));
  if (resp.data) {
    bool formatted = print_json_smart(resp.data);
    if (!formatted) fputs(resp.data, stdout);
    if (!formatted && resp.data[resp.len - 1] != '\n') fputc('\n', stdout);
    if (status >= 200 && status < 300) store_token_from_login(cfg, resp.data);
  }
  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);
  free(resp.data);
  return status >= 200 && status < 300 ? 0 : (int)(status ? status : 1);
}

int cmd_auth(Config *cfg, int argc, char **argv) {
  if (argc < 2) return 2;
  if (strcmp(argv[1], "me") == 0) return http_request(cfg, "GET", "/api/auth/me", NULL, NULL, NULL, NULL);
  if (strcmp(argv[1], "login") == 0 && argc >= 4) {
    char *u = json_escape(argv[2]), *p = json_escape(argv[3]);
    size_t n = strlen(u) + strlen(p) + 64;
    char *json = malloc(n);
    snprintf(json, n, "{\"username\":%s,\"password\":%s}", u, p);
    int r = login_request(cfg, "/api/auth/basic-login", json);
    free(u); free(p); free(json);
    return r;
  }
  if (strcmp(argv[1], "register") == 0 && argc >= 4) {
    const char *challenge = opt_value(argc, argv, "--challenge-token", "dev-local-cap-bypass");
    char *u = json_escape(argv[2]), *p = json_escape(argv[3]), *c = json_escape(challenge);
    size_t n = strlen(u) + strlen(p) + strlen(c) + 96;
    char *json = malloc(n);
    snprintf(json, n, "{\"username\":%s,\"password\":%s,\"challengeToken\":%s}", u, p, c);
    int r = login_request(cfg, "/api/auth/register-with-password", json);
    free(u); free(p); free(c);
    free(json);
    return r;
  }
  usage();
  return 2;
}

int cmd_tweet(Config *cfg, int argc, char **argv) {
  if (argc < 2) return 2;
  if (strcmp(argv[1], "create") == 0) {
    char *json = json_object_from_pairs(argc, argv, 2);
    int r = http_request(cfg, "POST", "/api/tweets/", json, "application/json", NULL, NULL);
    free(json);
    return r;
  }
  if (argc < 3) return 2;
  char path[1024];
  int r;
  if (strcmp(argv[1], "get") == 0) snprintf(path, sizeof(path), "/api/tweets/%s", argv[2]), r = http_request(cfg, "GET", path, NULL, NULL, NULL, NULL);
  else if (strcmp(argv[1], "like") == 0) snprintf(path, sizeof(path), "/api/tweets/%s/like", argv[2]), r = http_request(cfg, "POST", path, "{}", "application/json", NULL, NULL);
  else if (strcmp(argv[1], "retweet") == 0) snprintf(path, sizeof(path), "/api/tweets/%s/retweet", argv[2]), r = http_request(cfg, "POST", path, "{}", "application/json", NULL, NULL);
  else if (strcmp(argv[1], "delete") == 0) snprintf(path, sizeof(path), "/api/tweets/%s", argv[2]), r = http_request(cfg, "DELETE", path, NULL, NULL, NULL, NULL);
  else if (strcmp(argv[1], "reactions") == 0) snprintf(path, sizeof(path), "/api/tweets/%s/reactions", argv[2]), r = http_request(cfg, "GET", path, NULL, NULL, NULL, NULL);
  else return 2;
  return r;
}

int cmd_admin(Config *cfg, int argc, char **argv) {
  if (argc < 2) return 2;
  CURL *curl = curl_easy_init();
  if (!curl) die("curl init failed");
  char path[2048];
  int r = 2;
  if (strcmp(argv[1], "stats") == 0) r = http_request(cfg, "GET", "/api/admin/stats", NULL, NULL, NULL, NULL);
  else if (strcmp(argv[1], "users") == 0) {
    char *q = join_query(curl, argc, argv, 2);
    snprintf(path, sizeof(path), "/api/admin/users%s", q);
    r = http_request(cfg, "GET", path, NULL, NULL, NULL, NULL);
    free(q);
  } else if (strcmp(argv[1], "user") == 0 && argc >= 3) {
    snprintf(path, sizeof(path), "/api/admin/users/%s", argv[2]);
    r = http_request(cfg, "GET", path, NULL, NULL, NULL, NULL);
  } else if (strcmp(argv[1], "suspend") == 0 && argc >= 3) {
    const char *reason = opt_value(argc, argv, "--reason", "moderation action");
    const char *action = opt_value(argc, argv, "--action", "suspend");
    const char *duration = opt_value(argc, argv, "--duration", NULL);
    const char *notes = opt_value(argc, argv, "--notes", NULL);
    char *jr = json_escape(reason), *ja = json_escape(action), *jn = notes ? json_escape(notes) : NULL;
    char json[4096];
    snprintf(json, sizeof(json), "{\"reason\":%s,\"action\":%s%s%s%s%s}", jr, ja,
             duration ? ",\"duration\":" : "", duration ? duration : "",
             notes ? ",\"notes\":" : "", notes ? jn : "");
    snprintf(path, sizeof(path), "/api/admin/users/%s/suspend", argv[2]);
    r = http_request(cfg, "POST", path, json, "application/json", NULL, NULL);
    free(jr); free(ja); free(jn);
  } else if (strcmp(argv[1], "unsuspend") == 0 && argc >= 3) {
    snprintf(path, sizeof(path), "/api/admin/users/%s/unsuspend", argv[2]);
    r = http_request(cfg, "POST", path, "{}", "application/json", NULL, NULL);
  }
  curl_easy_cleanup(curl);
  if (r == 2) usage();
  return r;
}

static char *attachment_ref_from_item(const cJSON *item) {
  const char *keys[] = {"file_url", "url", "file_name", "filename"};
  if (cJSON_IsString(item)) return xstrdup(cJSON_GetStringValue(item));
  if (!cJSON_IsObject(item)) return NULL;
  for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); i++) {
    const cJSON *value = cJSON_GetObjectItemCaseSensitive(item, keys[i]);
    if (cJSON_IsString(value) && cJSON_GetStringValue(value)[0]) return xstrdup(cJSON_GetStringValue(value));
  }
  return NULL;
}

static char *attachment_ref_from_post_json(const char *json) {
  cJSON *root = cJSON_Parse(json);
  if (!root) return NULL;
  const cJSON *attachments = cJSON_GetObjectItemCaseSensitive(root, "attachments");
  char *ref = NULL;
  if (cJSON_IsArray(attachments)) {
    const cJSON *item = NULL;
    cJSON_ArrayForEach(item, attachments) {
      ref = attachment_ref_from_item(item);
      if (ref) break;
    }
  }
  cJSON_Delete(root);
  return ref;
}

static bool looks_like_upload_filename(const char *s) {
  size_t n = strlen(s);
  return n > 5 && !strchr(s, '/') && strchr(s, '.');
}

static bool has_flag(int argc, char **argv, const char *flag) {
  for (int i = 0; i < argc; i++) {
    if (strcmp(argv[i], flag) == 0) return true;
  }
  return false;
}

static char *attachment_url_for_ref(const char *ref) {
  if (strncmp(ref, "http://", 7) == 0 || strncmp(ref, "https://", 8) == 0 || strncmp(ref, "/api/", 5) == 0) {
    return xstrdup(ref);
  }
  if (looks_like_upload_filename(ref)) {
    size_t n = strlen(ref) + 14;
    char *path = malloc(n);
    if (!path) die("out of memory");
    snprintf(path, n, "/api/uploads/%s", ref);
    return path;
  }
  return xstrdup(ref);
}

static char *filename_from_ref(const char *ref, int index) {
  const char *start = strrchr(ref, '/');
  start = start ? start + 1 : ref;
  size_t n = strcspn(start, "?#");
  if (n == 0) {
    char fallback[64];
    snprintf(fallback, sizeof(fallback), "attachment-%d.bin", index + 1);
    return xstrdup(fallback);
  }
  char *name = malloc(n + 1);
  if (!name) die("out of memory");
  memcpy(name, start, n);
  name[n] = 0;
  return name;
}

static int ensure_download_dir(void) {
  if (mkdir("/tmp/tweeta-cli", 0700) != 0 && errno != EEXIST) {
    perror("/tmp/tweeta-cli");
    return 1;
  }
  if (mkdir("/tmp/tweeta-cli/downloads", 0700) != 0 && errno != EEXIST) {
    perror("/tmp/tweeta-cli/downloads");
    return 1;
  }
  return 0;
}

static int collect_attachment_refs(const char *json, char ***out_refs) {
  *out_refs = NULL;
  cJSON *root = cJSON_Parse(json);
  if (!root) return 0;
  const cJSON *attachments = cJSON_GetObjectItemCaseSensitive(root, "attachments");
  if (!cJSON_IsArray(attachments)) {
    cJSON_Delete(root);
    return 0;
  }

  char **refs = NULL;
  int count = 0;
  const cJSON *item = NULL;
  cJSON_ArrayForEach(item, attachments) {
    char *ref = attachment_ref_from_item(item);
    if (ref) {
      refs = xrealloc(refs, sizeof(*refs) * (size_t)(count + 1));
      refs[count++] = ref;
    }
  }
  cJSON_Delete(root);
  *out_refs = refs;
  return count;
}

int cmd_upload_get_attachment(Config *cfg, int argc, char **argv) {
  if (argc < 5 || strcmp(argv[1], "upload") != 0 || strcmp(argv[2], "get") != 0) return 2;
  const char *post_id = opt_value(argc, argv, "--id", NULL);
  if (!post_id) return 2;

  char path[1024];
  snprintf(path, sizeof(path), "/api/tweets/%s", post_id);
  char *json = NULL;
  size_t json_len = 0;
  int rc = http_capture(cfg, "GET", path, &json, &json_len);
  (void)json_len;
  if (rc != 0) {
    if (json) fputs(json, stderr);
    free(json);
    return rc;
  }

  bool all = has_flag(argc, argv, "--all");
  bool really_all = has_flag(argc, argv, "--really-all");
  if (really_all) all = true;

  char *ref = NULL;
  char **refs = NULL;
  int ref_count = 0;
  if (all) {
    ref_count = collect_attachment_refs(json, &refs);
  } else {
    ref = attachment_ref_from_post_json(json);
  }
  free(json);

  if (all) {
    if (ref_count <= 0) {
      fprintf(stderr, "tweeta: post has no attachment URLs or filenames\n");
      return 1;
    }
    if (ensure_download_dir() != 0) {
      for (int i = 0; i < ref_count; i++) free(refs[i]);
      free(refs);
      return 1;
    }
    int final_rc = 0;
    for (int i = 0; i < ref_count; i++) {
      char *url = attachment_url_for_ref(refs[i]);
      char *file = filename_from_ref(refs[i], i);
      char out_path[1024];
      snprintf(out_path, sizeof(out_path), "/tmp/tweeta-cli/downloads/%s", file);
      int dl = http_download_file(cfg, "GET", url, out_path, really_all && i == 0);
      if (dl == 0) fprintf(stderr, "%s\n", out_path);
      else final_rc = dl;
      free(url);
      free(file);
      free(refs[i]);
    }
    free(refs);
    return final_rc;
  }

  if (!ref) {
    fprintf(stderr, "tweeta: post has no attachment URL or filename\n");
    return 1;
  }

  char *url = attachment_url_for_ref(ref);
  int out_rc = http_stream(cfg, "GET", url);
  free(url);
  free(ref);
  return out_rc;
}

void endpoints(void) {
  puts(
      "Endpoint coverage: use `tweeta request METHOD /api/...` for any endpoint.\n"
      "Mounted API groups: /api/auth, /api/admin, /api/blocking, /api/bookmarks,\n"
      "/api/communities, /api/delegates, /api/tweets, /api/articles, /api/profile,\n"
      "/api/timeline, /api/public-tweets, /api/search, /api/upload, /api/uploads,\n"
      "/api/notifications, /api/dm, /api/push, /api/tenor, /api/unsplash,\n"
      "/api/scheduled, /api/reports, /api/translate, /api/trends, /api/lists,\n"
      "/api/explore, /api/muted, /api/mpi, /api/mpi/shop, /api/emojis, /api/owoembed.\n"
      "Admin examples: /api/admin/users, /api/admin/users/:id, /api/admin/posts,\n"
      "/api/admin/suspensions, /api/admin/reports, /api/admin/moderation-logs,\n"
      "/api/admin/badges, /api/admin/emojis, /api/admin/fact-checks, /api/admin/ip-bans,\n"
      "/api/admin/shop/products, /api/admin/shop/purchases.");
}
