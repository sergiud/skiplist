// Copyright (c) 2011 Sergiu Dotenco
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <cstddef>
#include <ctime>
#include <iostream>
#include <map>
#include <memory>

#include <boost/cstdint.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>

#include "skiplist.hpp"

void test_skiplist(std::size_t count, boost::random::mt19937& gen)
{
    Skiplist<int, int> s;
    boost::random::uniform_int_distribution<> d;

    std::cout << "performing " << count << " skip list insertions...\n";

    std::clock_t start = std::clock();

    for (std::size_t i = 0; i != count; ++i)
        s.insert(std::make_pair(d(gen), 1));

    double elapsed = double(std::clock() - start) / CLOCKS_PER_SEC;
    std::cout << "done. elapsed time: " << elapsed << "s\n";

    std::cout << "testing skip list element presence...\n";

    start = std::clock();

    for (std::size_t i = 0; i != count; ++i)
        s.count(d(gen));

    elapsed = double(std::clock() - start) / CLOCKS_PER_SEC;
    std::cout << "done. elapsed time: " << elapsed << "s\n";
}

void test_map(std::size_t count, boost::random::mt19937& gen)
{
    std::map<int, int> s;
    boost::random::uniform_int_distribution<> d;

    std::cout << "performing " << count << " map insertions...\n";

    std::clock_t start = std::clock();

    for (std::size_t i = 0; i != count; ++i)
        s.insert(std::make_pair(d(gen), 1));

    double elapsed = double(std::clock() - start) / CLOCKS_PER_SEC;
    std::cout << "done. elapsed time: " << elapsed << "s\n";

    std::cout << "testing map element presence...\n";

    start = std::clock();

    for (std::size_t i = 0; i != count; ++i)
        s.count(d(gen));

    elapsed = double(std::clock() - start) / CLOCKS_PER_SEC;
    std::cout << "done. elapsed time: " << elapsed << "s\n";
}

int main()
{
    boost::random::mt19937 gen(static_cast<boost::uint32_t>(std::time(NULL)));
    const std::size_t count = 20000000;

    try {
        test_skiplist(count, gen);
        test_map(count, gen);
    }
    catch (std::bad_alloc&) {
        std::cout << "error: not enough memory\n";
    }
}
