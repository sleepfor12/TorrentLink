#include "ui/input_ime_utils.h"

#include <QtGui/QInputMethodEvent>
#include <QtWidgets/QLineEdit>
#include <QtCore/QTimer>

#include <algorithm>

namespace pfd::ui {

namespace {

/// 通过 InputMethod 事件跟踪 QLineEdit 是否处于预编辑（中文等 IME 组字）阶段。
class LineEditImeComposeTracker final : public QObject {
 public:
  explicit LineEditImeComposeTracker(QLineEdit* lineEdit, QObject* parent)
      : QObject(parent), lineEdit_(lineEdit) {
    if (lineEdit_ != nullptr) {
      lineEdit_->installEventFilter(this);
    }
  }

  [[nodiscard]] bool isComposing() const { return composing_; }

 protected:
  bool eventFilter(QObject* watched, QEvent* event) override {
    if (watched == lineEdit_ && event->type() == QEvent::InputMethod) {
      const auto* ime = static_cast<QInputMethodEvent*>(event);
      composing_ = !ime->preeditString().isEmpty();
    }
    return QObject::eventFilter(watched, event);
  }

 private:
  QLineEdit* lineEdit_{nullptr};
  bool composing_{false};
};

}  // namespace

void connectImeAwareLineEditApply(QObject* lifetimeParent, QLineEdit* lineEdit, int debounceMs,
                                  std::function<void()> apply) {
  if (lifetimeParent == nullptr || lineEdit == nullptr) {
    return;
  }

  auto* tracker = new LineEditImeComposeTracker(lineEdit, lifetimeParent);
  auto* timer = new QTimer(lifetimeParent);
  timer->setSingleShot(true);

  const auto runApply = [apply, timer, tracker]() {
    if (tracker->isComposing()) {
      timer->setInterval(50);
      timer->start();
      return;
    }
    apply();
  };

  QObject::connect(lineEdit, &QLineEdit::textChanged, lifetimeParent,
                   [timer, tracker, debounceMs, runApply]() {
                     const int delay =
                         tracker->isComposing() ? 50 : std::max(0, debounceMs);
                     timer->stop();
                     timer->setInterval(delay);
                     timer->start();
                   });

  QObject::connect(timer, &QTimer::timeout, lifetimeParent, runApply);
}

}  // namespace pfd::ui
