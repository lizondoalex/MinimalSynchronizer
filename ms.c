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

static char*
execute_command(const char* command) {
  const int BUFFER_SIZE = 4096;
  FILE *pipe = NULL;
  char buffer[BUFFER_SIZE];

  char* result = NULL;
  size_t total_size = 0;

  pipe = popen(command, "r");
  if (pipe == NULL) {
    perror("failed to open pipe on execute_command");
    return NULL;
  }

  result = (char*) malloc(BUFFER_SIZE);
  if (result == NULL) {
    perror("failed to malloc in execute_command");
    pclose(pipe);
    return NULL;
  }
  result[0] = '\0';
  total_size = BUFFER_SIZE;

  while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
    size_t new_size = total_size;

    while (strlen(result) + strlen(buffer) + 1 > new_size) {
      new_size *= 2;
    }

    if (new_size > total_size) {
      char* temp_ptr = realloc(result, new_size);
      if (temp_ptr == NULL) {
        perror("realloc failed");
        free(result);
        pclose(pipe);
        return NULL;
      }
      result = temp_ptr;
      total_size = new_size;
    }

    strcat(result, buffer);
  }

  pclose(pipe);
  return result;
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
}


static void
*get_default_config_path(char *buffer, size_t buffer_size) {

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
  const char *config_json_string = json_object_to_json_string_ext(json, JSON_C_TO_STRING_PRETTY);
  fprintf(config, "%s\n", config_json_string);
  fclose(config);
}

static json_object*
get_default_json() {

  json_object *result = json_object_new_object();
  json_object *time = json_object_new_object();
  get_current_date(time);

  json_object_object_add(result, "ip", json_object_new_string("10.0.0.1"));
  json_object_object_add(result, "clientDirectory", json_object_new_string("shared"));
  json_object_object_add(result, "serverDirectory", json_object_new_string("shared"));
  json_object_object_add(result, "serverHostName", json_object_new_string("serverhostname"));
  json_object_object_add(result, "time", time);

  return result;

}

static void
create_default_config() {
  printf("Creating default configuration...\n");

  json_object *config_json = get_default_json();
  const char *s = json_object_to_json_string_ext(config_json, JSON_C_TO_STRING_PRETTY);
  printf("printing default json \n%s\n", s);


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
    const char *s = json_object_to_json_string_ext(parsed_json, JSON_C_TO_STRING_PRETTY);
    return parsed_json;
  }

  json_object *new_parsed_json = json_object_from_file(path);

  const char *s = json_object_to_json_string_ext(new_parsed_json, JSON_C_TO_STRING_PRETTY);
  printf("%s\n", s);
  return new_parsed_json;
}

static void
usage() {
  puts("usage: ms [option]\n");
  exit(1);
}

static json_object*
get_config_date() {
  json_object *config = read_config();
  json_object *result = json_object_new_object();
  json_object_object_get_ex(config, "time", &result);
  return result;
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
  json_object *config = read_config();
  if (config == NULL) {
    fprintf(stderr, "failed reading config\n");
    exit(1);
  }
  json_object *hostname;
  if (!json_object_object_get_ex(config, "serverHostName", &hostname)) {
    fprintf(stderr, "failed reading serverHostName\n");
    exit(1);
  }
  const char *hostString = json_object_to_json_string_ext(hostname, JSON_C_TO_STRING_PRETTY);
  json_object *host_ip;
  if (!json_object_object_get_ex(config, "ip", &host_ip)) {
    fprintf(stderr, "failed reading ip\n");
    exit(1);
  }
  const char *ipString= json_object_to_json_string_ext(host_ip, JSON_C_TO_STRING_PRETTY);
  json_object *this_date = get_config_date();

  char command[1024];
  printf("hostString = %s, ipString = %s\n",hostString, ipString );
  snprintf(command, sizeof(command), "ssh %s@%s 'ms time'", hostString, ipString);

  const char *time_string = execute_command(command);
  json_object *other_time = json_tokener_parse(time_string);

  if (other_time == NULL) {
      fprintf(stderr, "Error: Failed to parse JSON string.\n");
      exit(1);
  }

  printf("my time= \n %s\n", json_object_to_json_string_ext(this_date, JSON_C_TO_STRING_PRETTY));
  printf("other time= \n %s\n", json_object_to_json_string_ext(other_time , JSON_C_TO_STRING_PRETTY));

}


static void
diff() {

}

int main(int argc, char **argv){

  if (argc == 1) {
    usage();

  }

  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "status")) {
      status();
    } else if (!strcmp(argv[i], "init")) {
      create_default_config();
    } else if (!strcmp(argv[i], "diff")) {
      diff();
    } else if (!strcmp(argv[i], "config")) {
      char path[1024];
      get_default_config_path(path, sizeof(path));
      printf("The path to the config file is: %s\n", path);
    } else if (!strcmp(argv[i], "test")) {
      char *result = execute_command("load");
      printf("%s\n", result);
    } else if (!strcmp(argv[i], "time")) {
      json_object *date = get_config_date();
      printf("%s\n", json_object_to_json_string_ext(date, JSON_C_TO_STRING_PRETTY));
    }

  }
  return 1;
}
