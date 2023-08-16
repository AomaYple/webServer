#include "Node.h"

using namespace std;

Node::Node(string &&data, Node *next) noexcept : data{std::move(data)}, next{next} {}
