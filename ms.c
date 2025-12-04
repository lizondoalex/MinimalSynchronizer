#include <stdio.h>
#include <stdlib.h> //getenv()
#include <string.h> //strcmp(), strrchr(), strdup()
#include <sys/stat.h> //mkdir()
#include <errno.h> //check mkdir() errors
#include <json-c/json.h> //json-c library header
#include <time.h> //for server-client date comparison and check


static int
get_int_field(json_object *obj, const char *key) {
  json_object *field;
  if (json_object_object_get_ex(obj, key, &field)) {
    return json_object_get_int(field);
  }
  return 0;
}

static int
compare_dates(json_object *my_date, json_object *other_date) {
  const char *fields[] = {
    "tm_year", "tm_mon", "tm_mday", "tm_hour", "tm_min", "tm_sec"
  };

  int num_fields = sizeof(fields) / sizeof(fields[0]);

  for (int i = 0; i < num_fields; i++) {
    const char *key = fields[i];

    int val1 = get_int_field(my_date, key);
    int val2 = get_int_field(other_date, key);

    if (val1 < val2) {
      return -1;
    }
    if (val1 > val2) {
      return 1;
    }
  }

  return 0;
}

static json_object*
get_current_date() {
  time_t t = time(NULL);

  struct tm *current_time = localtime(&t);
  json_object *result = json_object_new_object();

  json_object_object_add(result, "tm_sec",   json_object_new_int(current_time->tm_sec));
  json_object_object_add(result, "tm_min",   json_object_new_int(current_time->tm_min));
  json_object_object_add(result, "tm_hour",  json_object_new_int(current_time->tm_hour));
  json_object_object_add(result, "tm_mday",  json_object_new_int(current_time->tm_mday));
  json_object_object_add(result, "tm_mon",   json_object_new_int(current_time->tm_mon));
  json_object_object_add(result, "tm_year",  json_object_new_int(current_time->tm_year));
  json_object_object_add(result, "tm_wday",  json_object_new_int(current_time->tm_wday));
  json_object_object_add(result, "tm_yday",  json_object_new_int(current_time->tm_yday));
  json_object_object_add(result, "tm_isdst", json_object_new_int(current_time->tm_isdst));

  return result;
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
  json_object *time = get_current_date();

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
  json_object *result = NULL;
  if (json_object_object_get_ex(config, "time", &result)) {
    json_object_get(result);
  }
  json_object_put(config);
  return result;
}

static void
update_date(const char *date) {

  json_object *time = json_tokener_parse(date);
  if (time == NULL) {
    printf("incorrect time passed\n");
    exit(1);
  }
  printf("%s\n", json_object_to_json_string_ext(time, JSON_C_TO_STRING_PRETTY));
  json_object *config = read_config();

  json_object_object_add(config, "time", time);

  write_json(config);
  json_object_put(config);
}
static void
update_date_json(json_object *date) {

  if (date == NULL) {
    printf("incorrect time passed\n");
    exit(1);
  }
  json_object *config = read_config();

  json_object_object_add(config, "time", date);

  write_json(config);
  json_object_put(config);
}

static char**
get_remote_info() {
  json_object *config = read_config();
  if (config == NULL) {
    fprintf(stderr, "failed reading config\n");
    exit(1);
  }
  json_object *hostname, *host_ip, *serverDirectory, *clientDirectory;

  if (!json_object_object_get_ex(config, "serverHostName", &hostname)) {
    fprintf(stderr, "failed reading serverHostName\n");
    exit(1);
  }
  if (!json_object_object_get_ex(config, "ip", &host_ip)) {
    fprintf(stderr, "failed reading ip\n");
    exit(1);
  }

  if (!json_object_object_get_ex(config, "serverDirectory", &serverDirectory)) {
    fprintf(stderr, "failed reading serverDirectory\n");
    exit(1);
  }

  if (!json_object_object_get_ex(config, "clientDirectory", &clientDirectory)) {
    fprintf(stderr, "failed reading clientDirectory\n");
    exit(1);
  }

  const char *hostString = json_object_get_string(hostname);
  const char *ipString= json_object_get_string(host_ip);
  const char *serverDirectoryString = json_object_get_string(serverDirectory);
  const char *clientDirectoryString = json_object_get_string(clientDirectory);

  char **result = malloc(sizeof(char*) * 4);
  result[0] = strdup(hostString);
  result[1] = strdup(ipString);
  result[2] = strdup(serverDirectoryString);
  result[3] = strdup(clientDirectoryString);

  json_object_put(config);

  return result;
}

static char*
execute_command_remote(const char* exec) {

  char **remoteInfo = get_remote_info();
  char *hostString = remoteInfo[0];
  char *ipString= remoteInfo[1];

  char command[1024];
  printf("hostString = %s, ipString = %s\n",hostString, ipString );
  snprintf(command, sizeof(command), "ssh %s@%s %s", hostString, ipString, exec);

  free(remoteInfo[0]);
  free(remoteInfo[1]);
  free(remoteInfo[2]);
  free(remoteInfo[3]);
  free(remoteInfo);

  return execute_command(command);
}

static void
status() {
  json_object *this_date = get_config_date();

  char *time_string = execute_command_remote("ms time");
  json_object *other_time = json_tokener_parse(time_string);

  if (other_time == NULL) {
      fprintf(stderr, "Error: Failed to parse JSON string.\n");
      exit(1);
  }

  printf("my time= \n %s\n", json_object_to_json_string_ext(this_date, JSON_C_TO_STRING_PRETTY));
  printf("other time= \n %s\n", json_object_to_json_string_ext(other_time , JSON_C_TO_STRING_PRETTY));

  int compare = compare_dates(this_date, other_time);

  if (compare == -1) {
    printf("This host is outdated\n");
  } else if (compare == 0) {
    printf("The server is on pair\n");
  } else {
    printf("The server is outdated\n");
  }

  free(time_string);
  json_object_put(other_time);
  json_object_put(this_date);

  exit(0);

}

static void
load() {
  const char *timeString = execute_command_remote("ms time");
  update_date(timeString);

  char **remoteInfo = get_remote_info();
  char *hostString = remoteInfo[0];
  char *ipString = remoteInfo[1];
  char *serverDirectoryString = remoteInfo[2];
  char *clientDirectoryString = remoteInfo[3];

  char command[4096];
  snprintf(command, sizeof(command), "rsync -avzh --delete -e ssh %s@%s:%s %s\n", hostString, ipString, serverDirectoryString, clientDirectoryString);
  char *result = execute_command(command);

  printf("%s\n", result);

  free(remoteInfo[0]);
  free(remoteInfo[1]);
  free(remoteInfo[2]);
  free(remoteInfo[3]);
  free(remoteInfo);

  free((void*)timeString);
  free(result);

  exit(0);
}
static void
save() {
  json_object *current_date = get_current_date();
  const char *timeString = json_object_to_json_string_ext(current_date, JSON_C_TO_STRING_PLAIN);
  update_date(timeString);
  char base64_command[2048];
  snprintf(base64_command, sizeof(base64_command), "echo '%s' | base64 -w 0", timeString);

  char *base64_string = execute_command(base64_command);

  base64_string[strcspn(base64_string, "\n")] = 0;

  char remote_command[2048];
  snprintf(remote_command, sizeof(remote_command), "ms update %s", base64_string);

  execute_command_remote(remote_command);

  char **remoteInfo = get_remote_info();
  char *hostString = remoteInfo[0];
  char *ipString = remoteInfo[1];
  char *serverDirectoryString = remoteInfo[2];
  char *clientDirectoryString = remoteInfo[3];

  char command[4096];
  snprintf(command, sizeof(command), "rsync -avzh --delete -e ssh %s %s@%s:%s\n", clientDirectoryString, hostString, ipString, serverDirectoryString);
  char *result = execute_command(command);

  printf("%s\n", result);

  free(remoteInfo[0]);
  free(remoteInfo[1]);
  free(remoteInfo[2]);
  free(remoteInfo[3]);
  free(remoteInfo);

  free((void*)timeString);
  free(result);

  exit(0);

}

static json_object*
json_from_timestamp(time_t t) {
  struct tm *time_info = localtime(&t);
  json_object *result = json_object_new_object();

  json_object_object_add(result, "tm_sec",   json_object_new_int(time_info->tm_sec));
  json_object_object_add(result, "tm_min",   json_object_new_int(time_info->tm_min));
  json_object_object_add(result, "tm_hour",  json_object_new_int(time_info->tm_hour));
  json_object_object_add(result, "tm_mday",  json_object_new_int(time_info->tm_mday));
  json_object_object_add(result, "tm_mon",   json_object_new_int(time_info->tm_mon));
  json_object_object_add(result, "tm_year",  json_object_new_int(time_info->tm_year));
  json_object_object_add(result, "tm_wday",  json_object_new_int(time_info->tm_wday));
  json_object_object_add(result, "tm_yday",  json_object_new_int(time_info->tm_yday));
  json_object_object_add(result, "tm_isdst", json_object_new_int(time_info->tm_isdst));

  return result;
}

static char* get_client_directory() {

}

static void
update() {
  json_object *config = read_config();

  json_object *dir_obj;
  char *client_dir= NULL;

  if (json_object_object_get_ex(config, "clientDirectory", &dir_obj)) {
    client_dir = strdup(json_object_get_string(dir_obj));
  }
  json_object_put(config);

  if (!client_dir) {
    fprintf(stderr, "Error: Could not get client directory.\n");
    return;
  }

  char command[4096];
  snprintf(command, sizeof(command),
           "find '%s' -type f -printf '%%T@\\n' | sort -rn | head -n 1",
           client_dir);

  char *output = execute_command(command);

  if (!output || strlen(output) < 1) {
    printf("No files found in %s\n", client_dir);
    free(client_dir);
    if (output) free(output);
    return;
  }

  time_t file_timestamp = (time_t) atoi(output);

  json_object *file_sys_date = json_from_timestamp(file_timestamp);

  json_object *config_date = get_config_date();

  printf("Checking file system vs config...\n");

  if (compare_dates(file_sys_date, config_date) == 1) {
    puts("Updating config time.\n");
    update_date_json(file_sys_date);
  } else {
    printf("Config is already up to date.\n");
    json_object_put(file_sys_date);
  }

  json_object_put(config_date);
  free(client_dir);
  free(output);
}

static void diff() {
    char **remoteInfo = get_remote_info();
    char *hostString = remoteInfo[0];
    char *ipString = remoteInfo[1];
    char *serverDirectoryString = remoteInfo[2];
    char *clientDirectoryString = remoteInfo[3];

    char command[4096];
    snprintf(command, sizeof(command),
             "rsync -avzni --delete -e ssh %s %s@%s:%s",
             clientDirectoryString, hostString, ipString, serverDirectoryString);
  printf("command is %s\n", command);

    printf("Calculating differences...\n");
    char *output = execute_command(command);

    if (!output || strlen(output) == 0) {
        printf("Everything is up to date.\n");
        free(remoteInfo[0]); free(remoteInfo[1]); free(remoteInfo[2]); free(remoteInfo[3]); free(remoteInfo);
        if(output) free(output);
        return;
    }

    char *line = strtok(output, "\n");
    while (line != NULL) {

        if (strstr(line, "sending incremental file list")) {
            line = strtok(NULL, "\n");
            continue;
        }

        if (strncmp(line, "*deleting", 9) == 0) {
            char *filename = line + 10;
            printf("Del: %s\n", filename);
        }

        else if (strncmp(line, "<f+++++++++", 11) == 0) {
            char *filename = line + 12;
            printf("New: %s\n", filename);
        }

        else if (strncmp(line, "<f", 2) == 0) {
             char *filename = line + 12;
             printf("Mod: %s\n", filename);
        }

        line = strtok(NULL, "\n");
    }

    free(remoteInfo[0]);
    free(remoteInfo[1]);
    free(remoteInfo[2]);
    free(remoteInfo[3]);
    free(remoteInfo);
    free(output);

    exit(0);
}

int main(const int argc, char **argv){

  if (argc == 1) {
    usage();

  }

  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "status")) {
      status();
    }
    else if (!strcmp(argv[i], "init")) {
      create_default_config();
    }
    else if (!strcmp(argv[i], "update")) {
      if (argc == 3) {
        const char *base64_string = argv[2];

        char decode_command[2048];
        snprintf(decode_command, sizeof(decode_command), "echo '%s' | base64 -d", base64_string);

        char *json_string = execute_command(decode_command);

        update_date(json_string);
        free(json_string);

        i++;
      } else {
        update();
      }
    }    else if (!strcmp(argv[i], "diff")) {
      diff();
    }
    else if (!strcmp(argv[i], "config")) {
      char path[1024];
      get_default_config_path(path, sizeof(path));
      printf("The path to the config file is: %s\n", path);
    }
    else if (!strcmp(argv[i], "load")) {
      load();
    }
    else if (!strcmp(argv[i], "save")) {
      save();
    }
    else if (!strcmp(argv[i], "time")) {
      json_object *date = get_config_date();
      printf("%s\n", json_object_to_json_string_ext(date, JSON_C_TO_STRING_PRETTY));
    } else {
      usage();
    }

  }
  return 1;
}
