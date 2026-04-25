#include "tweeta.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

const char *config_path(void) {
  static char path[PATH_MAX];
  const char *env = getenv("TWEETA_CONFIG");
  if (env && *env) return env;
  const char *xdg = getenv("XDG_CONFIG_HOME");
  if (xdg && *xdg) {
    snprintf(path, sizeof(path), "%s/tweeta-cli/config", xdg);
    return path;
  }
  const char *home = getenv("HOME");
  if (!home || !*home) home = ".";
  snprintf(path, sizeof(path), "%s/.config/tweeta-cli/config", home);
  return path;
}

static void mkdir_parents_for(const char *file) {
  char tmp[PATH_MAX];
  snprintf(tmp, sizeof(tmp), "%s", file);
  for (char *p = tmp + 1; *p; p++) {
    if (*p == '/') {
      *p = 0;
      mkdir(tmp, 0700);
      *p = '/';
    }
  }
}

void load_config(Config *cfg) {
  snprintf(cfg->base_url, sizeof(cfg->base_url), "%s", DEFAULT_BASE_URL);
  cfg->token[0] = 0;
  const char *base_env = getenv("TWEETA_BASE_URL");
  const char *tok_env = getenv("TWEETA_TOKEN");
  FILE *f = fopen(config_path(), "r");
  if (f) {
    char line[8192];
    while (fgets(line, sizeof(line), f)) {
      trim_newline(line);
      char *eq = strchr(line, '=');
      if (!eq) continue;
      *eq++ = 0;
      if (strcmp(line, "base_url") == 0) snprintf(cfg->base_url, sizeof(cfg->base_url), "%s", eq);
      if (strcmp(line, "token") == 0) snprintf(cfg->token, sizeof(cfg->token), "%s", eq);
    }
    fclose(f);
  }
  if (base_env && *base_env) snprintf(cfg->base_url, sizeof(cfg->base_url), "%s", base_env);
  if (tok_env && *tok_env) snprintf(cfg->token, sizeof(cfg->token), "%s", tok_env);
  size_t n = strlen(cfg->base_url);
  while (n > 1 && cfg->base_url[n - 1] == '/') cfg->base_url[--n] = 0;
}

void save_config(const Config *cfg) {
  const char *path = config_path();
  mkdir_parents_for(path);
  FILE *f = fopen(path, "w");
  if (!f) {
    perror(path);
    exit(1);
  }
  fprintf(f, "base_url=%s\n", cfg->base_url);
  fprintf(f, "token=%s\n", cfg->token);
  fclose(f);
  chmod(path, 0600);
}
