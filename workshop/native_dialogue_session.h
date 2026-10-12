#pragma once
#include "dialogue.h"
#include "native_text_viewport.h"
#include "text/dialogue_runtime.h"
#include "text/text_input.h"
#include <QObject>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
namespace Dialogue {
struct NativeSetup {
    Sandbox state;
    uint16_t seed = 42;
};
struct NativeKey {
    uint32_t delay = 0;
    int code = 0;
};
struct NativeBoundary {
    QString kind;
    uint64_t time = 0;
    int x = 0, y = 0, font = 0, pending = 0, entry = -1, offset = -1, value = 0;
    QByteArray pixels;
};
struct NativeResult {
    QByteArray pixels;
    uint64_t time = 0;
    int keysConsumed = 0, x = 0, y = 0, gold = 0, karma = 0, keys = 0;
    bool introduced = false, cancelled = false;
    QString error;
    QVector<NativeBoundary> boundaries;
    QVector<QPair<int, int>> opcodes;
};
NativeResult runNativeVirtual(const QByteArray &, const QByteArray &, const QByteArray &,
                              const NativeSetup &, const QVector<NativeKey> &,
                              uint64_t initialTime = 0);
class NativeDialogueSession : public QObject {
    Q_OBJECT
  public:
    NativeDialogueSession(QByteArray conversation, QByteArray normal, QByteArray runes,
                          NativeSetup setup, QObject *parent = nullptr);
    ~NativeDialogueSession() override;
    void start();
    void sendKey(int nativeKey);
    void cancel();
    bool running() const { return active.load(); }
  signals:
    void frameReady(const QImage &);
    void actionReady(const QString &, int entry, int offset);
    void finished(const QString &);

  private:
    QByteArray conversation, normal, runes;
    NativeSetup setup;
    std::atomic<bool> cancelled{false}, active{false};
    std::mutex mutex;
    std::condition_variable wake;
    std::deque<int> keys;
    std::thread worker;
};
} // namespace Dialogue
