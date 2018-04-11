
/********************/
/********************/
/* wtjl interpreter */
/********************/
/********************/
/**(C)2018AlexKizer**/
/********************/
/********************/

/* ``begin includes */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>

/* end includes */

/* ``begin macros */

#define TRACK_MEM

#define J_MEM_DEBUG false

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define CLR_NRM "\x1B[0m"
#define CLR_GRY "\x1B[30m"
#define CLR_RED "\x1B[31m"
#define CLR_GRN "\x1B[32m"
#define CLR_YEL "\x1B[33m"
#define CLR_BLU "\x1B[34m"
#define CLR_MAG "\x1B[35m"
#define CLR_CYN "\x1B[36m"
#define CLR_WHT "\x1B[37m"
#define CLR(clr, string) CLR_##clr string CLR_NRM

#define TESTS_PRELUDE printf(CLR(CYN, "Running tests.\n"));
#define TEST_SUITE printf("\n" CLR(CYN, "S ") CLR_YEL "\"%s\"\n" CLR_NRM, __func__);
#define TEST_RESULT_REST CLR_YEL "\"%s\"" CLR_NRM "\n", __func__);
#define TEST_PASS printf(CLR_GRN "✓ " TEST_RESULT_REST
#define TEST_FAIL glbl_tests_result = false; printf(CLR_RED "✗ " TEST_RESULT_REST
#define TESTS_RESULTS glbl_tests_result ? printf("\n" CLR_GRN "All tests passed!\n" CLR_NRM) : printf("\n" CLR_RED "One or more tests failed!\n" CLR_NRM);

/* end macros */

/* ``begin memory tracking */

int j_mem_offset = 0;

size_t j_mem_total_alloc = 0;
size_t j_mem_total_free = 0;

typedef struct _mem_node_t {
  void *ptr;
  size_t size;
  struct _mem_node_t *next;
} _mem_node_t;

_mem_node_t *_mem_list_head = NULL;
_mem_node_t *_mem_list_tail = NULL;

void *_jmalloc (size_t size) {
  if (J_MEM_DEBUG) {
    printf("(malloc) " CLR_YEL "%d\n" CLR_NRM, (int) size);
  }
  j_mem_total_alloc += size;
  if (_mem_list_tail == NULL) {
    _mem_list_head = calloc(1, sizeof(_mem_node_t));
    _mem_list_tail = _mem_list_head;
  } else {
    _mem_list_tail->next = calloc(1, sizeof(_mem_node_t));
    _mem_list_tail = _mem_list_tail->next;
  }
  _mem_list_tail->size = size;
  _mem_list_tail->ptr = malloc(size);
  return _mem_list_tail->ptr;
}

void _jfree (void *ptr) {
  if (!ptr) {
    return;
  }
  _mem_node_t *node = _mem_list_head;
  _mem_node_t *prev_node = node;
  if (!node) {
    goto finish;
  }
  while (node->next) {
    if (node->ptr == ptr) {
      break;
    }
    prev_node = node;
    node = node->next;
  }
  if (node) {
    if (prev_node) {
      prev_node->next = node->next;
    }
    if (J_MEM_DEBUG) {
      printf("free %d\n", (int) node->size);
    }
    j_mem_total_free += node->size;
    if (_mem_list_head == node) {
      _mem_list_head = prev_node;
    }
    if (_mem_list_tail == node) {
      _mem_list_tail = prev_node;
    }
    free(node->ptr);
    free(node);
    return;
  }
  finish:
  printf(CLR(RED, "Warning: free non malloc'd ptr.\n"));
  free(ptr);
}

size_t j_mem_size () {
  size_t size = 0;
  _mem_node_t *node = _mem_list_head;
  while (node) {
    size += node->size;
    node = node->next;
  }
  return (size_t) (size + j_mem_offset);
}

void j_mem_reset () {
  /*_mem_node_t *node = _mem_list_head;
  _mem_node_t *temp;
  while (node) {
    temp = node->next;
    free(node);
    node = temp;
  }
  _mem_list_head = NULL;
  _mem_list_tail = NULL;*/
}

#ifdef TRACK_MEM
  #define malloc(size) _jmalloc(size)
  #define free(ptr) _jfree(ptr)
#endif

/* end memory tracking */

/* ``begin forward declarations */

void begin ();

typedef struct _ll_item_t _ll_item_t;
typedef struct ll_t ll_t;

/* end forward declarations */

/* ``begin module system */

#define module_start(name) typedef struct module_##name##_t { void (*initialize) (); void (*cleanup) ();
#define module_end(name) } module_##name##_t; module_##name##_t name;
#define MODULE(name, members) module_start(name) members module_end(name)
//#define SETUP_MODULE(name) push_module_setup_fp(setup_##name); push_module_initialize_fp(name.initialize);
#define SETUP_MODULE(name) setup_##name(); name.initialize();
//#define SETUP_MODULES() setup_modules();

void (**module_setup_fps) () = NULL;
void (**module_initialize_fps) () = NULL;
size_t module_setup_fps_length = 0;
size_t module_initialize_fps_length = 0;

void push_module_setup_fp (void (*fp) ()) {
  module_setup_fps_length++;
  module_setup_fps = realloc(module_setup_fps, sizeof(char *) * module_setup_fps_length);
  module_setup_fps[module_setup_fps_length - 1] = fp;
}

void push_module_initialize_fp (void (*fp) ()) {
  module_initialize_fps_length++;
  module_initialize_fps = realloc(module_initialize_fps, sizeof(char *) * module_initialize_fps_length);
  module_initialize_fps[module_initialize_fps_length - 1] = fp;
}

void setup_modules () {
  for (int i = 0; i < module_setup_fps_length; i++) {
    printf("%d, ", i);
    printf("%p\n", module_setup_fps[i]);
    //module_setup_fps[i]();
  }
  for (int i = 0; i < module_initialize_fps_length; i++) {
    //module_initialize_fps[i]();
  }
}

/* end module system */

/* ``begin tokenizer declarations */

typedef enum token_type_primary_t {
  IDENTIFIER,
  KEYWORD,
  LITERAL,
  OPERATOR,
  GROUPING,
  DELIMITER,
  UNKNOWN_PRIMARY
} token_type_primary_t;

typedef enum token_type_secondary_t {
  KEYWORD_IF,
  KEYWORD_ITERATE,
  KEYWORD_AS,
  KEYWORD_VAR,
  LITERAL_QUOTE_D,
  LITERAL_QUOTE_S,
  LITERAL_QUOTE_B,
  LITERAL_INTEGER,
  LITERAL_DOUBLE,
  LITERAL_BOOLEAN,
  OPERATOR_STAR,
  OPERATOR_STARSTAR,
  OPERATOR_PLUS,
  OPERATOR_MINUS,
  OPERATOR_FSLASH,
  OPERATOR_COLON,
  OPERATOR_COLONCOLON,
  OPERATOR_ARROW_R,
  OPERATOR_ARROW_L,
  GROUPING_BRACE_L,
  GROUPING_BRACKET_L,
  GROUPING_PAREN_L,
  GROUPING_BRACE_R,
  GROUPING_BRACKET_R,
  GROUPING_PAREN_R,
  DELIMITER_SEMI,
  DELIMITER_COMMA,
  UNKNOWN_SECONDARY
} token_type_secondary_t;

typedef struct token_t {
  token_type_primary_t token_type_primary;
  token_type_secondary_t token_type_secondary;
  char *representation;
  char *file;
  size_t line;
  size_t column;
  size_t index;
} token_t;

/* end tokenizer declarations */

/* ``begin parser declarations */

typedef struct node_t {
  char *type;
  size_t num_children;
  struct node_t *children;
  struct node_t *parent;
  char *info_type;
  void *info;
} node_t;

typedef struct literal_info_t {
  token_t *token;
} integer_literal_info_t;

typedef struct operator_info_t {
  token_t *token;
} operator_info_t;

/* end parser declarations */

/* ``begin modules */

MODULE(ll,
  ll_t *(*new) ();
  void (*add) ();
  void (*delete) ();
  void (*deletef) ();
  void (*remove) ();
  void *(*get) ();
  size_t (*size) ();
  void (*set_type) ();
  char *(*get_type) ();
);

MODULE(scanner,
  void (*scan) ();
  bool (*done) ();
  char (*peek) ();
  char (*next) ();
  char *(*next_ptr) ();
  char *(*consume) ();
  char done_char;
)

MODULE(tokenizer,
  token_t *(*next) ();
  token_t *(*peek) ();
  token_t **(*consume) ();
  bool (*done) ();
  char *(*token_as_string) ();
)

MODULE(parser,
  void (*parse) ();
)

/* end modules */

/* ``begin ll structs */

typedef struct _ll_item_t { // private
  void *_value; // private
  _ll_item_t *_next; // private
  _ll_item_t *_prev; // private
} _ll_item_t;

typedef struct ll_t {
  _ll_item_t *_first; // private
  _ll_item_t *_last; // private
  size_t _last_index; // private
  _ll_item_t *_last_accessed; // private
  size_t _num_elements; // private
  char *_type; // private
} ll_t;

/* end ll structs */

/* ``begin arguments_t */

typedef struct arguments_t {
  bool valid;
  int difference_from_correct;
  char *program_name;
  char *file_name;
  bool test;
} arguments_t;

const int ARGUMENT_COUNT = 2;

void *arguments_t_new () {
  arguments_t *arguments = malloc(sizeof(arguments_t));
  arguments->valid = true;
  arguments->difference_from_correct = 0;
  arguments->program_name = NULL;
  arguments->file_name = NULL;
  arguments->test = false;
  return arguments;
}

void arguments_t_set_program_name (arguments_t *arguments, char *program_name) {
  if (arguments->program_name == NULL) {
    arguments->program_name = malloc(strlen(program_name) + 1);
  }
  strcpy(arguments->program_name, program_name);
}

void arguments_t_set_file_name (arguments_t *arguments, char *file_name) {
  if (arguments->file_name == NULL) {
    arguments->file_name = malloc(strlen(file_name) + 1);
  }
  strcpy(arguments->file_name, file_name);
}

void arguments_t_destroy (arguments_t *arguments) {
  if (arguments == NULL) {
    return;
  }
  free(arguments->program_name);
  free(arguments->file_name);
  free(arguments);
}

arguments_t *arguments_t_parse_arguments (int argc, char **argv) {
  arguments_t *arguments = arguments_t_new();
  for (int i = 0; i < argc; i++) {
    if (strcmp(argv[i], "--test") == 0) {
      arguments->valid = true;
      arguments->test = true;
      return arguments;
    }
  }
  arguments->difference_from_correct = argc - ARGUMENT_COUNT;
  if (arguments->difference_from_correct != 0) {
    arguments->valid = false;
    return arguments;
  }
  arguments->valid = true;
  for (int i = 0; i < argc; i++) {
    if (i == 0) {
      arguments->program_name = malloc(strlen(argv[i]) + 1);
      arguments_t_set_program_name(arguments, argv[i]);
    } else if (i == 1) {
      arguments->file_name = malloc(strlen(argv[i]) + 1);
      arguments_t_set_file_name(arguments, argv[i]);
    } else {
      break;
    }
  }
  return arguments;
}

/* end arguments_t */

/* ``begin globals */

arguments_t *glbl_arguments = NULL;

size_t glbl_tests_mem_start;

bool glbl_tests_result = true;

/* end globals */

/* ``begin cleanup */

void cleanup () {
  arguments_t_destroy(glbl_arguments);
  glbl_arguments = NULL;

  // cleanup each module
  scanner.cleanup();
  tokenizer.cleanup();
}

/* end cleanup */

/* ``begin ll */

ll_t *_ll_new () {
  ll_t *list = malloc(sizeof(ll_t));
  list->_first = NULL;
  list->_last = NULL;
  list->_num_elements = 0;
  list->_last_index = 0;
  list->_last_accessed = NULL;
  list->_type = NULL;
  return list;
}

void _ll_reset (ll_t *list) {
  list->_last_index = 0;
  list->_last_accessed = NULL;
}

void _ll_delete_internal (ll_t *list, bool do_free) {
  if (list->_first) {
    _ll_item_t *item = list->_first;
    while (item) {
      if (do_free) {
        free(item->_value);
      }
      _ll_item_t *next = item->_next;
      free(item);
      item = next;
    }
  }
  free(list->_type);
  free(list);
}

void _ll_delete (ll_t *list) {
  _ll_delete_internal(list, false);
}

void _ll_deletef (ll_t *list) {
  _ll_delete_internal(list, true);
}

void _ll_add (ll_t* list, void *value) {
  _ll_item_t *item = malloc(sizeof(_ll_item_t));
  item->_value = value;
  item->_next = NULL;
  item->_prev = NULL;
  if (!list->_first) {
    list->_first = item;
    list->_last = item;
  } else {
    if (list->_last) {
      list->_last->_next = item;
    }
    item->_prev = list->_last;
    list->_last = item;
  }
  list->_num_elements++;
  list->_last = item;
}

void _ll_set_last (ll_t *list, size_t index, _ll_item_t *item) {
  if (!item) {
    _ll_reset(list);
    return;
  }
  list->_last_index = index;
  list->_last_accessed = item;
}

_ll_item_t* _ll_get_start_item (ll_t *list, size_t index) {
  if (index >= list->_last_index && list->_last_accessed) {
    return list->_last_accessed;
  } else {
    _ll_reset(list);
    return list->_first;
  }
}

size_t _ll_get_start_index (ll_t *list, size_t index) {
  if (index >= list->_last_index && list->_last_accessed) {
    return list->_last_index;
  } else {
    _ll_reset(list);
    return 0;
  }
}

_ll_item_t *_ll_get_item (ll_t *list, size_t index, bool set_last) {
  if (index >= list->_num_elements || !list->_first) {
    return NULL;
  }

  _ll_item_t *item = _ll_get_start_item(list, index);
  size_t _index = _ll_get_start_index(list, index);

  int i = _index;

  while (i < index) {
    item = item->_next;
    i++;
  }

  if (set_last) {
    // TODO uncomment:
    //_ll_set_last(list, _index, item);
  }

  return item;
}

void *_ll_get (ll_t *list, size_t index) {
  _ll_item_t *item = _ll_get_item(list, index, true);
  return item ? item->_value : NULL;
}

void _ll_remove (ll_t *list, size_t index) {
  _ll_item_t *item = _ll_get_item(list, index, false);
  _ll_reset(list);
  if (item) {
    if (item->_prev && item->_next) {
      item->_prev->_next = item->_next;
      item->_next->_prev = item->_prev;
    } else if (item->_next) {
      item->_next->_prev = item->_prev;
    } else if (item->_prev) {
      item->_prev->_next = item->_next;
    }
  } else {
    return;
  }
  _ll_item_t *replacement = NULL;
  if (item->_next) {
    replacement = item->_next;
  } else if (item->_prev) {
    replacement = item->_prev;
  }
  if (list->_first == item) {
    list->_first = replacement;
  }
  if (list->_last == item) {
    list->_last = replacement;
  }
  list->_num_elements--;
  free(item);
}

size_t _ll_size (ll_t *list) {
  return list->_num_elements;
}

void _ll_set_type (ll_t *list, char *type) {
  list->_type = strdup(type);
}

char *_ll_get_type (ll_t *list) {
  return list->_type;
}

void ll_cleanup () {

}

void ll_initialize () {
}

void setup_ll () {
  ll.cleanup = ll_cleanup;
  ll.initialize = ll_initialize;
  ll.new = _ll_new;
  ll.add = _ll_add;
  ll.delete = _ll_delete;
  ll.deletef = _ll_deletef;
  ll.remove = _ll_remove;
  ll.get = _ll_get;
  ll.size = _ll_size;
  ll.set_type = _ll_set_type;
  ll.get_type = _ll_get_type;
}

/* end ll */

/* ``begin scanner */

char *_scanner_input;
size_t _scanner_input_length;
size_t _scanner_index = 0;
const char _scanner_done_char = (char) 3;

void _scanner_read_file () {
  FILE *file;
  file = fopen(glbl_arguments->file_name, "r");
  if (file == NULL) {
    fprintf(stderr, "Failed to open file.\n");
    exit(1);
  }

  size_t file_size;
  fseek(file, 0, SEEK_END);
  file_size = (size_t) ftell(file);
  rewind(file);

  _scanner_input_length = file_size + 1;
  _scanner_input = malloc(_scanner_input_length);
  _scanner_input[_scanner_input_length - 1] = '\0';

  size_t bytes_read = fread(_scanner_input, 1, file_size, file);
  if (bytes_read != file_size) {
    fprintf(stderr, "Failed to read file.\n");
    exit(1);
  }

  fclose(file);
}

void scanner_initialize () {
}

bool scanner_done () {
  return _scanner_index == _scanner_input_length;
}

char scanner_next () {
  if (scanner_done()) {
    return _scanner_done_char;
  }
  return _scanner_input[_scanner_index];
}

char *scanner_next_ptr () {
  if (scanner_done()) {
    return NULL;
  }
  return (char *) (_scanner_input + _scanner_index);
}

char scanner_peek (size_t ahead) {
  size_t index = _scanner_index + ahead;
  if (index >= _scanner_input_length) {
    return _scanner_done_char;
  }
  return _scanner_input[index];
}

char *scanner_consume (size_t num_chars) {
  if (num_chars == 0) {
    return NULL;
  }
  if (_scanner_index + num_chars >= _scanner_input_length) {
    num_chars = _scanner_input_length - _scanner_index;
  }
  char *representation = malloc(num_chars + 1);
  representation[num_chars] = '\0';
  strncpy(representation, _scanner_input + _scanner_index, num_chars);
  _scanner_index = _scanner_index + num_chars;
  return representation;
}

void scanner_scan () {
  _scanner_read_file();
}

void scanner_cleanup () {
  free(_scanner_input);
}

void setup_scanner () {
  scanner.initialize = scanner_initialize;
  scanner.scan = scanner_scan;
  scanner.done = scanner_done;
  scanner.next = scanner_next;
  scanner.next_ptr = scanner_next_ptr;
  scanner.peek = scanner_peek;
  scanner.consume = scanner_consume;
  scanner.cleanup = scanner_cleanup;
  scanner.done_char = _scanner_done_char;
}

/* end scanner */

/* ``begin tokenizer */

token_t **_tokenizer_tokens = NULL;
size_t _tokenizer_tokens_size = 0;
size_t _tokenizer_tokens_index = 0;
size_t _tokenizer_line = 0;
size_t _tokenizer_col = 0;

size_t _tokenizer_tokenize_whitespace () {
  size_t length = 0;
  while (scanner.next() == ' ' || scanner.next() == '\n') {
    if (scanner.next() == '\n') {
      _tokenizer_line++;
      _tokenizer_col = 0;
    } else {
      _tokenizer_col++;
    }
    length++;
    free(scanner.consume(1));
  }
  return length;
}

char *_tokenizer_tokenize_comment_single () {
  return NULL; // TODO
}

char *_tokenizer_tokenize_comment_multi () {
  return NULL; // TODO
}

char *_tokenizer_tokenize_keyword () {
  char *keywords [2] = {"if", "repeat"};
  char *representation = NULL;
  size_t length = 0;

  while (isalpha(scanner.peek(length))) {
    length++;
  }
  if (!length || isdigit(scanner.peek(length))) {
    goto _return;
  }
  if (strncmp("if", scanner.next_ptr(), length) == 0) {
    goto consume;
  }
  if (strncmp("repeat", scanner.next_ptr(), length) == 0) {
    goto consume;
  }

  goto _return;

  consume:
    representation = scanner.consume(length);

  _return:
    return representation;
}

char *_tokenizer_tokenize_identifier () {
  char *representation = NULL;
  size_t length = 0;
  while(isalpha(scanner.peek(length))) {
    length++;
  }
  if (length) {
    representation = scanner.consume(length);
  }
  return representation;
}

char *_tokenizer_tokenize_integer_literal () {
  char *representation = NULL;
  size_t length = 0;
  while (isdigit(scanner.peek(length))) {
    length++;
  }
  if (length) {
    representation = scanner.consume(length);
  }
  return representation;
}

char *_tokenizer_tokenize_operator () {
  char *representation = NULL;
  size_t length = 0;
  if (strncmp("++", scanner.next_ptr(), strlen("++")) == 0) {
    representation = scanner.consume(2);
    goto _return;
  }
  if (strncmp("+", scanner.next_ptr(), strlen("+")) == 0) {
    representation = scanner.consume(1);
    goto _return;
  }
  if (strncmp("--", scanner.next_ptr(), strlen("--")) == 0) {
    representation = scanner.consume(2);
    goto _return;
  }
  if (strncmp("-", scanner.next_ptr(), strlen("-")) == 0) {
    representation = scanner.consume(1);
    goto _return;
  }
  if (strncmp("/", scanner.next_ptr(), strlen("/")) == 0) {
    representation = scanner.consume(1);
    goto _return;
  }
  if (strncmp("**", scanner.next_ptr(), strlen("**")) == 0) {
    representation = scanner.consume(2);
    goto _return;
  }
  if (strncmp("*", scanner.next_ptr(), strlen("*")) == 0) {
    representation = scanner.consume(1);
    goto _return;
  }
  _return:
    return representation;
}

bool tokenizer_done () {
  return scanner.done();
}

void _tokenizer_skip_extras () {
  char *representation;
  for (; ; ) {
    _tokenizer_tokenize_whitespace(scanner.next());
    if (representation = _tokenizer_tokenize_comment_single()) {
      free(representation);
      continue;
    }
    if (representation = _tokenizer_tokenize_comment_multi()) {
      free(representation);
      continue;
    }
    break;
  }
}

bool _tokenizer_is_digit_string (char *string) {
  size_t length = 0;
  while (isdigit(string[length])) length++;
  return length == strlen(string);
}

token_type_secondary_t _tokenizer_representation_to_secondary (char *representation) {
  if (strcmp("if", representation) == 0) {
    return KEYWORD_IF;
  }
  if (strcmp("iterate", representation) == 0) {
    return KEYWORD_ITERATE;
  }
  if (strcmp("as", representation) == 0) {
    return KEYWORD_AS;
  }
  if (strcmp("var", representation) == 0) {
    return KEYWORD_VAR;
  }
  if (strcmp("+", representation) == 0) {
    return OPERATOR_PLUS;
  }
  if (_tokenizer_is_digit_string(representation)) {
    return LITERAL_INTEGER;
  }
  return UNKNOWN_SECONDARY;
}

token_t *tokenizer_peek (size_t ahead) {
  size_t index = _tokenizer_tokens_index + ahead;
  if (index >= _tokenizer_tokens_size) {
    return NULL;
  } 
  return _tokenizer_tokens[index];
}

token_t **tokenizer_consume (size_t num_tokens) {
  if (num_tokens == 0 || _tokenizer_tokens_index >= _tokenizer_tokens_size) {
    return NULL;
  }
  if (_tokenizer_tokens_index + num_tokens >= _tokenizer_tokens_size) {
    num_tokens = _tokenizer_tokens_size - _tokenizer_tokens_index;
  }
  token_t **tokens = malloc(num_tokens * sizeof(token_t *));
  for (int i = 0; i < num_tokens; i++) {
    token_t *token = malloc(sizeof(token_t));
    memcpy(token, _tokenizer_tokens[_tokenizer_tokens_index + i], sizeof(token_t));
    tokens[i] = token;
  }
  _tokenizer_tokens_index += num_tokens;
  return tokens;
}

token_t *_tokenizer_next () {
  char *representation = 0;
  token_t *token = NULL;

  _tokenizer_skip_extras();

  if ((representation = _tokenizer_tokenize_keyword()) != NULL) {
    token = malloc(sizeof(token_t));
    token->representation = representation;
    token->token_type_primary = KEYWORD;
    token->token_type_secondary = _tokenizer_representation_to_secondary(representation);
    goto _return;
  }
  if ((representation = _tokenizer_tokenize_identifier()) != NULL) {
    token = malloc(sizeof(token_t));
    token->representation = representation;
    token->token_type_primary = IDENTIFIER;
    token->token_type_secondary = _tokenizer_representation_to_secondary(representation);
    goto _return;
  }
  if ((representation = _tokenizer_tokenize_integer_literal()) != NULL) {
    token = malloc(sizeof(token_t));
    token->representation = representation;
    token->token_type_primary = LITERAL;
    token->token_type_secondary = _tokenizer_representation_to_secondary(representation);
    goto _return;
  }
  if ((representation = _tokenizer_tokenize_operator()) != NULL) {
    token = malloc(sizeof(token_t));
    token->representation = representation;
    token->token_type_primary = OPERATOR;
    token->token_type_secondary = _tokenizer_representation_to_secondary(representation);
    goto _return;
  }

  _return:
  return token;
}

char *tokenizer_token_as_string (token_t *token) {
  char *string = malloc(64);
  char *representation = token->representation;
  char *type_primary = malloc(64);
  char *type_secondary = malloc(64);
  switch (token->token_type_primary) {
    case (KEYWORD):
      strncpy(type_primary, "keyword", strlen("keyword"));
      type_primary[strlen("keyword") + 1] = '\0';
      break;
    case (IDENTIFIER):
      strncpy(type_primary, "identifier", strlen("identifier"));
      type_primary[strlen("identifier") + 1] = '\0';
      break;
    case (LITERAL):
      strncpy(type_primary, "literal", strlen("literal"));
      type_primary[strlen("literal") + 1] = '\0';
      break;
    case (OPERATOR):
      strncpy(type_primary, "operator", strlen("operator"));
      type_primary[strlen("operator") + 1] = '\0';
      break;
    case (GROUPING):
      strncpy(type_primary, "grouping", strlen("grouping"));
      type_primary[strlen("grouping") + 1] = '\0';
      break;
    case (DELIMITER):
      strncpy(type_primary, "delimiter", strlen("delimiter"));
      type_primary[strlen("delimiter") + 1] = '\0';
      break;
    default:
      strncpy(type_primary, "unknown", strlen("unknown"));
      type_primary[strlen("unknown") + 1] = '\0';
  }
  switch (token->token_type_secondary) {
    case (KEYWORD_IF):
      strncpy(type_secondary, "keyword_if", strlen("keyword_if"));
      type_secondary[strlen("keyword_if") + 1] = '\0';
      break;
    case (KEYWORD_ITERATE):
      strncpy(type_secondary, "keyword_iterate", strlen("keyword_iterate"));
      type_secondary[strlen("keyword_iterate") + 1] = '\0';
      break;
    case (KEYWORD_AS):
      strncpy(type_secondary, "keyword_as", strlen("keyword_as"));
      type_secondary[strlen("keyword_as") + 1] = '\0';
      break;
    case (KEYWORD_VAR):
      strncpy(type_secondary, "keyword_var", strlen("keyword_var"));
      type_secondary[strlen("keyword_var") + 1] = '\0';
      break;
    case (LITERAL_QUOTE_D):
      strncpy(type_secondary, "literal_quote_d", strlen("literal_quote_d"));
      type_secondary[strlen("literal_quote_d") + 1] = '\0';
      break;
    case (LITERAL_QUOTE_S):
      strncpy(type_secondary, "literal_quote_s", strlen("literal_quote_s"));
      type_secondary[strlen("literal_quote_s") + 1] = '\0';
      break;
    case (LITERAL_QUOTE_B):
      strncpy(type_secondary, "literal_quote_b", strlen("literal_quote_b"));
      type_secondary[strlen("literal_quote_b") + 1] = '\0';
      break;
    case (LITERAL_INTEGER):
      strncpy(type_secondary, "literal_integer", strlen("literal_integer"));
      type_secondary[strlen("literal_integer") + 1] = '\0';
      break;
    case (LITERAL_DOUBLE):
      strncpy(type_secondary, "literal_double", strlen("literal_double"));
      type_secondary[strlen("literal_double") + 1] = '\0';
      break;
    case (LITERAL_BOOLEAN):
      strncpy(type_secondary, "literal_boolean", strlen("literal_boolean"));
      type_secondary[strlen("literal_boolean") + 1] = '\0';
      break;
    case (OPERATOR_STAR):
      strncpy(type_secondary, "operator_star", strlen("operator_star"));
      type_secondary[strlen("operator_star") + 1] = '\0';
      break;
    case (OPERATOR_STARSTAR):
      strncpy(type_secondary, "operator_starstar", strlen("operator_starstar"));
      type_secondary[strlen("operator_starstar") + 1] = '\0';
      break;
    case (OPERATOR_PLUS):
      strncpy(type_secondary, "operator_plus", strlen("operator_plus"));
      type_secondary[strlen("operator_plus") + 1] = '\0';
      break;
    case (OPERATOR_MINUS):
      strncpy(type_secondary, "operator_minus", strlen("operator_minus"));
      type_secondary[strlen("operator_minus") + 1] = '\0';
      break;
    case (OPERATOR_FSLASH):
      strncpy(type_secondary, "operator_fslash", strlen("operator_fslash"));
      type_secondary[strlen("operator_fslash") + 1] = '\0';
      break;
    case (OPERATOR_COLON):
      strncpy(type_secondary, "operator_colon", strlen("operator_colon"));
      type_secondary[strlen("operator_colon") + 1] = '\0';
      break;
    case (OPERATOR_COLONCOLON):
      strncpy(type_secondary, "operator_coloncolon", strlen("operator_coloncolon"));
      type_secondary[strlen("operator_coloncolon") + 1] = '\0';
      break;
    case (OPERATOR_ARROW_R):
      strncpy(type_secondary, "operator_arrow_r", strlen("operator_arrow_r"));
      type_secondary[strlen("operator_arrow_r") + 1] = '\0';
      break;
    case (OPERATOR_ARROW_L):
      strncpy(type_secondary, "operator_arrow_l", strlen("operator_arrow_l"));
      type_secondary[strlen("operator_arrow_l") + 1] = '\0';
      break;
    case (GROUPING_BRACE_L):
      strncpy(type_secondary, "grouping_brace_l", strlen("grouping_brace_l"));
      type_secondary[strlen("grouping_brace_l") + 1] = '\0';
      break;
    case (GROUPING_BRACKET_L):
      strncpy(type_secondary, "grouping_bracket_l", strlen("grouping_bracket_l"));
      type_secondary[strlen("grouping_bracket_l") + 1] = '\0';
      break;
    case (GROUPING_PAREN_L):
      strncpy(type_secondary, "grouping_paren_l", strlen("grouping_paren_l"));
      type_secondary[strlen("grouping_paren_l") + 1] = '\0';
      break;
    case (GROUPING_BRACE_R):
      strncpy(type_secondary, "grouping_brace_r", strlen("grouping_brace_r"));
      type_secondary[strlen("grouping_brace_r") + 1] = '\0';
      break;
    case (GROUPING_BRACKET_R):
      strncpy(type_secondary, "grouping_bracket_r", strlen("grouping_bracket_r"));
      type_secondary[strlen("grouping_bracket_r") + 1] = '\0';
      break;
    case (GROUPING_PAREN_R):
      strncpy(type_secondary, "grouping_paren_r", strlen("grouping_paren_r"));
      type_secondary[strlen("grouping_paren_r") + 1] = '\0';
      break;
    case (DELIMITER_SEMI):
      strncpy(type_secondary, "delimiter_semi", strlen("delimiter_semi"));
      type_secondary[strlen("delimiter_semi") + 1] = '\0';
    case (DELIMITER_COMMA):
      strncpy(type_secondary, "delimiter_comma", strlen("delimiter_comma"));
      type_secondary[strlen("delimiter_comma") + 1] = '\0';
      break;
    default:
      strncpy(type_secondary, "unknown", strlen("unknown"));
      type_secondary[strlen("unknown") + 1] = '\0';
      break;
  }
  sprintf(string, "(token %s/%s \"%s\")", type_primary, type_secondary, representation);
  return string;
}

void tokenizer_cleanup () {

}

token_t *tokenizer_next () {
  return _tokenizer_tokens[_tokenizer_tokens_index];
}

void _tokenizer_tokenize () {
  scanner.scan();
  token_t **tokens = malloc(sizeof(token_t *));
  size_t tokens_size = 1;
  size_t tokens_index = 0;
  token_t *token;
  while (token = _tokenizer_next()) {
    if (tokens_index == tokens_size) {
      tokens_size = tokens_size * 2;
      tokens = realloc(tokens, tokens_size * sizeof(token_t *));
    }
    tokens[tokens_index] = token;
    tokens_index++;
  }
  _tokenizer_tokens = tokens;
  _tokenizer_tokens_size = tokens_index;
  scanner.cleanup();
}

void tokenizer_initialize () {
  _tokenizer_tokenize();
}

void setup_tokenizer () {
  tokenizer.initialize = tokenizer_initialize;
  tokenizer.next = tokenizer_next;
  tokenizer.peek = tokenizer_peek;
  tokenizer.consume = tokenizer_consume;
  tokenizer.done = tokenizer_done;
  tokenizer.token_as_string = tokenizer_token_as_string;
  tokenizer.cleanup = tokenizer_cleanup;
}

/* end tokenizer */

/* ``begin parser */

node_t *_parser_ast = NULL;

void _parser_expect_secondary (token_type_secondary_t type) {
  token_t *token;
  token = tokenizer.next();
  if (!token) {
    return;
  }
  if (token->token_type_secondary != type) {
    fprintf(stderr, "Parsing error; unexpected \"%s\".\n", tokenizer.token_as_string(token));
    exit(1);
  }
}

void _parser_expect_primary (token_type_primary_t type) {
  token_t *token;
  token = tokenizer.next();
  if (!token) {
    return;
  }
  if (token->token_type_primary != type) {
    fprintf(stderr, "Parsing error; unexpected \"%s\".\n", tokenizer.token_as_string(token));
    exit(1);
  }
}


node_t *_parser_try_declaration () {
  node_t *node = NULL;
  if (tokenizer.next()->token_type_secondary == KEYWORD_VAR) {
    tokenizer.consume(1); // TODO
    _parser_expect_primary(IDENTIFIER);
    tokenizer.consume(1); // TODO
    _parser_expect_secondary(DELIMITER_SEMI);
    tokenizer.consume(1); // TODO
    node = malloc(sizeof(node_t));
    node->type = strdup("declaration");
    node->num_children = 0;
    node->children = NULL;
    node->parent = NULL;
    node->info_type = NULL;
    node->info = NULL;
    return node;
  }
  return NULL;
}

void parser_parse () {
  node_t *node = NULL;
  if (node = _parser_try_declaration()) {
    _parser_ast = node;
    printf("node->type = \"%s\"\n", node->type);
    return;
  }
  fprintf(stderr, "Parsing error\n");
  exit(1);
}

void parser_cleanup () {

}

void parser_initialize () {

}

void setup_parser () {
  parser.parse = parser_parse;
  parser.cleanup = parser_cleanup;
  parser.initialize = parser_initialize;
}

/* end parser */

/* ``begin begin */

void begin () {
  printf("Interpretting `%s`.\n", glbl_arguments->file_name);
  parser.parse();
  /*token_t **tokens;
  while (tokens = tokenizer.consume(1)) {
    printf("%s\n", tokenizer.token_as_string(tokens[0]));
  }*/
}

/* end begin */

/* ``begin test ll */

void test_ll_iterate () {
  ll_t *list = ll.new();
  for (int i = 0; i < 100; i++) {
    ll.add(list, "hello, world");
  }
  for (int i = 0; i < 100; i++) {
    if (strcmp(ll.get(list, i), "hello, world") != 0) {
      printf("bad str cmp\n");
      TEST_FAIL;
      return;
    }
  }
  for (int i = 0; i < 100; i++) {
    ll.remove(list, ll.size(list) - 1);
  }
  if (ll.get(list, 0)) {
    printf("bad first\n");
    TEST_FAIL;
    return;
  }
  if (ll.size(list)) {
    printf("bad size\n");
    TEST_FAIL;
    return;
  }
  ll.delete(list);
  TEST_PASS;
}

void test_ll_basic () {
  ll_t *list = ll.new();
  if (!list) {
    TEST_FAIL;
    return;
  }
  char *foo = "foo";
  char *bar = "bar";
  ll.add(list, foo);
  ll.add(list, bar);
  if (((char *) ll.get(list, 0)) != foo) {
    TEST_FAIL;
    return;
  }
  if (((char *) ll.get(list, 1)) != bar) {
    TEST_FAIL;
    return;
  }
  ll.remove(list, 0);
  if (((char *) ll.get(list, 0)) != bar) {
    printf("not bar %s\n", (char *) ll.get(list, 0));
    TEST_FAIL;
    return;
  }
  ll.remove(list, 0);
  if (((char *) ll.get(list, 0)) != NULL) {
    TEST_FAIL;
    return;
  }
  ll.remove(list, 0);
  if (((char *) ll.get(list, 0)) != NULL) {
    TEST_FAIL;
    return;
  }
  ll.delete(list);
  TEST_PASS;
}

void test_ll_new_delete () {
  ll_t *list = ll.new();
  if (!list) {
    TEST_FAIL;
    return;
  }
  ll.delete(list);
  list = NULL;
  TEST_PASS;
}

void test_ll () {
  TEST_SUITE;
  test_ll_new_delete();
  test_ll_basic();
  test_ll_iterate();
}

/* end test ll */

/* ``begin test memory */

void test_memory () {
  TEST_SUITE;
  if (J_MEM_DEBUG) {
    printf("%d bytes not free'd\n", (int) (j_mem_size() - glbl_tests_mem_start));
  }
  printf("%d bytes malloc'd\n", (int) j_mem_total_alloc);
  printf("%d bytes free'd\n", (int) j_mem_total_free);
  if (j_mem_size() - glbl_tests_mem_start) {
    TEST_FAIL;
  } else {
    TEST_PASS;
  }
}

/* end test memory */

/* ``begin run_tests */

void run_tests () {
  glbl_tests_mem_start = j_mem_size();
  j_mem_total_alloc = 0;
  j_mem_total_free = 0;
  TESTS_PRELUDE;
  test_ll();
  test_memory();
  TESTS_RESULTS;
}

/* end run_tests */

/* ``begin main */

int main (int argc, char **argv) {
  SETUP_MODULE(ll);
  glbl_arguments = arguments_t_parse_arguments(argc, argv);
  if (!glbl_arguments->valid) {
    if (glbl_arguments->difference_from_correct < 0) {
      fprintf(stderr, "Too few arguments\n");
    }
    if (glbl_arguments->difference_from_correct > 0) {
      fprintf(stderr, "Too many arguments\n");
    }
    printf("usage: ./wtjl <filename> OR ./wtjl --test\n");
    exit(1);
  }
  if (glbl_arguments->test) {
    run_tests();
    exit(0);
  }
  SETUP_MODULE(scanner)
  SETUP_MODULE(tokenizer)
  SETUP_MODULE(parser)
  begin();
  cleanup();
  printf(CLR_YEL "%d bytes still allocated\n" CLR_NRM, (int) j_mem_size());
  return 0;
}

/* end main */

/********************/
/********************/
/* wtjl interpreter */
/********************/
/********************/
/**(C)2018AlexKizer**/
/********************/
/********************/

