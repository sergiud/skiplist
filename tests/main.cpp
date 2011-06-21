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

#define BOOST_TEST_MODULE Skiplist

#include <algorithm>
#include <utility>
#include <vector>

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/negative_binomial_distribution.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/test/unit_test.hpp>

#ifdef HAVE_VLD
#include <vld.h>
#endif // HAVE_VLD

#include "skiplist.hpp"

typedef Skiplist<int, int> IntSkipList;

BOOST_AUTO_TEST_CASE(sorting)
{
    IntSkipList s;

    s.insert(std::make_pair(1, 1));
    s.insert(std::make_pair(10, 2));
    s.insert(std::make_pair(5, 1));
    s.insert(std::make_pair(15, 1));
    s.insert(std::make_pair(0, 1));
    s.insert(std::make_pair(11, 1));
    s.insert(std::make_pair(8, 1));
    s.insert(std::make_pair(20, 1));
    s.insert(std::make_pair(-1, 1));

    std::vector<std::pair<int, int> > v(s.begin(), s.end());

    BOOST_CHECK_EQUAL(v[0].first,-1);
    BOOST_CHECK_EQUAL(v[1].first, 0);
    BOOST_CHECK_EQUAL(v[2].first, 1);
    BOOST_CHECK_EQUAL(v[3].first, 5);
    BOOST_CHECK_EQUAL(v[4].first, 8);
    BOOST_CHECK_EQUAL(v[5].first, 10);
    BOOST_CHECK_EQUAL(v[6].first, 11);
    BOOST_CHECK_EQUAL(v[7].first, 15);
    BOOST_CHECK_EQUAL(v[8].first, 20);
}

BOOST_AUTO_TEST_CASE(insertion)
{
    IntSkipList s;

    BOOST_CHECK(s.empty());

    BOOST_CHECK_EQUAL(s.insert(std::make_pair(1, 1))->first, 1);
    BOOST_CHECK_EQUAL(s.insert(std::make_pair(10, 2))->first, 10);
    BOOST_CHECK_EQUAL(s.insert(std::make_pair(5, 1))->first, 5);
    BOOST_CHECK_EQUAL(s.insert(std::make_pair(15, 1))->first, 15);
    BOOST_CHECK_EQUAL(s.insert(std::make_pair(0, 1))->first, 0);
    BOOST_CHECK_EQUAL(s.insert(std::make_pair(11, 1))->first, 11);
    BOOST_CHECK_EQUAL(s.insert(std::make_pair(8, 1))->first, 8);
    BOOST_CHECK_EQUAL(s.insert(std::make_pair(20, 1))->first, 20);
    BOOST_CHECK_EQUAL(s.insert(std::make_pair(-1, 1))->first, -1);

    BOOST_CHECK(s.find(1) != s.end());
    BOOST_CHECK(s.find(10) != s.end());
    BOOST_CHECK(s.find(15) != s.end());
    BOOST_CHECK(s.find(0) != s.end());
    BOOST_CHECK(s.find(11) != s.end());
    BOOST_CHECK(s.find(8) != s.end());
    BOOST_CHECK(s.find(20) != s.end());
    BOOST_CHECK(s.find(-1) != s.end());

    BOOST_CHECK(!s.empty());

    BOOST_CHECK_EQUAL(boost::distance(s), 9);
    BOOST_CHECK(s.size() == boost::distance(s));

    BOOST_CHECK_EQUAL(boost::distance(s), s.size());
    BOOST_CHECK_EQUAL(s.front().first, -1);
    BOOST_CHECK_EQUAL(s.back().first, 20);

    IntSkipList s1;
    s1.insert(std::make_pair(1, 1));

    BOOST_CHECK(s1.erase(s1.insert(std::make_pair(2, 1))) == s1.end());

    IntSkipList::iterator it2 =
        s1.insert(std::make_pair(2, 1));

    BOOST_CHECK(s1.erase(s1.find(1)) == it2);
    BOOST_CHECK(boost::distance(s1) == 1);

    BOOST_CHECK(s1.erase(s1.begin()) == s1.end());
    BOOST_CHECK(boost::distance(s1) == 0);
}

BOOST_AUTO_TEST_CASE(deletion)
{
    IntSkipList s;
    s.erase(s.insert(std::make_pair(1, 1)));

    BOOST_CHECK(s.empty());
    BOOST_CHECK_EQUAL(boost::distance(s), 0);

    s.insert(std::make_pair(10, 2));
    s.insert(std::make_pair(5, 1));
    s.insert(std::make_pair(15, 1));
    s.insert(std::make_pair(0, 1));
    s.insert(std::make_pair(11, 1));
    s.insert(std::make_pair(8, 1));
    s.insert(std::make_pair(20, 1));
    s.insert(std::make_pair(-1, 1));

    BOOST_CHECK_EQUAL(s.size(), 8);
    BOOST_CHECK_EQUAL(boost::distance(s), s.size());

    BOOST_CHECK_EQUAL(s.front().first, -1);
    BOOST_CHECK_EQUAL(s.back().first, 20);

    s.erase(s.find(-1));

    BOOST_CHECK_EQUAL(s.size(), 7);
    BOOST_CHECK_EQUAL(boost::distance(s), s.size());
    BOOST_CHECK_EQUAL(s.front().first, 0);

    s.erase(s.find(20));

    BOOST_CHECK_EQUAL(s.size(), 6);
    BOOST_CHECK_EQUAL(boost::distance(s), s.size());
    BOOST_CHECK_EQUAL(s.back().first, 15);
}

BOOST_AUTO_TEST_CASE(duplicates)
{
    IntSkipList s;

    s.insert(std::make_pair(1, 1));
    s.erase(s.find(1));

    BOOST_CHECK(s.empty());
    BOOST_CHECK_EQUAL(boost::distance(s), 0);
}

BOOST_AUTO_TEST_CASE(iterators)
{
    IntSkipList s;
    BOOST_CHECK_EQUAL(boost::distance(s), 0);

    s.insert(std::make_pair(10, 2));
    s.insert(std::make_pair(5, 1));
    s.insert(std::make_pair(15, 1));
    s.insert(std::make_pair(0, 1));
    s.insert(std::make_pair(11, 1));
    s.insert(std::make_pair(8, 1));
    s.insert(std::make_pair(20, 1));
    s.insert(std::make_pair(-1, 1));

    BOOST_CHECK_EQUAL(s.size(), 8);
    BOOST_CHECK_EQUAL(boost::distance(s), s.size());
    BOOST_CHECK_EQUAL(std::distance(s.rbegin(), s.rend()), s.size());
}

BOOST_AUTO_TEST_CASE(copy)
{
    IntSkipList s;
    BOOST_CHECK_EQUAL(boost::distance(s), 0);

    s.insert(std::make_pair(10, 2));
    s.insert(std::make_pair(5, 1));
    s.insert(std::make_pair(15, 1));
    s.insert(std::make_pair(0, 1));
    s.insert(std::make_pair(11, 1));
    s.insert(std::make_pair(8, 1));
    s.insert(std::make_pair(20, 1));
    s.insert(std::make_pair(-1, 1));

    IntSkipList s1 = s;

    BOOST_CHECK_EQUAL(s.size(), 8);
    BOOST_CHECK_EQUAL(s1.size(), 8);

    BOOST_CHECK_EQUAL(boost::distance(s), s.size());
    BOOST_CHECK_EQUAL(boost::distance(s1), s1.size());

    BOOST_CHECK_EQUAL(std::distance(s.rbegin(), s.rend()), s.size());
    BOOST_CHECK_EQUAL(std::distance(s1.rbegin(), s1.rend()), s1.size());
}

BOOST_AUTO_TEST_CASE(swap)
{
    IntSkipList s;
    BOOST_CHECK_EQUAL(boost::distance(s), 0);

    s.insert(std::make_pair(10, 2));
    s.insert(std::make_pair(5, 1));
    s.insert(std::make_pair(15, 1));
    s.insert(std::make_pair(0, 1));
    s.insert(std::make_pair(11, 1));
    s.insert(std::make_pair(8, 1));
    s.insert(std::make_pair(20, 1));
    s.insert(std::make_pair(-1, 1));

    IntSkipList s1 = s;

    s1.swap(s);

    BOOST_CHECK_EQUAL(s.size(), 8);
    BOOST_CHECK_EQUAL(s1.size(), 8);

    BOOST_CHECK_EQUAL(boost::distance(s), s.size());
    BOOST_CHECK_EQUAL(boost::distance(s1), s1.size());

    BOOST_CHECK_EQUAL(std::distance(s.rbegin(), s.rend()), s.size());
    BOOST_CHECK_EQUAL(std::distance(s1.rbegin(), s1.rend()), s1.size());
}

BOOST_AUTO_TEST_CASE(comparison)
{
    IntSkipList s;

    s.insert(std::make_pair(1, 1));
    s.insert(std::make_pair(10, 2));
    s.insert(std::make_pair(5, 1));
    s.insert(std::make_pair(15, 1));
    s.insert(std::make_pair(0, 1));
    s.insert(std::make_pair(11, 1));
    s.insert(std::make_pair(8, 1));
    s.insert(std::make_pair(20, 1));
    s.insert(std::make_pair(-1, 1));

    IntSkipList s1 = s;

    BOOST_CHECK(s == s1);
}

#ifdef HAVE_CPP0X

BOOST_AUTO_TEST_CASE(move)
{
    IntSkipList s;

    s.insert(std::make_pair(1, 1));
    s.insert(std::make_pair(10, 2));
    s.insert(std::make_pair(5, 1));
    s.insert(std::make_pair(15, 1));
    s.insert(std::make_pair(0, 1));
    s.insert(std::make_pair(11, 1));
    s.insert(std::make_pair(8, 1));
    s.insert(std::make_pair(20, 1));
    s.insert(std::make_pair(-1, 1));

    IntSkipList s1(std::move(s));

    BOOST_CHECK(s.empty());
    BOOST_CHECK(boost::distance(s) == 0);
    BOOST_CHECK_EQUAL(s1.size(), 9);

    IntSkipList s2;

    s2 = std::move(s1);

    BOOST_CHECK(s1.empty());
    BOOST_CHECK(boost::distance(s1) == 0);
    BOOST_CHECK_EQUAL(s2.size(), 9);
}

#endif // HAVE_CPP0X
