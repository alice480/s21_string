#include "s21_sscanf.h"

#include "s21_string.h"

int s21_sscanf(const char *str, const char *fstr, ...) {
  int eof_fl = check_EOF_string(str), res = 0;

  if (!eof_fl) {
    va_list va;
    va_start(va, fstr);
    int tokens_len = 0;

    char *formstr = (char *)fstr;
    int len = (int)s21_strlen(formstr);
    char *src = (char *)str;

    token tokens[BUFF_SIZE];

    while (*formstr && formstr < fstr + len) {
      tokens[tokens_len] = parse_tokens(&formstr, &va);
      tokens_len++;
    }

    write_tokens_to_memory(&src, tokens, tokens_len, &res);
    va_end(va);
  }

  return (eof_fl) ? eof_fl : res;
}

int s21_isspace(char c) { return (c == ' ' || c == '\n' || c == '\t'); }

int s21_isalpha(char c) {
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

int check_EOF_string(const char *src) {
  int res = -1;

  for (int i = 0; src[i]; i++) {
    if (!s21_isspace(src[i]) && src[i] != '\0') {
      res = 0;
      break;
    }
  }

  return res;
}

token parse_tokens(char **fstr, va_list *va) {
  token tok = {.addr = S21_NULL,
               .length_t = NONE_LENGTH,
               .spec = 0,
               .width = NONE_WIDTH,
               .width_n = 0};

  if (s21_isspace(**fstr)) {
    tok.spec = 'z';
    skip_spaces_in_str(fstr);
  }

  if (**fstr == '%' && !tok.spec) {
    (*fstr)++;

    fstr_parse_width(fstr, &tok);
    fstr_parse_length(fstr, &tok);
    fstr_parse_specifier(fstr, &tok);

    if (tok.width != WIDTH_AST) tok.addr = va_arg(*va, void *);

    if (tok.spec == 'p') tok.length_t = NONE_LENGTH;
  }

  if (s21_isalpha(**fstr) && !tok.spec) {
    s21_memset(tok.buff, '\0', BUFF_SIZE - 1);
    int i = 0;
    while (**fstr && !s21_isspace(**fstr) && **fstr != '%') {
      tok.buff[i++] = **fstr;
      (*fstr)++;
    }
    tok.spec = 'b';
  }

  return tok;
}

void skip_chars_in_buffer(char **src, int *fail, token *tok) {
  skip_spaces_in_str(src);

  int test = s21_strspn(*src, tok->buff);
  int len = s21_strlen(tok->buff);

  if (test != len)
    *fail = 1;
  else
    (*src) = (*src) + len;
}

void fstr_parse_width(char **fstr, token *tok) {
  if (**fstr == '*') {
    (*fstr)++;
    tok->width = WIDTH_AST;
  } else {
    int res = parse_number_from_fstr(fstr);

    if (res) {
      tok->width = WIDTH_NUMBER;
      tok->width_n = res;
    }
  }
}

int parse_number_from_fstr(char **fstr) {
  char tmp[BUFF_SIZE] = {'\0'};

  int res = 0, i = 0;

  while (**fstr >= '0' && **fstr <= '9') {
    tmp[i] = **fstr;
    (*fstr)++;
    i++;
  }

  res = s21_atoi(tmp);

  return res;
}

void fstr_parse_length(char **fstr, token *tok) {
  switch (**fstr) {
    case 'h':
      tok->length_t = LENGTH_SHORT;
      (*fstr)++;
      break;
    case 'l':
      tok->length_t = LENGTH_LONG;
      (*fstr)++;
      if (**fstr == 'l') {
        tok->length_t = LENGTH_LONG_LONG;
        (*fstr)++;
      }
      break;
    case 'L':
      tok->length_t = LENGTH_LONG_DOUBLE;
      (*fstr)++;
      break;
  }
}

int s21_atoi(const char *str) {
  int res = 0;
  int sign = 1;
  int overflow = 0;

  while (*str == ' ') str++;

  if (*str == '-') {
    str++;
    sign = -1;
  }

  if (*str == '+') {
    str++;
  }

  while (*str && *str <= '9') {
    res = res * 10 + (*str - '0');
    if (res < 0) {
      overflow = 1;
      break;
    }
    str++;
  }
  if (overflow)
    res = sign > 0 ? INT32_MAX : INT32_MIN;
  else
    res *= sign;

  return res;
}

void fstr_parse_specifier(char **fstr, token *tok) {
  tok->spec = (**fstr);
  (*fstr)++;
}

void write_tokens_to_memory(char **src, token *tokens, int tok_len, int *res) {
  char *start = *src;

  for (int i = 0, fail = 0; i < tok_len && !fail; i++) {
    char spec = tokens[i].spec;
    if (spec == 'c') write_char_to_memory(src, res, (tokens + i));
    if (spec == 'd') write_int_to_memory(src, &fail, res, (tokens + i));
    if (spec == 'i') write_unspec_int_to_memory(src, &fail, res, (tokens + i));
    if (spec == 'g' || spec == 'G' || spec == 'f')
      write_float_to_memory(src, res, (tokens + i));
    if (spec == 's') write_string_to_memory(src, &fail, res, (tokens + i));
    if (spec == 'u') write_unsigned_to_memory(src, &fail, res, (tokens + i));
    if (spec == 'p' || spec == 'x' || spec == 'X')
      write_hex_or_oct_to_memory(src, &fail, res, (tokens + i), 16);
    if (spec == 'o')
      write_hex_or_oct_to_memory(src, &fail, res, (tokens + i), 8);
    if (spec == 'n') *((int *)tokens[i].addr) = (*src) - start;
    if (spec == 'z') skip_spaces_in_str(src);
    if (spec == 'b') skip_chars_in_buffer(src, &fail, (tokens + i));
  }
}

/* Memory writing stage */

void skip_spaces_in_str(char **src) {
  while (**src && s21_isspace(**src)) {
    (*src)++;
  }
}

void write_char_to_memory(char **str, int *res, token *tok) {
  if (tok->width == WIDTH_AST) {
    (*str)++;
  } else {
    *(char *)tok->addr = **str;
    (*str)++;
    (*res)++;
  }
}

void write_int_to_memory(char **str, int *fail_flag, int *res, token *tok) {
  long long int result = 0;
  char buff[BUFF_SIZE] = {'\0'};

  *fail_flag = 1;

  if (s21_strspn(*str, "0123456789+-")) {
    int sign = s21_strspn(*str, "+-");
    if (!(sign > 1 || (sign && (tok->width_n <= 1 && tok->width)))) {
      buff[0] = **str;
      (*str)++;
      write_chars_to_buff(str, "0123456789", buff, tok->width_n, 1);
      if (tok->width != WIDTH_AST) (*res)++;
      *fail_flag = 0;
    }
  }

  result = s21_atoi(buff);

  /* Length handling & Writing to memory */
  if (tok->width != WIDTH_AST && !*fail_flag)
    int_type_converter(tok, result, 1);

  /* If width is specified, sscanf does not skip number */
  if (tok->width != WIDTH_NUMBER)
    write_chars_to_buff(str, "0123456789", S21_NULL, 0, 0);
}

void write_unspec_int_to_memory(char **str, int *fail_flag, int *res,
                                token *tok) {
  *fail_flag = 1;

  skip_spaces_in_str(str);

  if (s21_strspn(*str, "0x") == 2) {
    *fail_flag = 0;
    write_hex_or_oct_to_memory(str, fail_flag, res, tok, 16);
  } else if (s21_strspn(*str, "0") == 1) {
    *fail_flag = 0;
    write_hex_or_oct_to_memory(str, fail_flag, res, tok, 8);
  } else if (s21_strspn(*str, "+-0123456789")) {
    *fail_flag = 0;
    write_int_to_memory(str, fail_flag, res, tok);
  }
}

// if BUFF is S21_NULL, we simply skip chars
// if precision is <= 0, it does not do anyhting
void write_chars_to_buff(char **str, const char *chars, char *buff,
                         int16_t width, int start_ind) {
  while (**str && s21_strspn(*str, chars) != 0) {
    if ((width && start_ind >= width) || (s21_isspace(**str))) break;

    if (buff) buff[start_ind] = **str;

    (*str)++;
    start_ind++;
  }
}

void write_float_to_memory(char **str, int *res, token *tok) {
  int test = 0;

  if (tok->spec == 'f')
    test = s21_strspn(*str, "0123456789+-");
  else
    test = s21_strspn(*str, "0123456789eE+-NnaAifIF");

  if (test) {
    int sign = s21_strspn(*str, "+-");
    if (!(sign > 1 || (sign && (tok->width_n <= 1 && tok->width)))) {
      char buff[BUFF_SIZE] = {'\0'};
      int start_ind = 0;
      if (sign) {
        buff[0] = **str;
        start_ind = 1;
        (*str)++;
      }

      if (tok->spec == 'f')
        write_chars_to_buff(str, ".0123456789+-", buff, tok->width_n,
                            start_ind);
      else
        write_chars_to_buff(str, ".0123456789eE+-NnaAifIF", buff, tok->width_n,
                            start_ind);

      if (tok->width != WIDTH_AST) {
        long double result = s21_strtold(buff);
        (*res)++;
        float_type_converter(tok, result);
      }
    }
  }

  if (tok->width != WIDTH_NUMBER) {
    if (tok->spec == 'f')
      write_chars_to_buff(str, ".0123456789", S21_NULL, 0, 0);
    else
      write_chars_to_buff(str, ".0123456789eE+-NaAifIFn", S21_NULL, 0, 0);
  }
}

void write_string_to_memory(char **str, const int *fail_flag, int *res,
                            token *tok) {
  int succ = 0;
  char buff[BUFF_SIZE] = {'\0'};
  unsigned int i = 0;

  while (**str != '\0' && !succ && !(*fail_flag)) {
    if (!s21_isspace(**str)) {
      succ = 1;
      while (**str != '\0' && !(*fail_flag)) {
        buff[i] = **str;
        i++;

        if (tok->width == WIDTH_NUMBER && i >= tok->width_n) {
          break;
        }

        (*str)++;

        if (s21_isspace(**str)) {
          (*str)--;
          break;
        }
      }
    }

    (*str)++;
  }

  if (tok->width != WIDTH_AST && succ) {
    s21_strcpy((char *)tok->addr, buff);
    (*res)++;
  }
}

void write_unsigned_to_memory(char **str, int *fail_flag, int *res,
                              token *tok) {
  *fail_flag = 1;
  skip_spaces_in_str(str);
  char buff[BUFF_SIZE] = {'\0'};

  if (s21_strspn(*str, "0123456789+-")) {
    int sign = s21_strspn(*str, "+-");
    if (!((sign > 1 || (sign && (tok->width_n <= 1 && tok->width))))) {
      *fail_flag = 0;
      buff[0] = **str;
      (*str)++;

      write_chars_to_buff(str, "0123456789", buff, tok->width_n, 1);

      if (tok->width != WIDTH_AST) (*res)++;
    }
  }

  unsigned long long int result = s21_atoi(buff);

  if (tok->width != WIDTH_AST && !*fail_flag)
    unsigned_type_converter(tok, result, 1);

  if (tok->width != WIDTH_NUMBER)
    write_chars_to_buff(str, "0123456789", S21_NULL, 0, 0);
}

void write_hex_or_oct_to_memory(char **str, int *fail_flag, int *res,
                                token *tok, int base) {
  char *ptr = S21_NULL;
  int sign = 1;

  skip_spaces_in_str(str);

  if (**str == '-') {
    tok->width_n--;
    sign = -1;
    (*str)++;
  }
  if (s21_strspn(*str, "0123456789abcdefABCDEF") > 0 ||
      s21_strspn(*str, "xX0123456789abcdefABCDEF") >= 2) {
    unsigned long long int result = s21_strntollu(
        *str, &ptr, base, tok->width ? tok->width_n : s21_strlen(*str));
    if (tok->width != WIDTH_AST) {
      if (tok->spec == 'p')
        *(int *)tok->addr = (int)result;
      else
        unsigned_type_converter(tok, result, sign);
      *res += 1;
    } else {
      s21_strntollu(*str, &ptr, base,
                    tok->width ? tok->width_n : s21_strlen(*str));
    }

  } else {
    *fail_flag = 1;
  }
  *str = ptr;
}

unsigned long long int s21_strntollu(const char *string, char **endptr,
                                     int basis, int n_byte) {
  unsigned long long res = 0;
  short sign = 1;
  if (endptr) *endptr = S21_NULL;
  char dict[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  if (*string == '-') {
    sign = -1;
    string++;
  }
  if (basis == 16 &&
      (!s21_strncmp(string, "0x", 2) || !s21_strncmp(string, "0X", 2)))
    string += 2;
  long long val;
  short exit = 0;
  while (*string && n_byte && !exit) {
    char *tmp2;
    char current_sim =
        (*string >= 'a' && *string <= 'z') ? *string - 'a' + 'A' : *string;
    tmp2 = s21_strchr(dict, (int)current_sim);
    if (!tmp2) {
      exit = 1;
    } else {
      val = (tmp2 - dict) / sizeof(char);

      if (val >= basis) {
        exit = 1;
      } else {
        res = res * basis + val;
        string++;
        n_byte--;
      }
    }
    if (exit) *endptr = (char *)string;
  }
  return res * sign;
}

void unsigned_type_converter(token *tok, unsigned long long int result,
                             int sign) {
  if (tok->length_t == NONE_LENGTH) {
    *(unsigned int *)tok->addr = sign * (unsigned int)result;
  } else if (tok->length_t == LENGTH_SHORT) {
    *(unsigned short int *)tok->addr = sign * (unsigned short int)result;
  } else if (tok->length_t == LENGTH_LONG) {
    *(unsigned long int *)tok->addr = sign * (unsigned long int)result;
  } else if (tok->length_t == LENGTH_LONG_LONG) {
    *(unsigned long long int *)tok->addr =
        sign * (unsigned long long int)result;
  }
}

void int_type_converter(token *tok, long long int result, int sign) {
  if (tok->length_t == NONE_LENGTH) {
    *(int *)tok->addr = sign * (int)result;
  } else if (tok->length_t == LENGTH_SHORT) {
    *(short int *)tok->addr = sign * (short int)result;
  } else if (tok->length_t == LENGTH_LONG) {
    *(long int *)tok->addr = sign * (long int)result;
  } else if (tok->length_t == LENGTH_LONG_LONG) {
    *(long long int *)tok->addr = sign * (long long int)result;
  }
}

void float_type_converter(token *tok, long double result) {
  if (tok->length_t == NONE_LENGTH) *(float *)tok->addr = (float)result;
  if (tok->length_t == LENGTH_LONG) *(double *)tok->addr = (double)result;
  if (tok->length_t == LENGTH_LONG_DOUBLE)
    *(long double *)tok->addr = (long double)result;
}