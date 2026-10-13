#include <array>

// DOS starting-state offsets, audited against the decompiled engine.
struct StateField {
    const char *name;
    int offset, width;
    const char *help;
};
static constexpr std::array<StateField, 13> stateFields{
    {{"Party members", 0x2b5, 1, "Number of active party members."},
     {"Year", 0x2ce, 2, "Starting year."},
     {"Month", 0x2d7, 1, "Starting month."},
     {"Day", 0x2d8, 1, "Starting day of the month."},
     {"Hour", 0x2d9, 1, "Starting hour, 0–23."},
     {"Minute", 0x2db, 1, "Starting minute, 0–59."},
     {"Karma", 0x2e2, 1, "Starting karma, 0–99."},
     {"Food", 0x202, 2, "Starting food supply."},
     {"Gold", 0x204, 2, "Starting gold supply."},
     {"Keys", 0x206, 1, "Starting ordinary keys."},
     {"Gems", 0x207, 1, "Starting gems."},
     {"Torches", 0x208, 1, "Starting torches."},
     {"Skeleton keys", 0x20b, 1, "Starting skeleton keys."}}};
