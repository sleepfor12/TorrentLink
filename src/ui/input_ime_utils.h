#ifndef PFD_UI_INPUT_IME_UTILS_H
#define PFD_UI_INPUT_IME_UTILS_H

#include <functional>

class QObject;
class QLineEdit;

namespace pfd::ui {

/// 将 QLineEdit 与“实时过滤/搜索”回调相连：输入法组字（预编辑）期间不刷新，上屏后再应用；
/// debounceMs 为组字结束后的去抖间隔（毫秒），减轻快速键入时的刷新频率。
void connectImeAwareLineEditApply(QObject* lifetimeParent, QLineEdit* lineEdit, int debounceMs,
                                  std::function<void()> apply);

}  // namespace pfd::ui

#endif  // PFD_UI_INPUT_IME_UTILS_H
