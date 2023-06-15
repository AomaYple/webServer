#include "Buffer.h"

#include <algorithm>

using std::string_view, std::vector, std::pair;

Buffer::Buffer() : self{vector<char>(1024, 0)}, start{0}, end{0} {}

Buffer::Buffer(Buffer &&other) noexcept : self{std::move(other.self)}, start{other.start}, end{other.end} {}

auto Buffer::operator=(Buffer &&other) noexcept -> Buffer & {
    if (this != &other) {
        this->self = std::move(other.self);
        this->start = other.start;
        this->end = other.end;
    }
    return *this;
}

auto Buffer::write(string_view data) -> void {
    if (this->start > 0 && data.size() > this->self.size() - this->end) {
        std::rotate(this->self.begin(), this->self.begin() + this->start, this->self.begin() + this->end);

        this->end -= this->start;
        this->start = 0;
    }

    if (data.size() > this->self.size() - this->end) this->self.resize((data.size() + this->end - this->start) * 2);

    this->self.insert(this->self.begin() + this->end, data.begin(), data.end());

    this->end += data.size();
}

auto Buffer::read() -> string_view { return {this->self.begin() + this->start, this->self.begin() + this->end}; }

auto Buffer::writableData() -> pair<char *, unsigned int> {
    return {this->self.data() + this->end, this->self.size() - this->end};
}

auto Buffer::adjustWrite(unsigned int size) -> void {
    this->end += size;

    if (this->end == this->self.size() && this->start > 0) {
        std::rotate(this->self.begin(), this->self.begin() + this->start, this->self.begin() + this->end);

        this->end -= this->start;
        this->start = 0;
    }

    if (this->end == this->self.size()) this->self.resize(this->self.size() * 2);
}

auto Buffer::adjustRead(unsigned int size) -> void {
    this->start += size;

    if (this->start == this->end) this->start = this->end = 0;
}