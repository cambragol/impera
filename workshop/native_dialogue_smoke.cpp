#include "native_dialogue_smoke.h"
#include "native_dialogue_session.h"
#include "native_dialogue_frame.h"
#include <QApplication>
#include <QElapsedTimer>
#include <QThread>
#include <cstdio>
#include <stdexcept>

int nativeDialogueSmokeTest() {
    try {
        auto require = [](bool ok, const char *message) {
            if (!ok)
                throw std::runtime_error(message);
        };
        // Deliberately synthetic font, not substitute artwork for a player's game.
        QByteArray font(1024, char(0x81));
        auto talk = U5::encodeText("Name<Entry>Description<Entry>Hello<Key Wait><Entry>"
                                   "Work<Entry>Bye<Entry><Any><Label 15>@");
        Dialogue::NativeSetup setup;
        setup.state.introduced = true;
        QVector<Dialogue::NativeKey> keys{{0, 'x'}, {0, 'b'}, {0, 'y'}, {0, 'e'}, {0, 13}};
        auto first = Dialogue::runNativeVirtual(talk, font, font, setup, keys);
        auto second = Dialogue::runNativeVirtual(talk, font, font, setup, keys);
        require(first.error.isEmpty() && second.error.isEmpty(), "Native interpreter failed");
        require(first.keysConsumed == 5 && first.pixels == second.pixels &&
                    first.gameState == second.gameState && first.time == second.time,
                "Input consumption or session isolation failed");
        require(Dialogue::nativeGameFramePixels(font).size() == 320 * 200 &&
                    Dialogue::nativeFramedPixels(font, first.pixels).size() == 135 * 112,
                "Native frame geometry failed");
        bool rejected = false;
        try {
            auto missing = Dialogue::runNativeVirtual(talk, {}, font, setup, keys);
            rejected = !missing.error.isEmpty();
        } catch (const std::exception &) {
            rejected = true;
        }
        require(rejected, "Missing font was silently accepted");
        Dialogue::NativeTextViewport viewport;
        viewport.setConversationFrame(Dialogue::nativeConversationFrame(font));
        QImage pixels(128, 104, QImage::Format_RGB32);
        for (int y = 0; y < 104; ++y)
            for (int x = 0; x < 128; ++x)
                pixels.setPixel(x, y,
                                0xff000000u |
                                    IMPERA_TextPalette[uint8_t(first.pixels[y * 128 + x]) & 15]);
        viewport.setImage(pixels);
        viewport.setZoom(2);
        viewport.show();
        QApplication::processEvents();
        require(!viewport.grab().isNull() && viewport.logicalImage().size() == QSize(135, 112),
                "Packaged Playtest viewport failed to paint");
        Dialogue::NativeDialogueSession session(talk, font, font, setup);
        bool waiting = false;
        QObject::connect(
            &session, &Dialogue::NativeDialogueSession::actionReady, QApplication::instance(),
            [&](const QString &action, int, int) { waiting |= action == "Wait for key"; });
        session.start();
        QElapsedTimer timer;
        timer.start();
        while (!waiting && timer.elapsed() < 5000) {
            QApplication::processEvents();
            QThread::msleep(1);
        }
        require(waiting, "Packaged worker did not reach its wait");
        timer.restart();
        session.cancel();
        require(!session.running() && timer.elapsed() < 1000, "Worker cancellation failed");
        std::puts("PASS native dialogue packaged smoke test");
        return 0;
    } catch (const std::exception &error) {
        std::fprintf(stderr, "Native dialogue smoke test: %s\n", error.what());
        return 1;
    }
}
