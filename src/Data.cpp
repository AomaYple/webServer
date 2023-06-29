#include "Data.h"

using std::string, std::chrono::system_clock, std::jthread, std::source_location;

Data::Data(system_clock::time_point timestamp, jthread::id threadId, source_location sourceLocation, Level level,
           string &&information) noexcept
    : timestamp{timestamp}, threadId{threadId}, sourceLocation{sourceLocation}, level{level},
      information{std::move(information)} {}

Data::Data(Data &&other) noexcept
    : timestamp{other.timestamp}, threadId{other.threadId}, sourceLocation{other.sourceLocation}, level{other.level},
      information{std::move(other.information)} {}

auto Data::operator=(Data &&other) noexcept -> Data & {
    if (this != &other) {
        this->timestamp = other.timestamp;
        this->threadId = other.threadId;
        this->sourceLocation = other.sourceLocation;
        this->level = other.level;
        this->information = std::move(other.information);
    }
    return *this;
}
