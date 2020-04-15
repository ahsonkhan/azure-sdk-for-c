// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_test_definitions.h"
#include <az_json.h>
#include <az_span.h>

#include <setjmp.h>
#include <stdarg.h>

#include <cmocka.h>

#include <_az_cfg.h>

void test_json_token_null(void** state)
{
  (void)state;
  az_json_token token = az_json_token_null();
  assert_int_equal(token.kind, AZ_JSON_TOKEN_NULL);
  assert_false(token._internal.boolean);
  assert_int_equal(token._internal.number, 0);

  assert_ptr_equal(token._internal.string.ptr, NULL);
  assert_int_equal(token._internal.string.size, 0);

  assert_ptr_equal(token._internal.span.ptr, NULL);
  assert_int_equal(token._internal.span.size, 0);
}

void test_json_token_boolean(void** state)
{
  (void)state;
  az_json_token token = az_json_token_boolean(true);
  assert_int_equal(token.kind, AZ_JSON_TOKEN_BOOLEAN);
  assert_true(token._internal.boolean);
}

void test_json_token_number(void** state)
{
  (void)state;
  az_json_token token = az_json_token_number(10);
  assert_int_equal(token.kind, AZ_JSON_TOKEN_NUMBER);
  assert_int_equal(token._internal.number, 10);

  token = az_json_token_number(-10);
  assert_int_equal(token.kind, AZ_JSON_TOKEN_NUMBER);
  // Need to create double and not just use -10 directly or it would fail on x86-intel
  double expected = -10;
  assert_int_equal(token._internal.number, expected);
}

void test_json_parser_init(void** state)
{
  (void)state;
  az_span span = AZ_SPAN_FROM_STR("");
  az_json_parser parser;
  assert_int_equal(az_json_parser_init(&parser, span), AZ_OK);
}
