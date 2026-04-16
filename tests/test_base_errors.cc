#include <cerrno>
#include <gtest/gtest.h>
#include <system_error>

#include "base/errors.h"

namespace {

using pfd::base::Error;
using pfd::base::ErrorCode;

TEST(BaseErrors, DefaultConstructedIsSuccess) {
  const Error err;
  EXPECT_TRUE(err.isOk());
  EXPECT_FALSE(err.hasError());
  EXPECT_EQ(err.code(), ErrorCode::kSuccess);
  EXPECT_EQ(err.domain(), pfd::base::ErrorDomain::kNone);
  EXPECT_EQ(err.rawCode(), 0);
}

TEST(BaseErrors, DisplayMessageUsesFallbackWhenMessageEmpty) {
  const Error err(ErrorCode::kInvalidMagnet, QString(), QString());
  EXPECT_EQ(err.displayMessage(), QStringLiteral("Invalid magnet URI or parse failure"));
}

TEST(BaseErrors, DisplayMessagePrefersExplicitMessage) {
  const Error err(ErrorCode::kInvalidMagnet, QStringLiteral("自定义消息"), QString());
  EXPECT_EQ(err.displayMessage(), QStringLiteral("自定义消息"));
}

TEST(BaseErrors, ToIntFromIntRoundTrip) {
  const auto code = ErrorCode::kDiskFull;
  const auto value = pfd::base::toInt(code);
  EXPECT_EQ(pfd::base::fromInt(value), code);
}

TEST(BaseErrors, FromIntUnknownValueFallsBackToUnknown) {
  EXPECT_EQ(pfd::base::fromInt(999999u), ErrorCode::kUnknown);
}

TEST(BaseErrors, FromErrnoMapsCommonErrors) {
  const Error err1 = pfd::base::fromErrno(ENOENT, QStringLiteral("load file"));
  EXPECT_EQ(err1.code(), ErrorCode::kFileNotFound);
  EXPECT_EQ(err1.domain(), pfd::base::ErrorDomain::kSystem);
  EXPECT_EQ(err1.rawCode(), ENOENT);

  const Error err2 = pfd::base::fromErrno(ENOSPC, QStringLiteral("save resume"));
  EXPECT_EQ(err2.code(), ErrorCode::kDiskFull);
  EXPECT_EQ(err2.domain(), pfd::base::ErrorDomain::kSystem);
  EXPECT_EQ(err2.rawCode(), ENOSPC);
}

TEST(BaseErrors, FromStdErrorWithNoErrorReturnsSuccess) {
  const std::error_code ok;
  const Error err = pfd::base::fromStdError(ok);
  EXPECT_TRUE(err.isOk());
  EXPECT_EQ(err.code(), ErrorCode::kSuccess);
}

TEST(BaseErrors, FromStdErrorMapsErrcCommonCases) {
  const auto missing = pfd::base::fromStdError(
      std::make_error_code(std::errc::no_such_file_or_directory), QStringLiteral("open"));
  EXPECT_EQ(missing.code(), ErrorCode::kFileNotFound);

  const auto denied = pfd::base::fromStdError(
      std::make_error_code(std::errc::permission_denied), QStringLiteral("write"));
  EXPECT_EQ(denied.code(), ErrorCode::kPathPermissionDenied);

  const auto full = pfd::base::fromStdError(
      std::make_error_code(std::errc::no_space_on_device), QStringLiteral("save"));
  EXPECT_EQ(full.code(), ErrorCode::kDiskFull);
}

TEST(BaseErrors, DomainForCodeClassifiesValidationAndPersistence) {
  EXPECT_EQ(pfd::base::domainForCode(ErrorCode::kInvalidMagnet),
            pfd::base::ErrorDomain::kValidation);
  EXPECT_EQ(pfd::base::domainForCode(ErrorCode::kSaveResumeDataFailed),
            pfd::base::ErrorDomain::kPersistence);
}

TEST(BaseErrors, DefaultMessageIsNonEmptyForAllErrorCodesExceptSuccess) {
  for (std::uint32_t i = 0; i <= pfd::base::toInt(ErrorCode::kUnknown); ++i) {
    const auto code = pfd::base::fromInt(i);
    if (code == ErrorCode::kSuccess) {
      EXPECT_TRUE(pfd::base::defaultMessage(code).isEmpty());
    } else {
      EXPECT_FALSE(pfd::base::defaultMessage(code).isEmpty());
    }
  }
}

TEST(BaseErrors, FromIntRoundTripForAllDefinedCodes) {
  for (std::uint32_t i = 0; i <= pfd::base::toInt(ErrorCode::kUnknown); ++i) {
    const auto code = pfd::base::fromInt(i);
    EXPECT_EQ(pfd::base::toInt(code), i);
  }
}

}  // namespace
