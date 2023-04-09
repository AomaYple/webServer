#include "Buffer.h"

using std::string_view, std::pair;

Buffer::Buffer() : self(1024), start(0), end(0) {}

Buffer::Buffer(Buffer &&buffer) noexcept : self(std::move(buffer.self)), start(buffer.start), end(buffer.end) {
    buffer.start = buffer.end = 0;
}

auto Buffer::operator=(Buffer &&buffer) noexcept -> Buffer & {
    if (this != &buffer) {
        this->self = std::move(buffer.self);
        this->start = buffer.start;
        this->end = buffer.end;
        buffer.start = buffer.end = 0;
    }
    return *this;
}

auto Buffer::write(const string_view &data) -> void {
    if (data.size() > this->self.size() - this->end && this->start > 0) {
        std::copy(this->self.begin() + this->start, this->self.begin() + this->end, this->self.begin());
        this->end -= this->start;
        this->start = 0;
    }

    if (data.size() > this->self.size() - this->end)
        this->self.resize((data.size() + this->end - this->start) * 2);

    std::copy(data.begin(), data.end(), this->self.begin() + this->end);
    this->end += data.size();
}

auto Buffer::read() -> string_view {
    return {this->self.begin() + this->start, this->self.begin() + this->end};
}

auto Buffer::writableData() -> pair<char *, unsigned int> {
    return {this->self.data() + this->end, this->self.size() - this->end};
}

auto Buffer::adjustWrite(unsigned int size) -> void {
    this->end += size;

    if (this->end == this->self.size() && this->start > 0) {
        std::copy(this->self.begin() + this->start, this->self.begin() + this->end, this->self.begin());
        this->end -= this->start;
        this->start = 0;
    }

    if (this->end == this->self.size())
        this->self.resize(this->self.size() * 2);
}

auto Buffer::adjustRead(unsigned int size) -> void {
    this->start += size;

    if (this->start == this->end)
        this->start = this->end = 0;
}
