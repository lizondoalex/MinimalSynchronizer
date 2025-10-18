#include <stdio.h>
#include <stdlib.h> //getenv()
#include <string.h> //strcmp(), strrchr(), strdup()
#include <sys/stat.h> //mkdir()
#include <errno.h> //check mkdir() errors
#include <json-c/json.h> //json-c library header

void ensure_directory_exists(const char *path) {
  char *path_copy = strdup(path);

  for (char *p = path_copy + 1; *p; p++) {
    if (*p == '/') {
      p = '\0';

      if (mkdir(path_copy, 0755) != 0 && errno != EEXIST) {
        fprintf(stderr, "Error creating directory %s\n", path_copy);
        perror("mkdir");
        free(path_copy);
        return;
      }

      *p = '/';
    }
  }
  if (mkdir(path_copy, 0755) != 0 && errno != EEXIST) {
    fprintf(stderr, "Error creating directory %s\n", path_copy);
    perror("mkdir");
  }
  free(path_copy);
}

void create_default_config(const char *path) {
  printf("Creating default configuration...");

  char *path_copy = strdup(path);

  size_t size = strlen(path_copy);

  if (size > 0 && path_copy[size -1] == '/') {
    path_copy[size -1] = '\0';
  }

  json_object *root = json_object_new_object();

  json_object_object_add(root, "ip", json_object_new_string("127.0.0.1"));

  ensure_directory_exists(path);

  const char *config_file_path;
  config_file_path = snprintf(config_file_path, sizeof(config_file_path), "");
}

int main(int argc, char **argv){

  char config_path[1024];
  const char *home_dir = getenv("HOME");

  if (home_dir == NULL) {
    fprintf(stderr, "Error getting the HOME environment variable.\n");
    return 1;
  }

  snprintf(config_path, sizeof(config_path), "%s/.config/minisync/config.jsonc", home_dir);


  struct json_object *parsed_json = json_object_from_file(config_path);

  if (parsed_json == NULL) {
    fprintf(stderr, "Error opening or parsing JSON file from %s\n", config_path);
    return 1;
  }

  struct json_object *ip;

  if (!json_object_object_get_ex(parsed_json, "ip", &ip)) {
    fprintf(stderr, "Error finding key 'ip' in JSON.\n");
    json_object_put(parsed_json);
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


}
