#include "native_dialogue_session.h"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstring>
namespace Dialogue {
namespace {
struct Execution {
    NativeTextWindow window;
    ImperaTalk runtime{};
    ImperaTalkStorage storage{};
    ImperaInput input{};
    uint16_t cycle = 0, numpad = 0, buffering = 0;
    uint8_t animate = 0;
    NativeSetup setup;
    NativeResult result;
    QVector<QByteArray> names;
    std::array<uint8_t, 64> inventory{};
    std::array<int, 12> supplies{};
    uint64_t elapsed = 0;
    int charset = 0, entry = -1, offset = -1, consumed = 0;
    bool capture = false;
    std::function<bool()> cancelled;
    std::function<int()> poll;
    std::function<void(int)> sleep;
    std::function<void(const QImage &)> present;
    std::function<void(const QString &, int, int)> action;
    static Execution &self(void *host) { return *static_cast<Execution *>(host); }
    bool stopped() const { return runtime.error || (cancelled && cancelled()); }
    void delay(int ms) {
        if (sleep)
            sleep(ms);
        elapsed += ms;
    }
    void publish(const QString &kind = {}, int value = 0) {
        if (capture)
            result.boundaries.append({kind, elapsed, window.cursor().currentX,
                                      window.cursor().currentY, charset, *runtime.pending, entry,
                                      offset, value, window.indexedPixels()});
        if (present && !stopped())
            present(window.image());
    }
    void fail(const char *text) {
        runtime.error = 1;
        runtime.errorText = text;
    }
    Execution(const QByteArray &bytes, const QByteArray &normal, const QByteArray &runes,
              const NativeSetup &s)
        : window(normal, runes), setup(s) {
        window.setRecordGlyphs(false);
        U5::require(!bytes.isEmpty() && bytes.size() <= 1024, "Conversation exceeds native bounds");
        U5::require(s.state.partySize >= 1 && s.state.partySize <= 6 && s.state.gold >= 0 &&
                        s.state.gold <= 9999 && s.state.karma >= 0 && s.state.karma <= 99,
                    "Invalid Playtest starting state");
        auto doc = parse(bytes);
        for (const auto &issue : doc.issues())
            U5::require(!issue.error, issue.message);
        IMPERA_TalkBind(&runtime, &storage, size_t(bytes.size()));
        std::memcpy(storage.data, bytes.constData(), size_t(bytes.size()));
        auto party = s.state.partyNames;
        if (party.isEmpty())
            party.append(s.state.avatar);
        while (party.size() < s.state.partySize)
            party.append(party.size() == 1 ? "Iolo" : "Shamino");
        for (const QString &name : party) {
            QByteArray encoded = name.left(8).toLatin1();
            U5::require(QString::fromLatin1(encoded) == name.left(8),
                        "Party names must use DOS ASCII");
            for (unsigned char ch : encoded)
                U5::require(ch >= 32 && ch < 127, "Invalid party name");
            encoded += QByteArray(9 - encoded.size(), char(0));
            names.append(encoded);
        }
        runtime.host = this;
        runtime.cancelled = [](void *h) {
            auto &e = self(h);
            return int(e.cancelled && e.cancelled());
        };
        runtime.print = [](void *h, uint16_t ch) {
            auto &e=self(h);
            if(!e.stopped()) e.window.print(ch);
        };
        runtime.string = [](void *h, const char *str) {
            auto &e=self(h);
            if(!e.stopped()) e.window.printString(str);
        };
        runtime.column = [](void *h) { return int(self(h).window.cursor().currentX); };
        runtime.font = [](void *h, int index) {
            auto &e = self(h);
            if (index == 1 && !e.hasRunes) {
                e.fail("RUNES.CH is required for this conversation");
                return;
            }
            e.charset = index;
            e.window.selectFont(index);
        };
        runtime.getChar = [](void *h) { return IMPERA_InputChar(&self(h).input); };
        runtime.getString = [](void *h, char *str, int limit) {
            IMPERA_InputString(&self(h).input, str, limit);
        };
        runtime.poll = [](void *h) {
            auto &e = self(h);
            e.numpad = 0;
            int key = e.poll ? e.poll() : 0;
            if (key & 256) {
                e.numpad = 1;
                key &= 255;
            }
            if (key)
                ++e.consumed;
            return key;
        };
        runtime.waitTicks = [](void *h, int ticks) { self(h).delay(ticks * 55); };
        runtime.frame = [](void *) {};     // Map rendering omitted, never world effects.
        runtime.clearKeys = [](void *) {}; // The current engine's ClearKbdBuffer is a stub.
        runtime.introduced = [](void *h) { return int(self(h).setup.state.introduced); };
        runtime.introduce = [](void *h) { self(h).setup.state.introduced = true; };
        runtime.partySize = [](void *h) { return self(h).setup.state.partySize; };
        runtime.name = [](void *h, int index) {
            return reinterpret_cast<const uint8_t *>(self(h).names[index].constData());
        };
        runtime.karma = [](void *h) { return self(h).setup.state.karma; };
        runtime.changeKarma = [](void *h, int delta) {
            auto &v = self(h).setup.state.karma;
            v = std::clamp(v + delta, 0, 99);
        };
        runtime.pay = [](void *h, int amount) {
            auto &gold = self(h).setup.state.gold;
            if (gold < amount)
                return 1;
            gold -= amount;
            return 0;
        };
        runtime.give = [](void *h, uint8_t code) {
            auto &e = self(h);
            if (code < 64) {
                auto &item = e.inventory[code];
                item = uint8_t(std::min(99, int(item) + 1));
            } else if (code == 65)
                e.supplies[0] = std::min(9999, e.supplies[0] + 1);
            else if (code == 66)
                e.setup.state.gold = std::min(9999, e.setup.state.gold + 1);
            else if (code >= 67 && code <= 71)
                e.supplies[code - 66] = std::min(99, e.supplies[code - 66] + 1);
            else if (code >= 72 && code <= 74)
                e.supplies[code - 66] = 255;
            else if (code == 75)
                e.supplies[9] = std::min(99, e.supplies[9] + 1);
        };
        runtime.join = [](void *h) {
            self(h).fail("Recruitment requires a full game test");
            return 1;
        };
        runtime.guards = [](void *h) { self(h).fail("Calling guards requires a full game test"); };
        runtime.greetingRoll = [](void *h) {
            // Exactly one platform-native first draw, isolated from other sessions.
            static std::mutex rng;
            std::lock_guard<std::mutex> lock(rng);
            std::srand(self(h).setup.seed);
            return std::rand() % 2;
        };
        runtime.observe = [](void *h, const uint8_t *cursor) {
            auto &e = self(h);
            const int absolute = int(cursor - e.storage.data);
            int start = 0, entry = 0;
            for (int i = 0; i < absolute;) {
                int code = e.storage.data[i];
                if (!code) {
                    ++entry;
                    start = i + 1;
                }
                i += 1 + operandCount(code);
            }
            e.entry = entry;
            e.offset = absolute - start;
            if (e.capture)
                e.result.opcodes.append({absolute, *cursor});
            if (!*e.runtime.pending && *cursor >= 129 && *cursor <= 159 && *cursor != 141 &&
                *cursor != 144)
                if (e.action)
                    e.action(actionName(*cursor), entry, e.offset);
        };
        input.advance = window.advanceFlag();
        input.cycle = &cycle;
        input.numpad = &numpad;
        input.buffering = &buffering;
        input.cursorBase = 5;
        input.cursorCount = 4;
        input.map = 13;
        input.driver = 1;
        input.animate = &animate;
        input.host = this;
        input.cancelled = [](void *h) { return int(self(h).stopped()); };
        input.print = runtime.print;
        input.poll = runtime.poll;
        input.waitTicks = runtime.waitTicks;
        input.frame = runtime.frame;
        input.backspace = [](void *h, int n) { self(h).window.backspace(n); };
        hasRunes = runes.size() == 1024;
        window.setSleeper([this](int ms) { delay(ms); });
        window.setObserver([this](const QString &kind, int value) { publish(kind, value); });
    }
    bool hasRunes = false;
    NativeResult run() {
        publish("start");
        IMPERA_TalkRun(&runtime);
        if (!stopped())
            IMPERA_TalkFlush(&runtime);
        result.pixels = window.indexedPixels();
        result.time = elapsed;
        result.keysConsumed = consumed;
        result.x = window.cursor().currentX;
        result.y = window.cursor().currentY;
        result.gold = setup.state.gold;
        result.karma = setup.state.karma;
        result.keys = supplies[1];
        result.introduced = setup.state.introduced;
        result.cancelled = cancelled && cancelled();
        if (runtime.error)
            result.error = QString::fromUtf8(runtime.errorText ? runtime.errorText
                                                               : "Conversation execution failed");
        if (present)
            present(window.image());
        return result;
    }
};
} // namespace
NativeResult runNativeVirtual(const QByteArray &bytes, const QByteArray &font,
                              const QByteArray &runes, const NativeSetup &setup,
                              const QVector<NativeKey> &keys, uint64_t initialTime) {
    Execution e(bytes, font, runes, setup);
    e.capture = true;
    e.elapsed = initialTime;
    size_t index = 0;
    bool scheduled = false;
    uint64_t ready = 0;
    e.poll = [&] {
        if (index >= size_t(keys.size())) {
            e.fail("Script exhausted before native input completed");
            return 0;
        }
        if (!scheduled) {
            ready = e.elapsed + keys[int(index)].delay;
            scheduled = true;
        }
        if (e.elapsed < ready)
            return 0;
        scheduled = false;
        return keys[int(index++)].code;
    };
    e.sleep = [&](int) {
        if (e.elapsed > 300000)
            e.fail("Virtual conversation time limit exceeded");
    };
    return e.run();
}
NativeDialogueSession::NativeDialogueSession(QByteArray c, QByteArray n, QByteArray r,
                                             NativeSetup s, QObject *parent)
    : QObject(parent), conversation(std::move(c)), normal(std::move(n)), runes(std::move(r)),
      setup(std::move(s)) {}
NativeDialogueSession::~NativeDialogueSession() { cancel(); }
void NativeDialogueSession::start() {
    cancel();
    cancelled = false;
    active = true;
    {
        std::lock_guard<std::mutex> lock(mutex);
        keys.clear();
    }
    worker = std::thread([this] {
        try {
            Execution e(conversation, normal, runes, setup);
            e.cancelled = [this] { return cancelled.load(); };
            e.poll = [this] {
                std::lock_guard<std::mutex> lock(mutex);
                if (keys.empty())
                    return 0;
                int key = keys.front();
                keys.pop_front();
                return key;
            };
            e.sleep = [this](int ms) {
                std::unique_lock<std::mutex> lock(mutex);
                wake.wait_for(lock, std::chrono::milliseconds(ms),
                              [this] { return cancelled.load(); });
            };
            e.present = [this](const QImage &image) {
                if (!cancelled)
                    emit frameReady(image);
            };
            e.action = [this](const QString &name, int entry, int offset) {
                if (!cancelled)
                    emit actionReady(name, entry, offset);
            };
            auto result = e.run();
            active = false;
            if (!cancelled)
                emit finished(result.error.isEmpty() ? "Conversation ended" : result.error);
        } catch (const std::exception &error) {
            active = false;
            if (!cancelled)
                emit finished(QString::fromUtf8(error.what()));
        }
    });
}
void NativeDialogueSession::sendKey(int key) {
    if (key < 1 || key > 511 || !active || cancelled)
        return;
    std::lock_guard<std::mutex> lock(mutex);
    if (keys.size() < 4096)
        keys.push_back(key);
}
void NativeDialogueSession::cancel() {
    cancelled = true;
    wake.notify_all();
    if (worker.joinable())
        worker.join();
    active = false;
}
} // namespace Dialogue
