// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <limits>
#include <utility>

#include "components/cbor/cbor_reader.h"

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

/* Leveraging RFC 7049 examples from
   https://github.com/cbor/test-vectors/blob/master/appendix_a.json. */
namespace cbor {

TEST(CBORReaderTest, TestReadUint) {
  struct UintTestCase {
    const int64_t value;
    const std::vector<uint8_t> cbor_data;
  };

  static const UintTestCase kUintTestCases[] = {
      {0, {0x00}},
      {1, {0x01}},
      {23, {0x17}},
      {24, {0x18, 0x18}},
      {std::numeric_limits<uint8_t>::max(), {0x18, 0xff}},
      {1LL << 8, {0x19, 0x01, 0x00}},
      {std::numeric_limits<uint16_t>::max(), {0x19, 0xff, 0xff}},
      {1LL << 16, {0x1a, 0x00, 0x01, 0x00, 0x00}},
      {std::numeric_limits<uint32_t>::max(), {0x1a, 0xff, 0xff, 0xff, 0xff}},
      {1LL << 32, {0x1b, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00}},
      {std::numeric_limits<int64_t>::max(),
       {0x1b, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}},
  };

  for (const UintTestCase& test_case : kUintTestCases) {
    SCOPED_TRACE(testing::Message() << "testing uint: " << test_case.value);

    base::Optional<CBORValue> cbor = CBORReader::Read(test_case.cbor_data);
    ASSERT_TRUE(cbor.has_value());
    ASSERT_EQ(cbor.value().type(), CBORValue::Type::UNSIGNED);
    EXPECT_EQ(cbor.value().GetInteger(), test_case.value);
  }
}

TEST(CBORReaderTest, TestUintEncodedWithNonMinimumByteLength) {
  static const std::vector<uint8_t> non_minimal_uint_encodings[] = {
      // Uint 23 encoded with 1 byte.
      {0x18, 0x17},
      // Uint 255 encoded with 2 bytes.
      {0x19, 0x00, 0xff},
      // Uint 65535 encoded with 4 byte.
      {0x1a, 0x00, 0x00, 0xff, 0xff},
      // Uint 4294967295 encoded with 8 byte.
      {0x1b, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff},

      // When decoding byte has more than one syntax error, the first syntax
      // error encountered during deserialization is returned as the error code.
      {
          0xa2,        // map with non-minimally encoded key
          0x17,        // key 24
          0x61, 0x42,  // value :"B"

          0x18, 0x17,  // key 23 encoded with extra byte
          0x61, 0x45   // value "E"
      },
      {
          0xa2,        // map with out of order and non-minimally encoded key
          0x18, 0x17,  // key 23 encoded with extra byte
          0x61, 0x45,  // value "E"
          0x17,        // key 23
          0x61, 0x42   // value :"B"
      },
      {
          0xa2,        // map with duplicate non-minimally encoded key
          0x18, 0x17,  // key 23 encoded with extra byte
          0x61, 0x45,  // value "E"
          0x18, 0x17,  // key 23 encoded with extra byte
          0x61, 0x42   // value :"B"
      },
  };

  int test_case_index = 0;
  CBORReader::DecoderError error_code;
  for (const auto& non_minimal_uint : non_minimal_uint_encodings) {
    SCOPED_TRACE(testing::Message()
                 << "testing element at index : " << test_case_index++);

    base::Optional<CBORValue> cbor =
        CBORReader::Read(non_minimal_uint, &error_code);
    EXPECT_FALSE(cbor.has_value());
    EXPECT_EQ(error_code, CBORReader::DecoderError::NON_MINIMAL_CBOR_ENCODING);
  }
}

TEST(CBORReaderTest, TestReadNegativeInt) {
  struct NegativeIntTestCase {
    const int64_t negative_int;
    const std::vector<uint8_t> cbor_data;
  };

  static const NegativeIntTestCase kNegativeIntTestCases[] = {
      {-1LL, {0x20}},
      {-24LL, {0x37}},
      {-25LL, {0x38, 0x18}},
      {-256LL, {0x38, 0xff}},
      {-1000LL, {0x39, 0x03, 0xe7}},
      {-1000000LL, {0x3a, 0x00, 0x0f, 0x42, 0x3f}},
      {-4294967296LL, {0x3a, 0xff, 0xff, 0xff, 0xff}},
      {std::numeric_limits<int64_t>::min(),
       {0x3b, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}}};

  for (const NegativeIntTestCase& test_case : kNegativeIntTestCases) {
    SCOPED_TRACE(testing::Message()
                 << "testing negative int : " << test_case.negative_int);

    base::Optional<CBORValue> cbor = CBORReader::Read(test_case.cbor_data);
    ASSERT_TRUE(cbor.has_value());
    ASSERT_EQ(cbor.value().type(), CBORValue::Type::NEGATIVE);
    EXPECT_EQ(cbor.value().GetInteger(), test_case.negative_int);
  }
}

TEST(CBORReaderTest, TestReadBytes) {
  struct ByteTestCase {
    const std::vector<uint8_t> value;
    const std::vector<uint8_t> cbor_data;
  };

  static const ByteTestCase kByteStringTestCases[] = {
      // clang-format off
      {{}, {0x40}},
      {{0x01, 0x02, 0x03, 0x04}, {0x44, 0x01, 0x02, 0x03, 0x04}},
      // clang-format on
  };

  int element_index = 0;
  for (const ByteTestCase& test_case : kByteStringTestCases) {
    SCOPED_TRACE(testing::Message()
                 << "testing string test case at : " << element_index++);

    base::Optional<CBORValue> cbor = CBORReader::Read(test_case.cbor_data);
    ASSERT_TRUE(cbor.has_value());
    ASSERT_EQ(cbor.value().type(), CBORValue::Type::BYTE_STRING);
    EXPECT_EQ(cbor.value().GetBytestring(), test_case.value);
  }
}

TEST(CBORReaderTest, TestReadString) {
  struct StringTestCase {
    const std::string value;
    const std::vector<uint8_t> cbor_data;
  };

  static const StringTestCase kStringTestCases[] = {
      {"", {0x60}},
      {"a", {0x61, 0x61}},
      {"IETF", {0x64, 0x49, 0x45, 0x54, 0x46}},
      {"\"\\", {0x62, 0x22, 0x5c}},
      {"\xc3\xbc", {0x62, 0xc3, 0xbc}},
      {"\xe6\xb0\xb4", {0x63, 0xe6, 0xb0, 0xb4}},
      {"\xf0\x90\x85\x91", {0x64, 0xf0, 0x90, 0x85, 0x91}},
  };

  for (const StringTestCase& test_case : kStringTestCases) {
    SCOPED_TRACE(testing::Message()
                 << "testing string value : " << test_case.value);

    base::Optional<CBORValue> cbor = CBORReader::Read(test_case.cbor_data);
    ASSERT_TRUE(cbor.has_value());
    ASSERT_EQ(cbor.value().type(), CBORValue::Type::STRING);
    EXPECT_EQ(cbor.value().GetString(), test_case.value);
  }
}

TEST(CBORReaderTest, TestReadStringWithNUL) {
  static const struct {
    const std::string value;
    const std::vector<uint8_t> cbor_data;
  } kStringTestCases[] = {
      {std::string("string_without_nul"),
       {0x72, 0x73, 0x74, 0x72, 0x69, 0x6E, 0x67, 0x5F, 0x77, 0x69, 0x74, 0x68,
        0x6F, 0x75, 0x74, 0x5F, 0x6E, 0x75, 0x6C}},
      {std::string("nul_terminated_string\0", 22),
       {0x76, 0x6E, 0x75, 0x6C, 0x5F, 0x74, 0x65, 0x72, 0x6D, 0x69, 0x6E, 0x61,
        0x74, 0x65, 0x64, 0x5F, 0x73, 0x74, 0x72, 0x69, 0x6E, 0x67, 0x00}},
      {std::string("embedded\0nul", 12),
       {0x6C, 0x65, 0x6D, 0x62, 0x65, 0x64, 0x64, 0x65, 0x64, 0x00, 0x6E, 0x75,
        0x6C}},
      {std::string("trailing_nuls\0\0", 15),
       {0x6F, 0x74, 0x72, 0x61, 0x69, 0x6C, 0x69, 0x6E, 0x67, 0x5F, 0x6E, 0x75,
        0x6C, 0x73, 0x00, 0x00}},
  };

  for (const auto& test_case : kStringTestCases) {
    SCOPED_TRACE(testing::Message()
                 << "testing string with nul bytes :" << test_case.value);

    base::Optional<CBORValue> cbor = CBORReader::Read(test_case.cbor_data);
    ASSERT_TRUE(cbor.has_value());
    ASSERT_EQ(cbor.value().type(), CBORValue::Type::STRING);
    EXPECT_EQ(cbor.value().GetString(), test_case.value);
  }
}

TEST(CBORReaderTest, TestReadStringWithInvalidByteSequenceAfterNUL) {
  // UTF-8 validation should not stop at the first NUL character in the string.
  // That is, a string with an invalid byte sequence should fail UTF-8
  // validation even if the invalid character is located after one or more NUL
  // characters. Here, 0xA6 is an unexpected continuation byte.
  static const std::vector<uint8_t> string_with_invalid_continuation_byte = {
      0x63, 0x00, 0x00, 0xA6};
  CBORReader::DecoderError error_code;
  base::Optional<CBORValue> cbor =
      CBORReader::Read(string_with_invalid_continuation_byte, &error_code);
  EXPECT_FALSE(cbor.has_value());
  EXPECT_EQ(error_code, CBORReader::DecoderError::INVALID_UTF8);
}

TEST(CBORReaderTest, TestReadArray) {
  static const std::vector<uint8_t> kArrayTestCaseCbor = {
      // clang-format off
      0x98, 0x19,  // array of 25 elements
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c,
        0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
        0x18, 0x18, 0x19,
      // clang-format on
  };

  base::Optional<CBORValue> cbor = CBORReader::Read(kArrayTestCaseCbor);
  ASSERT_TRUE(cbor.has_value());
  const CBORValue cbor_array = std::move(cbor.value());
  ASSERT_EQ(cbor_array.type(), CBORValue::Type::ARRAY);
  ASSERT_THAT(cbor_array.GetArray(), testing::SizeIs(25));

  std::vector<CBORValue> array;
  for (int i = 0; i < 25; i++) {
    SCOPED_TRACE(testing::Message() << "testing array element at index " << i);

    ASSERT_EQ(cbor_array.GetArray()[i].type(), CBORValue::Type::UNSIGNED);
    EXPECT_EQ(cbor_array.GetArray()[i].GetInteger(),
              static_cast<int64_t>(i + 1));
  }
}

TEST(CBORReaderTest, TestReadMapWithMapValue) {
  static const std::vector<uint8_t> kMapTestCaseCbor = {
      // clang-format off
      0xa4,  // map with 4 key value pairs:
        0x18, 0x18,  // 24
        0x63, 0x61, 0x62, 0x63,  // "abc"

        0x60,  // ""
        0x61, 0x2e,  // "."

        0x61, 0x62,  // "b"
        0x61, 0x42,  // "B"

        0x62, 0x61, 0x61,  // "aa"
        0x62, 0x41, 0x41,  // "AA"
      // clang-format on
  };

  base::Optional<CBORValue> cbor = CBORReader::Read(kMapTestCaseCbor);
  ASSERT_TRUE(cbor.has_value());
  const CBORValue cbor_val = std::move(cbor.value());
  ASSERT_EQ(cbor_val.type(), CBORValue::Type::MAP);
  ASSERT_EQ(cbor_val.GetMap().size(), 4u);

  const CBORValue key_uint(24);
  ASSERT_EQ(cbor_val.GetMap().count(key_uint), 1u);
  ASSERT_EQ(cbor_val.GetMap().find(key_uint)->second.type(),
            CBORValue::Type::STRING);
  EXPECT_EQ(cbor_val.GetMap().find(key_uint)->second.GetString(), "abc");

  const CBORValue key_empty_string("");
  ASSERT_EQ(cbor_val.GetMap().count(key_empty_string), 1u);
  ASSERT_EQ(cbor_val.GetMap().find(key_empty_string)->second.type(),
            CBORValue::Type::STRING);
  EXPECT_EQ(cbor_val.GetMap().find(key_empty_string)->second.GetString(), ".");

  const CBORValue key_b("b");
  ASSERT_EQ(cbor_val.GetMap().count(key_b), 1u);
  ASSERT_EQ(cbor_val.GetMap().find(key_b)->second.type(),
            CBORValue::Type::STRING);
  EXPECT_EQ(cbor_val.GetMap().find(key_b)->second.GetString(), "B");

  const CBORValue key_aa("aa");
  ASSERT_EQ(cbor_val.GetMap().count(key_aa), 1u);
  ASSERT_EQ(cbor_val.GetMap().find(key_aa)->second.type(),
            CBORValue::Type::STRING);
  EXPECT_EQ(cbor_val.GetMap().find(key_aa)->second.GetString(), "AA");
}

TEST(CBORReaderTest, TestReadMapWithIntegerKeys) {
  static const std::vector<uint8_t> kMapWithIntegerKeyCbor = {
      // clang-format off
      0xA4,                 // map with 4 key value pairs
         0x01,              // key : 1
         0x61, 0x61,        // value : "a"

         0x09,              // key : 9
         0x61, 0x62,        // value : "b"

         0x19, 0x03, 0xE7,  // key : 999
         0x61, 0x63,        // value "c"

         0x19, 0x04, 0x57,  // key : 1111
         0x61, 0x64,        // value : "d"
      // clang-format on
  };

  base::Optional<CBORValue> cbor = CBORReader::Read(kMapWithIntegerKeyCbor);
  ASSERT_TRUE(cbor.has_value());
  const CBORValue cbor_val = std::move(cbor.value());
  ASSERT_EQ(cbor_val.type(), CBORValue::Type::MAP);
  ASSERT_EQ(cbor_val.GetMap().size(), 4u);

  const CBORValue key_1(1);
  ASSERT_EQ(cbor_val.GetMap().count(key_1), 1u);
  ASSERT_EQ(cbor_val.GetMap().find(key_1)->second.type(),
            CBORValue::Type::STRING);
  EXPECT_EQ(cbor_val.GetMap().find(key_1)->second.GetString(), "a");

  const CBORValue key_9(9);
  ASSERT_EQ(cbor_val.GetMap().count(key_9), 1u);
  ASSERT_EQ(cbor_val.GetMap().find(key_9)->second.type(),
            CBORValue::Type::STRING);
  EXPECT_EQ(cbor_val.GetMap().find(key_9)->second.GetString(), "b");

  const CBORValue key_999(999);
  ASSERT_EQ(cbor_val.GetMap().count(key_999), 1u);
  ASSERT_EQ(cbor_val.GetMap().find(key_999)->second.type(),
            CBORValue::Type::STRING);
  EXPECT_EQ(cbor_val.GetMap().find(key_999)->second.GetString(), "c");

  const CBORValue key_1111(1111);
  ASSERT_EQ(cbor_val.GetMap().count(key_1111), 1u);
  ASSERT_EQ(cbor_val.GetMap().find(key_1111)->second.type(),
            CBORValue::Type::STRING);
  EXPECT_EQ(cbor_val.GetMap().find(key_1111)->second.GetString(), "d");
}

TEST(CBORReaderTest, TestReadMapWithNegativeIntegersKeys) {
  static const std::vector<uint8_t> kMapWithIntegerKeyCbor = {
      // clang-format off
      0xA3,                 // map with 3 key value pairs
         0x20,              // key : -1
         0x01,

         0x21,              // key : -2
         0x02,

         0x38, 0x63,        // key : -100
         0x03,
      // clang-format on
  };

  base::Optional<CBORValue> cbor = CBORReader::Read(kMapWithIntegerKeyCbor);
  ASSERT_TRUE(cbor.has_value());
  const CBORValue cbor_val = std::move(cbor.value());
  ASSERT_EQ(cbor_val.type(), CBORValue::Type::MAP);
  ASSERT_EQ(cbor_val.GetMap().size(), 3u);

  const CBORValue key_1(-1);
  ASSERT_EQ(cbor_val.GetMap().count(key_1), 1u);
  ASSERT_EQ(cbor_val.GetMap().find(key_1)->second.type(),
            CBORValue::Type::UNSIGNED);
  EXPECT_EQ(cbor_val.GetMap().find(key_1)->second.GetInteger(), 1);

  const CBORValue key_2(-2);
  ASSERT_EQ(cbor_val.GetMap().count(key_2), 1u);
  ASSERT_EQ(cbor_val.GetMap().find(key_2)->second.type(),
            CBORValue::Type::UNSIGNED);
  EXPECT_EQ(cbor_val.GetMap().find(key_2)->second.GetInteger(), 2);

  const CBORValue key_100(-100);
  ASSERT_EQ(cbor_val.GetMap().count(key_100), 1u);
  ASSERT_EQ(cbor_val.GetMap().find(key_100)->second.type(),
            CBORValue::Type::UNSIGNED);
  EXPECT_EQ(cbor_val.GetMap().find(key_100)->second.GetInteger(), 3);
}

TEST(CBORReaderTest, TestReadMapWithArray) {
  static const std::vector<uint8_t> kMapArrayTestCaseCbor = {
      // clang-format off
      0xa2,  // map of 2 pairs
        0x61, 0x61,  // "a"
        0x01,

        0x61, 0x62,  // "b"
        0x82,        // array with 2 elements
          0x02,
          0x03,
      // clang-format on
  };

  base::Optional<CBORValue> cbor = CBORReader::Read(kMapArrayTestCaseCbor);
  ASSERT_TRUE(cbor.has_value());
  const CBORValue cbor_val = std::move(cbor.value());
  ASSERT_EQ(cbor_val.type(), CBORValue::Type::MAP);
  ASSERT_EQ(cbor_val.GetMap().size(), 2u);

  const CBORValue key_a("a");
  ASSERT_EQ(cbor_val.GetMap().count(key_a), 1u);
  ASSERT_EQ(cbor_val.GetMap().find(key_a)->second.type(),
            CBORValue::Type::UNSIGNED);
  EXPECT_EQ(cbor_val.GetMap().find(key_a)->second.GetInteger(), 1u);

  const CBORValue key_b("b");
  ASSERT_EQ(cbor_val.GetMap().count(key_b), 1u);
  ASSERT_EQ(cbor_val.GetMap().find(key_b)->second.type(),
            CBORValue::Type::ARRAY);

  const CBORValue nested_array = cbor_val.GetMap().find(key_b)->second.Clone();
  ASSERT_EQ(nested_array.GetArray().size(), 2u);
  for (int i = 0; i < 2; i++) {
    ASSERT_THAT(nested_array.GetArray()[i].type(), CBORValue::Type::UNSIGNED);
    EXPECT_EQ(nested_array.GetArray()[i].GetInteger(),
              static_cast<int64_t>(i + 2));
  }
}

TEST(CBORReaderTest, TestReadMapWithTextStringKeys) {
  static const std::vector<uint8_t> kMapTestCase{
      // clang-format off
      0xa2,  // map of 2 pairs
        0x61, 'k', // text string "k"
        0x61, 'v',

        0x63, 'f', 'o', 'o', // text string "foo"
        0x63, 'b', 'a', 'r',
      // clang-format on
  };

  CBORReader::DecoderError error_code;
  base::Optional<CBORValue> cbor = CBORReader::Read(kMapTestCase, &error_code);
  ASSERT_TRUE(cbor.has_value());
  ASSERT_EQ(cbor->type(), CBORValue::Type::MAP);
  ASSERT_EQ(cbor->GetMap().size(), 2u);

  const CBORValue key_k("k");
  ASSERT_EQ(cbor->GetMap().count(key_k), 1u);
  ASSERT_EQ(cbor->GetMap().find(key_k)->second.type(), CBORValue::Type::STRING);
  EXPECT_EQ(cbor->GetMap().find(key_k)->second.GetString(), "v");

  const CBORValue key_foo("foo");
  ASSERT_EQ(cbor->GetMap().count(key_foo), 1u);
  ASSERT_EQ(cbor->GetMap().find(key_foo)->second.type(),
            CBORValue::Type::STRING);
  EXPECT_EQ(cbor->GetMap().find(key_foo)->second.GetString(), "bar");
}

TEST(CBORReaderTest, TestReadMapWithByteStringKeys) {
  static const std::vector<uint8_t> kMapTestCase{
      // clang-format off
      0xa2,  // map of 2 pairs
        0x41, 'k', // byte string "k"
        0x41, 'v',

        0x43, 'f', 'o', 'o', // byte string "foo"
        0x43, 'b', 'a', 'r',
      // clang-format on
  };

  CBORReader::DecoderError error_code;
  base::Optional<CBORValue> cbor = CBORReader::Read(kMapTestCase, &error_code);
  ASSERT_TRUE(cbor.has_value());
  ASSERT_EQ(cbor->type(), CBORValue::Type::MAP);
  ASSERT_EQ(cbor->GetMap().size(), 2u);

  const CBORValue key_k(std::vector<uint8_t>{'k'});
  ASSERT_EQ(cbor->GetMap().count(key_k), 1u);
  ASSERT_EQ(cbor->GetMap().find(key_k)->second.type(),
            CBORValue::Type::BYTE_STRING);
  EXPECT_EQ(cbor->GetMap().find(key_k)->second.GetBytestring(),
            std::vector<uint8_t>{'v'});

  const CBORValue key_foo(std::vector<uint8_t>{'f', 'o', 'o'});
  ASSERT_EQ(cbor->GetMap().count(key_foo), 1u);
  ASSERT_EQ(cbor->GetMap().find(key_foo)->second.type(),
            CBORValue::Type::BYTE_STRING);
  static const std::vector<uint8_t> kBarBytes{'b', 'a', 'r'};
  EXPECT_EQ(cbor->GetMap().find(key_foo)->second.GetBytestring(), kBarBytes);
}

TEST(CBORReaderTest, TestReadMapWithMixedKeys) {
  // Example adopted from:
  // https://wicg.github.io/webpackage/draft-yasskin-http-origin-signed-responses.html
  static const uint8_t kMapTestCase[] = {
      // clang-format off
      0xa6, // map of 6 pairs
        0x0a, // 10
        0x00,

        0x18, 0x64, // 100
        0x01,

        0x20, // -1
        0x02,

        // This entry is not in the example, but added to test byte string key
        0x42, 'x', 'y', // byte string "xy"
        0x03,

        0x61, 'z', // text string "z"
        0x04,

        0x62, 'a', 'a', // text string "aa"
        0x05,

      /*
        0x81, 0x18, 0x64, // [100] (array as map key is not yet supported)
        0x06,

        0x81, 0x20,  // [-1] (array as map key is not yet supported)
        0x07,

        0xf4, // false (boolean  as map key is not yet supported)
        0x08,
      */
      // clang-format on
  };

  CBORReader::DecoderError error_code;
  base::Optional<CBORValue> cbor = CBORReader::Read(kMapTestCase, &error_code);
  ASSERT_TRUE(cbor.has_value());
  ASSERT_EQ(cbor->type(), CBORValue::Type::MAP);
  ASSERT_EQ(cbor->GetMap().size(), 6u);

  std::vector<CBORValue> keys;
  keys.emplace_back(10);
  keys.emplace_back(100);
  keys.emplace_back(-1);
  keys.emplace_back(CBORValue::BinaryValue{'x', 'y'});
  keys.emplace_back("z");
  keys.emplace_back("aa");
  for (size_t i = 0; i < keys.size(); ++i) {
    SCOPED_TRACE(testing::Message() << "testing key at index: " << i);
    ASSERT_EQ(cbor->GetMap().count(keys[i]), 1u);
    ASSERT_EQ(cbor->GetMap().find(keys[i])->second.type(),
              CBORValue::Type::UNSIGNED);
    EXPECT_EQ(cbor->GetMap().find(keys[i])->second.GetInteger(),
              static_cast<int>(i));
  }
}

TEST(CBORReaderTest, TestReadNestedMap) {
  static const std::vector<uint8_t> kNestedMapTestCase = {
      // clang-format off
      0xa2,  // map of 2 pairs
        0x61, 0x61,  // "a"
        0x01,

        0x61, 0x62,  // "b"
        0xa2,        // map of 2 pairs
          0x61, 0x63,  // "c"
          0x02,

          0x61, 0x64,  // "d"
          0x03,
      // clang-format on
  };

  base::Optional<CBORValue> cbor = CBORReader::Read(kNestedMapTestCase);
  ASSERT_TRUE(cbor.has_value());
  const CBORValue cbor_val = std::move(cbor.value());
  ASSERT_EQ(cbor_val.type(), CBORValue::Type::MAP);
  ASSERT_EQ(cbor_val.GetMap().size(), 2u);

  const CBORValue key_a("a");
  ASSERT_EQ(cbor_val.GetMap().count(key_a), 1u);
  ASSERT_EQ(cbor_val.GetMap().find(key_a)->second.type(),
            CBORValue::Type::UNSIGNED);
  EXPECT_EQ(cbor_val.GetMap().find(key_a)->second.GetInteger(), 1u);

  const CBORValue key_b("b");
  ASSERT_EQ(cbor_val.GetMap().count(key_b), 1u);
  const CBORValue nested_map = cbor_val.GetMap().find(key_b)->second.Clone();
  ASSERT_EQ(nested_map.type(), CBORValue::Type::MAP);
  ASSERT_EQ(nested_map.GetMap().size(), 2u);

  const CBORValue key_c("c");
  ASSERT_EQ(nested_map.GetMap().count(key_c), 1u);
  ASSERT_EQ(nested_map.GetMap().find(key_c)->second.type(),
            CBORValue::Type::UNSIGNED);
  EXPECT_EQ(nested_map.GetMap().find(key_c)->second.GetInteger(), 2u);

  const CBORValue key_d("d");
  ASSERT_EQ(nested_map.GetMap().count(key_d), 1u);
  ASSERT_EQ(nested_map.GetMap().find(key_d)->second.type(),
            CBORValue::Type::UNSIGNED);
  EXPECT_EQ(nested_map.GetMap().find(key_d)->second.GetInteger(), 3u);
}

TEST(CBORReaderTest, TestIntegerRange) {
  static const std::vector<uint8_t> kMaxPositiveInt = {
      0x1b, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  static const std::vector<uint8_t> kMinNegativeInt = {
      0x3b, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

  base::Optional<CBORValue> max_positive_int =
      CBORReader::Read(kMaxPositiveInt);
  ASSERT_TRUE(max_positive_int.has_value());
  EXPECT_EQ(max_positive_int.value().GetInteger(), INT64_MAX);

  base::Optional<CBORValue> min_negative_int =
      CBORReader::Read(kMinNegativeInt);
  ASSERT_TRUE(min_negative_int.has_value());
  EXPECT_EQ(min_negative_int.value().GetInteger(), INT64_MIN);
}

TEST(CBORReaderTest, TestIntegerOutOfRangeError) {
  static const std::vector<uint8_t> kOutOfRangePositiveInt = {
      0x1b, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

  static const std::vector<uint8_t> kOutOfRangeNegativeInt = {
      0x3b, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

  CBORReader::DecoderError error_code;
  base::Optional<CBORValue> positive_int_out_of_range_cbor =
      CBORReader::Read(kOutOfRangePositiveInt, &error_code);
  EXPECT_FALSE(positive_int_out_of_range_cbor.has_value());
  EXPECT_EQ(error_code, CBORReader::DecoderError::OUT_OF_RANGE_INTEGER_VALUE);

  base::Optional<CBORValue> negative_int_out_of_range_cbor =
      CBORReader::Read(kOutOfRangeNegativeInt, &error_code);
  EXPECT_FALSE(negative_int_out_of_range_cbor.has_value());
  EXPECT_EQ(error_code, CBORReader::DecoderError::OUT_OF_RANGE_INTEGER_VALUE);
}

TEST(CBORReaderTest, TestReadSimpleValue) {
  static const struct {
    const CBORValue::SimpleValue value;
    const std::vector<uint8_t> cbor_data;
  } kSimpleValueTestCases[] = {
      {CBORValue::SimpleValue::FALSE_VALUE, {0xf4}},
      {CBORValue::SimpleValue::TRUE_VALUE, {0xf5}},
      {CBORValue::SimpleValue::NULL_VALUE, {0xf6}},
      {CBORValue::SimpleValue::UNDEFINED, {0xf7}},
  };

  int test_element_index = 0;
  for (const auto& test_case : kSimpleValueTestCases) {
    SCOPED_TRACE(testing::Message()
                 << "testing simple value at index : " << test_element_index++);

    base::Optional<CBORValue> cbor = CBORReader::Read(test_case.cbor_data);
    ASSERT_TRUE(cbor.has_value());
    ASSERT_EQ(cbor.value().type(), CBORValue::Type::SIMPLE_VALUE);
    EXPECT_EQ(cbor.value().GetSimpleValue(), test_case.value);
  }
}

TEST(CBORReaderTest, TestReadUnsupportedFloatingPointNumbers) {
  static const std::vector<uint8_t> floating_point_cbors[] = {
      // 16 bit floating point value.
      {0xf9, 0x10, 0x00},
      // 32 bit floating point value.
      {0xfa, 0x10, 0x00, 0x00, 0x00},
      // 64 bit floating point value.
      {0xfb, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

  for (const auto& unsupported_floating_point : floating_point_cbors) {
    SCOPED_TRACE(testing::Message()
                 << "testing unsupported floating point : "
                 << testing::PrintToString(unsupported_floating_point));
    CBORReader::DecoderError error_code;
    base::Optional<CBORValue> cbor =
        CBORReader::Read(unsupported_floating_point, &error_code);
    EXPECT_FALSE(cbor.has_value());
    EXPECT_EQ(error_code,
              CBORReader::DecoderError::UNSUPPORTED_FLOATING_POINT_VALUE);
  }
}

TEST(CBORReaderTest, TestIncompleteCBORDataError) {
  static const std::vector<uint8_t> incomplete_cbor_list[] = {
      // Additional info byte corresponds to unsigned int that corresponds
      // to 2 additional bytes. But actual data encoded  in one byte.
      {0x19, 0x03},
      // CBOR bytestring of length 3 encoded with additional info of length 4.
      {0x44, 0x01, 0x02, 0x03},
      // CBOR string data "IETF" of length 4 encoded with additional info of
      // length 5.
      {0x65, 0x49, 0x45, 0x54, 0x46},
      // CBOR array of length 1 encoded with additional info of length 2.
      {0x82, 0x02},
      // CBOR map with single key value pair encoded with additional info of
      // length 2.
      {0xa2, 0x61, 0x61, 0x01},
  };

  int test_element_index = 0;
  for (const auto& incomplete_data : incomplete_cbor_list) {
    SCOPED_TRACE(testing::Message() << "testing incomplete data at index : "
                                    << test_element_index++);

    CBORReader::DecoderError error_code;
    base::Optional<CBORValue> cbor =
        CBORReader::Read(incomplete_data, &error_code);
    EXPECT_FALSE(cbor.has_value());
    EXPECT_EQ(error_code, CBORReader::DecoderError::INCOMPLETE_CBOR_DATA);
  }
}

// While RFC 7049 allows CBOR map keys with all types, current decoder only
// supports unsigned integer and string keys.
TEST(CBORReaderTest, TestUnsupportedMapKeyFormatError) {
  static const std::vector<uint8_t> kMapWithUintKey = {
      // clang-format off
      0xa2,        // map of 2 pairs

        0x82, 0x01, 0x02,  // invalid key : [1, 2]
        0x02,              // value : 2

        0x61, 0x64,  // key : "d"
        0x03,        // value : 3
      // clang-format on
  };

  CBORReader::DecoderError error_code;
  base::Optional<CBORValue> cbor =
      CBORReader::Read(kMapWithUintKey, &error_code);
  EXPECT_FALSE(cbor.has_value());
  EXPECT_EQ(error_code, CBORReader::DecoderError::INCORRECT_MAP_KEY_TYPE);
}

TEST(CBORReaderTest, TestUnknownAdditionalInfoError) {
  static const std::vector<uint8_t> kUnknownAdditionalInfoList[] = {
      // "IETF" encoded with major type 3 and additional info of 28.
      {0x7C, 0x49, 0x45, 0x54, 0x46},
      // "\"\\" encoded with major type 3 and additional info of 29.
      {0x7D, 0x22, 0x5c},
      // "\xc3\xbc" encoded with major type 3 and additional info of 30.
      {0x7E, 0xc3, 0xbc},
      // "\xe6\xb0\xb4" encoded with major type 3 and additional info of 31.
      {0x7F, 0xe6, 0xb0, 0xb4},
      // Major type 7, additional information 28: unassigned.
      {0xFC},
      // Major type 7, additional information 29: unassigned.
      {0xFD},
      // Major type 7, additional information 30: unassigned.
      {0xFE},
      // Major type 7, additional information 31: "break" stop code for
      // indefinite-length items.
      {0xFF},
  };

  int test_element_index = 0;
  for (const auto& incorrect_cbor : kUnknownAdditionalInfoList) {
    SCOPED_TRACE(testing::Message()
                 << "testing data at index : " << test_element_index++);

    CBORReader::DecoderError error_code;
    base::Optional<CBORValue> cbor =
        CBORReader::Read(incorrect_cbor, &error_code);
    EXPECT_FALSE(cbor.has_value());
    EXPECT_EQ(error_code, CBORReader::DecoderError::UNKNOWN_ADDITIONAL_INFO);
  }
}

TEST(CBORReaderTest, TestTooMuchNestingError) {
  static const std::vector<uint8_t> kZeroDepthCBORList[] = {
      // Unsigned int with value 100.
      {0x18, 0x64},
      // CBOR bytestring of length 4.
      {0x44, 0x01, 0x02, 0x03, 0x04},
      // CBOR string of corresponding to "IETF.
      {0x64, 0x49, 0x45, 0x54, 0x46},
      // Empty CBOR array.
      {0x80},
      // Empty CBOR Map
      {0xa0},
  };

  int test_element_index = 0;
  for (const auto& zero_depth_data : kZeroDepthCBORList) {
    SCOPED_TRACE(testing::Message()
                 << "testing zero nested data : " << test_element_index++);
    CBORReader::DecoderError error_code;
    base::Optional<CBORValue> cbor =
        CBORReader::Read(zero_depth_data, &error_code, 0);
    EXPECT_TRUE(cbor.has_value());
    EXPECT_EQ(error_code, CBORReader::DecoderError::CBOR_NO_ERROR);
  }

  // Corresponds to a CBOR structure with a nesting depth of 2:
  //      {"a": 1,
  //       "b": [2, 3]}
  static const std::vector<uint8_t> kNestedCBORData = {
      // clang-format off
      0xa2,  // map of 2 pairs
        0x61, 0x61,  // "a"
        0x01,

        0x61, 0x62,  // "b"
        0x82,        // array with 2 elements
          0x02,
          0x03,
      // clang-format on
  };

  CBORReader::DecoderError error_code;
  base::Optional<CBORValue> cbor_single_layer_max =
      CBORReader::Read(kNestedCBORData, &error_code, 1);
  EXPECT_FALSE(cbor_single_layer_max.has_value());
  EXPECT_EQ(error_code, CBORReader::DecoderError::TOO_MUCH_NESTING);

  base::Optional<CBORValue> cbor_double_layer_max =
      CBORReader::Read(kNestedCBORData, &error_code, 2);
  EXPECT_TRUE(cbor_double_layer_max.has_value());
  EXPECT_EQ(error_code, CBORReader::DecoderError::CBOR_NO_ERROR);
}

TEST(CBORReaderTest, TestOutOfOrderKeyError) {
  static const std::vector<uint8_t> kMapsWithUnsortedKeys[] = {
      // clang-format off
      {0xa2,  // map with 2 keys with same major type and length
         0x61, 0x62,  // key "b"
         0x61, 0x42,  // value :"B"

         0x61, 0x61,  // key "a" (out of order byte-wise lexically)
         0x61, 0x45   // value "E"
      },
      {0xa2,  // map with 2 keys with different major type
         0x61, 0x62,        // key "b"
         0x02,              // value 2

         // key 1000 (out of order since lower major type sorts first)
         0x19, 0x03, 0xe8,
         0x61, 0x61,        // value a
      },
      {0xa2,  // map with 2 keys with same major type
         0x19, 0x03, 0xe8,  // key 1000  (out of order due to longer length)
         0x61, 0x61,        //value "a"

         0x0a,              // key 10
         0x61, 0x62},       // value "b"
      {0xa2,  // map with 2 text string keys
         0x62, 'a', 'a', // key text string "aa"
                         // (out of order due to longer length)
         0x02,

         0x61, 'b',   // key "b"
         0x01,
      },
      {0xa2,  // map with 2 byte string keys
         0x42, 'x', 'x', // key byte string "xx"
                         // (out of order due to longer length)
         0x02,

         0x41, 'y',  // key byte string "y"
         0x01,
      },
      //clang-format on
  };

  int test_element_index = 0;
  CBORReader::DecoderError error_code;
  for (const auto& unsorted_map : kMapsWithUnsortedKeys) {
    testing::Message scope_message;
    scope_message << "testing unsorted map : " << test_element_index++;
    SCOPED_TRACE(scope_message);

    base::Optional<CBORValue> cbor =
        CBORReader::Read(unsorted_map, &error_code);
    EXPECT_FALSE(cbor.has_value());
    EXPECT_EQ(error_code, CBORReader::DecoderError::OUT_OF_ORDER_KEY);
  }
}

TEST(CBORReaderTest, TestDuplicateKeyError) {
  static const std::vector<uint8_t> kMapWithDuplicateKey = {
      // clang-format off
      0xa6,  // map of 6 pairs:
        0x60,  // ""
        0x61, 0x2e,  // "."

        0x61, 0x62,  // "b"
        0x61, 0x42,  // "B"

        0x61, 0x62,  // "b" (Duplicate key)
        0x61, 0x43,  // "C"

        0x61, 0x64,  // "d"
        0x61, 0x44,  // "D"

        0x61, 0x65,  // "e"
        0x61, 0x44,  // "D"

        0x62, 0x61, 0x61,  // "aa"
        0x62, 0x41, 0x41,  // "AA"
      // clang-format on
  };

  CBORReader::DecoderError error_code;

  base::Optional<CBORValue> cbor =
      CBORReader::Read(kMapWithDuplicateKey, &error_code);
  EXPECT_FALSE(cbor.has_value());
  EXPECT_EQ(error_code, CBORReader::DecoderError::DUPLICATE_KEY);
}

// Leveraging Markus Kuhn’s UTF-8 decoder stress test. See
// http://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt for details.
TEST(CBORReaderTest, TestIncorrectStringEncodingError) {
  static const std::vector<uint8_t> utf8_character_encodings[] = {
      // Corresponds to utf8 encoding of "퟿" (section 2.3.1 of stress test).
      {0x63, 0xED, 0x9F, 0xBF},
      // Corresponds to utf8 encoding of "" (section 2.3.2 of stress test).
      {0x63, 0xEE, 0x80, 0x80},
      // Corresponds to utf8 encoding of "�"  (section 2.3.3 of stress test).
      {0x63, 0xEF, 0xBF, 0xBD},
  };

  int test_element_index = 0;
  CBORReader::DecoderError error_code;
  for (const auto& cbor_byte : utf8_character_encodings) {
    SCOPED_TRACE(testing::Message() << "testing cbor data utf8 encoding : "
                                    << test_element_index++);

    base::Optional<CBORValue> correctly_encoded_cbor =
        CBORReader::Read(cbor_byte, &error_code);
    EXPECT_TRUE(correctly_encoded_cbor.has_value());
    EXPECT_EQ(error_code, CBORReader::DecoderError::CBOR_NO_ERROR);
  }

  // Incorrect UTF8 encoding referenced by section 3.5.3 of the stress test.
  std::vector<uint8_t> impossible_utf_byte{0x64, 0xfe, 0xfe, 0xff, 0xff};
  base::Optional<CBORValue> incorrectly_encoded_cbor =
      CBORReader::Read(impossible_utf_byte, &error_code);
  EXPECT_FALSE(incorrectly_encoded_cbor.has_value());
  EXPECT_EQ(error_code, CBORReader::DecoderError::INVALID_UTF8);
}

TEST(CBORReaderTest, TestExtraneousCBORDataError) {
  static const std::vector<uint8_t> zero_padded_cbor_list[] = {
      // 1 extra byte after a 2-byte unsigned int.
      {0x19, 0x03, 0x05, 0x00},
      // 1 extra byte after a 4-byte cbor byte array.
      {0x44, 0x01, 0x02, 0x03, 0x04, 0x00},
      // 1 extra byte after a 4-byte string.
      {0x64, 0x49, 0x45, 0x54, 0x46, 0x00},
      // 1 extra byte after CBOR array of length 2.
      {0x82, 0x01, 0x02, 0x00},
      // 1 extra key value pair after CBOR map of size 2.
      {0xa1, 0x61, 0x63, 0x02, 0x61, 0x64, 0x03},
  };

  int test_element_index = 0;
  for (const auto& extraneous_cbor_data : zero_padded_cbor_list) {
    SCOPED_TRACE(testing::Message()
                 << "testing cbor extraneous data : " << test_element_index++);

    CBORReader::DecoderError error_code;
    base::Optional<CBORValue> cbor =
        CBORReader::Read(extraneous_cbor_data, &error_code);
    EXPECT_FALSE(cbor.has_value());
    EXPECT_EQ(error_code, CBORReader::DecoderError::EXTRANEOUS_DATA);
  }
}

TEST(CBORReaderTest, TestUnsupportedSimplevalue) {
  static const std::vector<uint8_t> unsupported_simple_values[] = {
      // Simple value (0, unassigned)
      {0xE0},
      // Simple value (19, unassigned)
      {0xF3},
      // Simple value (24, reserved)
      {0xF8, 0x18},
      // Simple value (28, reserved)
      {0xF8, 0x1C},
      // Simple value (29, reserved)
      {0xF8, 0x1D},
      // Simple value (30, reserved)
      {0xF8, 0x1E},
      // Simple value (31, reserved)
      {0xF8, 0x1F},
      // Simple value (32, unassigned)
      {0xF8, 0x20},
      // Simple value (255, unassigned)
      {0xF8, 0xFF},
  };

  for (const auto& unsupported_simple_val : unsupported_simple_values) {
    SCOPED_TRACE(testing::Message()
                 << "testing unsupported cbor simple value  : "
                 << ::testing::PrintToString(unsupported_simple_val));

    CBORReader::DecoderError error_code;
    base::Optional<CBORValue> cbor =
        CBORReader::Read(unsupported_simple_val, &error_code);
    EXPECT_FALSE(cbor.has_value());
    EXPECT_EQ(error_code, CBORReader::DecoderError::UNSUPPORTED_SIMPLE_VALUE);
  }
}

}  // namespace cbor
