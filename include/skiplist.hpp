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
#include <cmath>
#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>
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

/**
 * @brief Skip list implementation.
 *
 * Skip list is a data structure that can be used in place of balanced trees.
 * The implementation is based on the paper
 *
 * @blockquote
 * Pugh, William (June 1990). "Skip Lists: A Probabilistic Alternative to
 * Balanced Trees". Communications of the ACM 33 (6): 668�676
 * @endblockquote
 *
 * This specific implementation supports bidirectional iteration through the
 * container by storing a pointer to a predecessor in each node.
 */
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
class Skiplist
{
    struct Node;

    typedef std::pair<Key, T> pair_type;

public:
    typedef Key key_type;
    typedef T mapped_type;
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
        friend class Skiplist;

        explicit const_iterator(const Skiplist* parent, const Node* node = NULL)
            : parent(parent)
            , node(node)
        {
        }

        const Node* node;
        const Skiplist* parent;
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
        friend class Skiplist;

        explicit iterator(Skiplist* parent, Node* node = NULL)
            : const_iterator(parent, node)
        {
        }
    };

    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    Skiplist()
    {
        xinit();
    }

    explicit Skiplist(const Allocator& allocator)
        : allocator_(allocator)
        , nallocator_(allocator)
        , eallocator_(allocator)
    {
        xinit();
    }

    explicit Skiplist(const Engine& engine,
            const Allocator& allocator = Allocator())
        : engine_(engine)
        , allocator_(allocator)
        , nallocator_(allocator)
        , eallocator_(allocator)
    {
        xinit();
    }

    explicit Skiplist(const Compare& predicate, const Distribution& distribution,
        const Engine& engine = Engine(), const Allocator& allocator = Allocator())
        : engine_(engine)
        , allocator_(allocator)
        , nallocator_(allocator)
        , eallocator_(allocator)
        , compare_(predicate)
        , distribution_(distribution)
    {
        xinit();
    }

    Skiplist(const Skiplist& other)
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

    Skiplist(std::initializer_list<value_type> values,
        const Allocator& allocator = Allocator())
        : nallocator_(other.nallocator_)
        , allocator_(other.allocator_)
        , eallocator_(other.eallocator_)
    {
        xinit();
        insert(values.begin(), values.end());
    }

    Skiplist(Skiplist&& other)
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
    {
        other.size_ = 0;
        other.block_ = NULL;
        other.head_ = NULL;
        other.tail_ = NULL;
        other.end_ = NULL;
    }

    Skiplist& operator=(Skiplist&& other)
    {
        if (get_allocator() != other.get_allocator())
            *this = other; // deep copy
        else
            Skiplist(std::move(other)).swap(*this);

        return *this;
    }

#endif // HAVE_CPP0X

    ~Skiplist()
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

    size_type max_level() const
    {
        return 32;
    }

    void clear()
    {
        xclear();
    }

    const_iterator cbegin() const
    {
        return const_cast<Skiplist*>(this)->begin();
    }

    const_iterator cend() const
    {
        return const_cast<Skiplist*>(this)->end();
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
        return empty() ? end() : iterator(this, head_);
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
        return const_cast<Skiplist*>(this)->front();
    }

    reference front()
    {
        return *begin();
    }

    const_reference back() const
    {
        return const_cast<Skiplist*>(this)->back();
    }

    reference back()
    {
        return tail_->previous->next.front()->value;
    }

    void swap(Skiplist& other)
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

    Skiplist& operator=(std::initializer_list<value_type> values)
    {
        clear();
        insert(values);

        return *this;
    }

#endif // HAVE_CPP0X

    iterator insert(const_reference value)
    {
        return insert(end(), value);
    }

    iterator insert(const_iterator where, const_reference value)
    {
        return xinsert(where, value);
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
        NodePtrVector update(max_level());

        // Collect node's predecessors
        for (next_size_type i = 0; i != count; ++i) {
            while (currentNode->next.size() <= i)
                currentNode = currentNode->previous;

            update[i] = currentNode;
        }

        bool stop = false;

        // Let the predecessors point to successors of the node being deleted
        Node* const previous = update.front();

        assert(!node->next.empty());

        if (Node* nextNode = node->next.front())
            nextNode->previous = previous;

        for (next_size_type i = 0; i != head_->next.size() && !stop; ++i) {
            if (!update[i] || update[i]->next[i] != node)
                stop = true;
            else {
                Element* succesor = node->next[i];
                update[i]->next[i] = succesor;
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

    iterator find(const key_type& key)
    {
        Node* node = head_;

        typename ElementPtrVector::difference_type level =
            static_cast<ElementPtrVector::difference_type>
                (node->next.size() - 1);

        for ( ; level >= 0; --level) {
            while (node->next[level] &&
                compare_(node->next[level]->value.first, key))
                node = node->next[level];
        }

        assert(node);
        assert(!node->next.empty());

        iterator result;
        Element* candidate = node->next.front();

        if (candidate && xequal(candidate->value.first, key))
            result = iterator(this, node);
        else
            result = end();

        return result;
    }

    const_iterator find(const_reference value) const
    {
        return const_cast<Skiplist*>(this)->find(value);
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
    iterator xinsert(const_iterator where, V value)
    {
        assert(where.parent == this && "Invalid iterator");

        iterator result(this);

        Node* currentNode = head_;
        typename ElementPtrVector::difference_type i =
            static_cast<ElementPtrVector::difference_type>
                (currentNode->next.size() - 1);

        NodePtrVector update(max_level());

        for ( ; i >= 0; --i) {
            while (currentNode->next[i] &&
                compare_(currentNode->next[i]->value.first, value.first))
                currentNode = currentNode->next[i];

            update[i] = currentNode;
        }

        assert(currentNode);

        if (!currentNode->next.empty() && currentNode->next.front() &&
            xequal(currentNode->next.front()->value.first, value.first)) {
            // The key already exists: update the value
            Element* node = currentNode->next.front();
            node->value.second = value.second;
            result.node = node->previous;
        }
        else {
            typedef typename ElementPtrVector::size_type next_size_type;

            next_size_type level = next_level();
            assert(level > 0);

            if (level > head_->next.size()) {
                // Adjust the number of nodes in the header
                for (next_size_type i = head_->next.size(); i != level; ++i)
                    update[i] = head_;

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
                Element* successor = update[i]->next[i];
                node->next.push_back(successor);
                update[i]->next[i] = node;
            }

            assert(!node->next.empty());

            if (Element* succesor = node->next.front()) {
                // Let the successor of the new node point to it
                succesor->previous = node;
            }

            if (tail_ == head_ ||
                compare_(static_cast<const Element*>
                    (tail_)->value.first, node->value.first)) {
                // Ensure that the last node doesn't have any successors
                assert(std::count_if(node->next.begin(), node->next.end(),
                    std::not1(std::bind2nd(std::equal_to<Element*>(),
                        static_cast<Element*>(NULL)))) == 0);
                end_->previous = tail_ = node;
            }

            node->previous = update.front();
            assert(node->previous->next.front() == node);

            ++size_;

            result.node = node->previous;
        }

        return result;
    }

    Element* xconstruct(
#ifdef HAVE_CPP0X
        Element&& value
#else // !HAVE_CPP0X
        const Element& value
#endif
        )
    {
        Element* e = eallocator_.allocate(1);
        eallocator_.construct(e,
#ifdef HAVE_CPP0X
            std::forward<Element>(value)
#else // !HAVE_CPP0X
            Element(value)
#endif // HAVE_CPP0X
        );

        return e;
    }

    void xdestroy(Element* e)
    {
        eallocator_.destroy(e);
        eallocator_.deallocate(e, 1);
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
};

template<class Key, class T, class Distribution, class Engine, class Compare,
    class Allocator>
inline typename Skiplist<Key, T, Distribution, Engine, Compare,
       Allocator>::const_iterator& Skiplist<Key, T, Distribution, Engine,
       Compare, Allocator>::const_iterator::operator++()
{
    assert(node != parent->end_ && "Iterator passed behind the end");
    assert(node && "Iterator is not dereferencable");

    if (node == parent->tail_->previous)
        node = parent->end_;
    else
        node = node->next.front();

    return *this;
}

template<class Key, class T, class Distribution, class Engine, class Compare,
class Allocator>
inline typename Skiplist<Key, T, Distribution, Engine, Compare,
       Allocator>::const_iterator& Skiplist<Key, T, Distribution, Engine,
       Compare, Allocator>::const_iterator::operator--()
{
    assert(node != parent->head_ && "Iterator passed behind the start");
    assert(node && "Iterator is not dereferencable");

    if (node == parent->end_)
        node = node->previous->previous;
    else
        node = node->previous;

    return *this;
}

template<class Key, class T, class Distribution, class Engine, class Compare,
    class Allocator>
inline typename Skiplist<Key, T, Distribution, Engine, Compare,
       Allocator>::const_reference Skiplist<Key, T, Distribution, Engine,
       Compare, Allocator>::const_iterator::operator*()
{
    assert(node && "Iterator is not dereferencable");
    assert(!node->next.empty() && "Invalid iterator");

    return node->next.front()->value;
}

template<class Key, class T, class Distribution, class Engine, class Compare,
    class Allocator>
inline typename Skiplist<Key, T, Distribution, Engine, Compare,
       Allocator>::const_pointer Skiplist<Key, T, Distribution, Engine, Compare,
       Allocator>::const_iterator::operator->()
{
    assert(node && "Iterator is not dereferencable");
    assert(!node->next.empty() && "Invalid iterator");

    return &node->next.front()->value;
}

// Returns x == y
template<class Key, class T, class Distribution, class Engine, class Compare,
    class Allocator>
inline bool operator==(const Skiplist<Key, T, Distribution, Engine, Compare,
        Allocator>& lhs, const Skiplist<Key, T, Distribution, Engine, Compare,
        Allocator>& rhs)
{
    return lhs.size() == rhs.size() &&
        std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

// Returns !(x == y)
template<class Key, class T, class Distribution, class Engine, class Compare,
    class Allocator>
inline bool operator!=(const Skiplist<Key, T, Distribution, Engine, Compare,
        Allocator>& lhs, const Skiplist<Key, T, Distribution, Engine, Compare,
        Allocator>& rhs)
{
    return !(lhs == rhs);
}

template<class Key, class T, class Distribution, class Engine, class Compare,
    class Allocator>
inline bool operator<(const Skiplist<Key, T, Distribution, Engine, Compare,
        Allocator>& lhs, const Skiplist<Key, T, Distribution, Engine, Compare,
        Allocator>& rhs)
{
    return std::lexicographical_compare(lhs.begin(), lhs.end(),
        rhs.begin(), rhs.end());
}

// // Returns !(x > y)
template<class Key, class T, class Distribution, class Engine, class Compare,
    class Allocator>
inline bool operator<=(const Skiplist<Key, T, Distribution, Engine, Compare,
        Allocator>& lhs, const Skiplist<Key, T, Distribution, Engine, Compare,
        Allocator>& rhs)
{
    return !(lhs > rhs);
}

// Returns y < x
template<class Key, class T, class Distribution, class Engine, class Compare,
    class Allocator>
inline bool operator>(const Skiplist<Key, T, Distribution, Engine, Compare,
        Allocator>& lhs, const Skiplist<Key, T, Engine, Compare, Allocator>&
        rhs)
{
    return rhs < lhs;
}

// Returns !(x < y)
template<class Key, class T, class Distribution, class Engine, class Compare,
    class Allocator>
inline bool operator>=(const Skiplist<Key, T, Distribution, Engine, Compare,
        Allocator>& lhs, const Skiplist<Key, T, Distribution, Engine, Compare,
        Allocator>& rhs)
{
    return !(lhs < rhs);
}

namespace std {

// Provide a specialization for std::swap

template<class Key, class T, class Distribution, class Engine, class Compare,
class Allocator> inline void swap(Skiplist<Key, T, Distribution, Engine,
        Compare, Allocator>& lhs, Skiplist<Key, T, Distribution, Engine,
        Compare, Allocator>& rhs)
{
    lhs.swap(rhs)
}

} // namespace std

#endif // SKIPLIST_HPP
