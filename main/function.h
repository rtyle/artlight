#pragma once

#include <functional>

namespace function {

// compose functions f after g
//	C f(B)
//	B g(A)
//	A a
//	C c = f(g(a))
template <typename A, typename B, typename C>
std::function<C(A)> compose(
	std::function<C(B)> f, std::function<B(A)> g) {
    return [f, g](A a) {return f(g(a));};
}

// combine functions f after g and h
//	C f(B, B)
//	B g(A)
//	B h(A)
//	A a
//	C c= f(g(a), h(a))
template <typename A, typename B, typename C>
std::function<C(A)> combine(
	std::function<C(B, B)> f, std::function<B(A)> g, std::function<B(A)> h) {
    return [f, g, h](A a) {return f(g(a), h(a));};
}

}
