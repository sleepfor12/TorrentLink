#include "base/errors.h"

#include <cerrno>
#include <system_error>

namespace pfd::base {

std::uint32_t toInt(ErrorCode c) {
  return static_cast<std::uint32_t>(c);
}

ErrorCode fromInt(std::uint32_t n) {
  switch (n) {
    case static_cast<std::uint32_t>(ErrorCode::kSuccess):
      return ErrorCode::kSuccess;
    case static_cast<std::uint32_t>(ErrorCode::kInvalidMagnet):
      return ErrorCode::kInvalidMagnet;
    case static_cast<std::uint32_t>(ErrorCode::kInvalidTorrentPath):
      return ErrorCode::kInvalidTorrentPath;
    case static_cast<std::uint32_t>(ErrorCode::kInvalidSavePath):
      return ErrorCode::kInvalidSavePath;
    case static_cast<std::uint32_t>(ErrorCode::kSavePathNotWritable):
      return ErrorCode::kSavePathNotWritable;
    case static_cast<std::uint32_t>(ErrorCode::kInvalidArgument):
      return ErrorCode::kInvalidArgument;
    case static_cast<std::uint32_t>(ErrorCode::kFileNotFound):
      return ErrorCode::kFileNotFound;
    case static_cast<std::uint32_t>(ErrorCode::kFileReadFailed):
      return ErrorCode::kFileReadFailed;
    case static_cast<std::uint32_t>(ErrorCode::kFileWriteFailed):
      return ErrorCode::kFileWriteFailed;
    case static_cast<std::uint32_t>(ErrorCode::kDirectoryCreateFailed):
      return ErrorCode::kDirectoryCreateFailed;
    case static_cast<std::uint32_t>(ErrorCode::kDiskFull):
      return ErrorCode::kDiskFull;
    case static_cast<std::uint32_t>(ErrorCode::kPathPermissionDenied):
      return ErrorCode::kPathPermissionDenied;
    case static_cast<std::uint32_t>(ErrorCode::kAddTorrentFailed):
      return ErrorCode::kAddTorrentFailed;
    case static_cast<std::uint32_t>(ErrorCode::kDuplicateTorrent):
      return ErrorCode::kDuplicateTorrent;
    case static_cast<std::uint32_t>(ErrorCode::kMetadataTimeout):
      return ErrorCode::kMetadataTimeout;
    case static_cast<std::uint32_t>(ErrorCode::kTorrentRemoved):
      return ErrorCode::kTorrentRemoved;
    case static_cast<std::uint32_t>(ErrorCode::kLoadTasksFailed):
      return ErrorCode::kLoadTasksFailed;
    case static_cast<std::uint32_t>(ErrorCode::kSaveTasksFailed):
      return ErrorCode::kSaveTasksFailed;
    case static_cast<std::uint32_t>(ErrorCode::kLoadResumeDataFailed):
      return ErrorCode::kLoadResumeDataFailed;
    case static_cast<std::uint32_t>(ErrorCode::kSaveResumeDataFailed):
      return ErrorCode::kSaveResumeDataFailed;
    case static_cast<std::uint32_t>(ErrorCode::kUnknown):
      return ErrorCode::kUnknown;
    default:
      return ErrorCode::kUnknown;
  }
}

ErrorDomain domainForCode(ErrorCode c) {
  switch (c) {
    case ErrorCode::kSuccess:
      return ErrorDomain::kNone;
    case ErrorCode::kInvalidMagnet:
    case ErrorCode::kInvalidTorrentPath:
    case ErrorCode::kInvalidSavePath:
    case ErrorCode::kSavePathNotWritable:
    case ErrorCode::kInvalidArgument:
      return ErrorDomain::kValidation;
    case ErrorCode::kFileNotFound:
    case ErrorCode::kFileReadFailed:
    case ErrorCode::kFileWriteFailed:
    case ErrorCode::kDirectoryCreateFailed:
    case ErrorCode::kDiskFull:
    case ErrorCode::kPathPermissionDenied:
      return ErrorDomain::kIo;
    case ErrorCode::kAddTorrentFailed:
    case ErrorCode::kDuplicateTorrent:
    case ErrorCode::kMetadataTimeout:
    case ErrorCode::kTorrentRemoved:
      return ErrorDomain::kLibtorrent;
    case ErrorCode::kLoadTasksFailed:
    case ErrorCode::kSaveTasksFailed:
    case ErrorCode::kLoadResumeDataFailed:
    case ErrorCode::kSaveResumeDataFailed:
      return ErrorDomain::kPersistence;
    case ErrorCode::kUnknown:
      return ErrorDomain::kUnknown;
  }
  return ErrorDomain::kUnknown;
}

QString defaultMessage(ErrorCode c) {
  switch (c) {
    case ErrorCode::kSuccess:
      return QString();
    case ErrorCode::kInvalidMagnet:
      return QStringLiteral("Invalid magnet URI or parse failure");
    case ErrorCode::kInvalidTorrentPath:
      return QStringLiteral("Invalid or unreadable torrent file path");
    case ErrorCode::kInvalidSavePath:
      return QStringLiteral("Save path is empty or invalid");
    case ErrorCode::kSavePathNotWritable:
      return QStringLiteral("Save path is not writable");
    case ErrorCode::kInvalidArgument:
      return QStringLiteral("Invalid argument");
    case ErrorCode::kFileNotFound:
      return QStringLiteral("File not found");
    case ErrorCode::kFileReadFailed:
      return QStringLiteral("Failed to read file");
    case ErrorCode::kFileWriteFailed:
      return QStringLiteral("Failed to write file");
    case ErrorCode::kDirectoryCreateFailed:
      return QStringLiteral("Failed to create directory");
    case ErrorCode::kDiskFull:
      return QStringLiteral("Disk is full");
    case ErrorCode::kPathPermissionDenied:
      return QStringLiteral("Path permission denied");
    case ErrorCode::kAddTorrentFailed:
      return QStringLiteral("Failed to add torrent");
    case ErrorCode::kDuplicateTorrent:
      return QStringLiteral("Torrent task already exists");
    case ErrorCode::kMetadataTimeout:
      return QStringLiteral("Metadata retrieval timed out");
    case ErrorCode::kTorrentRemoved:
      return QStringLiteral("Torrent task has been removed");
    case ErrorCode::kLoadTasksFailed:
      return QStringLiteral("Failed to load task list");
    case ErrorCode::kSaveTasksFailed:
      return QStringLiteral("Failed to save task list");
    case ErrorCode::kLoadResumeDataFailed:
      return QStringLiteral("Failed to load resume data");
    case ErrorCode::kSaveResumeDataFailed:
      return QStringLiteral("Failed to save resume data");
    case ErrorCode::kUnknown:
      return QStringLiteral("Unknown error");
  }
  return QStringLiteral("Unknown error");
}

QString Error::displayMessage() const {
  if (!message_.isEmpty())
    return message_;
  return defaultMessage(code_);
}

namespace {

ErrorCode mapErrnoToCode(int err) {
  if (err == 0) {
    return ErrorCode::kSuccess;
  }
  const std::error_code ec(err, std::generic_category());
  if (ec == std::errc::no_such_file_or_directory) {
    return ErrorCode::kFileNotFound;
  }
  if (ec == std::errc::no_space_on_device) {
    return ErrorCode::kDiskFull;
  }
  if (ec == std::errc::permission_denied || ec == std::errc::operation_not_permitted) {
    return ErrorCode::kPathPermissionDenied;
  }
  return ErrorCode::kUnknown;
}

QString makeDetail(const QString& context, const std::error_code& ec) {
  if (context.isEmpty()) {
    return QStringLiteral("category=%1, code=%2")
        .arg(QString::fromLatin1(ec.category().name()))
        .arg(ec.value());
  }
  return QStringLiteral("%1 | category=%2, code=%3")
      .arg(context)
      .arg(QString::fromLatin1(ec.category().name()))
      .arg(ec.value());
}

}  // namespace

Error fromStdError(const std::error_code& ec, const QString& context) {
  if (!ec) {
    return Error();
  }

  const ErrorCode code = mapErrnoToCode(ec.value());
  const QString msg = QString::fromStdString(ec.message());
  return Error(code, msg, makeDetail(context, ec), ErrorDomain::kSystem, ec.value());
}

Error fromErrno(int err, const QString& context) {
  if (err == 0) {
    return Error();
  }
  const std::error_code ec(err, std::generic_category());
  return fromStdError(ec, context);
}

}  // namespace pfd::base
