#ifndef TWEETA_H
#define TWEETA_H

#include <curl/curl.h>
#include <stdbool.h>
#include <stddef.h>

#define VERSION "0.1.0"
#define DEFAULT_BASE_URL "https://tweeta.tiago.zip"
#define UA "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/124.0 Safari/537.36 TweetapusAgentCLI/0.1"

typedef struct {
  char *data;
  size_t len;
} Buffer;

typedef struct {
  char base_url[1024];
  char token[4096];
} Config;

typedef enum {
  PAYLOAD_NONE,
  PAYLOAD_BODY,
  PAYLOAD_QUERY,
  PAYLOAD_FILE
} PayloadKind;

typedef struct {
  const char *group;
  const char *action;
  const char *method;
  const char *path;
  int path_args;
  PayloadKind payload;
  const char *file_field;
} Route;

void die(const char *msg);
char *xstrdup(const char *s);
void *xrealloc(void *p, size_t n);
size_t write_cb(char *ptr, size_t size, size_t nmemb, void *userdata);
void trim_newline(char *s);
char *read_file(const char *path);
char *urlenc(CURL *curl, const char *s);
char *join_query(CURL *curl, int argc, char **argv, int start);
char *json_escape(const char *s);
char *json_object_from_pairs(int argc, char **argv, int start);
char *json_object_from_options(int argc, char **argv, int start);
char *query_from_options(CURL *curl, int argc, char **argv, int start);
bool print_json_readable(const char *body);
const char *opt_value(int argc, char **argv, const char *name, const char *def);

const char *config_path(void);
void load_config(Config *cfg);
void save_config(const Config *cfg);

int http_request(Config *cfg, const char *method, const char *path, const char *body, const char *ctype, const char *file_field, const char *file_path);
int http_capture(Config *cfg, const char *method, const char *path, char **out, size_t *out_len);
int http_stream(Config *cfg, const char *method, const char *path);
int http_download_file(Config *cfg, const char *method, const char *path, const char *output_path, bool also_stdout);
struct curl_slist *build_headers(const Config *cfg, const char *method, const char *path, const char *content_type);

int cmd_named_route(Config *cfg, int argc, char **argv);
void usage(void);
int cmd_request(Config *cfg, int argc, char **argv);
int cmd_auth(Config *cfg, int argc, char **argv);
int cmd_tweet(Config *cfg, int argc, char **argv);
int cmd_admin(Config *cfg, int argc, char **argv);
int cmd_upload_get_attachment(Config *cfg, int argc, char **argv);
void endpoints(void);

#endif
