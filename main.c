#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <pwd.h>
#include <assert.h>
#include <wchar.h>
#include <locale.h>
#include <sys/wait.h>

#define DB_FILENAME ".wodo.db"
#define MAX_FILENAME_SIZE 255
#define DB_FILEPATH_MAX_BUFFER (MAX_FILENAME_SIZE + MAX_FILENAME_SIZE + MAX_FILENAME_SIZE) // /<home>/<user>/file

// ---------------------------------------- Utils Start ----------------------------------------

static size_t read_file(const char *filename, char **content) {
    FILE *fptr = fopen(filename, "r");

    if (fptr == NULL) {
        fprintf(stderr, "could not open file %s due to: %s\n", filename, strerror(errno));
        exit(1);
    }

    fseek(fptr, 0, SEEK_END);
    const size_t stream_size = ftell(fptr);
    rewind(fptr);

    if (*content != NULL) {
        *content = malloc((stream_size + 1) * sizeof(char));

        if (*content == NULL) {
            fprintf(stderr, "could not allocate memory enough: %s\n", strerror(errno));
            exit(1);
        }

        const size_t read_size = fread(*content, 1, stream_size, fptr);

        if (read_size != stream_size) {
            fprintf(stderr, "could not read file %s due to: %s\n", filename, strerror(errno));
            fclose(fptr);
            exit(1);
        }

        (*content)[stream_size] = '\0';
    }

    fclose(fptr);

    return stream_size;
}

static bool file_exists(const char *filepath) {
    return access(filepath, F_OK) == 0;
}

static const char *get_user_home_folder() {
    const char *home = getenv("HOME");

    if (home == NULL) return getpwuid(getuid())->pw_dir;

    return home;
}

static bool compare_paths(const char *a, const char *b) {
    size_t i = 0;

    while (i < MAX_FILENAME_SIZE) {
        if (a[i] != b[i]) return false;

        if (a[i] == '\0' && b[i] == '\0') return true;

        i++;
    }

    return false;
}

static bool is_number(const char *string) {
    for (size_t i = 0; string[i] != '\0'; ++i)
        if (!(string[i] >= '0' && string[i] <= '9')) return false;

    return true;
}

static inline bool is_symbol(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

// ---------------------------------------- Utils End ----------------------------------------

typedef enum {
    AK_ADD = 1,
    AK_REMOVE,
    AK_VIEW,
    AK_GET,
    AK_OPEN,
} ArgumentKind;

typedef struct {
    ArgumentKind kind;
    const char *value;
} Arguments;

typedef struct DbFile DbFile;

struct DbFile {
    size_t id;
    char *filepath;

    DbFile *next;
};

typedef struct {
    size_t last_id;
    size_t size;
    DbFile *head;
    DbFile *tail;
} Database;

typedef enum {
    TK_TEXT = 0,    // any text like: x, Todo, or "Lesson 011 sdlk", ...
    TK_SPACE,       // just spaces
    TK_SEP,         // default separator
    TK_SYMBOL,      // symbols Like week days
    TK_COLON,       // hour separator
    TK_NUMBER,      // 0123456789
    TK_BR,          // line break
} Token_Kind;

typedef struct Token Token;

struct Token {
    Token_Kind kind;

    const char *value;
    size_t value_size;

    Token *next;
};

typedef struct {
    const char *content;
    size_t content_size, cursor, bot;

    Token *head;
    Token *tail;
} Lexer;

// ---------------------------------------- Database Start ----------------------------------------

static const char *get_database_filepath() {
    const char *user_home_folder = get_user_home_folder();

    static char database_filepath[DB_FILEPATH_MAX_BUFFER];

    snprintf(database_filepath, sizeof(database_filepath), "%s/%s", user_home_folder, DB_FILENAME);

    return database_filepath;
}

static void load_filepath_on_database(Database *database, char *filepath, size_t id) {
    DbFile *file = malloc(sizeof(DbFile));

    if (file == NULL) {
        fprintf(stderr, "ERROR: could not load file \"%s\" to database: %s\n", filepath, strerror(errno));
        exit(1);
    }

    file->id = id;
    file->filepath = strdup(filepath);

    if (file->filepath == NULL) {
        fprintf(stderr, "ERROR: could not allocate memory for file path: %s\n", strerror(errno));
        exit(1);
    }

    file->next = NULL;

    if (database->head == NULL) {
        database->head = file;
        database->tail = file;
    } else {
        database->tail = database->tail->next = file;
    }
}

static Database load_database() {
    const char *database_filepath = get_database_filepath();

    Database database = {0};

    if (file_exists(database_filepath)) {
        FILE *file = fopen(database_filepath, "rb");

        if (file == NULL) {
            fprintf(stderr, "could not open database file: %s\n", strerror(errno));
            exit(1);
        }

        fread(&database.size, sizeof(size_t), 1, file);
        fread(&database.last_id, sizeof(size_t), 1, file);

        for (size_t i = 0; i < database.size; ++i) {
            size_t filepath_size;
            size_t id;

            fread(&id, sizeof(size_t), 1, file);
            fread(&filepath_size, sizeof(size_t), 1, file);

            char *filepath = malloc(filepath_size);

            if (filepath == NULL) {
                fprintf(stderr, "coult not allocate %ld byte for db file path: %s\n", filepath_size, strerror(errno));
                exit(1);
            }

            fread(filepath, sizeof(char), filepath_size, file);

            load_filepath_on_database(&database, filepath, id);

            free(filepath);
        }
    }

    return database;
}

static void save_database(Database *database) {
    const char *database_filepath = get_database_filepath();

    FILE *file = fopen(database_filepath, "wb");

    if (file == NULL) {
        fprintf(stderr, "could not open database file: %s\n", strerror(errno));
        exit(1);
    }

    fwrite(&database->size, sizeof(size_t), 1, file);
    fwrite(&database->last_id, sizeof(size_t), 1, file);

    for (DbFile *db_file = database->head; db_file != NULL; db_file = db_file->next) {
        size_t filepath_size = strlen(db_file->filepath) + 1;

        fwrite(&db_file->id, sizeof(size_t), 1, file);
        fwrite(&filepath_size, sizeof(size_t), 1, file);
        fwrite(db_file->filepath, sizeof(char), filepath_size, file);
    }

    fclose(file);
}

static size_t gen_id(Database *database) {
    return ++database->last_id;
}

static void free_database(Database *database) {
    DbFile *file = database->head;

    while (file != NULL) {
        DbFile *next = file->next;

        free(file->filepath);
        free(file);

        file = next;
    }
}

static bool filepath_exists_on_database(Database *database, const char *filepath) {
    for (DbFile *file = database->head; file != NULL; file = file->next)
        if (compare_paths(filepath, file->filepath)) return true;

    return false;
}

static DbFile *get_database_file_by_id(const Database *database, size_t id) {
    for (DbFile *file = database->head; file != NULL; file = file->next)
        if (file->id == id) return file;

    return NULL;
}

static void add_file_to_database(Database *database, const char *filepath) {
    DbFile *file = malloc(sizeof(DbFile));

    if (file == NULL) {
        fprintf(stderr, "ERROR: could not add file \"%s\" to database: %s\n", filepath, strerror(errno));
        exit(1);
    }

    file->filepath = strdup(filepath);

    if (file->filepath == NULL) {
        fprintf(stderr, "ERROR: could not allocate memory for file path: %s\n", strerror(errno));
        exit(1);
    }

    file->id = gen_id(database);
    file->next = NULL;

    if (database->head == NULL) {
        database->head = file;
        database->tail = file;
    } else {
        database->tail = database->tail->next = file;
    }

    database->size++;
}

// ---------------------------------------- Database End ----------------------------------------

// ---------------------------------------- Lexer Start ----------------------------------------

static Lexer create_lexer(const char *content, size_t content_size) {
    return (Lexer){
        .content = content,
        .content_size = content_size,
        .cursor = 0,
        .bot = 0
    };
}

static char chr(Lexer *lexer) {
    return lexer->content[lexer->cursor];
}

static inline bool is_digit(char chr) {
    return chr >= '0' && chr <= '9';
}

static inline void advance_cursor(Lexer *lexer) {
    if (lexer->cursor < lexer->content_size) lexer->cursor++;
}

static void save_token(Lexer *lexer, Token_Kind kind) {
    Token *token = malloc(sizeof(Token));

    token->value = lexer->content + lexer->bot;
    token->value_size = lexer->cursor - lexer->bot;
    token->kind = kind;
    token->next = NULL;

    if (lexer->head == NULL) {
        lexer->head = token;
        lexer->tail = token;
    } else {
        lexer->tail = lexer->tail->next = token;
    }
}

static void tokenize_space(Lexer *lexer) {
    while (chr(lexer) == ' ') advance_cursor(lexer);

    save_token(lexer, TK_SPACE);
}

static void tokenize_symbol(Lexer *lexer) {
    while (is_symbol(chr(lexer))) advance_cursor(lexer);

    save_token(lexer, TK_SYMBOL);
}

static void tokenize_br(Lexer *lexer) {
    advance_cursor(lexer);

    save_token(lexer, TK_BR);
}

static void tokenize_single(Lexer *lexer, Token_Kind kind) {
    advance_cursor(lexer);

    save_token(lexer, kind);
}

static void tokenize_number(Lexer *lexer) {
    while (is_digit(chr(lexer))) advance_cursor(lexer);

    save_token(lexer, TK_NUMBER);
}

static void tokenize_text(Lexer *lexer) {
    do advance_cursor(lexer); while (chr(lexer) != '"');

    advance_cursor(lexer);

    save_token(lexer, TK_TEXT);
}

static Token *tokenize(Lexer *lexer) {
    while (lexer->cursor < lexer->content_size) {
        lexer->bot = lexer->cursor;

        switch (chr(lexer)) {
            case ' ': tokenize_space(lexer); break;
            case '"': tokenize_text(lexer); break;
            case '-': tokenize_single(lexer, TK_SEP); break;
            case ':': tokenize_single(lexer, TK_COLON); break;
            case '0': case '1': case '2':
            case '3': case '4': case '5':
            case '6': case '7': case '8':
            case '9': tokenize_number(lexer); break;
            case '\n': tokenize_br(lexer); break;
            default: {
                if (is_symbol(chr(lexer))) {
                    tokenize_symbol(lexer);
                    break;
                }

                fprintf(stderr, "\033[1;31merror\033[0m invalid file format. unexpected char \"%c\"\n", chr(lexer));
                exit(1);
            }
        }
    }

    return lexer->head;
}

// ---------------------------------------- Lexer End ----------------------------------------

// ---------------------------------------- Parser Start ----------------------------------------

typedef enum {
    S_TODO,
    S_IN_PROGRESS,
    S_DONE,
} State;

typedef enum {
    WD_SUNDAY = 0,
    WD_MONDAY,
    WD_TUESDAY,
    WD_WEDNESDAY,
    WD_THURSDAY,
    WD_FRIDAY,
    WD_SATURDAY,
} Week_Day;

typedef struct {
    char hour[2];
    char minute[2];
} Time;

typedef struct {
    char day[2];
    char month[2];
    char year[2];
} Date;

typedef struct Line Line;

struct Line {
    State state;
    Week_Day week_day;

    Date date;
    Time start;
    Time end;

    const char *text;
    size_t text_size;

    Line *next;
};

typedef struct {
    Line *head;
    Line *tail;

    Token *tokens;
} Parser;

// Week days
static const char *sunday = "Sunday";
static const size_t sunday_size = 6;
static const char *monday = "Monday";
static const size_t monday_size = 6;
static const char *tuesday = "Tuesday";
static const size_t tuesday_size = 7;
static const char *wednesday = "Wednesday";
static const size_t wednesday_size = 9;
static const char *thursday = "Thursday";
static const size_t thursday_size = 8;
static const char *friday = "Friday";
static const size_t friday_size = 6;
static const char *saturday = "Saturday";
static const size_t saturday_size = 8;

// States
static const char *todo = "Todo";
static const size_t todo_size = 4;
static const char *in_progress = "Doing";
static const size_t in_progress_size = 5;
static const char *done = "Done";
static const size_t done_size = 4;

static bool cmp_sized_strings(const char *a, const char *b, size_t len_a, size_t len_b) {
    if (len_a != len_b) return false;

    for (size_t i = 0; i < len_a; ++i)
        if (a[i] != b[i]) return false;

    return true;
}

static int parse_char_to_number(char n) {
    switch (n) {
        case '0': return 0;
        case '1': return 1;
        case '2': return 2;
        case '3': return 3;
        case '4': return 4;
        case '5': return 5;
        case '6': return 6;
        case '7': return 7;
        case '8': return 8;
        case '9': return 9;
        default:
            fprintf(stderr, "\033[1;31merror\033[0m invalid file format. invalid digit %c\n", n);
            exit(1);
    }

    return 0;
}

static int parse_year(const char *year, size_t size) {
    if (size != 2) {
        fprintf(stderr, "\033[1;31merror\033[0m invalid file format. invalid year format %.*s\n", (int)size, year);
        exit(1);
    }

    char a = parse_char_to_number(year[0]);
    char b = parse_char_to_number(year[1]);


    return a * 10 + b;
}

static int parse_month(const char *month, size_t size) {
    if (size != 2) {
        fprintf(stderr, "\033[1;31merror\033[0m invalid file format. invalid month format %.*s\n", (int)size, month);
        exit(1);
    }

    char a = parse_char_to_number(month[0]);
    char b = parse_char_to_number(month[1]);

    int n = a * 10 + b;

    if (n < 1 || n > 12) {
        fprintf(stderr, "\033[1;31merror\033[0m invalid file format. invalid month %.*s\n", (int)size, month);
        exit(1);
    }

    return n;
}

static int parse_month_day(const char *string, size_t size) {
    if (size != 2) {
        fprintf(stderr, "\033[1;31merror\033[0m invalid file format. invalid day format %.*s\n", (int)size, string);
        exit(1);
    }

    char a = string[0];
    char b = string[1];

    int v = parse_char_to_number(a) * 10;

    switch (a) {
        case '0':
        case '1':
        case '2':
            v += parse_char_to_number(b);
            break;
        case '3': {
            int n = parse_char_to_number(b);

            if (n > 1) {
                fprintf(stderr, "\033[1;31merror\033[0m invalid file format. invalid month day %c%c\n", a, b);
                exit(1);
            }

            v += n;

            break;
        }
        default:
            fprintf(stderr, "\033[1;31merror\033[0m invalid file format. unexpected month day %.*s\n", 2, string);
            exit(1);
    }

    return v;
}

static int parse_hour(const char *string, size_t size) {
    if (size != 2) {
        fprintf(stderr, "\033[1;31merror\033[0m invalid file format. invalid hour format %.*s\n", (int)size, string);
        exit(1);
    }

    char a = string[0];
    char b = string[1];

    int an = parse_char_to_number(a);

    if (an > 2) {
        fprintf(stderr, "\033[1;31merror\033[0m invalid file format. invalid hour %c%c\n", a, b);
        exit(1);
    }

    int bn = parse_char_to_number(a);

    if (an == 2 && bn > 3) {
        fprintf(stderr, "\033[1;31merror\033[0m invalid file format. invalid hour %c%c\n", a, b);
        exit(1);
    }

    return an * 10 + bn;
}

static int parse_minute(const char *string, size_t size) {
    if (size != 2) {
        fprintf(stderr, "\033[1;31merror\033[0m invalid file format. invalid minute format %.*s\n", (int)size, string);
        exit(1);
    }

    char a = string[0];
    char b = string[1];

    int an = parse_char_to_number(a);

    if (an > 5) {
        fprintf(stderr, "\033[1;31merror\033[0m invalid file format. invalid minute %c%c\n", a, b);
        exit(1);
    }

    int bn = parse_char_to_number(a);

    return an * 10 + bn;
}

static Parser create_parser(Token *tokens) {
    return (Parser){
        .head = NULL,
        .tail = NULL,
        .tokens = tokens,
    };
}

static Token *expect_kind(Parser *parser, Token_Kind kind) {
    Token *token = parser->tokens;

    if (token->kind != kind) {
        fprintf(stderr, "\033[1;31merror\033[0m invalid file format. unexpected token \"%.*s\"\n", (int)token->value_size, token->value);
        exit(1);
    }

    parser->tokens = parser->tokens->next;

    return token;
}

static Token *consume_token(Parser *parser) {
    if (parser->tokens == NULL) return NULL;

    Token *token = parser->tokens;

    parser->tokens = parser->tokens->next;

    return token;
}

static Week_Day parse_week_day(const char *sized_string, size_t size) {
    if (cmp_sized_strings(sunday, sized_string, sunday_size, size)) {
        return WD_SUNDAY;
    } else if (cmp_sized_strings(monday, sized_string, monday_size, size)) {
        return WD_MONDAY;
    } else if (cmp_sized_strings(tuesday, sized_string, tuesday_size, size)) {
        return WD_TUESDAY;
    } else if (cmp_sized_strings(wednesday, sized_string, wednesday_size, size)) {
        return WD_WEDNESDAY;
    } else if (cmp_sized_strings(thursday, sized_string, thursday_size, size)) {
        return WD_THURSDAY;
    } else if (cmp_sized_strings(friday, sized_string, friday_size, size)) {
        return WD_FRIDAY;
    } else if (cmp_sized_strings(saturday, sized_string, saturday_size, size)) {
        return WD_SATURDAY;
    } else {
        fprintf(stderr, "\033[1;31merror\033[0m invalid file format. unrecognized week day \"%.*s\"\n", (int)size, sized_string);
        exit(1);
    }

    return -1;
}

static State parse_state(Parser *parser) {
    Token *statet = expect_kind(parser, TK_SYMBOL);

    if (cmp_sized_strings(todo, statet->value, todo_size, statet->value_size)) {
        return S_TODO;
    } else if (cmp_sized_strings(in_progress, statet->value, in_progress_size, statet->value_size)) {
        return S_IN_PROGRESS;
    } else if (cmp_sized_strings(done, statet->value, done_size, statet->value_size)) {
        return S_DONE;
    } else {
        fprintf(stderr, "\033[1;31merror\033[0m invalid file format. unrecognized state \"%.*s\"\n", (int)statet->value_size, statet->value);
        exit(1);
    }

    return -1;
}

static Date parse_date(Parser *parser) {
    Date date = {0};

    Token *dayt = expect_kind(parser, TK_NUMBER);
    expect_kind(parser, TK_SEP);
    Token *montht = expect_kind(parser, TK_NUMBER);
    expect_kind(parser, TK_SEP);
    Token *yeart = expect_kind(parser, TK_NUMBER);

    parse_month_day(dayt->value, dayt->value_size);
    date.day[0] = dayt->value[0];
    date.day[1] = dayt->value[1];

    parse_month(montht->value, montht->value_size);
    date.month[0] = montht->value[0];
    date.month[1] = montht->value[1];

    parse_year(yeart->value, yeart->value_size);
    date.year[0] = yeart->value[0];
    date.year[1] = yeart->value[1];

    return date;
}

static Time parse_time(Parser *parser) {
    Time time = {0};

    Token *hourt = expect_kind(parser, TK_NUMBER);
    expect_kind(parser, TK_COLON);
    Token *minutet = expect_kind(parser, TK_NUMBER);

    parse_hour(hourt->value, hourt->value_size);
    time.hour[0] = hourt->value[0];
    time.hour[1] = hourt->value[1];

    parse_minute(minutet->value, minutet->value_size);
    time.minute[0] = minutet->value[0];
    time.minute[1] = minutet->value[1];

    return time;
}

static Line *parse_line(Parser *parser) {
    Line *line = malloc(sizeof(Line));

    line->next = NULL;

    // parse text
    Token *text = expect_kind(parser, TK_TEXT);

    line->text = text->value + 1;
    line->text_size = text->value_size - 2;

    expect_kind(parser, TK_SPACE);

    // parse week day
    Token *weekday = expect_kind(parser, TK_SYMBOL);

    line->week_day = parse_week_day(weekday->value, weekday->value_size);

    expect_kind(parser, TK_SPACE);

    line->date = parse_date(parser);

    expect_kind(parser, TK_SPACE);

    line->start = parse_time(parser);

    if (parser->tokens != NULL && parser->tokens->kind == TK_SPACE) consume_token(parser);
    expect_kind(parser, TK_SEP);
    if (parser->tokens != NULL && parser->tokens->kind == TK_SPACE) consume_token(parser);

    line->end = parse_time(parser);

    expect_kind(parser, TK_SPACE);

    line->state = parse_state(parser);

    return line;
}

static Line *parse_lines(Parser *parser) {
    while (parser->tokens != NULL) {
        if (parser->tokens->kind == TK_BR) {
            parser->tokens = parser->tokens->next;
            continue;
        }

        Line *line = parse_line(parser);

        if (parser->head == NULL) {
            parser->head = parser->tail = line;
        } else {
            parser->tail = parser->tail->next = line;
        }
    }

    return parser->head;
}

// ---------------------------------------- Parser End ----------------------------------------

// ---------------------------------------- Compile Start ----------------------------------------

static Line *compile_file(const char *filepath, char **content) {
    size_t file_size = read_file(filepath, content);

    Lexer lexer = create_lexer(*content, file_size);

    Token *tokens = tokenize(&lexer);

    Parser parser = create_parser(tokens);

    return parse_lines(&parser);
}

// ---------------------------------------- Compile End ----------------------------------------

// ---------------------------------------- Data View Start ----------------------------------------

static void display_lines(Line *lines, char *padding) {
    size_t max_string_size = 0;

    for (Line *line = lines; line != NULL; line = line->next) {
        if (line->text_size > max_string_size)
            max_string_size = line->text_size;
    }

    for (Line *line = lines; line != NULL; line = line->next) {
        printf("%s", padding);
        printf("%.*s    %*.s", (int)line->text_size, line->text, (int)(max_string_size - line->text_size), "");

        switch (line->week_day) {
            case WD_SUNDAY:    printf("Sunday   "); break;
            case WD_MONDAY:    printf("Monday   "); break;
            case WD_TUESDAY:   printf("Tuesday  "); break;
            case WD_WEDNESDAY: printf("Wednesday"); break;
            case WD_THURSDAY:  printf("Thursday "); break;
            case WD_FRIDAY:    printf("Friday   "); break;
            case WD_SATURDAY:  printf("Saturday "); break;
        }

        printf(" ");

        printf("%c%c", line->date.day[0], line->date.day[1]);
        printf("-");
        printf("%c%c", line->date.month[0], line->date.month[1]);
        printf("-");
        printf("%c%c", line->date.year[0], line->date.year[1]);
        printf(" ");

        printf("%c%c:%c%c - %c%c:%c%c",
                line->start.hour[0], line->start.hour[1],
                line->start.minute[0], line->start.minute[1],
                line->end.hour[0], line->end.hour[1],
                line->end.minute[0], line->end.minute[1]);

        printf(" ");

        switch (line->state) {
            case S_TODO: printf("Todo"); break;
            case S_IN_PROGRESS: printf("Doing"); break;
            case S_DONE: printf("Done"); break;
        }

        printf("\n");
    }
}

// ---------------------------------------- Data View End ----------------------------------------

static void usage(FILE *stream, const char *program_name, char *error_message, ...) {
    va_list args;

    fprintf(stream, "%s [add|remove|get|view|open|help]\n", program_name);
    fprintf(stream, "\n");
    fprintf(stream, "    add     a    [filename]           add a new file to the tracking system\n");
    fprintf(stream, "    remove  r    [filename|id]        remove a file from the tracking system\n");
    fprintf(stream, "    get     g    [filename|id]        get specific file by path or id\n");
    fprintf(stream, "    open    o    [filename|id]        open a file using vim by path or id\n");
    fprintf(stream, "    view    v                         view files\n");
    fprintf(stream, "    help    h                         display this message\n");

    if (error_message != NULL) {
        va_start(args, error_message);
        fprintf(stream, "\n\033[1;31merror: \033[0m");
        vfprintf(stream, error_message, args);
        fprintf(stream, "\n");
        va_end(args);
    }
}

static char *shift(int *argc, char ***argv) {
    if (*argc == 0) {
        return NULL;
    }

    (*argc)--;

    return *((*argv)++);
}

static inline bool arg_cmp(const char *actual, const char *expected, const char *alternative) {
    return strncmp(actual, expected, strlen(actual)) == 0 || strncmp(actual, alternative, strlen(actual)) == 0;
}

static int add_action(const char *filename) {
    Database database = load_database();

    char *abs_path = realpath(filename, NULL);

    if (abs_path == NULL) {
        fprintf(stderr, "\033[1;31merror:\033[0m invalid filename\n");

        return 1;
    }

    if (filepath_exists_on_database(&database, abs_path)) {
        fprintf(stderr, "\033[1;31merror:\033[0m you already have this file added\n");
        return 1;
    }

    char *content;

    compile_file(abs_path, &content);

    add_file_to_database(&database, abs_path);

    save_database(&database);

    free(content);
    free_database(&database);
    free(abs_path);

    return 0;
}

// `file_identifier` can be an Id or the path to the file
static int remove_action(const char *file_identifier) {
    if (is_number(file_identifier)) {
        size_t id = strtoul(file_identifier, NULL, 10);

        Database database = load_database();

        // TODO: abstract this to a function
        DbFile *slow = NULL;
        DbFile *fast = database.head;

        while (fast != NULL) {
            if (fast->id == id) {
                if (slow == NULL) {
                    database.head = fast->next;
                } else {
                    slow->next = fast->next;
                }

                if (database.tail == fast) {
                    database.tail = slow;
                }

                free(fast->filepath);
                free(fast);

                database.size--;

                break;
            }

            slow = fast;
            fast = fast->next;
        }

        save_database(&database);
        free_database(&database);

        return 0;
    }

    Database database = load_database();

    char *abs_path = realpath(file_identifier, NULL);

    if (abs_path == NULL) {
        fprintf(stderr, "\033[1;31merror:\033[0m invalid filename\n");

        return 1;
    }

    // TODO: abstract this to a function
    DbFile *slow = NULL;
    DbFile *fast = database.head;

    while (fast != NULL) {
        if (compare_paths(fast->filepath, abs_path)) {
            if (slow == NULL) {
                database.head = fast->next;
            } else {
                slow->next = fast->next;
            }

            if (database.tail == fast) {
                database.tail = slow;
            }

            free(fast->filepath);
            free(fast);

            database.size--;

            break;
        }

        slow = fast;
        fast = fast->next;
    }

    save_database(&database);
    free_database(&database);
    free(abs_path);

    return 0;
}

static int view_action() {
    Database database = load_database();

    if (database.size == 0) {
        printf("There is no file yet\n");

        return 0;
    }

    printf("Files:\n\n");

    for (DbFile *file = database.head; file != NULL; file = file->next) {
        printf("    [ID:%ld] %s\n", file->id, file->filepath);

        char *content;

        Line *lines = compile_file(file->filepath, &content);

        printf("\n");
        display_lines(lines, "        ");

        if (file->next != NULL) {
            printf("\n");
        }
        
        free(content);
    }
    printf("\n");

    return 0;
}

// `file_identifier` can be an Id or the path to the file
static int get_action(const char *program_name, const char *file_identifier) {
    if (is_number(file_identifier)) {
        size_t id = strtoul(file_identifier, NULL, 10);

        Database database = load_database();

        const DbFile *file = get_database_file_by_id(&database, id);

        if (file == NULL) {
            fprintf(stderr, "\033[1;31merror:\033[0m there is no file with id %s in the database\n", file_identifier);
            return 1;
        }

        char *content;

        Line *lines = compile_file(file->filepath, &content);

        display_lines(lines, "");

        free_database(&database);

        return 0;
    }

    char *abs_path = realpath(file_identifier, NULL);

    if (abs_path == NULL) {
        fprintf(stderr, "\033[1;31merror:\033[0m invalid filename\n");

        return 1;
    }

    Database database = load_database();

    if (!filepath_exists_on_database(&database, abs_path)) {
        fprintf(stderr, "\033[1;31merror:\033[0m the file %s does not exists in the database\n", file_identifier);
        fprintf(stderr, "use \"%s add %s\" to add this file in the database\n", program_name, file_identifier);
        return 1;
    }

    char *content;

    Line *lines = compile_file(abs_path, &content);

    display_lines(lines, "");

    free_database(&database);
    
    return 0;
}

static void open_vim_at(const char *filepath) {
    pid_t pid = fork();

    if (pid == 0) {
        execlp("vim", "vim", filepath, (char *)NULL);
    } else {
        wait(NULL);
    }
}

static int open_action(const char *program_name, const char *file_identifier) {
    if (is_number(file_identifier)) {
        size_t id = strtoul(file_identifier, NULL, 10);

        Database database = load_database();

        const DbFile *file = get_database_file_by_id(&database, id);

        if (file == NULL) {
            fprintf(stderr, "\033[1;31merror:\033[0m there is no file with id %s in the database\n", file_identifier);
            return 1;
        }

        open_vim_at(file->filepath);

        free_database(&database);
        return 0;
    }

    char *abs_path = realpath(file_identifier, NULL);

    if (abs_path == NULL) {
        fprintf(stderr, "\033[1;31merror:\033[0m invalid filename\n");

        return 1;
    }

    Database database = load_database();

    if (!filepath_exists_on_database(&database, abs_path)) {
        fprintf(stderr, "\033[1;31merror:\033[0m the file %s does not exists in the database\n", file_identifier);
        fprintf(stderr, "use \"%s add %s\" to add this file in the database\n", program_name, file_identifier);
        return 1;
    }

    open_vim_at(abs_path);

    return 0;
}

int main(int argc, char **argv) {
    Arguments args = {0};

    const char *program_name = shift(&argc, &argv);

    char *arg;

    while ((arg = shift(&argc, &argv)) != NULL) {
        if (arg_cmp(arg, "help", "h")) {
            usage(stdout, program_name, NULL);

            return 0;
        } else if (arg_cmp(arg, "add", "a")) {
            const char *value = shift(&argc, &argv);

            if (value == NULL) {
                usage(stderr, program_name, "option \"%s\" expects a filename", arg);

                return 1;
            }

            args.kind = AK_ADD;
            args.value = value;
        } else if (arg_cmp(arg, "remove", "r")) {
            const char *value = shift(&argc, &argv);

            if (value == NULL) {
                usage(stderr, program_name, "option \"%s\" expects a filename", arg);

                return 1;
            }

            args.kind = AK_REMOVE;
            args.value = value;
        } else if (arg_cmp(arg, "view", "v")) {
            args.kind = AK_VIEW;
        } else if (arg_cmp(arg, "get", "g")) {
            const char *value = shift(&argc, &argv);

            if (value == NULL) {
                usage(stderr, program_name, "the option \"%s\" expects a filename", arg);

                return 1;
            }

            args.kind = AK_GET;
            args.value = value;
        } else if (arg_cmp(arg, "open", "o")) {
            const char *value = shift(&argc, &argv);

            if (value == NULL) {
                usage(stderr, program_name, "the option \"%s\" expects a filename", arg);

                return 1;
            }

            args.kind = AK_OPEN;
            args.value = value;
        } else {
            if (*arg == '-') {
                usage(stderr, program_name, "flag %s does not exists", arg);
            } else {
                usage(stderr, program_name, "option \"%s\" does not exists", arg);
            }

            return 1;
        }
    }

    switch (args.kind) {
        case AK_ADD: return add_action(args.value);
        case AK_REMOVE: return remove_action(args.value);
        case AK_VIEW: return view_action();
        case AK_GET: return get_action(program_name, args.value);
        case AK_OPEN: return open_action(program_name, args.value);
        default:
            usage(stderr, program_name, NULL);
            return 1;
    }

    return 0;
}
