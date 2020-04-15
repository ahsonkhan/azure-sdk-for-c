// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_test_definitions.h"
#include <az_context.h>
#include <az_http.h>
#include <az_http_internal.h>
#include <az_http_policy_logging_private.h>
#include <az_http_private.h>
#include <az_http_transport.h>
#include <az_log.h>
#include <az_log_internal.h>

#include <setjmp.h>
#include <stdarg.h>

#include <cmocka.h>

#include <_az_cfg.h>

#define TEST_EXPECT_SUCCESS(exp) assert_true(az_succeeded(exp))

static bool _log_invoked_for_http_request = false;
static bool _log_invoked_for_http_response = false;

static inline void _reset_log_invocation_status()
{
  _log_invoked_for_http_request = false;
  _log_invoked_for_http_response = false;
}

static void _log_listener(az_log_classification classification, az_span message)
{
  switch (classification)
  {
    case AZ_LOG_HTTP_REQUEST:
      _log_invoked_for_http_request = true;
      assert_true(az_span_is_content_equal(
          message,
          AZ_SPAN_FROM_STR("HTTP Request : GET https://www.example.com\n"
                           "\tHeader1 : Value1\n"
                           "\tHeader2 : ZZZZYYYYXXXXWWWWVVVVUU ... SSSRRRRQQQQPPPPOOOONNNN\n"
                           "\tHeader3 : 1111112222223333334444 ... 55666666777777888888abc")));
      break;
    case AZ_LOG_HTTP_RESPONSE:
      _log_invoked_for_http_response = true;
      assert_true(az_span_is_content_equal(
          message,
          AZ_SPAN_FROM_STR("HTTP Response (3456ms) : 404 Not Found\n"
                           "\tHeader11 : Value11\n"
                           "\tHeader22 : NNNNOOOOPPPPQQQQRRRRSS ... UUUVVVVWWWWXXXXYYYYZZZZ\n"
                           "\tHeader33\n"
                           "\tHeader44 : cba8888887777776666665 ... 44444333333222222111111\n"
                           "\n"
                           " -> HTTP Request : GET https://www.example.com\n"
                           "\tHeader1 : Value1\n"
                           "\tHeader2 : ZZZZYYYYXXXXWWWWVVVVUU ... SSSRRRRQQQQPPPPOOOONNNN\n"
                           "\tHeader3 : 1111112222223333334444 ... 55666666777777888888abc")));
      break;
    default:
      assert_true(false);
      break;
  }
}

static void _log_listener_NULL(az_log_classification classification, az_span message)
{
  switch (classification)
  {
    case AZ_LOG_HTTP_REQUEST:
      _log_invoked_for_http_request = true;
      assert_true(az_span_is_content_equal(message, AZ_SPAN_FROM_STR("HTTP Request : NULL")));
      break;
    case AZ_LOG_HTTP_RESPONSE:
      _log_invoked_for_http_response = true;
      assert_true(az_span_is_content_equal(message, AZ_SPAN_FROM_STR("")));
      break;
    default:
      assert_true(false);
      break;
  }
}

void test_az_log(void** state)
{
  (void)state;
  // Set up test values etc.
  //  uint8_t hrb_buf[4 * 1024] = { 0 };
  uint8_t headers[4 * 1024] = { 0 };
  _az_http_request hrb = { 0 };
  az_span url = AZ_SPAN_FROM_STR("https://www.example.com");
  TEST_EXPECT_SUCCESS(az_http_request_init(
      &hrb,
      &az_context_app,
      az_http_method_get(),
      url,
      url.size,
      AZ_SPAN_FROM_BUFFER(headers),
      AZ_SPAN_FROM_STR("AAAAABBBBBCCCCCDDDDDEEEEEFFFFFGGGGGHHHHHIIIIIJJJJJKKKKK")));

  TEST_EXPECT_SUCCESS(
      az_http_request_append_header(&hrb, AZ_SPAN_FROM_STR("Header1"), AZ_SPAN_FROM_STR("Value1")));

  TEST_EXPECT_SUCCESS(az_http_request_append_header(
      &hrb,
      AZ_SPAN_FROM_STR("Header2"),
      AZ_SPAN_FROM_STR("ZZZZYYYYXXXXWWWWVVVVUUUUTTTTSSSSRRRRQQQQPPPPOOOONNNN")));

  TEST_EXPECT_SUCCESS(_az_http_request_mark_retry_headers_start(&hrb));

  TEST_EXPECT_SUCCESS(az_http_request_append_header(
      &hrb,
      AZ_SPAN_FROM_STR("Header3"),
      AZ_SPAN_FROM_STR("111111222222333333444444555555666666777777888888abc")));

  uint8_t response_buf[1024] = { 0 };
  az_span response_builder = AZ_SPAN_FROM_BUFFER(response_buf);

  az_span response_span
      = AZ_SPAN_FROM_STR("HTTP/1.1 404 Not Found\r\n"
                         "Header11: Value11\r\n"
                         "Header22: NNNNOOOOPPPPQQQQRRRRSSSSTTTTUUUUVVVVWWWWXXXXYYYYZZZZ\r\n"
                         "Header33:\r\n"
                         "Header44: cba888888777777666666555555444444333333222222111111\r\n"
                         "\r\n"
                         "KKKKKJJJJJIIIIIHHHHHGGGGGFFFFFEEEEEDDDDDCCCCCBBBBBAAAAA");
  az_span_copy(response_builder, response_span);
  response_builder = az_span_slice(response_builder, 0, response_span.size);
  assert_int_equal(response_builder.size, response_span.size);

  az_http_response response = { 0 };
  TEST_EXPECT_SUCCESS(az_http_response_init(&response, response_builder));
  // Finish setting up

  {
    // null request
    _reset_log_invocation_status();
    az_log_set_callback(_log_listener_NULL);
    _az_http_policy_logging_log_http_request(NULL);
    assert_true(_log_invoked_for_http_request == true);
    assert_true(_log_invoked_for_http_response == false);
  }
  // Actual test below
  {
    // Verify that log callback gets invoked, and with the correct classification type.
    // Also, our callback function does the verification for the message content.
    _reset_log_invocation_status();
    az_log_set_callback(_log_listener);
    assert_true(_log_invoked_for_http_request == false);
    assert_true(_log_invoked_for_http_response == false);

    _az_http_policy_logging_log_http_request(&hrb);
    assert_true(_log_invoked_for_http_request == true);
    assert_true(_log_invoked_for_http_response == false);

    _az_http_policy_logging_log_http_response(&response, 3456, &hrb);
    assert_true(_log_invoked_for_http_request == true);
    assert_true(_log_invoked_for_http_response == true);
  }
  {
    _reset_log_invocation_status();
    az_log_set_callback(NULL);

    // Verify that user can unset log callback, and we are not going to call the previously set one.
    assert_true(_log_invoked_for_http_request == false);
    assert_true(_log_invoked_for_http_response == false);

    _az_http_policy_logging_log_http_request(&hrb);
    _az_http_policy_logging_log_http_response(&response, 3456, &hrb);

    assert_true(_log_invoked_for_http_request == false);
    assert_true(_log_invoked_for_http_response == false);

    {
      // Verify that our internal should_write() function would return false if none is listening.
      assert_true(az_log_should_write(AZ_LOG_HTTP_REQUEST) == false);
      assert_true(az_log_should_write(AZ_LOG_HTTP_RESPONSE) == false);

      // If a callback is set, and no classifications are specified, we are going to log all of them
      // (and customer is going to get all of them).
      az_log_set_callback(_log_listener);

      assert_true(az_log_should_write(AZ_LOG_HTTP_REQUEST) == true);
      assert_true(az_log_should_write(AZ_LOG_HTTP_RESPONSE) == true);
    }

    // Verify that if customer specifies the classifications, we'll only invoking the logging
    // callback with the classification that's in the list of customer-provided classifications, and
    // nothing is going to happen when our code attempts to log a classification that's not in that
    // list.
    az_log_classification const classifications[] = { AZ_LOG_HTTP_REQUEST, AZ_LOG_END_OF_LIST };
    az_log_set_classifications(classifications);

    assert_true(az_log_should_write(AZ_LOG_HTTP_REQUEST) == true);
    assert_true(az_log_should_write(AZ_LOG_HTTP_RESPONSE) == false);

    _az_http_policy_logging_log_http_request(&hrb);
    _az_http_policy_logging_log_http_response(&response, 3456, &hrb);

    assert_true(_log_invoked_for_http_request == true);
    assert_true(_log_invoked_for_http_response == false);
  }

  az_log_set_classifications(NULL);
  az_log_set_callback(NULL);
}
