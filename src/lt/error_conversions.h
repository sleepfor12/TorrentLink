#ifndef PFD_LT_ERROR_CONVERSIONS_H
#define PFD_LT_ERROR_CONVERSIONS_H

#include <QtCore/QString>

#include <cstdint>
#include <system_error>

#include "base/errors.h"

namespace pfd::lt {

// 提示转换器当前处理的是哪类 lt 错误场景，便于在没有强类型 lt::error_code
// 的情况下，先建立稳定的“转换入口”。
enum class LtErrorHint : std::uint8_t {
  kGeneric = 0,
  kAddTorrent,
  kMetadata,
  kStorage,
  kNetwork,
};

[[nodiscard]] pfd::base::Error fromLibtorrentError(int rawCode, const QString& rawMessage,
                                                   const QString& context = {},
                                                   LtErrorHint hint = LtErrorHint::kGeneric);

[[nodiscard]] pfd::base::Error fromLibtorrentError(const std::error_code& ec,
                                                   const QString& context = {},
                                                   LtErrorHint hint = LtErrorHint::kGeneric);

}  // namespace pfd::lt

#endif  // PFD_LT_ERROR_CONVERSIONS_H
