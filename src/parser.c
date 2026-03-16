#define CL_ARRAY_IMPLEMENTATION

#include <math.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "parser.h"
#include "arr.h"
#include "date.h"

#define task_beginning_character_descriptor '%'
#define property_beginning_character_descriptor '.'

static size_t       cursor = 0;
static size_t       bot = 0;
static int          line = 1;
static int          col = 1;
static char         *content = NULL;
static size_t       content_length = 0;
static wodo_task_t  *tasks = CL_ARRAY_INIT;
static const char   *filename;

static struct {
    int length;
    int capacity;
    wodo_location_t stack[8];
} location_snapshots = {
    .length = 0,
    .capacity = 8,
    .stack = {0}
};

static inline void push_location_snapshot() {
    assert(location_snapshots.length < location_snapshots.capacity && "reached maximum location snapshots stack");

    location_snapshots.stack[location_snapshots.length++] = (wodo_location_t){
        .line = line,
        .col = col
    };
}

static inline wodo_location_t pop_location_snapshot() {
    assert(location_snapshots.length > 0 && "trying to pop from empty location snapshots stack");

    return location_snapshots.stack[--location_snapshots.length];
}

static inline void parser_error_no_quit(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    fprintf(stderr, "%s:%d:%d error: ", filename, line, col);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");

    va_end(args);
}

static inline void parser_error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    printf("%s:%d:%d error: ", filename, line, col);
    vprintf(fmt, args);
    printf("\n");

    va_end(args);

    exit(1);
}

static inline bool is_bol(void) {
    return col == 1;
}

static inline bool is_empty(void) {
    return cursor >= content_length;
}

static inline void advance_cursor(void) {
    if (cursor < content_length) {
        if (content[cursor] == '\n') {
            line++;
            col = 1;
        } else {
            col++;
        }

        cursor++;
    }
}

static inline char chr(void) {
    if (is_empty()) return '\0';
    return content[cursor];
}

static void skip_char(char c) {
    if (is_empty()) parser_error("expected '%c' but got EOF", c);
    if (chr() != c) parser_error("expected '%c' but got '%c'", c, chr());
    advance_cursor();
}

static inline bool is_whitespace(char chr) {
    return chr == ' ';
}

static inline bool is_linebreak(char chr) {
    return chr == '\n';
}

static inline bool is_lowercase_alpha(char chr) {
    return chr >= 'a' && chr <= 'z';
}

static inline bool is_number(char chr) {
    return chr >= '0' && chr <= '9';
}

static inline bool is_valid_tag(char chr) {
    return is_lowercase_alpha(chr) || chr == '_';
}

static inline bool str_slice_eq(const char *slice, size_t slice_len, const char *str) {
    size_t str_len = strlen(str);

    if (slice_len != str_len) return false;

    return strncmp(slice, str, str_len) == 0;
}

static wodo_task_state_t parse_task_state_property(void) {
    // skip whitespaces
    while (!is_empty() && is_whitespace(chr())) advance_cursor();

    if (is_empty()) parser_error("reached EOF before defining 'state' property");

    bot = cursor;

    // consume state
    while (is_lowercase_alpha(chr())) advance_cursor();

    if (str_slice_eq(&content[bot], cursor - bot, "todo")) {
        return Wodo_Task_State_Todo;
    } else if (str_slice_eq(&content[bot], cursor - bot, "doing")) {
        return Wodo_Task_State_Doing;
    } else if (str_slice_eq(&content[bot], cursor - bot, "blocked")) {
        return Wodo_Task_State_Blocked;
    } else if (str_slice_eq(&content[bot], cursor - bot, "done")) {
        return Wodo_Task_State_Done;
    }

    parser_error("unrecognized task state '%.*s'", (int)(cursor - bot), &content[bot]);

    return -1;
}

static wodo_node_t *parse_task_tags_property(void) {
    // skip white spaces
    while (!is_empty() && is_whitespace(chr())) advance_cursor();

    wodo_node_t *tags = CL_ARRAY_INIT;

    if (is_empty()) return tags;

    // consume all tags
    while (!is_empty() && chr() != '\n') {
        bot = cursor;

        push_location_snapshot();

        // consume one tag
        while (!is_empty() && is_valid_tag(chr())) advance_cursor();

        wodo_node_t tag = {
            .location = pop_location_snapshot(),
            .string.value = &content[bot],
            .string.length = cursor - bot
        };

        cl_arr_push(tags, tag);

        // consume whitespaces
        while (!is_empty() && is_whitespace(chr())) advance_cursor();
    }

    return tags;
}

static int parse_fixed_size_number_and_convert_to_int(int size) {
    bot = cursor;

    while (!is_empty() && is_number(chr())) advance_cursor();

    if (size != (int)(cursor - bot)) return -1;

    int n = 0;

     // 2026
     // 6 * (10 ** 0) = 6
     // 2 * (10 ** 1) = 20
     // 0 * (10 ** 2) = 0
     // 2 * (10 ** 3) = 2000
    for (int i = 0; i < size; i++) {
        int digit = content[cursor - i - 1] - '0';
        n += digit * pow(10, i);
    }

    return n;
}

static wodo_datetime_t parse_task_date_property(void) {
    // skip whitespaces
    while (!is_empty() && is_whitespace(chr())) advance_cursor();

    if (is_empty()) {
        parser_error("reached EOF before defining 'date' property");
    }

    // start parsing date
    int year = parse_fixed_size_number_and_convert_to_int(4);
    if (year == -1) parser_error("couldn't parse 'date' property correctly");
    skip_char('-');
    int month = parse_fixed_size_number_and_convert_to_int(2);
    if (month == -1) parser_error("couldn't parse 'date' property correctly");
    skip_char('-');
    int day = parse_fixed_size_number_and_convert_to_int(2);
    if (day == -1) parser_error("couldn't parse 'date' property correctly");
    skip_char(' ');
    int hour = parse_fixed_size_number_and_convert_to_int(2);
    if (hour == -1) parser_error("couldn't parse 'date' property correctly");
    skip_char(':');
    int minute = parse_fixed_size_number_and_convert_to_int(2);
    if (minute == -1) parser_error("couldn't parse 'date' property correctly");
    skip_char(':');
    int second = parse_fixed_size_number_and_convert_to_int(2);
    if (second == -1) parser_error("couldn't parse 'date' property correctly");

    int multiplier = 1; // 1 or -1 e.g. -03:00 or +03:00
    int tz_offset_hours = 0;
    int tz_offset_minutes = 0;

    switch (chr()) {
        case 'Z': // I's UTC
            advance_cursor();
            break;
        case '-': {
            skip_char('-');
            tz_offset_hours = parse_fixed_size_number_and_convert_to_int(2);
            if (tz_offset_hours == -1) parser_error("couldn't parse 'date' property correctly");
            skip_char(':');
            tz_offset_minutes = parse_fixed_size_number_and_convert_to_int(2);
            if (tz_offset_minutes == -1) parser_error("couldn't parse 'date' property correctly");
            multiplier = -1;
        } break;
        case '+': {
            skip_char('+');
            tz_offset_hours = parse_fixed_size_number_and_convert_to_int(2);
            if (tz_offset_hours == -1) parser_error("couldn't parse 'date' property correctly");
            skip_char(':');
            tz_offset_minutes = parse_fixed_size_number_and_convert_to_int(2);
            if (tz_offset_minutes == -1) parser_error("couldn't parse 'date' property correctly");
            multiplier = 1;
        } break;
        default: parser_error("couldn't parse 'date' property correctly");
    }

    tz_offset_minutes += tz_offset_hours * 60;
    tz_offset_minutes = tz_offset_minutes * multiplier;

    wodo_datetime_t datetime = {
        .year = year,
        .month = month,
        .day = day,
        .hour = hour,
        .minute = minute,
        .second = second,
        .tz_offset = tz_offset_minutes
    };

    if (!validate_datetime(datetime))
        parser_error("couldn't parse 'date' property correctly because the datetime informed is invalid");

    return datetime;
}

static wodo_task_t parse_task() {
    wodo_task_t task = {0};

    bool parsed_tags_property = false;
    bool parsed_date_property = false;
    bool parsed_state_property = false;

    // skip 'task_beginning_character_descriptor'
    advance_cursor(); 

    // skip whitespaces
    while (is_whitespace(chr())) advance_cursor();

    bot = cursor;

    push_location_snapshot();

    // consume task title
    while (!is_linebreak(chr())) advance_cursor();

    task.title = (wodo_node_t){
        .location = pop_location_snapshot(),
        .string.length = cursor - bot,
        .string.value = &content[bot]
    };

    // advance until next instruction
    while (!is_empty() && (is_whitespace(chr()) || is_linebreak(chr()))) advance_cursor();

    if (is_empty()) parser_error("reached EOF before defining required property 'date'");

    if (chr() == task_beginning_character_descriptor && is_bol()) {
        parser_error("starting another task without defining required property 'date'");
    } else if (chr() != property_beginning_character_descriptor || !is_bol()) {
        parser_error("starting task description before defining required property 'date'");
    }

    // start parsing properties
    while (chr() == property_beginning_character_descriptor && is_bol()) {
        push_location_snapshot();

        // skip 'property_beginning_character_descriptor'
        advance_cursor();

        bot = cursor;

        // consume property name
        while (!is_empty() && !is_linebreak(chr()) && !is_whitespace(chr())) advance_cursor();

        const char *s_property_name = &content[bot];
        size_t s_property_size = cursor - bot;

        if (str_slice_eq(s_property_name, s_property_size, "state")) {
            wodo_task_state_t state = parse_task_state_property();

            parsed_state_property = true;
            task.state_property = (wodo_node_t){
                .location = pop_location_snapshot(),
                .state = state
            };
        } else if (str_slice_eq(s_property_name, s_property_size, "tags")) {
            wodo_node_t *tags = parse_task_tags_property();

            parsed_tags_property = true;
            task.tags_property = (wodo_node_t){
                .location = pop_location_snapshot(),
                .node_array = tags
            };
        } else if (str_slice_eq(s_property_name, s_property_size, "date")) {
            wodo_datetime_t date = parse_task_date_property();

            parsed_date_property = true;
            task.date_property = (wodo_node_t){
                .location = pop_location_snapshot(),
                .datetime = date,
            };
        } else if (str_slice_eq(s_property_name, s_property_size, "remind")) {
            task.remind_property = (wodo_node_t){
                .location = pop_location_snapshot(),
                .boolean = true
            };
        } else {
            parser_error_no_quit("invalid property name '%.*s'", (int)s_property_size, s_property_name);
        }

        // skip empty lines and white spaces
        while (!is_empty() && (is_whitespace(chr()) || is_linebreak(chr()))) advance_cursor();
    }

    if (!parsed_date_property) 
        parser_error("starting task description before defining required property 'date'");

    if (!parsed_state_property) 
        parser_error("starting task description before defining required property 'state'");

    if (!parsed_tags_property)
        task.tags_property = (wodo_node_t){
            .location = {0},
            .node_array = CL_ARRAY_INIT
        };

    bot = cursor;

    // parse task description
    
    bool has_description_text = false;

    // beginning of the description no matter if it have text or not
    push_location_snapshot();

    while (!is_empty()) {
        if (is_bol() && chr() == task_beginning_character_descriptor) break;

        // searchs for the first line with text and saves the description snapshot at that place
        if (chr() != ' ' && !has_description_text) {
            push_location_snapshot();
            has_description_text = true;
        }

        advance_cursor();
    }

    if (has_description_text) {
        size_t description_length = cursor - bot;

        task.description = (wodo_node_t){
            .location = pop_location_snapshot(),
            .string.value = &content[bot],
            .string.length = description_length
        };
    } else {
        task.description = (wodo_node_t){
            .location = pop_location_snapshot(),
            .string.value = NULL,
            .string.length = 0
        };
    }

    // discard empty line description location
    if (has_description_text) pop_location_snapshot();

    return task;
}

/*
 * returns a CL_ARRAY
 */
wodo_task_t *parse_tasks(const char *content_filename, const char *file_content, size_t length) {
    filename = content_filename;
    content = (char*)file_content;
    content_length = length;

    while (!is_empty()) {
        switch (chr()) {
            case task_beginning_character_descriptor: {
                wodo_task_t task = parse_task();

                cl_arr_push(tasks, task);
            } break;
            case ' ':
            case '\n':
                advance_cursor();
                break; // skip
            default:
                parser_error("unexpected character %c", chr());
                break;
        }
    }

    return tasks;
}

void reset_parser_state(void) {
    cursor = 0;
    bot = 0;
    line = 1;
    col = 1;
    content = NULL;
    content_length = 0;
    filename = NULL;
    tasks = CL_ARRAY_INIT;
    location_snapshots.length = 0;
}
