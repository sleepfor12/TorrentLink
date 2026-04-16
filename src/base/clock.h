#ifndef PFD_BASE_CLOCK_H
#define PFD_BASE_CLOCK_H

#include <QtCore/QtGlobal>

#include <functional>

namespace pfd::base {

class Clock {
public:
  using NowMsFn = std::function<qint64()>;

  [[nodiscard]] qint64 nowMs() const;
  void setNowMsProvider(NowMsFn fn);
  void reset();

private:
  NowMsFn provider_;
};

}  // namespace pfd::base

#endif  // PFD_BASE_CLOCK_H
