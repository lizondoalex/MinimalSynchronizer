#include <stdio.h>
#include <stdlib.h> //getenv()
#include <string.h> //strcmp(), strrchr(), strdup()
#include <sys/stat.h> //mkdir()
#include <errno.h> //check mkdir() errors
#include <json-c/json.h> //json-c library header
#include <time.h> //for server-client date comparison and check


static void
get_current_date(json_object *result) {
  time_t t = time(NULL);

  struct tm *current_time = localtime(&t);

  json_object_object_add(result, "tm_sec",   json_object_new_int(current_time->tm_sec));
  json_object_object_add(result, "tm_min",   json_object_new_int(current_time->tm_min));
  json_object_object_add(result, "tm_hour",  json_object_new_int(current_time->tm_hour));
  json_object_object_add(result, "tm_mday",  json_object_new_int(current_time->tm_mday));
  json_object_object_add(result, "tm_mon",   json_object_new_int(current_time->tm_mon));
  json_object_object_add(result, "tm_year",  json_object_new_int(current_time->tm_year));
  json_object_object_add(result, "tm_wday",  json_object_new_int(current_time->tm_wday));
  json_object_object_add(result, "tm_yday",  json_object_new_int(current_time->tm_yday));
  json_object_object_add(result, "tm_isdst", json_object_new_int(current_time->tm_isdst));

}

static void
ensure_directory_exists(const char *path) {

  char *path_copy = strdup(path);

  if (path_copy == NULL) {
    fprintf(stderr, "strdup failed\n");
    return;
  }
  for (char *p = path_copy + 1; *p; p++) {
    if (*p == '/') {
      *p = '\0';

      if (mkdir(path_copy, 0755) != 0) {
        if (errno != EEXIST) {
          fprintf(stderr, "Error creating directory %s\n", path_copy);
          perror("mkdir");
          free(path_copy);
          return;
        }
      } else {
        printf("Creating directory %s\n", path);
      }

      *p = '/';
    }
  }
  if (mkdir(path_copy, 0755) != 0) {
    if (errno != EEXIST) {
      fprintf(stderr, "Error creating directory %s\n", path_copy);
      perror("mkdir");
      free(path_copy);
      return;
    }
  } else {
    printf("Creating directory %s\n", path);
  }
  printf("Finished creating directories\n");
}


static void
*get_default_config_path(char *buffer, size_t buffer_size) {
  puts("getting default config path");

  const char *home_dir = getenv("HOME");

  if (home_dir == NULL) {
    fprintf(stderr, "Error getting the HOME environment variable.\n");
    return nullptr;
  }

  snprintf(buffer, buffer_size, "%s/.config/minisync/config.jsonc", home_dir);

}

static FILE
*get_default_config_file() {
  char default_config_path[1024];
  get_default_config_path(default_config_path, sizeof(default_config_path));
  char *default_config_dir = strdup(default_config_path);

  char *last_slash = strrchr(default_config_dir, '/');

  *last_slash = '\0';
  ensure_directory_exists(default_config_dir);

  FILE *config = fopen(default_config_path, "w");
  return config;
}


static void
write_json(json_object *json) {
  FILE *config = get_default_config_file();
  char *config_json_string = json_object_to_json_string_ext(json, JSON_C_TO_STRING_PRETTY);
  fprintf(config, "%s\n", config_json_string);
  fclose(config);
}

static json_object
*get_default_json() {

  json_object *default_json = json_object_new_object();
  get_current_date(default_json);


}

static void
create_default_config() {
  printf("Creating default configuration...\n");

  json_object *config_json= get_default_json();

  write_json(config_json);

  printf("Finished creating default config\n");
}

static json_object*
read_config() {
  char path[1024];

  get_default_config_path(path, sizeof(path));
  json_object *parsed_json= json_object_from_file(path);

  if (parsed_json == NULL) {
    fprintf(stdout , "Error opening or parsing JSON file from %s\n", path);
    printf("Would you like to create a fresh default config? [y/n]: ");

    int response = getchar();

    if (response == 'y' || response == 'Y'){
      create_default_config();
    } else {
      return nullptr;
    }
  } else {
    return parsed_json;
  }

  json_object *new_parsed_json = json_object_from_file(path);

  return new_parsed_json;
}

static void
usage() {
  puts("usage: ms [option]\n");
  exit(1);
}

static struct tm
get_config_date() {
  json_object *config = read_config();

  json_object *j_sec, *j_min, *j_hour, *j_mday, *j_mon, *j_year, *j_wday, *j_yday, *j_isdst;

  json_object_object_get_ex(config, "tm_sec",   &j_sec);
  json_object_object_get_ex(config, "tm_min",   &j_min);
  json_object_object_get_ex(config, "tm_hour",  &j_hour);
  json_object_object_get_ex(config, "tm_mday",  &j_mday);
  json_object_object_get_ex(config, "tm_mon",   &j_mon);
  json_object_object_get_ex(config, "tm_year",  &j_year);
  json_object_object_get_ex(config, "tm_wday",  &j_wday);
  json_object_object_get_ex(config, "tm_yday",  &j_yday);
  json_object_object_get_ex(config, "tm_isdst", &j_isdst);

}

static void
update_date() {

  json_object *time = json_object_new_object();
  get_current_date(time);
  json_object *config = read_config();

  json_object_object_add(config, "time", time);

  write_json(config);
  json_object_put(config);
}

static void
status() {

}

int main(int argc, char **argv){

  update_date();

  json_object *json = read_config();
  char *config_json_string = json_object_to_json_string_ext(json, JSON_C_TO_STRING_PRETTY);
  printf("current config: %s\n", config_json_string);
  json_object *time = json_object_new_object();
  get_current_date(time);
  char *s = json_object_to_json_string_ext(time, JSON_C_TO_STRING_PRETTY);
  printf("current time = %s\n", s);

  return 1;


  if (argc == 1) {
    usage();
  }

  for (int i = 1; i < argc; i++) {

    if (!strcmp(argv[i], "status")) {
      status();
      return 0;
    } if (!strcmp(argv[i], "init")) {
      create_default_config();
      return 0;
    }

  }
  return 1;


/*
  struct json_object *ip;

  if (!json_object_object_get_ex(new_parsed_json, "ip", &ip)) {
    fprintf(stderr, "Error finding key 'ip' in JSON.\n");
    json_object_put(new_parsed_json);
    return 1;
  }

  if (!json_object_is_type(ip , json_type_string)) {
    fprintf(stderr, "Error: Key \"name\" is not a string.\n");
    json_object_put(parsed_json);
    return 1;
  }

  const char *name_ip = json_object_get_string(ip);
  printf("your ip is %s\n", name_ip);

  json_object_put(parsed_json);

  return 0;

  */

}
