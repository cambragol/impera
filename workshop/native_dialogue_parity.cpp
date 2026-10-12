#include "native_dialogue_session.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <iostream>
using namespace Dialogue;
static void require(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
static QByteArray read(const QString &path) {
    QFile file(path);
    require(file.open(QIODevice::ReadOnly), "Cannot read native artifact");
    return file.readAll();
}
static uint32_t hash(const QByteArray &bytes) {
    uint32_t value = 2166136261u;
    for (unsigned char ch : bytes)
        value = (value ^ ch) * 16777619u;
    return value;
}
int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    try {
        require(argc == 2, "Expected reference artifact directory");
        int cases = 0;
        size_t boundaries = 0;
        const QDir root(QString::fromLocal8Bit(argv[1]));
        for (const QString &name : root.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            const QString path = root.filePath(name) + "/";
            auto manifest = QJsonDocument::fromJson(read(path + "manifest.json")).object();
            auto context = manifest["context"].toObject();
            NativeSetup setup;
            setup.seed = uint16_t(manifest["seed"].toInt());
            setup.state.partyNames.clear();
            for (auto party : context["party"].toArray())
                setup.state.partyNames.append(party.toString());
            setup.state.avatar = setup.state.partyNames.first();
            setup.state.partySize = setup.state.partyNames.size();
            setup.state.gold = context["gold"].toInt();
            setup.state.karma = context["karma"].toInt();
            setup.state.introduced = context["introduced"].toInt() != 0;
            QVector<NativeKey> keys;
            for (auto line : read(path + "keys.tsv").split('\n')) {
                auto fields = line.simplified().split(' ');
                if (fields.size() == 2)
                    keys.append({fields[0].toUInt(), fields[1].toInt()});
            }
            QVector<QJsonObject> renders;
            QVector<QPair<int, int>> opcodes;
            uint64_t initialTime = 0;
            for (auto line : read(path + "events.jsonl").split('\n')) {
                if (line.isEmpty())
                    continue;
                auto event = QJsonDocument::fromJson(line).object();
                const QString kind = event["event"].toString();
                if (kind == "start")
                    initialTime = uint64_t(event["ms"].toDouble());
                if (kind == "glyph" || kind == "scroll" || kind == "fill")
                    renders.append(event);
                if (kind == "opcode")
                    opcodes.append({event["a"].toInt(), event["value"].toInt()});
            }
            auto result =
                runNativeVirtual(read(path + "conversation.tlk-entry"), read(path + "normal.font"),
                                 read(path + "runes.font"), setup, keys, initialTime);
            require(result.error.isEmpty(), qPrintable(result.error));
            require(result.pixels == read(path + "window.bin"),
                    "Workshop/native final pixels differ");
            require(result.time == uint64_t(manifest["ms"].toDouble()),
                    "Workshop/native virtual time differs");
            require(result.keysConsumed == manifest["keys_consumed"].toInt(),
                    "Workshop/native input consumption differs");
            require(result.x == manifest["cursor"].toArray()[0].toInt() &&
                        result.y == manifest["cursor"].toArray()[1].toInt(),
                    "Workshop/native cursor differs");
            require(result.gold == manifest["gold"].toInt() &&
                        result.karma == manifest["karma"].toInt() &&
                        result.keys == manifest["keys"].toInt(),
                    "Workshop/native state changes differ");
            require(result.introduced == (manifest["introduced_flags"].toInt() != 0),
                    "Workshop/native introduction differs");
            require(result.opcodes == opcodes,
                    "Workshop/native branch decisions or operands differ");
            int index = 0;
            for (const auto &actual : result.boundaries) {
                if (actual.kind == "start")
                    continue;
                require(index < renders.size(), "Extra Workshop render boundary");
                const auto expected = renders[index++];
                if (actual.kind != expected["event"].toString() ||
                    actual.time != uint64_t(expected["ms"].toDouble()) ||
                    actual.x != expected["x"].toInt() || actual.y != expected["y"].toInt() ||
                    actual.font != expected["font"].toInt() ||
                    actual.pending != expected["operand"].toInt() ||
                    actual.entry != expected["entry"].toInt() ||
                    actual.offset != expected["offset"].toInt() ||
                    actual.value != expected["value"].toInt() ||
                    hash(actual.pixels) != uint32_t(expected["pixels"].toDouble())) {
                    std::cerr << name.toStdString() << " boundary " << index << " "
                              << actual.kind.toStdString() << " time " << actual.time << " vs "
                              << expected["ms"].toInt() << "\n";
                    throw std::runtime_error("Workshop/native render boundary differs");
                }
                ++boundaries;
            }
            require(index == renders.size(), "Missing Workshop render boundaries");
            ++cases;
        }
        require(cases >= 10, "Reference corpus is incomplete");
        std::cout << "PASS standalone Workshop host: " << cases << " native conversations, "
                  << boundaries << " render boundaries; pixels, branches, input and time match\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << "\n";
        return 1;
    }
}
