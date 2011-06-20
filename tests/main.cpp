#define BOOST_TEST_MODULE Skiplist

#include <utility>

#include <boost/random/mersenne_twister.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/test/unit_test.hpp>

#include "skiplist.hpp"

BOOST_AUTO_TEST_CASE(insertion)
{
    Skiplist<int, int, boost::random::mt19937> s;

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

    BOOST_CHECK(!s.empty());

    BOOST_CHECK_EQUAL(boost::distance(s), 9);
    BOOST_CHECK(s.size() == boost::distance(s));

    BOOST_CHECK_EQUAL(boost::distance(s), s.size());
    BOOST_CHECK_EQUAL(s.front().first, -1);
    BOOST_CHECK_EQUAL(s.back().first, 20);
}

BOOST_AUTO_TEST_CASE(deletion)
{
    Skiplist<int, int, boost::random::mt19937> s;
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

    BOOST_CHECK_EQUAL(boost::distance(s), s.size());
    BOOST_CHECK_EQUAL(s.front().first, -1);
    BOOST_CHECK_EQUAL(s.back().first, 20);
}
