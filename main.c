#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <wchar.h>
#include <sys/wait.h>
#include <time.h>
#include "./database.h"
#include "crypt.h"

#define DB_FILENAME ".wodo.db"
#define CONFIG_FILENAME ".wodo.conf.db"
#define MAX_FILENAME_SIZE 255
#define MAX_FILTER_SIZE 10
#define DB_FILEPATH_MAX_BUFFER (MAX_FILENAME_SIZE + MAX_FILENAME_SIZE + MAX_FILENAME_SIZE) // /<home>/<user>/file

// initialized in `main`
static Database *global_database;

// ---------------------------------------- Utils Start ----------------------------------------

typedef struct {
    int day, month, year, hour, minute;
} SysDateTime;

static SysDateTime today_date;
static time_t today_date_timestamp;

static SysDateTime get_today_date(void) {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    // Extract day, month, and year
    int day = tm.tm_mday;    // Day of the month (1-31)
    int month = tm.tm_mon + 1; // Month (0-11), so add 1
    int year = tm.tm_year + 1900; // Year since 1900, so add 1900
    int hour = tm.tm_hour;
    int minute = tm.tm_min;

    return (SysDateTime){.day = day, .month = month, .year = year, .hour = hour, .minute = minute };
}

static time_t get_timestamp(SysDateTime sys_date_time) {
    struct tm t = {0};

    t.tm_year = sys_date_time.year - 1900;
    t.tm_mon = sys_date_time.month - 1;
    t.tm_mday = sys_date_time.day;
    t.tm_hour = sys_date_time.hour;
    t.tm_min = sys_date_time.minute;
    t.tm_sec = 0;

    time_t timestamp = mktime(&t);

    if (timestamp == -1) {
        fprintf(stderr, "could not convert to timestamp\n");
        exit(1);
    }

    return timestamp;
}

static size_t read_file(const char *filename, char **content) {
    FILE *fptr = fopen(filename, "r");

    if (fptr == NULL) {
        fprintf(stderr, "could not open file %s due to: %s\n", filename, strerror(errno));
        exit(1);
    }

    fseek(fptr, 0, SEEK_END);
    const size_t stream_size = ftell(fptr);
    rewind(fptr);

    if (content != NULL) {
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

static inline bool is_symbol(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static void show_current_selected_file(const Database *database) {
    (void)database;
    // TODO: use config.h to display current selected file
    return;
    /* if (database->selected_file == NULL) return;

    fprintf(stdout, "current selected file: [ID:%ld] %s\n\n", database->selected_file->id, database->selected_file->filepath); */
}

// ---------------------------------------- Utils End ----------------------------------------

typedef enum {
    AK_ADD = 1,
    AK_REMOVE,
    AK_VIEW,
    AK_GET,
    AK_OPEN,
    AK_TODAY,
    AK_LIST,
    AK_FILTER,
    AK_SELECT
} ArgumentKind;

typedef struct {
    ArgumentKind kind;
    char *arg1;
    char *arg2;
} Arguments;

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

// ---------------------------------------- Lexer Start ----------------------------------------

static Lexer create_lexer(const char *content, size_t content_size) {
    return (Lexer){
        .content = content,
        .content_size = content_size,
        .cursor = 0,
        .bot = 0
    };
}

static inline char chr(Lexer *lexer) {
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
    token->value_size = lexer->cursor - lexer->bot; // TODO: FIXME: search how to handle utf-8 strings to allow portuguese accent marks
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
    while (lexer->cursor < lexer->content_size && chr(lexer) != '\0') {
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
    S_TODO = 0,
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

typedef enum {
    TS_PAST = 0,
    TS_PRESENT,
    TS_FUTURE
} Time_State;

typedef struct {
    char hour[2];
    int hour_value;
    char minute[2];
    int minute_value;
} Time;

typedef struct {
    char day[2];
    int day_value;
    char month[2];
    int month_value;
    char year[2];
    int year_value;
} Date;

typedef struct Line Line;

struct Line {
    int indent;

    State state;
    Week_Day week_day;

    Date date;
    Time start;
    Time end;

    Time_State time_state;

    const char *text;
    size_t text_size;

    Line *next;
};

typedef bool (*LineFilterPredicate)(const Line *line, const void *data);

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

static SysDateTime from_date_and_time_to_sysdatetime(Date date, Time time) {
    return (SysDateTime){
        .year= date.year_value + 2000,
        .month = date.month_value,
        .day = date.day_value,
        .hour = time.hour_value,
        .minute = time.minute_value
    };
}

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

    int bn = parse_char_to_number(b);

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

    int bn = parse_char_to_number(b);

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

    date.day_value = parse_month_day(dayt->value, dayt->value_size);
    date.day[0] = dayt->value[0];
    date.day[1] = dayt->value[1];

    date.month_value = parse_month(montht->value, montht->value_size);
    date.month[0] = montht->value[0];
    date.month[1] = montht->value[1];

    date.year_value = parse_year(yeart->value, yeart->value_size);
    date.year[0] = yeart->value[0];
    date.year[1] = yeart->value[1];

    return date;
}

static Time parse_time(Parser *parser) {
    Time time = {0};

    Token *hourt = expect_kind(parser, TK_NUMBER);
    expect_kind(parser, TK_COLON);
    Token *minutet = expect_kind(parser, TK_NUMBER);

    time.hour_value = parse_hour(hourt->value, hourt->value_size);
    time.hour[0] = hourt->value[0];
    time.hour[1] = hourt->value[1];

    time.minute_value = parse_minute(minutet->value, minutet->value_size);
    time.minute[0] = minutet->value[0];
    time.minute[1] = minutet->value[1];

    return time;
}

static Line *parse_line(Parser *parser) {
    Line *line = malloc(sizeof(Line));

    line->next = NULL;

    if (parser->tokens != NULL && parser->tokens->kind == TK_SPACE) {
        Token *indent = consume_token(parser);

        line->indent = indent->value_size;
    } else {
        line->indent = 0;
    }

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

    time_t start_timestamp = get_timestamp(from_date_and_time_to_sysdatetime(line->date, line->start));
    time_t end_timestamp = get_timestamp(from_date_and_time_to_sysdatetime(line->date, line->end));

    if (today_date_timestamp >= start_timestamp && today_date_timestamp <= end_timestamp) {
        line->time_state = TS_PRESENT;
    } else if (today_date_timestamp < start_timestamp) {
        line->time_state = TS_FUTURE;
    } else if (today_date_timestamp > end_timestamp) {
        line->time_state = TS_PAST;
    } else {
        fprintf(stderr, "could not parse time state\n");
        exit(1);
    }

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

Line *filter_lines(const Line *lines, LineFilterPredicate predicate, void *data) {
    Line *head = NULL;
    Line *tail = NULL;

    for (const Line *line = lines; line != NULL; line = line->next) {
        if (predicate(line, data)) {
            Line *copy = malloc(sizeof(Line));

            if (copy == NULL) {
                perror("Buy more RAM");
                exit(1);
            }

            memcpy(copy, line, sizeof(Line));

            copy->next = NULL;

            if (head == NULL) {
                tail = head = copy;
            } else {
                tail = tail->next = copy;
            }
        }
    }

    return head;
}

void free_lines(Line *lines) {
    Line *line = lines;

    while (line != NULL) {
        Line *next = line->next;

        free(line);

        line = next;
    }
}

// ---------------------------------------- Parser End ----------------------------------------

// ---------------------------------------- Compile Start ----------------------------------------

static Line *compile_file(char *content, size_t content_size) {
    Lexer lexer = create_lexer(content, content_size);

    Token *tokens = tokenize(&lexer);

    Parser parser = create_parser(tokens);

    return parse_lines(&parser);
}

// ---------------------------------------- Compile End ----------------------------------------

// ---------------------------------------- Data View Start ----------------------------------------

static void display_line(Line *line, char *padding, size_t max_string_size) {
    if (line->state == S_DONE) {
        printf("\033[0;32m");
    } else {
        switch (line->time_state) {
            case TS_PAST: {
                if (line->state == S_TODO) {
                    printf("\033[0;31m");
                } else if (line->state == S_IN_PROGRESS) {
                    printf("\033[0;33m");
                }
            } break;
            case TS_FUTURE:
            case TS_PRESENT: {
                if (line->state == S_IN_PROGRESS) {
                    printf("\033[0;36m");
                }
            } break;
            default: break;
        }
    }

    int space = max_string_size - line->text_size - line->indent;
    printf("%s%*.s", padding, line->indent, "");
    printf("%.*s    %*.s", (int)line->text_size, line->text, space, "");

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

    printf("\033[0m\n");
}

static void display_lines_and_free(Line *lines, char *padding) {
    size_t max_string_size = 0;

    for (Line *line = lines; line != NULL; line = line->next) {
        size_t size = line->text_size + line->indent;

        if (size > max_string_size)
            max_string_size = size;
    }

    Line *line = lines;

    while (line != NULL) {
        Line *next = line->next;

        display_line(line, padding, max_string_size);
        free(line);

        line = next;
    }
}

static bool filter_today_lines(const Line *line, const void *data) {
    (void)data;

    return (line->date.day_value == today_date.day &&
            line->date.month_value == today_date.month &&
            line->date.year_value + 2000 == today_date.year);
}

static bool filter_lines_by_state(const Line *line, const void *data) {
    State state = *(State*)data;

    return line->state == state;
}

static bool filter_lines_by_time_state(const Line *line, const void *data) {
    Time_State state = *(Time_State*)data;

    return line->time_state == state;
}

static int display_today_lines_and_free(Line *lines, char *padding) {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    // Extract day, month, and year
    int day = tm.tm_mday;    // Day of the month (1-31)
    int month = tm.tm_mon + 1; // Month (0-11), so add 1
    int year = tm.tm_year + 1900; // Year since 1900, so add 1900

    size_t max_string_size = 0;

    for (Line *line = lines; line != NULL; line = line->next) {
        size_t size = line->text_size + line->indent;

        if (size > max_string_size)
            max_string_size = size;
    }

    int count = 0;

    Line *line = lines;

    while (line != NULL) {
        Line *next = line->next;

        if (line->date.day_value == day && line->date.month_value == month && line->date.year_value + 2000 == year) {
            display_line(line, padding, max_string_size);
            free(line);
            count++;
        }

        line = next;
    }

    return count;
}

// ---------------------------------------- Data View End ----------------------------------------

static void usage(FILE *stream, const char *program_name, char *error_message, ...) {
    va_list args;

    fprintf(stream, "%s [option] [arguments]\n", program_name);
    fprintf(stream, "\n");
    fprintf(stream, "    add     a    [title] [filename]                       add a new file to the tracking system\n");
    fprintf(stream, "    remove  r    [id] or nothing if selected              remove a file from the tracking system\n");
    fprintf(stream, "    get     g    [id] or nothing if selected              get specific file by path or id\n");
    fprintf(stream, "    open    o    [id] or nothing if selected              open a file using vim by path or id\n");
    fprintf(stream, "    select  s    [id]                                     set file as selected so you can use it easier\n");
    fprintf(stream, "    view    v                                             view files\n");
    fprintf(stream, "    list    l                                             list files\n");
    fprintf(stream, "    today   t                                             view today tasks\n");
    fprintf(stream, "    filter  f                                             filter tasks\n");
    fprintf(stream, "                 [ts=past|future|present]                 search for time state\n");
    fprintf(stream, "                 [s=todo|doing|done]                      search for current task state\n");
    fprintf(stream, "    help    h                                             display this message\n");

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

static int add_action(char *name, char *filepath) {
    char *abs_path = realpath(filepath, NULL);

    if (abs_path == NULL) {
        fprintf(stderr, "\033[1;31merror:\033[0m invalid filepath\n");

        return 1;
    }

    size_t abs_path_size = strlen(abs_path);
    size_t name_size = strlen(name);

    Database_Db_File *file = malloc(sizeof(Database_Db_File));
    file->deleted = false;
    file->filepath = malloc(abs_path_size + 1);
    file->name = malloc(name_size + 1);

    memcpy(file->filepath, abs_path, strlen(abs_path));
    memcpy(file->name, name, strlen(name));

    file->filepath[abs_path_size] = '\0';
    file->name[name_size] = '\0';

    unsigned char *hash = hash_bytes(abs_path, strlen(abs_path));

    memcpy(file->large_identifier, hash, LARGE_IDENTIFIER_SIZE);
    memcpy(file->short_identifier, hash, SHORT_IDENTIFIER_SIZE);

    free(hash);

    database_status_code_t status_code = database_add_file(global_database, file);
    
    if (status_code != DATABASE_OK_STATUS_CODE) {
        fprintf(stderr, "\033[1;31merror:\033[0m could not add file due to: %s\n", database_status_code_string(status_code));
        database_free(global_database);

        return status_code;
    }

    char *content = NULL;

    size_t content_size = read_file(abs_path, &content);

    free_lines(compile_file(content, content_size));

    free(content);
    database_save(global_database);
    database_free(global_database);

    return 0;
}

// `file_identifier` is the sha1
static int remove_action(const char file_identifier[40]) {
    database_status_code_t status_code;

    // TODO: use config.h
    /* if (file_identifier == NULL) {
        id = database.current_selected_file_id;
    } else  */

    Database_Db_File *file = NULL;

    status_code = database_get_file_by_identifier(&file, global_database, file_identifier);

    if (status_code == DATABASE_NOT_FOUND_STATUS_CODE) {
        database_free(global_database);

        return 0;
    }

    if (status_code != DATABASE_OK_STATUS_CODE) {
        database_free(global_database);

        return status_code;
    }

    if (file == NULL) return 0;

    database_delete_file(file);

    database_save(global_database);
    database_free(global_database);

    return 0;
}

static int view_action(void) {
    if (global_database->length == 0) {
        return 0;
    }

    show_current_selected_file(global_database);

    database_foreach_begin(*global_database) {
        printf("\033[1;33m%.*s\033[0m %s\n", SHORT_IDENTIFIER_SIZE, it->short_identifier, it->name);

        char *content;

        size_t content_size = read_file(it->filepath, &content);

        Line *lines = compile_file(content, content_size);

        display_lines_and_free(lines, "    ");

        printf("\n");

        free(content);
   } database_foreach_end;

    database_free(global_database);

    return 0;
}

static int today_action(void) {
    show_current_selected_file(global_database);

    database_foreach_begin(*global_database) {
        char *content;

        size_t content_size = read_file(it->filepath, &content);

        Line *lines = compile_file(content, content_size);

        Line *lines_filtered = filter_lines(lines, filter_today_lines, NULL);

        if (lines_filtered != NULL) {
            printf("\033[1;33m%.*s\033[0m %s\n", SHORT_IDENTIFIER_SIZE, it->short_identifier, it->name);

            printf("\n");
            display_today_lines_and_free(lines_filtered, "    ");

            if (i < global_database->length - 1) {
                printf("\n");
            }
        }

        free_lines(lines);
        free(content);
    } database_foreach_end;

    printf("\n");

    database_free(global_database);

    return 0;
}

static int list_action(void) {
    show_current_selected_file(global_database);

    database_foreach_begin(*global_database) {
        printf("\033[1;33m%.*s\033[0m %s\n", SHORT_IDENTIFIER_SIZE, it->short_identifier, it->name);
    } database_foreach_end;

    database_free(global_database);

    return 0;
}

static bool is_state_filter(const char *filter, size_t size) {
    if (size != 1) return false;

    return filter[0] == 's';
}

static bool is_time_state_filter(const char *filter, size_t size) {
    if (size != 2) return false;

    return filter[0] == 't' && filter[1] == 's';
}

typedef struct {
    bool state;
    State state_value;

    bool time_state;
    Time_State time_state_value;
} Filters;

static int filter_action(const char *program_name, const char *filter) {
    size_t filter_size = strlen(filter);

    if (filter_size > MAX_FILTER_SIZE) {
        usage(stderr, program_name, "your filter exceded the max size: %d", MAX_FILTER_SIZE);
        return 1;
    }

    int filter_key_size = -1;
    char filter_key[filter_size];
    char filter_value[filter_size];

    for (size_t i = 0; i < filter_size; ++i) {
        if (filter[i] == '=') {
            filter_key_size = i;
            break;
        }
    }

    if (filter_key_size == -1) {
        usage(stderr, program_name, "missing filter key");
        return 1;
    } else if (filter_size - filter_key_size - 1 /* the equal sign */ <= 0) {
        usage(stderr, program_name, "missing filter value ex: %.*s=...", filter_key_size, filter);
        return 1;
    }

    memcpy(filter_key, filter, filter_key_size);

    bool state_filter = is_state_filter(filter_key, filter_key_size);
    bool time_state_filter = is_time_state_filter(filter_key, filter_key_size);

    if (!state_filter && !time_state_filter) {
        usage(stderr, program_name, "invalid filter %.*s. Look at available filters", filter_key_size, filter_key);
        return 1;
    }

    int filter_value_size = filter_size - filter_key_size - 1;

    memcpy(filter_value, filter + filter_key_size + 1, filter_value_size);

    if (global_database->length == 0) {
        printf("There is no file yet\n");

        return 0;
    }

    show_current_selected_file(global_database);

    Filters filters = {0};

    if (state_filter) {
        filters.state = true;

        const char *todo_option = "todo";
        const size_t todo_option_size = 4;
        const char *doing_option = "doing";
        const size_t doing_option_size = 5;
        const char *done_option = "done";
        const size_t done_option_size = 4;

        if (cmp_sized_strings(todo_option, filter_value, todo_option_size, filter_value_size)) {
            filters.state_value = S_TODO;
        } else if (cmp_sized_strings(doing_option, filter_value, doing_option_size, filter_value_size)) {
            filters.state_value = S_IN_PROGRESS;
        } else if (cmp_sized_strings(done_option, filter_value, done_option_size, filter_value_size)) {
            filters.state_value = S_DONE;
        } else {
            usage(stderr, program_name, "invalid filter value %.*s=%.*s. Look at available filters", filter_key_size, filter_key, filter_value_size, filter_value);
            return 1;
        }
    }

    if (time_state_filter) {
        filters.time_state = true;

        const char *past_option = "past";
        const size_t past_option_size = 4;
        const char *present_option = "present";
        const size_t present_option_size = 7;
        const char *future_option = "future";
        const size_t future_option_size = 6;

        if (cmp_sized_strings(past_option, filter_value, past_option_size, filter_value_size)) {
            filters.time_state_value = TS_PAST;
        } else if (cmp_sized_strings(present_option, filter_value, present_option_size, filter_value_size)) {
            filters.time_state_value = TS_PRESENT;
        } else if (cmp_sized_strings(future_option, filter_value, future_option_size, filter_value_size)) {
            filters.time_state_value = TS_FUTURE;
        } else {
            usage(stderr, program_name, "invalid filter value %.*s=%.*s. Look at available filters", filter_key_size, filter_key, filter_value_size, filter_value);
            return 1;
        }
    }

    database_foreach_begin(*global_database) {
        char *content;

        size_t content_size = read_file(it->filepath, &content);

        Line *lines = compile_file(content, content_size);

        if (filters.state) {
            Line *lines_filtered = filter_lines(lines, filter_lines_by_state, &filters.state_value);

            if (lines != NULL) free_lines(lines);

            lines = lines_filtered;
        }

        if (filters.time_state) {
            Line *lines_filtered = filter_lines(lines, filter_lines_by_time_state, &filters.time_state_value);

            if (lines != NULL) free_lines(lines);

            lines = lines_filtered;
        }

        if (lines != NULL) {
            // TODO: create a FMT macro to the id part
            printf("\033[1;33m%.*s\033[0m %s\n", SHORT_IDENTIFIER_SIZE, it->short_identifier, it->name);

            display_lines_and_free(lines, "    ");

            printf("\n");
        }

        free(content);
    } database_foreach_end;

    database_free(global_database);

    return 0;
}

static int select_action(const char *program_name, const char *file_identifier) {
    (void)program_name;
    (void)file_identifier;
    // TODO: use conf.h to select
    return 0;
    /* DbFile *file;

    if (is_number(file_identifier)) {
        size_t id = strtoul(file_identifier, NULL, 10);

        file = get_database_file_by_id(&database, id);

        if (file == NULL) {
            fprintf(stderr, "\033[1;31merror:\033[0m there is no file with id %s in the database\n", file_identifier);

            free_database(&database);
            return 1;
        }
    } else {
        char *abs_path = realpath(file_identifier, NULL);

        if (abs_path == NULL) {
            fprintf(stderr, "\033[1;31merror:\033[0m invalid filename\n");

            return 1;
        }

        file = get_database_file_by_filepath(&database, abs_path);

        if (file == NULL) {
            fprintf(stderr, "\033[1;31merror:\033[0m the file %s does not exists in the database\n", file_identifier);
            fprintf(stderr, "use \"%s add %s\" to add this file in the database\n", program_name, file_identifier);

            return 1;
        }
    }

    database.current_selected_file_id = file->id;

    save_database(&database);

    fprintf(stdout, "selected file changed successfully to \"%s\"\n", file->filepath);

    free_database(&database);

    return 0; */
}

// `file_identifier` can be an Id or the path to the file
static int get_action(const char *program_name, const char *file_identifier) {
    // TODO: use conf.h to get the selected file
    /* if (file_identifier == NULL) {
        file = database.selected_file;
    } else  */
    Database_Db_File *file = NULL;
    database_status_code_t status_code = DATABASE_OK_STATUS_CODE;

    if ((status_code = database_get_file_by_identifier(&file, global_database, file_identifier)) != DATABASE_OK_STATUS_CODE) {
        if (status_code == DATABASE_NOT_FOUND_STATUS_CODE) {
            fprintf(stderr, "\033[1;31merror:\033[0m the file %s does not exists in the database\n", file_identifier);
            fprintf(stderr, "use \"%s add %s\" to add this file in the database\n", program_name, file_identifier);
        } else {
            fprintf(stderr, "\033[1;31merror:\033[0m could not get \"%s\" due to: %s\n", file_identifier, database_status_code_string(status_code));
        }

        database_free(global_database);
        return 1;
    }
 
    char *content;

    size_t content_size = read_file(file->filepath, &content);

    Line *lines = compile_file(content, content_size);

    display_lines_and_free(lines, "");

    database_free(global_database);
    free(content);

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
    Database_Db_File *file = NULL;
    database_status_code_t status_code = DATABASE_OK_STATUS_CODE;

    // TODO: use conf.h to get selected file
    /* if (file_identifier == NULL) {
        file = database.selected_file;
    } else */

    if ((status_code = database_get_file_by_identifier(&file, global_database, file_identifier)) != DATABASE_OK_STATUS_CODE) {
        if (status_code == DATABASE_NOT_FOUND_STATUS_CODE) {
            fprintf(stderr, "\033[1;31merror:\033[0m the file %s does not exists in the database\n", file_identifier);
            fprintf(stderr, "use \"%s add %s\" to add this file in the database\n", program_name, file_identifier);
        } else {
            fprintf(stderr, "\033[1;31merror:\033[0m could not open \"%s\" due to: %s\n", file_identifier, database_status_code_string(status_code));
        }

        database_free(global_database);

        return 1;
    }

    open_vim_at(file->filepath);

    return 0;
}

int main(int argc, char **argv) {
    database_status_code_t status_code = database_load(&global_database);

    if (status_code != DATABASE_OK_STATUS_CODE) {
        fprintf(stderr, "error: could not open database due to: %s\n", database_status_code_string(status_code));
        return status_code;
    }

    today_date = get_today_date();
    today_date_timestamp = get_timestamp(today_date);

    Arguments args = {0};

    const char *program_name = shift(&argc, &argv);

    char *arg;

    bool does_have_selected_file = false; // database.current_selected_file_id != 0; // TODO: use conf.h to get selected file

    while ((arg = shift(&argc, &argv)) != NULL) {
        if (arg_cmp(arg, "help", "h")) {
            usage(stdout, program_name, NULL);

            return 0;
        } else if (arg_cmp(arg, "add", "a")) {
            char *arg1 = shift(&argc, &argv);

            if (arg1 == NULL) {
                usage(stderr, program_name, "option \"%s\" expects a name and a filename", arg);

                return 1;
            }

            char *arg2 = shift(&argc, &argv);

            if (arg2 == NULL) {
                usage(stderr, program_name, "option \"%s\" expects a name and a filename", arg);

                return 1;
            }

            args.kind = AK_ADD;
            args.arg1 = arg1;
            args.arg2 = arg2;
        } else if (arg_cmp(arg, "remove", "r")) {
            char *value = shift(&argc, &argv);

            if (value == NULL && !does_have_selected_file) {
                usage(stderr, program_name, "option \"%s\" expects an id. you don't have any selected files", arg);

                return 1;
            }

            args.kind = AK_REMOVE;
            args.arg1 = value;
        } else if (arg_cmp(arg, "view", "v")) {
            args.kind = AK_VIEW;
        } else if (arg_cmp(arg, "get", "g")) {
            char *value = shift(&argc, &argv);

            if (value == NULL && !does_have_selected_file) {
                usage(stderr, program_name, "the option \"%s\" expects an id. you don't have any selected files", arg);

                return 1;
            }

            args.kind = AK_GET;
            args.arg1 = value;
        } else if (arg_cmp(arg, "open", "o")) {
            char *value = shift(&argc, &argv);

            if (value == NULL && !does_have_selected_file) {
                usage(stderr, program_name, "the option \"%s\" expects an id. you don't have any selected file", arg);

                return 1;
            }

            args.kind = AK_OPEN;
            args.arg1 = value;
        } else if (arg_cmp(arg, "today", "t")) {
            args.kind = AK_TODAY;
        } else if (arg_cmp(arg, "list", "l")) {
            args.kind = AK_LIST;
        } else if (arg_cmp(arg, "filter", "f")) {
            char *value = shift(&argc, &argv);

            if (value == NULL) {
                usage(stderr, program_name, "the option \"%s\" expects a filter", arg);

                return 1;
            }

            args.kind = AK_FILTER;
            args.arg1 = value;
        } else if (arg_cmp(arg, "select", "s")) {
            char *value = shift(&argc, &argv);

            if (value == NULL) {
                usage(stderr, program_name, "the option \"%s\" expects an id", arg);

                return 1;
            }

            args.kind = AK_SELECT;
            args.arg1 = value;
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
        case AK_ADD: return add_action(args.arg1, args.arg2);
        case AK_REMOVE: return remove_action(args.arg1);
        case AK_VIEW: return view_action();
        case AK_GET: return get_action(program_name, args.arg1);
        case AK_OPEN: return open_action(program_name, args.arg1);
        case AK_TODAY: return today_action();
        case AK_LIST: return list_action();
        case AK_FILTER: return filter_action(program_name, args.arg1);
        case AK_SELECT: return select_action(program_name, args.arg1);
        default:
            usage(stderr, program_name, NULL);
            return 1;
    }

    database_free(global_database);

    return 0;
}
