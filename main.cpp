#include "Completion.h"
#include "Event.h"
#include "Log.h"
#include "Ring.h"
#include "Server.h"
#include "Submission.h"

int main() {
    Ring ring;
    Server server{9999, ring.getSubmission()};

    ring.forEach([](const Completion &completion) {
        unsigned long long data{completion.getData()};
        Event event{reinterpret_cast<Event &>(data)};
        int result{completion.getResult()};

        switch (event.type) {
            case Type::ACCEPT:
                Log::add(std::source_location::current(), Level::INFO, std::to_string(result));

                break;
            case Type::RECEIVE:
                break;
            case Type::SEND:
                break;
            case Type::CLOSE:
                break;
            case Type::TIMEOUT:
                break;
        }
    });
    return 0;
}
