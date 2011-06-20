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
        const_iterator& operator--();
        const_iterator operator--(int);
        const_reference operator*();
        const_pointer operator->();
        bool operator==(const const_iterator& other) const;
        bool operator!=(const const_iterator& other) const;

    private:
        friend class Skiplist;

        explicit const_iterator(const Skiplist* list, const Node* node = NULL);

        const Node* node;
        const Skiplist* list;
    };

    class iterator
        : public const_iterator
    {
    public:
        explicit iterator(Skiplist* list, Node* node = NULL)
            : const_iterator(list, node)
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
            return const_cast<reference>(const_iterator::operator->());
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
        insert(values.begin(), values.end());
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
        return eallocator_.max_size();
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
        return const_iterator(this, &end_);
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
        return iterator(this, &first_);
    }

    iterator end()
    {
        return iterator(this, &end_);
    }

    reverse_iterator rbegin()
    {
        return reverse_iterator(end());
    }

    reverse_iterator rend()
    {
        return reverse_iterator(begin());
    }

    const_reference front() const
    {
        return *begin();
    }

    reference front()
    {
        return *begin();
    }

    const_reference back() const
    {
        *return end_->previous;
    }

    reference back()
    {
        *return end_->previous;
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
            result.node = node;
        }
        else {
            typedef typename ElementPtrVector::size_type next_size_type;
            next_size_type level = next_level();

            if (level > first_.next.size()) {
                // Adjust the number of nodes in the header
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
            }

            if (node->next.front()) {
                // Let the successor of the new node point to it
                node->next.front()->previous = node;
            }

            if (end_.previous == &first_ ||
                compare_(static_cast<const Element*>
                    (end_.previous)->value.first, node->value.first)) {
                // Ensure that the last node doesn't have any successors
                assert(std::count_if(node->next.begin(), node->next.end(),
                    std::not1(std::bind2nd(std::equal_to<Element*>(),
                        static_cast<Element*>(NULL)))) == 0);
                end_.previous = node;
            }

            node->previous = update.front();
            assert(node->previous->next.front() == node);

            ++size_;

            result.node = node;
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
        assert(where != end() && "Invalid iterator");

        Node* const node = const_cast<Node*>(where.node);
        Node* currentNode = node->previous;

        typedef typename ElementPtrVector::size_type next_size_type;

        next_size_type count = currentNode->next.size();
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

        if (node->next.front())
            node->next.front()->previous = previous;

        for (next_size_type i = 0; i != first_.next.size() && !stop; ++i) {
            if (!update[i] || update[i]->next[i] != node)
                stop = true;
            else {
                Element* succesor = node->next[i];
                update[i]->next[i] = succesor;
            }
        }

        if (node == end_.previous) {
            // Ensure that the last node doesn't have any successors
            assert(std::count_if(previous->next.begin(), previous->next.end(),
                std::not1(std::bind2nd(std::equal_to<Element*>(),
                static_cast<Element*>(NULL)))) == 0);
            end_.previous = previous;
        }

        xdestroy(static_cast<Element*>(node));

        // Remove all trailing non-null element pointers
        ElementPtrVector::reverse_iterator it =
            std::find_if(first_.next.rbegin(), first_.next.rend(),
                std::not1(std::bind2nd(std::equal_to<Element*>(),
                    static_cast<Element*>(NULL))));
        first_.next.erase(it.base(), first_.next.end());

        --size_;
    }

    iterator find(const key_type& key)
    {
        Node* node = xfind(&first_, key, this);

        if (!node)
            node = &end_;

        return iterator(this, node);
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
        typename Allocator::template rebind<Element*>::other> ElementPtrVector;
    typedef std::vector<Node*,
        typename Allocator::template rebind<Node*>::other> NodePtrVector;

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
        typename ElementPtrVector::difference_type level =
            static_cast<ElementPtrVector::difference_type>
                (x->next.size() - 1);

        for ( ; level >= 0; --level) {
            while (x->next[level] &&
                s->compare_(x->next[level]->value.first, key))
                x = x->next[level];
        }

        assert(x);
        assert(!x->next.empty());

        Node* result = NULL;
        Element* candidate = x->next.front();

        if (candidate && s->xequal(candidate->value.first, key))
            result = candidate;

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
    Node end_;
};

template<class Key, class T, class Engine, class Compare, class Allocator>
inline Skiplist<Key, T, Engine, Compare,
    Allocator>::const_iterator::const_iterator()
    : node(NULL)
    , list(NULL)
{
}

template<class Key, class T, class Engine, class Compare, class Allocator>
inline Skiplist<Key, T, Engine, Compare,
    Allocator>::const_iterator::const_iterator(const Skiplist* list,
        const Node* node)
    : list(list)
    , node(node)
{
}

template<class Key, class T, class Engine, class Compare, class Allocator>
inline typename Skiplist<Key, T, Engine, Compare, Allocator>::const_iterator&
Skiplist<Key, T, Engine, Compare, Allocator>::const_iterator::operator++()
{
    if (node->next.front() == list->end_.previous)
        node = &list->end_;
    else
        node = node->next.front();

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
inline typename Skiplist<Key, T, Engine, Compare, Allocator>::const_iterator&
    Skiplist<Key, T, Engine, Compare, Allocator>::const_iterator::operator--()
{
    assert(node->previous);
    node = node->previous->previous;

    if (!node)
        node = &list->end_;

    return *this;
}

template<class Key, class T, class Engine, class Compare, class Allocator>
inline typename Skiplist<Key, T, Engine, Compare, Allocator>::const_iterator
    Skiplist<Key, T, Engine, Compare, Allocator>::const_iterator::operator--(int)
{
    const_iterator tmp(*this);
    --*this;

    return tmp;
}

template<class Key, class T, class Engine, class Compare, class Allocator>
inline typename Skiplist<Key, T, Engine, Compare, Allocator>::const_reference
Skiplist<Key, T, Engine, Compare, Allocator>::const_iterator::operator*()
{
    return node->next.front()->value;
}

template<class Key, class T, class Engine, class Compare, class Allocator>
inline typename Skiplist<Key, T, Engine, Compare, Allocator>::const_pointer
    Skiplist<Key, T, Engine, Compare, Allocator>::const_iterator::operator->()
{
    return &node->next.front()->value;
}

template<class Key, class T, class Engine, class Compare, class Allocator>
inline bool Skiplist<Key, T, Engine, Compare,
    Allocator>::const_iterator::operator==(const const_iterator& other) const
{
    assert(list == other.list);
    return node == other.node;
}

template<class Key, class T, class Engine, class Compare, class Allocator>
inline bool Skiplist<Key, T, Engine, Compare,
    Allocator>::const_iterator::operator!=(const const_iterator& other) const
{
    assert(list == other.list);
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
