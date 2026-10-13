#pragma once
#include "text/game_frame.h"
#include <QByteArray>
#include <QImage>
namespace Dialogue {
QByteArray nativeGameFramePixels(const QByteArray &normalFont);
QImage nativeConversationFrame(const QByteArray &normalFont);
QByteArray nativeFramedPixels(const QByteArray &normalFont, const QByteArray &textPixels);
} // namespace Dialogue
