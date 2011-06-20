/**
 * @file
 * @author Sergiu Dotenco
 */

#ifndef SKIPLIST_HPP
#define SKIPLIST_HPP

#pragma once

#include <algorithm>
#include <cassert>
#include <cmath>
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
#endif // HAVE_CPP0X

template
<
    class Key,
    class T,
    class Engine,
    class Compare = std::less<Key>,
    class Allocator = std::allocator<std::pair<const Key, T> >
>
class Skiplist
{
    struct Node;

public:
    typedef Key key_type;
    typedef T mapped_type;
    typedef Allocator allocator_type;
    typedef std::pair<const Key, T> value_type;
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
              T,
              difference_type,
              pointer,
              reference
          >
    {
    public:
        const_iterator();
        const_iterator& operator++();
        const_iterator operator++(int);
        const_reference operator*();
        const_pointer operator->();
        bool operator==(const const_iterator& other) const;
        bool operator!=(const const_iterator& other) const;

    private:
        friend class Skiplist;

        explicit const_iterator(const Skiplist* list, const Node* node = NULL);

        const Node* node_;
        const Skiplist* list_;
    };

    class iterator
        : public const_iterator
    {
    public:
        explicit iterator(Skiplist* list, Node* node = NULL)
            : const_iterator(list, node)
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
        , eallocator_(allocator)
    {
        xinit();
    }

    explicit Skiplist(const Engine& engine,
            const Allocator& allocator = Allocator())
        : engine_(engine)
        , allocator_(allocator)
        , eallocator_(allocator)
    {
        xinit();
    }

    explicit Skiplist(const Compare& predicate, const Engine& engine = Engine(),
        const Allocator& allocator = Allocator())
        : engine_(engine)
        , allocator_(allocator)
        , eallocator_(allocator)
        , compare_(predicate)
    {
        xinit();
    }

    Skiplist(const Skiplist& other)
        : engine_(other.engine_)
        , allocator_(other.allocator_)
        , size_(other.size_)
        , compare_(other.compare_)
    {
        // TODO: copy header
    }

#ifdef HAVE_CPP0X
    Skiplist(std::initializer_list<value_type> values,
        const Allocator& allocator = Allocator())
        : eallocator_(allocator)
    {
    }

    Skiplist(Skiplist&& other)
    {
    }

#endif // HAVE_CPP0X

    ~Skiplist()
    {
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
        return allocator_.max_size();
    }

    size_type max_level() const
    {
        return 32;
    }

    void clear()
    {
    }

    const_iterator cbegin() const
    {
        return const_iterator(this, &first_);
    }

    const_iterator cend() const
    {
        return const_iterator(this);
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
        return const_iterator(this, &first_);
    }

    iterator end()
    {
        return const_iterator(this);
    }

    void swap(Skiplist& other)
    {
        using std::swap;  // Allow ADL

        swap(engine_, other.engine_);
        swap(allocator_, other.allocator_);
        swap(size_, other.size_);
        swap(compare_, other.compare_);
        first_.swap(other.first_);
    }

    Skiplist& operator=(const Skiplist& other)
    {
        Skiplist(other).swap(*this);
        return *this;
    }

    allocator_type get_allocator() const
    {
        return allocator_;
    }

    iterator insert(const_reference value)
    {
        iterator result(this);

        Node* currentNode = &first_;
        typename next_vector_type::difference_type i =
            static_cast<next_vector_type::difference_type>
                (currentNode->next.size() - 1);

        node_vector_type update(max_level());

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
            Element* e = currentNode->next.front();
            e->value.second = value.second;
            result.node_ = e;
        }
        else {
            typedef typename next_vector_type::size_type next_size_type;
            next_size_type level = next_level();

            if (level > first_.next.size()) {
                // Adjust the header
                for (next_size_type i = first_.next.size(); i != level; ++i)
                    update[i] = &first_;

                next_size_type count = level - first_.next.size();
                first_.next.insert(first_.next.end(), count, NULL);
            }

            Element* node = xconstruct(Element(value));
            node->next.reserve(level);

            // Perform the actual insertion
            for (next_size_type i = 0; i != level; ++i) {
                Element* successor = update[i]->next[i];
                node->next.push_back(successor);
                update[i]->next[i] = node;

                if (successor) {
                    // Let the successor of the new node point to it
                    successor->previous = node;
                }
            }

            node->previous = update.front();

            ++size_;

            result.node_ = node;
        }

        return result;
    }

    template<class InputIterator>
    void insert(InputIterator first, InputIterator last)
    {
        for ( ; first != last; ++first)
            insert(*first);
    }

    void erase(iterator where)
    {
        Node* const node = const_cast<Node*>(where.node_);
        Node* currentNode = node->previous;

        typedef typename next_vector_type::size_type next_size_type;

        next_size_type count = currentNode->next.size();
        node_vector_type update(max_level());

        // Collect node's predecessors
        for (next_size_type i = 0; i != count; ++i) {
            while (currentNode->next.size() <= i)
                currentNode = currentNode->previous;

            update[i] = currentNode;
        }

        currentNode = node;
        bool stop = false;

        // Let the predecessors point to successors of the node being deleted
        Node* const previous = update.front();

        for (next_size_type i = 0; i != first_.next.size() && !stop; ++i) {
            if (!update[i] || update[i]->next[i] != currentNode)
                stop = true;
            else {
                Element* e = currentNode->next[i];
                update[i]->next[i] = e;

                if (e)
                    e->previous = previous;
            }
        }

        xdestroy(static_cast<Element*>(currentNode));

        while (!first_.next.empty() && !first_.next.back())
            first_.next.pop_back();

        --size_;
    }

    iterator find(const key_type& key)
    {
        Node* x = xfind(&first_, key, this);
        return iterator(this, x);
    }

    const_iterator find(const_reference value) const
    {
        const Node* x = xfind(&first_, key, this);
        return const_iterator();
    }

private:
    struct Element;

    friend struct Element;
    friend class const_iterator;

    typedef std::vector<Element*,
        typename Allocator::template rebind<Element*>::other>
            next_vector_type;
    typedef std::vector<Node*,
        typename Allocator::template rebind<Node*>::other>
            node_vector_type;

    struct Node
    {
        Node()
            : previous(NULL)
        {
        }

        virtual ~Node()
        {
        }

        void swap(Node& other)
        {
            next.swap(other);
        }

        //! Node's successors.
        next_vector_type next;
        //! Node's predecessor.
        Node* previous;
    };

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

    typedef typename Allocator::template rebind<Element>::other
        element_allocator;

    void xinit()
    {
        size_ = 0;
        last_ = NULL;
        end_.previous = &first_;
    }

    size_type next_level()
    {
        const double p = 0.5;
        const double max = static_cast<double>(engine_.max() - engine_.min());

        size_type level = 1;

        while (level < max_level() && engine_() / max < p)
            ++level;

        return level;
    }

    bool xequal(const key_type& lhs, const key_type& rhs) const
    {
        return !compare_(lhs, rhs) && !compare_(rhs, lhs);
    }

    template<class Node, class S>
    static Node* xfind(Node* x, const key_type& key, S* s)
    {
        typename next_vector_type::difference_type level =
            static_cast<next_vector_type::difference_type>
                (x->next.size() - 1);

        for ( ; level >= 0; --level) {
            while (x->next[level] &&
                s->compare_(x->next[level]->value.first, key))
                x = x->next[level];
        }

        assert(x);
        assert(!x->next.empty());

        Node* result = NULL;

        if (s->xequal(x->next.front()->value.first, key))
            result = x->next.front();

        return result;
    }

    Element* xconstruct(const Element& value)
    {
        Element* e = eallocator_.allocate(1);
        eallocator_.construct(e, value);

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
    element_allocator eallocator_;
    Node first_;
    Node* last_;
    Node end_;
};

template<class Key, class T, class Engine, class Compare, class Allocator>
inline Skiplist<Key, T, Engine, Compare,
    Allocator>::const_iterator::const_iterator()
    : node_(NULL)
    , list_(NULL)
{
}

template<class Key, class T, class Engine, class Compare, class Allocator>
inline Skiplist<Key, T, Engine, Compare,
    Allocator>::const_iterator::const_iterator(const Skiplist* list,
        const Node* node)
    : list_(list)
    , node_(node)
{
}

template<class Key, class T, class Engine, class Compare, class Allocator>
inline typename Skiplist<Key, T, Engine, Compare, Allocator>::const_iterator&
Skiplist<Key, T, Engine, Compare, Allocator>::const_iterator::operator++()
{
    node_ = node_->next.front();
    return *this;
}

template<class Key, class T, class Engine, class Compare, class Allocator>
inline typename Skiplist<Key, T, Engine, Compare, Allocator>::const_iterator
Skiplist<Key, T, Engine, Compare, Allocator>::const_iterator::operator++(int)
{
    const_iterator tmp(*this);
    ++*this;

    return tmp;
}

template<class Key, class T, class Engine, class Compare, class Allocator>
inline typename Skiplist<Key, T, Engine, Compare, Allocator>::const_reference
Skiplist<Key, T, Engine, Compare, Allocator>::const_iterator::operator*()
{
    return node_->next.front()->value;
}

template<class Key, class T, class Engine, class Compare, class Allocator>
inline typename Skiplist<Key, T, Engine, Compare, Allocator>::const_pointer
    Skiplist<Key, T, Engine, Compare, Allocator>::const_iterator::operator->()
{
    return &node_->next.front()->value;
}

template<class Key, class T, class Engine, class Compare, class Allocator>
inline bool Skiplist<Key, T, Engine, Compare,
    Allocator>::const_iterator::operator==(const const_iterator& other) const
{
    assert(list_ == other.list_);
    return node_ == other.node_;
}

template<class Key, class T, class Engine, class Compare, class Allocator>
inline bool Skiplist<Key, T, Engine, Compare,
    Allocator>::const_iterator::operator!=(const const_iterator& other) const
{
    assert(list_ == other.list_);
    return !(*this == other);
}

namespace std {

// Provide a specialization for std::swap

template<class Key, class T, class Engine, class Compare, class Allocator>
inline void swap(Skiplist<Key, T, Engine, Compare, Allocator>& lhs,
                 Skiplist<Key, T, Engine, Compare, Allocator>& rhs)
{
    lhs.swap(rhs)
}

} // namespace std

#endif // SKIPLIST_HPP
