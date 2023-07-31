#include "Event.h"

#include "../network/UserData.h"
#include "AcceptEvent.h"
#include "CancelEvent.h"
#include "CloseEvent.h"
#include "ReceiveEvent.h"
#include "SendEvent.h"
#include "TimeoutEvent.h"

using std::unique_ptr, std::make_unique;

auto Event::create(Type type) -> unique_ptr<Event> {
    switch (type) {
        case Type::ACCEPT:
            return make_unique<AcceptEvent>();
        case Type::TIMEOUT:
            return make_unique<TimeoutEvent>();
        case Type::RECEIVE:
            return make_unique<ReceiveEvent>();
        case Type::SEND:
            return make_unique<SendEvent>();
        case Type::CANCEL:
            return make_unique<CancelEvent>();
        case Type::CLOSE:
            return make_unique<CloseEvent>();
    }

    return nullptr;
}
