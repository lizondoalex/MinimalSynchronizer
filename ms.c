#include <stdio.h>
#include <stdlib.h> //getenv()
#include <string.h> //strcmp(), strrchr(), strdup()
#include <sys/stat.h> //mkdir()
#include <errno.h> //check mkdir() errors
#include <json-c/json.h> //json-c library header

void ensure_directory_exists(const char *path) {
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
    printf("mkdir no ha devuelto bien, este es el path %s\n", path_copy);
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

void create_default_config(const char *path) {
  printf("Creating default configuration...\n");

  char *path_copy = strdup(path);

  size_t size = strlen(path_copy);

  if (size > 0 && path_copy[size -1] == '/') {
    path_copy[size -1] = '\0';
  }

  json_object *config_json= json_object_new_object();

  json_object_object_add(config_json, "ip", json_object_new_string("10.0.0.1"));

  ensure_directory_exists(path);

  char config_file_path[1024];
  snprintf(config_file_path, sizeof(config_file_path), "%s/config.jsonc", path_copy);
  printf("path is %s\n", config_file_path);

  FILE *config_fp = fopen(config_file_path, "w");

  char *config_json_string = json_object_to_json_string_ext(config_json, JSON_C_TO_STRING_PRETTY);

  fprintf(config_fp, "%s\n", config_json_string);
  fclose(config_fp);
  json_object_put(config_json);
  printf("Finished creating default config\n");
}

int main(int argc, char **argv){

  char config_path[1024];
  const char *home_dir = getenv("HOME");

  if (home_dir == NULL) {
    fprintf(stderr, "Error getting the HOME environment variable.\n");
    return 1;
  }

  snprintf(config_path, sizeof(config_path), "%s/.config/minisync/config.jsonc", home_dir);

  printf("config path es %s\n", config_path);

  json_object *parsed_json = json_object_from_file(config_path);

  if (parsed_json == NULL) {
    fprintf(stdout , "Error opening or parsing JSON file from %s\n", config_path);
    printf("Would you like to create a fresh default config? [y/n]: ");

    int response = getchar();

    if (response == 'y' || response == 'Y'){
      char config_dir_path[1024];
      snprintf(config_dir_path, sizeof(config_dir_path), "%s/.config/minisync", home_dir);
      create_default_config(config_dir_path);
    } else {
      return 1;
    }
  }
  json_object *new_parsed_json = json_object_from_file(config_path);

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


}
