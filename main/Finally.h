#pragma once

#include <functional>

/// Construct a Finally object in a scope where exceptions may be thrown.
/// Regardless of how the scope is exited,
/// the lambda function that this object is constructed with will be called
/// when the Finally object is destroyed.
/// This emulates the try-catch-finally behavior of other languages.
struct Finally {
private:
    std::function<void()> finally;
public:
    Finally(std::function<void()> && finally_) : finally(finally_) {}
    ~Finally() {finally();}
};
