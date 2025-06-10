// wow google's code really sucks
/**
 * Copyright 2021 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 **/

#include "shell.h"

#include <stdbool.h>
#include <stdio.h>

#include "shell_config.h"


int (*__read_char__)(void);
void (*__write_char__)(char c);

cmd_t table[MAX_COMMANDS+1];
int shell_table_size;

static volatile int __cmd_exec_status;

/*
 * To reduce the shell size the history feature
 * is made optional. Skip history feature if
 * SHELL_NO_HISTORY is defined.
 */
#ifndef SHELL_NO_HISTORY
static volatile int total_num_commands = 0;
static volatile int curr_command_ptr = 0;
static char cmd_history[NUM_HISTORY_ENTRIES][LINE_BUFF_SIZE];
#endif

static volatile bool __echo = ECHO_INIT_VALUE;  // Should be set in the Makefile

void set_read_char(int (*func)(void)) { __read_char__ = func; }

void set_write_char(void (*func)(char)) { __write_char__ = func; }

__attribute__((weak)) void setup(void) {
  // to be provided by the user
}

__attribute__((weak)) void loop(void) {
  // to be provided by the user
}

// This method is a place holder to prepend prompt
__attribute__((weak)) void prepend_prompt() {
  ;  // nothing by default
}

// Check if prompt is active, by default true.
// Platforms can decide a logic for this.
__attribute__((weak)) int active_prompt() { return TRUE; }

static void delete(void) {
  __write_char__(BACK_SPACE);
  __write_char__(SPACE);
  __write_char__(BACK_SPACE);
}

static void clear_prompt(int char_count) {
  while (char_count) {
    delete ();
    char_count--;
  }
}

/*
 * To reduce the shell size the history feature
 * is made optional. Skip history feature if
 * SHELL_NO_HISTORY is defined.
 */
#ifndef SHELL_NO_HISTORY
static void handle_up_arrow(char *cmd_buff, int *char_count) {
  if (curr_command_ptr < total_num_commands - NUM_HISTORY_ENTRIES ||
      curr_command_ptr == 0) {
    printf("%s", cmd_buff);
    return;
  }

  memset(cmd_buff, 0, LINE_BUFF_SIZE);

  curr_command_ptr--;
  int index = (curr_command_ptr % NUM_HISTORY_ENTRIES);
  memcpy(cmd_buff, &cmd_history[index], LINE_BUFF_SIZE);
  *char_count = strlen(cmd_buff);

  printf("%s", cmd_buff);
}

static void handle_down_arrow(char *cmd_buff, int *char_count) {
  memset(cmd_buff, 0, LINE_BUFF_SIZE);
  *char_count = 0;
  if (curr_command_ptr == total_num_commands) return;

  curr_command_ptr++;
  int index = (curr_command_ptr % NUM_HISTORY_ENTRIES);
  memcpy(cmd_buff, &cmd_history[index], LINE_BUFF_SIZE);
  *char_count = strlen(cmd_buff);

  printf("%s", cmd_buff);
}

static void add_command_to_history(const char *cmd_str) {

  //add only if command is not empty
  if (cmd_str == NULL || strcmp(cmd_str, "") == 0) {
    return;
  }
  int index = total_num_commands % NUM_HISTORY_ENTRIES;
  memcpy(&cmd_history[index], cmd_str, LINE_BUFF_SIZE);
  total_num_commands++;
  curr_command_ptr = total_num_commands;
}

static int show_history(int argc, char **argv) {
  uint32_t end_index = total_num_commands-1;
  uint32_t beg_index = 0;
  if (total_num_commands > NUM_HISTORY_ENTRIES) {
    beg_index = total_num_commands - NUM_HISTORY_ENTRIES;
  }
  for (uint32_t index = beg_index; index <= end_index; ++index) {
    printf("%s\n", cmd_history[index % NUM_HISTORY_ENTRIES]);
  }

  return 0;
}


#endif  // SHELL_NO_HISTORY


#ifndef SHELL_NO_TAB_COMPLETE

static int prefix_match(char *sub, int len, const char *str) {
  if (sub == NULL || str == NULL || len <= 0 || len > strlen(str)) {
    return FALSE;
  }

  for (int i = 0; i<len; ++i) {
    if (sub[i] != str[i]) {
      return FALSE;
    }
  }

  return TRUE;
}
static void handle_tab(char *cmd_buff, int *char_count) {
  if (cmd_buff == NULL || char_count <= 0) {
    return;
  }

  int i = 0;
  int match_count = 0;
  int last_match = -1;
  while (table[i].command_name[0] != 0) { //loop over all commands

    //if prefix matches, print that as one of the options
    if (prefix_match(cmd_buff, *char_count, table[i].command_name)) {
      match_count++;
      last_match = i;
      printf("\n%s", table[i].command_name);
    }

    i++;
  }

  // if only one match, then that's the command to be executed
  if (match_count == 1) {
    memcpy(cmd_buff, table[last_match].command_name, LINE_BUFF_SIZE);
    *char_count = strlen(cmd_buff);
  }

  // print current line with old/updated command
  if (match_count) {
    printf("\n");
    prepend_prompt();
    printf(PROMPT);
    printf("%s", cmd_buff);
  }
}

#endif  // SHELL_NO_TAB_COMPLETE

static int parse_line(char **argv, char *line_buff, int argument_size) {
  int argc = 0;
  int pos = 0;
  int length = strlen(line_buff);

  while (pos <= length) {
    if (line_buff[pos] != '\t' && line_buff[pos] != SPACE &&
        line_buff[pos] != END_OF_LINE)
      argv[argc++] = &line_buff[pos];

    for (; line_buff[pos] != '\t' && line_buff[pos] != SPACE &&
           line_buff[pos] != END_OF_LINE;
         pos++)
      ;

    if (line_buff[pos] == '\t' || line_buff[pos] == SPACE)
      line_buff[pos] = END_OF_LINE;

    pos++;
  }

  return argc;
}

static void execute(int argc, char **argv) {
  int match_found = FALSE;

  for (int i = 0; table[i].command_name[0] != 0; i++) {
    if (strcmp(argv[0], table[i].command_name) == 0) {
      __cmd_exec_status = table[i].command(argc, &argv[0]);
      match_found = TRUE;
      break;
    }
  }

  if (match_found == FALSE) {
    printf("\"%s\": command not found. Use \"help\" to list all command.\n",
           argv[0]);
    __cmd_exec_status = -1;
  }
}

static void shell(void) {
  int s, argc;
  int count = 0;
  int special_key = 0;
  char c;

  char line_buff[LINE_BUFF_SIZE];
  char *argv[MAX_ARG_COUNT];

  for (int i = 0; i < LINE_BUFF_SIZE; i++) line_buff[i] = 0;

  for (int i = 0; i < MAX_ARG_COUNT; i++) argv[i] = NULL;

  prepend_prompt();
  printf(PROMPT);

  while (TRUE) {
    if (!active_prompt()) {
      continue;
    }

    s = __read_char__();
    if (s != -1) {
      c = (char)s;

      if (c == CARRIAGE_RETURN || c == NEW_LINE) {
        line_buff[count] = END_OF_LINE;
        __write_char__(NEW_LINE);
        break;
      }

      if (c == DELETE || c == BACK_SPACE) {
        if (!__echo) {
          delete ();
          delete ();
        }

        // guard against the count going negative!
        if (count == 0) continue;

        count--;

        line_buff[count] = END_OF_LINE;
        delete ();
      } else if (c == ESCAPE) {
        special_key = 1;
        continue;
      } else if (c == SQUARE_BRACKET_OPEN && special_key == 1) {
        special_key = 2;
        continue;
      } else if ((c == 'C' || c == 'D') && special_key != 0) {
        /* Ignore left/right arrow keys */
        special_key = 0;
        continue;
      } else if ((c == 'A' || c == 'B') && special_key == 2) {
        if (!__echo) {
          clear_prompt(count + 4);
        } else {
          clear_prompt(count);
        }
/*
 * To reduce the shell size the history feature
 * is made optional. Skip history feature if
 * SHELL_NO_HISTORY is defined.
 */
#ifndef SHELL_NO_HISTORY
        if (c == 'A') {
          handle_up_arrow(line_buff, &count);
        } else {
          handle_down_arrow(line_buff, &count);
        }
#endif  // SHELL_NO_HISTORY
        special_key = 0;
        continue;
      }
#ifndef SHELL_NO_TAB_COMPLETE
      else if (c == TAB) {
        handle_tab(line_buff, &count);
        continue;
      }
#endif //SHELL_NO_TAB_COMPLETE
      else {
        line_buff[count] = c;
        count++;
      }
      if (__echo && c != DELETE && c != BACK_SPACE) {
        __write_char__(c);
      }
    } else {
      loop();
    }
  }

/*
 * To reduce the shell size the history feature
 * is made optional. Skip history feature if
 * SHELL_NO_HISTORY is defined.
 */
#ifndef SHELL_NO_HISTORY
  add_command_to_history(line_buff);
#endif

  // parse the line_buff
  argc = parse_line(argv, line_buff, MAX_ARG_COUNT);

  // execute the parsed commands
  if (argc > 0) execute(argc, argv);
}

static int build_info(int argc, char **argv) {
  printf("Build: [" SHELL_VERSION ":" USER_REPO_VERSION "] - [" BUILD_USER
         "@" BUILD_HOST "] - " __DATE__ " - " __TIME__ "\n");
  return 0;
}

int exec(char *cmd_str) {
  int argc;

  // TODO: this takes too much stack space. Optimize!
  char *argv[MAX_ARG_COUNT];

  // parse the line_buff
  argc = parse_line(argv, cmd_str, MAX_ARG_COUNT);

  // execute the parsed commands
  if (argc > 0) execute(argc, argv);

  return __cmd_exec_status;
}

cmd get_function_addr(char *cmd_str) {
  for (int i = 0; table[i].command_name != NULL; i++) {
    if (strcmp(cmd_str, table[i].command_name) == 0) {
      return table[i].command;
    }
  }
  return NULL;
}

int help(int argc, char **argv) {
  int i = 0;
  /* Default to Verbose */
  bool verbose = true;

  if (argc > 1 && (strcmp(argv[1], "-l")==0)) {
    verbose = false;
  } else {
    printf("use: help -l for list only.\n\n");
  }

  while (table[i].command_name[0] != 0) {
    printf(table[i].command_name);

    if (verbose) {
      printf("\n\t");
      printf(table[i].command_help);
    }

    printf("\n");
    i++;
  }

  return 0;
}

int cmd_exec_status(int argc, char **argv) {
  printf("%d\n", __cmd_exec_status);
  return 0;
}

/**
 * @brief spwans the prompt
 *
 */
void prompt() {
    ADD_CMD("history", "Show command history", show_history);
    ADD_CMD("help", "Prints all available commands", help);
    ADD_CMD("status", "Returns exit status of last executed command", cmd_exec_status);    
    setup();

    while (TRUE) {
        shell();
    }
}



// // DO NOT REMOVE THESE   
// AUTO_CMD(version, "Prints details of the build", build_info);
// ADD_CMD(help, "Prints all available commands", help);


void ADD_CMD(const char *name, const char *help, cmd command) {
    if (shell_table_size != MAX_COMMANDS) {
        strcpy(table[shell_table_size].command_name, name);
        strcpy(table[shell_table_size].command_help, help);
        table[shell_table_size].command = command;
        shell_table_size++;
    }
}
