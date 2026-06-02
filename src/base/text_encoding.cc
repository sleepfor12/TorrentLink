#include "base/text_encoding.h"

#include <QtCore/QString>

namespace pfd::base {
namespace {

[[nodiscard]] bool isContinuationByte(unsigned char c) {
  return (c & 0xC0u) == 0x80u;
}

}  // namespace

bool isValidUtf8(const char* data, int size) {
  if (data == nullptr || size <= 0) {
    return true;
  }
  int i = 0;
  while (i < size) {
    const unsigned char c = static_cast<unsigned char>(data[i]);
    if (c <= 0x7Fu) {
      ++i;
      continue;
    }
    int need = 0;
    if ((c & 0xE0u) == 0xC0u) {
      need = 1;
      if (c < 0xC2u) {
        return false;
      }
    } else if ((c & 0xF0u) == 0xE0u) {
      need = 2;
    } else if ((c & 0xF8u) == 0xF0u) {
      need = 3;
      if (c > 0xF4u) {
        return false;
      }
    } else {
      return false;
    }
    if (i + need >= size) {
      return false;
    }
    for (int j = 1; j <= need; ++j) {
      if (!isContinuationByte(static_cast<unsigned char>(data[i + j]))) {
        return false;
      }
    }
    if (need == 2 && c == 0xE0u && static_cast<unsigned char>(data[i + 1]) < 0xA0u) {
      return false;
    }
    if (need == 3 && c == 0xF0u && static_cast<unsigned char>(data[i + 1]) < 0x90u) {
      return false;
    }
    if (need == 3 && c == 0xF4u && static_cast<unsigned char>(data[i + 1]) > 0x8Fu) {
      return false;
    }
    i += need + 1;
  }
  return true;
}

QString QStringFromStdBytes(const std::string& bytes) {
  if (bytes.empty()) {
    return {};
  }
  const int size = static_cast<int>(bytes.size());
  const char* data = bytes.data();
  if (isValidUtf8(data, size)) {
    return QString::fromUtf8(data, size);
  }
#if defined(Q_OS_WIN)
  return QString::fromLocal8Bit(data, size);
#else
  return QString::fromUtf8(data, size);
#endif
}

QString sanitizeHumanReadableText(const QString& text) {
  QString out = text;
  for (int i = 0; i < out.size(); ++i) {
    const QChar ch = out.at(i);
    if (ch.unicode() >= 0x20 || ch == QLatin1Char('\n') || ch == QLatin1Char('\r') ||
        ch == QLatin1Char('\t')) {
      continue;
    }
    out[i] = QChar::ReplacementCharacter;
  }
  return out;
}

}  // namespace pfd::base
