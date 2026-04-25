#include "tweeta.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
  curl_global_init(CURL_GLOBAL_DEFAULT);
  Config cfg;
  load_config(&cfg);
  if (argc < 2 || strcmp(argv[1], "help") == 0 || strcmp(argv[1], "--help") == 0) {
    usage();
    return 0;
  }
  int rc = 0;
  if (strcmp(argv[1], "config") == 0) {
    if (argc >= 4 && strcmp(argv[2], "set-base") == 0) snprintf(cfg.base_url, sizeof(cfg.base_url), "%s", argv[3]), save_config(&cfg);
    else if (argc >= 4 && strcmp(argv[2], "set-token") == 0) snprintf(cfg.token, sizeof(cfg.token), "%s", argv[3]), save_config(&cfg);
    else if (argc >= 3 && strcmp(argv[2], "show") == 0) printf("base_url=%s\ntoken=%s\nconfig=%s\n", cfg.base_url, cfg.token[0] ? "(set)" : "", config_path());
    else usage(), rc = 2;
  } else {
    int upload_attachment = cmd_upload_get_attachment(&cfg, argc, argv);
    if (upload_attachment != 2) rc = upload_attachment;
    else {
    int named = cmd_named_route(&cfg, argc, argv);
    if (named != 2) rc = named;
    else if (strcmp(argv[1], "request") == 0) rc = cmd_request(&cfg, argc - 1, argv + 1);
  else if (strcmp(argv[1], "get") == 0 && argc >= 3) rc = http_request(&cfg, "GET", argv[2], NULL, NULL, NULL, NULL);
  else if (strcmp(argv[1], "delete") == 0 && argc >= 3) rc = http_request(&cfg, "DELETE", argv[2], NULL, NULL, NULL, NULL);
  else if ((strcmp(argv[1], "post") == 0 || strcmp(argv[1], "patch") == 0 || strcmp(argv[1], "put") == 0) && argc >= 3) {
    const char *json = opt_value(argc, argv, "--json", "{}");
    char method[8];
    snprintf(method, sizeof(method), "%s", argv[1]);
    for (char *p = method; *p; p++) if (*p >= 'a' && *p <= 'z') *p = (char)(*p - 32);
    rc = http_request(&cfg, method, argv[2], json, "application/json", NULL, NULL);
  } else if (strcmp(argv[1], "auth") == 0) rc = cmd_auth(&cfg, argc - 1, argv + 1);
  else if (strcmp(argv[1], "me") == 0) rc = http_request(&cfg, "GET", "/api/auth/me", NULL, NULL, NULL, NULL);
  else if (strcmp(argv[1], "timeline") == 0) {
    CURL *curl = curl_easy_init();
    char *q = join_query(curl, argc, argv, 2);
    char path[2048];
    snprintf(path, sizeof(path), "/api/timeline/%s", q);
    rc = http_request(&cfg, "GET", path, NULL, NULL, NULL, NULL);
    free(q);
    curl_easy_cleanup(curl);
  } else if (strcmp(argv[1], "tweet") == 0) rc = cmd_tweet(&cfg, argc - 1, argv + 1);
  else if (strcmp(argv[1], "profile") == 0 && argc >= 4 && strcmp(argv[2], "get") == 0) {
    char path[1024];
    snprintf(path, sizeof(path), "/api/profile/%s", argv[3]);
    rc = http_request(&cfg, "GET", path, NULL, NULL, NULL, NULL);
  } else if (strcmp(argv[1], "profile") == 0 && argc >= 4 && strcmp(argv[2], "follow") == 0) {
    char path[1024];
    snprintf(path, sizeof(path), "/api/profile/%s/follow", argv[3]);
    rc = http_request(&cfg, "POST", path, "{}", "application/json", NULL, NULL);
  } else if (strcmp(argv[1], "profile") == 0 && argc >= 4 && strcmp(argv[2], "unfollow") == 0) {
    char path[1024];
    snprintf(path, sizeof(path), "/api/profile/%s/follow", argv[3]);
    rc = http_request(&cfg, "DELETE", path, NULL, NULL, NULL, NULL);
  } else if (strcmp(argv[1], "profile") == 0 && argc >= 4 && strcmp(argv[2], "followers") == 0) {
    char path[1024];
    snprintf(path, sizeof(path), "/api/profile/%s/followers", argv[3]);
    rc = http_request(&cfg, "GET", path, NULL, NULL, NULL, NULL);
  } else if (strcmp(argv[1], "profile") == 0 && argc >= 4 && strcmp(argv[2], "following") == 0) {
    char path[1024];
    snprintf(path, sizeof(path), "/api/profile/%s/following", argv[3]);
    rc = http_request(&cfg, "GET", path, NULL, NULL, NULL, NULL);
  } else if (strcmp(argv[1], "upload") == 0 && argc >= 4 && strcmp(argv[2], "media") == 0) {
    rc = http_request(&cfg, "POST", "/api/upload", NULL, NULL, "file", argv[3]);
  } else if (strcmp(argv[1], "admin") == 0) rc = cmd_admin(&cfg, argc - 1, argv + 1);
  else if (strcmp(argv[1], "endpoints") == 0) endpoints();
  else usage(), rc = 2;
    }
  }
  curl_global_cleanup();
  return rc;
}
