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

/**
 * @brief Skip list based data structures.
 * @file skiplist.hpp
 * @author Sergiu Dotenco
 */

#ifndef SKIPLIST_HPP
#define SKIPLIST_HPP

#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#if !defined(HAVE_CPP0X) && _MSC_VER >= 1600
#define HAVE_CPP0X
#endif // !defined(HAVE_CPP0X) && _MSC_VER >= 1600

#ifdef HAVE_CPP0X
#include <initializer_list>
#include <random>
#endif // HAVE_CPP0X

#ifdef HAVE_BOOST
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/negative_binomial_distribution.hpp>
#endif // HAVE_BOOST

namespace detail {

//! Identity selection.
template<class T>
struct Identity
{
    const T& operator()(const T& value) const
    {
        return value;
    }
};

//! Selects the first value of a pair.
template<class T>
struct Select1st
{
    const typename T::first_type& operator()(const T& value) const
    {
        return value.first;
    }
};

} // namespace detail

/**
 * @brief Skip list implementation.
 *
 * Skip list is a data structure that can be used in place of balanced trees.
 * The implementation is based on the paper
 *
 * @blockquote
 * Pugh, William (June 1990). "Skip Lists: A Probabilistic Alternative to
 * Balanced Trees". Communications of the ACM 33 (6): 668-676
 * @endblockquote
 *
 * This specific implementation supports bidirectional iteration through the
 * container by storing a pointer to a predecessor in each node.
 */
template
<
      class Key
    , class T
    , class KeyOfValue
    , class Distribution
#ifdef HAVE_CPP0X
        = std::negative_binomial_distribution<std::size_t>
#elif defined(HAVE_BOOST)
        = boost::random::negative_binomial_distribution<std::size_t>
#endif // HAVE_CPP0X
    , class Engine
#ifdef HAVE_CPP0X
        = std::default_random_engine
#elif defined(HAVE_BOOST)
        = boost::random::mt19937
#endif // HAVE_CPP0X
    , class Compare = std::less<Key>
    , class Allocator = std::allocator<std::pair<const Key, T> >
>
class SkipList
{
    struct Node;
public:
    typedef Key key_type;
    typedef Allocator allocator_type;
    typedef Distribution distribution_type;
    typedef typename allocator_type::value_type value_type;
    typedef typename allocator_type::reference reference;
    typedef typename allocator_type::pointer pointer;
    typedef typename allocator_type::const_reference const_reference;
    typedef typename allocator_type::const_pointer const_pointer;
    typedef typename allocator_type::size_type size_type;
    typedef typename allocator_type::difference_type difference_type;
    typedef Engine engine_type;
    typedef Compare key_compare;

    class const_iterator
        : public std::iterator
          <
              std::bidirectional_iterator_tag,
              value_type,
              difference_type,
              pointer,
              reference
          >
    {
    public:
        const_iterator& operator++();
        const_iterator& operator--();
        const_reference operator*();
        const_pointer operator->();

        const_iterator()
            : node(NULL)
            , parent(NULL)
        {
        }

        const_iterator operator++(int)
        {
            const_iterator tmp(*this);
            ++*this;

            return tmp;
        }

        const_iterator operator--(int)
        {
            const_iterator tmp(*this);
            --*this;

            return tmp;
        }

        bool operator==(const const_iterator& other) const
        {
            assert(parent == other.parent);
            return node == other.node;
        }

        bool operator!=(const const_iterator& other) const
        {
            assert(parent == other.parent);
            return !(*this == other);
        }

    private:
        friend class SkipList;

        explicit const_iterator(const SkipList* parent, const Node* node = NULL)
            : parent(parent)
            , node(node)
        {
        }

        const Node* node;
        const SkipList* parent;
    };

    class iterator
        : public const_iterator
    {
    public:
        iterator()
        {
        }

        iterator& operator++()
        {
            const_iterator::operator++();
            return *this;
        }

        iterator operator++(int)
        {
            iterator tmp(*this);
            ++*this;
            return tmp;
        }

        iterator& operator--()
        {
            const_iterator::operator--();
            return *this;
        }

        iterator operator--(int)
        {
            iterator tmp(*this);
            --*this;
            return tmp;
        }

        reference operator*()
        {
            return const_cast<reference>(const_iterator::operator*());
        }

        pointer operator->()
        {
            return const_cast<pointer>(const_iterator::operator->());
        }

    private:
        friend class SkipList;

        explicit iterator(SkipList* parent, Node* node = NULL)
            : const_iterator(parent, node)
        {
        }
    };

    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    SkipList()
    {
        xinit();
    }

    explicit SkipList(const Allocator& allocator)
        : allocator_(allocator)
        , nallocator_(allocator)
        , eallocator_(allocator)
    {
        xinit();
    }

    explicit SkipList(const Engine& engine,
            const Allocator& allocator = Allocator())
        : engine_(engine)
        , allocator_(allocator)
        , nallocator_(allocator)
        , eallocator_(allocator)
    {
        xinit();
    }

    explicit SkipList(const Distribution& distribution,
            const Engine& engine = Engine(),
            const Compare& predicate = Compare(),
            const Allocator& allocator = Allocator())
        : engine_(engine)
        , allocator_(allocator)
        , nallocator_(allocator)
        , eallocator_(allocator)
        , compare_(predicate)
        , distribution_(distribution)
    {
        xinit();
    }

    SkipList(const SkipList& other)
        : engine_(other.engine_)
        , nallocator_(other.nallocator_)
        , allocator_(other.allocator_)
        , eallocator_(other.eallocator_)
        , compare_(other.compare_)
        , distribution_(other.distribution_)
    {
        xinit();
        insert(other.cbegin(), other.cend());
    }

#ifdef HAVE_CPP0X

    SkipList(std::initializer_list<value_type> values,
        const Allocator& allocator = Allocator())
        : nallocator_(other.nallocator_)
        , allocator_(other.allocator_)
        , eallocator_(other.eallocator_)
    {
        xinit();
        insert(values.begin(), values.end());
    }

    SkipList(SkipList&& other)
        : engine_(std::move(other.engine_))
        , nallocator_(std::move(other.nallocator_))
        , allocator_(std::move(other.allocator_))
        , eallocator_(std::move(other.eallocator_))
        , compare_(std::move(other.compare_))
        , size_(other.size_)
        , block_(other.block_)
        , head_(other.head_)
        , tail_(other.tail_)
        , end_(other.end_)
        , distribution_(std::move(other.distribution_))
        , update_(std::move(other.update_))
    {
        other.size_ = 0;
        other.block_ = NULL;
        other.head_ = NULL;
        other.tail_ = NULL;
        other.end_ = NULL;
    }

    SkipList& operator=(SkipList&& other)
    {
        if (get_allocator() != other.get_allocator())
            *this = other; // deep copy
        else
            SkipList(std::move(other)).swap(*this);

        return *this;
    }

#endif // HAVE_CPP0X

    SkipList& operator=(const SkipList& other)
    {
        SkipList(other).swap(*this);
        return *this;
    }

    ~SkipList()
    {
        if (block_)
            xtidy();
    }

    bool empty() const
    {
        return size_ == 0;
    }

    size_type size() const
    {
        return size_;
    }

    size_type max_size() const
    {
        return eallocator_.max_size();
    }

    void clear()
    {
        xclear();
    }

    const_iterator cbegin() const
    {
        return const_iterator(this, xbegin());
    }

    const_iterator cend() const
    {
        return const_iterator(this, end_);
    }

    const_iterator begin() const
    {
        return cbegin();
    }

    const_iterator end() const
    {
        return cend();
    }

    iterator begin()
    {
        return iterator(this, xbegin());
    }

    iterator end()
    {
        return iterator(this, end_);
    }

    reverse_iterator rbegin()
    {
        return reverse_iterator(end());
    }

    reverse_iterator rend()
    {
        return reverse_iterator(begin());
    }

    const_reverse_iterator crbegin() const
    {
        return const_reverse_iterator(end());
    }

    const_reverse_iterator crend() const
    {
        return const_reverse_iterator(begin());
    }

    const_reverse_iterator rbegin() const
    {
        return crbegin();
    }

    const_reverse_iterator rend() const
    {
        return crend();
    }

    const_reference front() const
    {
        return *begin();
    }

    reference front()
    {
        return const_cast<reference>
            (static_cast<const SkipList*>(this)->front());
    }

    const_reference back() const
    {
        return tail_->previous->next.front()->value;
    }

    reference back()
    {
        return const_cast<reference>
            (static_cast<const SkipList*>(this)->back());
    }

    void swap(SkipList& other)
    {
        using std::swap;  // Allow ADL

        swap(engine_, other.engine_);
        swap(allocator_, other.allocator_);
        swap(size_, other.size_);
        swap(compare_, other.compare_);
        swap(block_, other.block_);
        swap(head_, other.head_);
        swap(tail_, other.tail_);
        swap(end_, other.end_);
        swap(distribution_, other.distribution_);
    }

    distribution_type get_distribution() const
    {
        return distribution_;
    }

    allocator_type get_allocator() const
    {
        return allocator_;
    }

#ifdef HAVE_CPP0X

    template<class V>
    iterator emplace(const_iterator where, V&& value)
    {
        return xinsert(where, std::forward<V>(value));
    }

    template<class V>
    iterator emplace(V&& value)
    {
        return emplace(end(), std::forward<V>(value));
    }

    void insert(std::initializer_list<value_type> values)
    {
        xinsert(values.begin(), values.end());
    }

    SkipList& operator=(std::initializer_list<value_type> values)
    {
        clear();
        insert(values);

        return *this;
    }

#endif // HAVE_CPP0X

    std::pair<iterator, bool> insert(const_reference value)
    {
        return insert(end(), value);
    }

    std::pair<iterator, bool> insert(const_iterator hint, const_reference value)
    {
        return xinsert(hint, value);
    }

    template<class InputIterator>
    void insert(const_iterator where, InputIterator first, InputIterator last)
    {
        for ( ; first != last; ++first)
            insert(where, *first);
    }

    template<class InputIterator>
    void insert(InputIterator first, InputIterator last)
    {
        for ( ; first != last; ++first)
            insert(*first);
    }

    iterator erase(const_iterator where)
    {
        assert(where.parent == this && "Invalid iterator");
        assert(where != end() && "Iterator is not dereferencable");

        Node* const node = where.node->next.front();
        Node* currentNode = node->previous;

        iterator result = node == tail_ ? end() :
            iterator(this, node->previous->next.front());

        typedef typename ElementPtrVector::size_type next_size_type;

        next_size_type count = node->next.size();

        if (update_.size() < count)
            update_.resize(count);

        // Collect node's predecessors
        for (next_size_type i = 0; i != count; ++i) {
            while (currentNode->next.size() <= i)
                currentNode = currentNode->previous;

            update_[i] = currentNode;
        }

        bool stop = false;

        // Let the predecessors point to successors of the node being deleted
        Node* const previous = update_.front();

        assert(!node->next.empty());

        if (Node* nextNode = node->next.front())
            nextNode->previous = previous;

        for (next_size_type i = 0; i != head_->next.size() && !stop; ++i) {
            if (!update_[i] || update_[i]->next[i] != node)
                stop = true;
            else {
                Element* succesor = node->next[i];
                update_[i]->next[i] = succesor;
            }
        }

        if (node == tail_) {
            // Ensure that the last node doesn't have any successors by counting
            // the number of null pointers. The result must correspond to the
            // size of the next vector.
            assert(std::count_if(previous->next.begin(), previous->next.end(),
                std::not1(std::bind2nd(std::equal_to<Element*>(),
                    static_cast<Element*>(NULL)))) == 0);
            end_->previous = tail_ = previous;
        }

        xdestroy(static_cast<Element*>(node));

        // Remove all trailing non-null element pointers
        ElementPtrVector::reverse_iterator it =
            std::find_if(head_->next.rbegin(), head_->next.rend(),
                std::not1(std::bind2nd(std::equal_to<Element*>(),
                    static_cast<Element*>(NULL))));
        head_->next.erase(it.base(), head_->next.end());

        --size_;

        return result;
    }

    size_type count(const key_type& key) const
    {
        std::pair<const_iterator, const_iterator> range = equal_range(key);
        return std::distance(range.first, range.second);
    }

    iterator find(const key_type& key)
    {
        return iterator(this, xfind(key));
    }

    const_iterator find(const key_type& key) const
    {
        return const_iterator(this, xfind(key));
    }

    const_iterator lower_bound(const key_type& key) const
    {
        return find(key);
    }

    iterator lower_bound(const key_type& key)
    {
        return find(key);
    }

    const_iterator upper_bound(const key_type& key) const
    {
        const_iterator it = lower_bound(key);
        return ++it;
    }

    iterator upper_bound(const key_type& key)
    {
        iterator it = lower_bound(key);
        return ++it;
    }

    std::pair<iterator, iterator> equal_range(const key_type& key)
    {
        return std::make_pair(lower_bound(key), upper_bound(key));
    }

    std::pair<const_iterator, const_iterator>
        equal_range(const key_type& key) const
    {
        return std::make_pair(lower_bound(key), upper_bound(key));
    }

private:
    struct Element;

    friend struct Element;
    friend class const_iterator;

    typedef std::vector<Element*,
        typename Allocator::template rebind<Element*>::other> ElementPtrVector;
    typedef std::vector<Node*,
        typename Allocator::template rebind<Node*>::other> NodePtrVector;

    //! Node with successors.
    struct Node
    {
        Node()
            : previous(NULL)
        {
        }

        void swap(Node& other)
        {
            next.swap(other);
            std::swap(previous, other.previous);
        }

        //! Node's successors.
        ElementPtrVector next;
        //! Node's predecessor.
        Node* previous;
    };

    //! A value node.
    struct Element
        : Node
    {
        Element()
        {
        }

        explicit Element(const_reference value)
            : value(value)
        {
        }

        void swap(Element& other)
        {
            using std::swap;

            Node::swap(other);
            swap(value, other.value);
        }

        //! Node's value.
        value_type value;
    };

    typedef typename Allocator::template rebind<Node>::other NodeAllocator;
    typedef typename Allocator::template rebind<Element>::other
        ElementAllocator;

    void xinit()
    {
        size_ = 0;
        block_ = nallocator_.allocate(2);
        head_ = block_;
        end_ = block_ + 1;
        nallocator_.construct(head_, Node());
        nallocator_.construct(end_, Node());
        end_->previous = tail_ = head_;
    }

    void xclear()
    {
        if (size_) {
            Node* node = tail_;

            while (node != head_) {
                Node* previous = node->previous;
                xdestroy(static_cast<Element*>(node));
                node = previous;
            }

            size_ = 0;
        }
    }

    void xtidy()
    {
        assert(block_);
        assert(head_);
        assert(tail_);
        assert(end_);

        xclear();

        nallocator_.destroy(head_);
        nallocator_.destroy(end_);
        nallocator_.deallocate(block_, 2);
        block_ = head_ = tail_ = end_ = NULL;
        update_.clear();
    }

    size_type next_level()
    {
        return distribution_(engine_) + 1;
    }

    bool xequal(const key_type& lhs, const key_type& rhs) const
    {
        return !compare_(lhs, rhs) && !compare_(rhs, lhs);
    }

    template<class V>
    std::pair<iterator, bool> xinsert(const_iterator where, V value)
    {
        assert(where.parent == this && "Invalid iterator");

        iterator result(this);
        bool inserted = true;

        Node* currentNode = head_;

        typename ElementPtrVector::difference_type i =
            static_cast<ElementPtrVector::difference_type>
                (head_->next.size() - 1);

        if (update_.size() < head_->next.size())
            update_.resize(head_->next.size());

        Element* nextNode;
        const key_type& key = KeyOfValue()(value);
        bool stop;

        for ( ; i >= 0; --i) {
            stop = false;

            while (!stop) {
                nextNode = currentNode->next[i];

                if (!nextNode || !compare_(KeyOfValue()(nextNode->value), key))
                    stop = true;
                else
                    currentNode = nextNode;
            }

            update_[i] = currentNode;
        }

        assert(currentNode);
        Element* valueNode;

        if (!currentNode->next.empty() &&
            (valueNode = currentNode->next.front()) &&
            xequal(KeyOfValue()(valueNode->value), KeyOfValue()(value))) {
            result.node = valueNode; // Key already exists
            inserted = false;
        }
        else {
            typedef typename ElementPtrVector::size_type next_size_type;

            next_size_type level = next_level();
            assert(level > 0);

            if (level > head_->next.size()) {
                update_.resize(level);

                // Adjust the number of nodes in the header
                for (next_size_type i = head_->next.size(); i != level; ++i)
                    update_[i] = head_;

                next_size_type count = level - head_->next.size();
                head_->next.insert(head_->next.end(), count, NULL);
            }

            Element* node = xconstruct(
#ifdef HAVE_CPP0X
                Element(std::forward<value_type>(value))
#else // !HAVE_CPP0X
                Element(value)
#endif // HAVE_CPP0X
            );

            node->next.reserve(level);

            // Perform the actual insertion
            for (next_size_type i = 0; i != level; ++i) {
                Element* successor = update_[i]->next[i];
                node->next.push_back(successor);
                update_[i]->next[i] = node;
            }

            assert(!node->next.empty());

            if (Element* succesor = node->next.front()) {
                // Let the successor of the new node point to it
                succesor->previous = node;
            }

            node->previous = update_.front();

            if (tail_ == head_ || node->previous == tail_) {
                // Ensure that the last node doesn't have any successors
                assert(std::count_if(node->next.begin(), node->next.end(),
                    std::not1(std::bind2nd(std::equal_to<Element*>(),
                        static_cast<Element*>(NULL)))) == 0);
                end_->previous = tail_ = node;
            }

            assert(node->previous->next.front() == node);

            ++size_;

            result.node = node->previous;
        }

        return std::make_pair(result, inserted);
    }

    Element* xconstruct(
#ifdef HAVE_CPP0X
        Element&& value
#else // !HAVE_CPP0X
        const Element& value
#endif
        )
    {
        Element* node = eallocator_.allocate(1);
        eallocator_.construct(node,
#ifdef HAVE_CPP0X
            std::forward<Element>(value)
#else // !HAVE_CPP0X
            Element(value)
#endif // HAVE_CPP0X
        );

        return node;
    }

    void xdestroy(Element* e)
    {
        eallocator_.destroy(e);
        eallocator_.deallocate(e, 1);
    }

    Node* xbegin() const
    {
        return empty() ? end_ : head_;
    }

    Node* xfind(const key_type& key) const
    {
        Node* node = head_;

        typename ElementPtrVector::difference_type level =
            static_cast<ElementPtrVector::difference_type>
            (node->next.size() - 1);

        Element* nextNode;

        for ( ; level >= 0; --level) {
            while ((nextNode = node->next[level]) &&
                compare_(KeyOfValue()(nextNode->value), key))
                node = nextNode;
        }

        assert(node);
        Node* result;

        if (!node->next.empty() &&
            xequal(KeyOfValue()(node->next.front()->value), key))
            result = node;
        else
            result = end_;

        return result;
    }

    engine_type engine_;
    allocator_type allocator_;
    size_type size_;
    key_compare compare_;
    NodeAllocator nallocator_;
    ElementAllocator eallocator_;
    Node* block_;
    Node* head_;
    Node* tail_;
    Node* end_;
    distribution_type distribution_;
    NodePtrVector update_;
};

template<class Key, class T, class KeyOfValue, class Distribution, class Engine,
    class Compare, class Allocator>
inline typename SkipList<Key, T, KeyOfValue, Distribution, Engine, Compare,
       Allocator>::const_iterator& SkipList<Key, T, KeyOfValue, Distribution,
       Engine, Compare, Allocator>::const_iterator::operator++()
{
    assert(node != parent->end_ && "Iterator passed behind the end");
    assert(node && "Iterator is not dereferencable");

    if (node == parent->tail_->previous)
        node = parent->end_;
    else
        node = node->next.front();

    return *this;
}

template<class Key, class T, class KeyOfValue, class Distribution, class Engine,
    class Compare, class Allocator>
inline typename SkipList<Key, T, KeyOfValue, Distribution, Engine, Compare,
       Allocator>::const_iterator& SkipList<Key, T, KeyOfValue, Distribution,
       Engine, Compare, Allocator>::const_iterator::operator--()
{
    assert(node != parent->head_ && "Iterator passed behind the start");
    assert(node && "Iterator is not dereferencable");

    if (node == parent->end_) {
        assert(node->previous);
        node = node->previous->previous;
    }
    else
        node = node->previous;

    return *this;
}

template<class Key, class T, class KeyOfValue, class Distribution, class Engine,
    class Compare, class Allocator>
inline typename SkipList<Key, T, KeyOfValue, Distribution, Engine, Compare,
       Allocator>::const_reference SkipList<Key, T, KeyOfValue, Distribution,
       Engine, Compare, Allocator>::const_iterator::operator*()
{
    assert(node && "Iterator is not dereferencable");
    assert(!node->next.empty() && "Invalid iterator");

    return node->next.front()->value;
}

template<class Key, class T, class KeyOfValue, class Distribution, class Engine,
    class Compare, class Allocator>
inline typename SkipList<Key, T, KeyOfValue, Distribution, Engine, Compare,
       Allocator>::const_pointer SkipList<Key, T, KeyOfValue, Distribution,
       Engine, Compare, Allocator>::const_iterator::operator->()
{
    assert(node && "Iterator is not dereferencable");
    assert(!node->next.empty() && "Invalid iterator");

    return &node->next.front()->value;
}

// Returns x == y
template<class Key, class T, class KeyOfValue, class Distribution, class Engine,
    class Compare, class Allocator>
inline bool operator==(const SkipList<Key, T, KeyOfValue, Distribution, Engine,
        Compare, Allocator>& lhs, const SkipList<Key, T, KeyOfValue,
        Distribution, Engine, Compare, Allocator>& rhs)
{
    return lhs.size() == rhs.size() &&
        std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

// Returns !(x == y)
template<class Key, class T, class KeyOfValue, class Distribution, class Engine,
    class Compare, class Allocator>
inline bool operator!=(const SkipList<Key, T, KeyOfValue, Distribution, Engine,
        Compare, Allocator>& lhs, const SkipList<Key, T, KeyOfValue,
        Distribution, Engine, Compare, Allocator>& rhs)
{
    return !(lhs == rhs);
}

template<class Key, class T, class KeyOfValue, class Distribution, class Engine,
    class Compare, class Allocator>
inline bool operator<(const SkipList<Key, T, KeyOfValue, Distribution, Engine,
        Compare, Allocator>& lhs, const SkipList<Key, T, KeyOfValue,
        Distribution, Engine, Compare, Allocator>& rhs)
{
    return std::lexicographical_compare(lhs.begin(), lhs.end(),
        rhs.begin(), rhs.end());
}

// // Returns !(x > y)
template<class Key, class T, class KeyOfValue, class Distribution, class Engine,
    class Compare, class Allocator>
inline bool operator<=(const SkipList<Key, T, KeyOfValue, Distribution, Engine,
        Compare, Allocator>& lhs, const SkipList<Key, T, KeyOfValue,
        Distribution, Engine, Compare, Allocator>& rhs)
{
    return !(lhs > rhs);
}

// Returns y < x
template<class Key, class T, class KeyOfValue, class Distribution, class Engine,
    class Compare, class Allocator>
inline bool operator>(const SkipList<Key, T, KeyOfValue, Distribution, Engine,
        Compare, Allocator>& lhs, const SkipList<Key, T, KeyOfValue,
        Distribution, Engine, Compare, Allocator>& rhs)
{
    return rhs < lhs;
}

// Returns !(x < y)
template<class Key, class T, class KeyOfValue, class Distribution, class Engine,
    class Compare, class Allocator>
inline bool operator>=(const SkipList<Key, T, KeyOfValue, Distribution, Engine,
        Compare, Allocator>& lhs, const SkipList<Key, T, KeyOfValue,
        Distribution, Engine, Compare, Allocator>& rhs)
{
    return !(lhs < rhs);
}

template
<
      class Key
    , class T
    , class Distribution
#ifdef HAVE_CPP0X
        = std::negative_binomial_distribution<std::size_t>
#elif defined(HAVE_BOOST)
        = boost::random::negative_binomial_distribution<std::size_t>
#endif // HAVE_CPP0X
    , class Engine
#ifdef HAVE_CPP0X
        = std::default_random_engine
#elif defined(HAVE_BOOST)
        = boost::random::mt19937
#endif // HAVE_CPP0X
    , class Compare = std::less<Key>
    , class Allocator = std::allocator<std::pair<const Key, T> >
>
class SkipListMap
    : public SkipList
      <
          Key,
          std::pair<const Key, T>,
          detail::Select1st<std::pair<const Key, T> >,
          Distribution,
          Engine,
          Compare,
          Allocator
      >
{
public:
    SkipListMap()
    {
    }

    template<class InputIterator>
    SkipListMap(InputIterator first, InputIterator last,
        const Compare& compare = Compare())
        : SkipList(Distribution(), Engine(), compare)
    {
        insert(first, last);
    }

    explicit SkipListMap(const Allocator& allocator)
        : SkipList(allocator)
    {
    }

    explicit SkipListMap(const Engine& engine,
        const Allocator& allocator = Allocator())
        : SkipList(engine, allocator)
    {
    }

    explicit SkipListMap(const Distribution& distribution,
        const Engine& engine = Engine(),
        const Compare& predicate = Compare(),
        const Allocator& allocator = Allocator())
        : SkipList(distribution, engine, compare, allocator)
    {
    }

#ifdef HAVE_CPP0X

    SkipListMap(SkipListMap&& other)
        : SkipList(std::move(other))
    {
    }

    SkipListMap(std::initializer_list<value_type> values,
        const Allocator& allocator = Allocator())
        : SkipList(values, allocator)
    {
        xinit();
        insert(values.begin(), values.end());
    }

    SkipListMap& operator=(SkipListMap&& other)
    {
        *static_cast<SkipList*>(this) = std::move(other);
        return *this;
    }

#endif // HAVE_CPP0X

    T& operator[](const key_type& key)
    {
        iterator pos = find(key);
        return pos != end() ? pos->second :
            insert(std::make_pair(key, T())).first->second;
    }
};

template
<
      class Key
    , class Distribution
#ifdef HAVE_CPP0X
        = std::negative_binomial_distribution<std::size_t>
#elif defined(HAVE_BOOST)
        = boost::random::negative_binomial_distribution<std::size_t>
#endif // HAVE_CPP0X
    , class Engine
#ifdef HAVE_CPP0X
        = std::default_random_engine
#elif defined(HAVE_BOOST)
        = boost::random::mt19937
#endif // HAVE_CPP0X
    , class Compare = std::less<Key>
    , class Allocator = std::allocator<const Key>
>
class SkipListSet
    : public SkipList
      <
          Key,
          const Key,
          detail::Identity<Key>,
          Distribution,
          Engine,
          Compare,
          Allocator
      >
{
public:
    SkipListSet()
    {
    }

    template<class InputIterator>
    SkipListSet(InputIterator first, InputIterator last,
        const Compare& compare = Compare())
        : SkipList(Distribution(), Engine(), compare)
    {
        insert(first, last);
    }

    explicit SkipListSet(const Allocator& allocator)
        : SkipList(allocator)
    {
    }

    explicit SkipListSet(const Engine& engine,
        const Allocator& allocator = Allocator())
        : SkipList(engine, allocator)
    {
    }

    explicit SkipListSet(const Distribution& distribution,
        const Engine& engine = Engine(),
        const Compare& predicate = Compare(),
        const Allocator& allocator = Allocator())
        : SkipList(distribution, engine, compare, allocator)
    {
    }

#ifdef HAVE_CPP0X

    SkipListSet(SkipListSet&& other)
        : SkipList(std::move(other))
    {
    }

    SkipListSet(std::initializer_list<value_type> values,
        const Allocator& allocator = Allocator())
        : SkipList(values, allocator)
    {
        xinit();
        insert(values.begin(), values.end());
    }

    SkipListSet& operator=(SkipListSet&& other)
    {
        *static_cast<SkipList*>(this) = std::move(other);
        return *this;
    }

#endif // HAVE_CPP0X
};

namespace std {

// Provide a specialization for std::swap

template<class Key, class T, class KeyOfValue, class Distribution, class Engine,
    class Compare, class Allocator>
inline void swap(SkipList<Key, T, KeyOfValue, Distribution, Engine, Compare,
        Allocator>& lhs, SkipList<Key, T, KeyOfValue, Distribution, Engine,
        Compare, Allocator>& rhs)
{
    lhs.swap(rhs)
}

} // namespace std

#endif // SKIPLIST_HPP
