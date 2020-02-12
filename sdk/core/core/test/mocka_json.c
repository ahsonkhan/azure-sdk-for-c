#include <stdarg.h>
#include <stddef.h>

#include <setjmp.h>
#include <stdint.h>

#include <az_json.h>
#include <az_span.h>

#include <cmocka.h>

static void test_json_token_null(void ** state) {
  (void)state; /* unused */
  az_json_token token = az_json_token_null();
  assert_int_equal(token.kind, AZ_JSON_TOKEN_NULL);
  assert_false(token.value.boolean);
  assert_int_equal(token.value.number, 0);

  assert_ptr_equal(az_span_ptr(token.value.string), NULL);
  assert_int_equal(az_span_capacity(token.value.string), 0);
  assert_int_equal(az_span_length(token.value.string), 0);

  assert_ptr_equal(az_span_ptr(token.value.span), NULL);
  assert_int_equal(az_span_capacity(token.value.span), 0);
  assert_int_equal(az_span_length(token.value.span), 0);
}

static void test_json_token_boolean(void ** state) {
  (void)state; /* unused */
  az_json_token token = az_json_token_boolean(true);
  assert_int_equal(token.kind, AZ_JSON_TOKEN_BOOLEAN);
  assert_true(token.value.boolean);
}

static void test_json_token_number(void ** state) {
  (void)state; /* unused */
  az_json_token token = az_json_token_number(10);
  assert_int_equal(token.kind, AZ_JSON_TOKEN_NUMBER);
  assert_int_equal(token.value.number, 10);

  token = az_json_token_number(-10);
  assert_int_equal(token.kind, AZ_JSON_TOKEN_NUMBER);
  assert_int_equal(token.value.number, -10);
}

static void test_json_parser_init(void ** state) {
  (void)state; /* unused */
  az_span span = AZ_SPAN_FROM_STR("");
  az_json_parser parser;
  assert_int_equal(az_json_parser_init(&parser, span), AZ_OK);
}

int main(void) {
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_json_token_null),
    cmocka_unit_test(test_json_parser_init),
    cmocka_unit_test(test_json_token_boolean),
    cmocka_unit_test(test_json_token_number),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}