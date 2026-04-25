#include "tweeta.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

typedef struct {
  uint32_t *limbs;
  size_t len;
  size_t cap;
} Big;

static char *base64_nopad(const unsigned char *data, size_t len) {
  static const char tab[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  size_t out_len = ((len + 2) / 3) * 4;
  char *out = malloc(out_len + 1);
  if (!out) die("out of memory");
  size_t j = 0;
  for (size_t i = 0; i < len; i += 3) {
    uint32_t v = (uint32_t)data[i] << 16;
    if (i + 1 < len) v |= (uint32_t)data[i + 1] << 8;
    if (i + 2 < len) v |= data[i + 2];
    out[j++] = tab[(v >> 18) & 63];
    out[j++] = tab[(v >> 12) & 63];
    out[j++] = (i + 1 < len) ? tab[(v >> 6) & 63] : '=';
    out[j++] = (i + 2 < len) ? tab[v & 63] : '=';
  }
  while (j && out[j - 1] == '=') j--;
  out[j] = 0;
  return out;
}

static void u64_to_base(uint64_t value, unsigned base, char *out, size_t out_len) {
  static const char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
  char tmp[80];
  size_t pos = 0;
  if (base < 2 || base > 36 || out_len == 0) return;
  if (value == 0) tmp[pos++] = '0';
  while (value && pos < sizeof(tmp)) {
    tmp[pos++] = digits[value % base];
    value /= base;
  }
  size_t n = pos < out_len - 1 ? pos : out_len - 1;
  for (size_t i = 0; i < n; i++) out[i] = tmp[pos - 1 - i];
  out[n] = 0;
}

static void big_init(Big *b, uint32_t v) {
  b->cap = 8;
  b->limbs = calloc(b->cap, sizeof(uint32_t));
  if (!b->limbs) die("out of memory");
  b->len = 1;
  b->limbs[0] = v;
}

static void big_reserve(Big *b, size_t n) {
  if (n <= b->cap) return;
  while (b->cap < n) b->cap *= 2;
  b->limbs = xrealloc(b->limbs, b->cap * sizeof(uint32_t));
}

static void big_xor_byte(Big *b, unsigned char c) {
  b->limbs[0] ^= c;
}

static void big_mul_u32(Big *b, uint32_t m) {
  uint64_t carry = 0;
  for (size_t i = 0; i < b->len; i++) {
    uint64_t v = (uint64_t)b->limbs[i] * m + carry;
    b->limbs[i] = (uint32_t)v;
    carry = v >> 32;
  }
  while (carry) {
    big_reserve(b, b->len + 1);
    b->limbs[b->len++] = (uint32_t)carry;
    carry >>= 32;
  }
}

static char *fnv_hash_js(const char *s) {
  Big b;
  big_init(&b, 2166136261u);
  for (const unsigned char *p = (const unsigned char *)s; *p; p++) {
    big_xor_byte(&b, *p);
    big_mul_u32(&b, 16777619u);
  }
  while (b.len > 1 && b.limbs[b.len - 1] == 0) b.len--;
  size_t max = b.len * 8 + 1;
  char *hex = calloc(max, 1);
  if (!hex) die("out of memory");
  size_t pos = 0;
  pos += (size_t)snprintf(hex + pos, max - pos, "%x", b.limbs[b.len - 1]);
  for (size_t i = b.len - 1; i-- > 0;) pos += (size_t)snprintf(hex + pos, max - pos, "%08x", b.limbs[i]);
  char *out = calloc(33, 1);
  if (!out) die("out of memory");
  size_t hlen = strlen(hex);
  if (hlen > 32) {
    memcpy(out, hex, 32);
  } else {
    memset(out, '0', 32 - hlen);
    memcpy(out + (32 - hlen), hex, hlen);
  }
  free(hex);
  free(b.limbs);
  return out;
}

static char *path_only(const char *path) {
  const char *q = strchr(path, '?');
  size_t n = q ? (size_t)(q - path) : strlen(path);
  char *out = malloc(n + 1);
  if (!out) die("out of memory");
  memcpy(out, path, n);
  out[n] = 0;
  return out;
}

static char *make_tweetapus_header(const char *method, const char *path, const char *token) {
  long long ms = (long long)time(NULL) * 1000LL;
  char ts30[64], ua_len[32], binding_src[2048], *binding, *lang, *tz;
  u64_to_base((uint64_t)ms, 30, ts30, sizeof(ts30));
  u64_to_base((uint64_t)strlen(UA), 36, ua_len, sizeof(ua_len));
  snprintf(binding_src, sizeof(binding_src), "%s:%s", method, path);
  binding = base64_nopad((const unsigned char *)binding_src, strlen(binding_src));
  lang = base64_nopad((const unsigned char *)"en-USen", 7);
  tz = base64_nopad((const unsigned char *)"UTC-9", 5);
  char nonce[17];
  snprintf(nonce, sizeof(nonce), "%08lx%08lx", (unsigned long)time(NULL), (unsigned long)getpid());
  char canvas[1201];
  memset(canvas, 'a', sizeof(canvas) - 1);
  canvas[sizeof(canvas) - 1] = 0;
  size_t need = 1800 + strlen(binding) + strlen(token ? token : "");
  char *h = malloc(need);
  if (!h) die("out of memory");
  snprintf(h, need,
           "%s.%lldabcdef.%x.%x.%s.%s.%s.%x.%x.%x.%x.%x.%x.%llx.%llx.%x.%x.%s.%s.%s.%s.%s",
           canvas, ms, 8, 8, ua_len, lang, lang, 1920, 1080, 24, 1280, 720, 1000,
           ms, ms - 1234, 8, 35, tz, "abcdef123456", ts30, nonce, binding);
  free(binding);
  free(lang);
  free(tz);
  return h;
}

struct curl_slist *build_headers(const Config *cfg, const char *method, const char *path, const char *content_type) {
  struct curl_slist *headers = NULL;
  char tmp[10000];
  if (cfg->token[0]) {
    snprintf(tmp, sizeof(tmp), "Authorization: Bearer %s", cfg->token);
    headers = curl_slist_append(headers, tmp);
  }
  if (content_type) {
    snprintf(tmp, sizeof(tmp), "Content-Type: %s", content_type);
    headers = curl_slist_append(headers, tmp);
  }
  char *clean = path_only(path);
  char *tweetapus = make_tweetapus_header(method, clean, cfg->token);
  size_t sig_need = strlen(tweetapus) + strlen(UA) + strlen(method) + strlen(clean) + 1;
  char *sig_src = malloc(sig_need);
  if (!sig_src) die("out of memory");
  snprintf(sig_src, sig_need, "%s%s%s%s", tweetapus, UA, method, clean);
  char *sig = fnv_hash_js(sig_src);
  const char *transit_src = "[1,1,1,1,8,8,124,456,789,123,456,789,123,456,789,123]";
  char *transit = base64_nopad((const unsigned char *)transit_src, strlen(transit_src));
  snprintf(tmp, sizeof(tmp), "User-Agent: %s", UA);
  headers = curl_slist_append(headers, tmp);
  headers = curl_slist_append(headers, "Accept: application/json");
  headers = curl_slist_append(headers, "X-Tweetapus-Client-Id: WebFetcher");
  snprintf(tmp, sizeof(tmp), "X-Tweetapus: %s", tweetapus);
  headers = curl_slist_append(headers, tmp);
  snprintf(tmp, sizeof(tmp), "X-Tweetapus-MsGnarly: %s", sig);
  headers = curl_slist_append(headers, tmp);
  snprintf(tmp, sizeof(tmp), "X-Tweetapus-Transit: %s", transit);
  headers = curl_slist_append(headers, tmp);
  headers = curl_slist_append(headers, "X-Tweetapus-Brands: e30");
  headers = curl_slist_append(headers, "X-Request-Token: 00000000000000000000000000000000");
  snprintf(tmp, sizeof(tmp), "Origin: %s", cfg->base_url);
  headers = curl_slist_append(headers, tmp);
  snprintf(tmp, sizeof(tmp), "Referer: %s/app/", cfg->base_url);
  headers = curl_slist_append(headers, tmp);
  headers = curl_slist_append(headers, "Sec-Fetch-Site: same-origin");
  free(clean);
  free(tweetapus);
  free(sig_src);
  free(sig);
  free(transit);
  return headers;
}

int http_request(Config *cfg, const char *method, const char *path, const char *body, const char *ctype, const char *file_field, const char *file_path) {
  CURL *curl = curl_easy_init();
  if (!curl) die("curl init failed");
  Buffer resp = {0};
  char url[4096];
  if (strncmp(path, "http://", 7) == 0 || strncmp(path, "https://", 8) == 0) {
    snprintf(url, sizeof(url), "%s", path);
  } else {
    snprintf(url, sizeof(url), "%s%s%s", cfg->base_url, path[0] == '/' ? "" : "/", path);
  }
  const char *api_path = strstr(url, "/api/");
  if (!api_path) api_path = path;
  struct curl_slist *headers = build_headers(cfg, method, api_path, file_path ? NULL : ctype);
  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 120L);
  curl_mime *mime = NULL;
  if (file_path) {
    mime = curl_mime_init(curl);
    curl_mimepart *part = curl_mime_addpart(mime);
    curl_mime_name(part, file_field ? file_field : "file");
    curl_mime_filedata(part, file_path);
    curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
  } else if (body) {
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
  }
  CURLcode rc = curl_easy_perform(curl);
  long status = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
  if (rc != CURLE_OK) {
    fprintf(stderr, "tweeta: curl: %s\n", curl_easy_strerror(rc));
    status = 1;
  } else {
    bool formatted = resp.data && print_json_readable(resp.data);
    if (resp.data && !formatted) fputs(resp.data, stdout);
    if (!formatted && (!resp.data || resp.len == 0 || resp.data[resp.len - 1] != '\n')) fputc('\n', stdout);
  }
  if (mime) curl_mime_free(mime);
  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);
  free(resp.data);
  return status >= 200 && status < 300 ? 0 : (int)(status ? status : 1);
}

int http_capture(Config *cfg, const char *method, const char *path, char **out, size_t *out_len) {
  CURL *curl = curl_easy_init();
  if (!curl) die("curl init failed");
  Buffer resp = {0};
  char url[4096];
  if (strncmp(path, "http://", 7) == 0 || strncmp(path, "https://", 8) == 0) {
    snprintf(url, sizeof(url), "%s", path);
  } else {
    snprintf(url, sizeof(url), "%s%s%s", cfg->base_url, path[0] == '/' ? "" : "/", path);
  }
  const char *api_path = strstr(url, "/api/");
  if (!api_path) api_path = path;
  struct curl_slist *headers = build_headers(cfg, method, api_path, NULL);
  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 120L);
  CURLcode rc = curl_easy_perform(curl);
  long status = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);
  if (rc != CURLE_OK) {
    fprintf(stderr, "tweeta: curl: %s\n", curl_easy_strerror(rc));
    free(resp.data);
    return 1;
  }
  *out = resp.data ? resp.data : xstrdup("");
  *out_len = resp.len;
  return status >= 200 && status < 300 ? 0 : (int)(status ? status : 1);
}

static size_t stdout_cb(char *ptr, size_t size, size_t nmemb, void *userdata) {
  (void)userdata;
  return fwrite(ptr, size, nmemb, stdout) * size;
}

int http_stream(Config *cfg, const char *method, const char *path) {
  CURL *curl = curl_easy_init();
  if (!curl) die("curl init failed");
  char url[4096];
  if (strncmp(path, "http://", 7) == 0 || strncmp(path, "https://", 8) == 0) {
    snprintf(url, sizeof(url), "%s", path);
  } else {
    snprintf(url, sizeof(url), "%s%s%s", cfg->base_url, path[0] == '/' ? "" : "/", path);
  }
  const char *api_path = strstr(url, "/api/");
  if (!api_path) api_path = path;
  struct curl_slist *headers = build_headers(cfg, method, api_path, NULL);
  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, stdout_cb);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 120L);
  CURLcode rc = curl_easy_perform(curl);
  long status = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);
  if (rc != CURLE_OK) {
    fprintf(stderr, "tweeta: curl: %s\n", curl_easy_strerror(rc));
    return 1;
  }
  return status >= 200 && status < 300 ? 0 : (int)(status ? status : 1);
}

int http_download_file(Config *cfg, const char *method, const char *path, const char *output_path, bool also_stdout) {
  char *body = NULL;
  size_t len = 0;
  int rc = http_capture(cfg, method, path, &body, &len);
  if (rc != 0) {
    free(body);
    return rc;
  }

  FILE *f = fopen(output_path, "wb");
  if (!f) {
    perror(output_path);
    free(body);
    return 1;
  }
  if (len && fwrite(body, 1, len, f) != len) {
    perror(output_path);
    fclose(f);
    free(body);
    return 1;
  }
  fclose(f);

  if (also_stdout && len) {
    if (fwrite(body, 1, len, stdout) != len) {
      perror("stdout");
      free(body);
      return 1;
    }
  }
  free(body);
  return 0;
}
