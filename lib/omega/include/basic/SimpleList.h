#if ! defined _Simple_List_h
#define _Simple_List_h 1

/*
 *  Linked lists with an interface like a bit of libg++'s SLSimple_List class
 */

#include <assert.h>
#include <basic/Iterator.h>
#include <basic/Collection.h>
#include <basic/Link.h>

namespace omega {
  
#define Simple_List Omega_Simple_List
#define Simple_List_Iterator Omega_Simple_List_Iterator

template<class T> class Simple_List_Iterator;

// A TEMPORARY HACK - ERROR IF YOU TRY TO USE "INDEX" - FERD

template<class T> class Simple_List : public Sequence<T> {
public:
  Simple_List(const Simple_List<T> &l)
  { contents = l.contents ? new List_Element<T>(*l.contents) : 0; }
  Simple_List() { contents = 0; }
  virtual ~Simple_List() { delete contents; }

  Iterator<T> *new_iterator();
  const T &operator[](int) const;
  T &operator[](int);

    
  int size() const;
  int length() const { return size(); }
  int empty() const { return size() == 0; }

  T &front() const;

// insertion/deletion on a list invalidates any iterators
// that are on/after the element added/removed

  T remove_front();	       

  void prepend(const T &item);
  void append(const T &item);

  void del_front();
  void clear();

  void join(Simple_List<T> &consumed);

  int index(const T &) const {
		assert(0&&"ILLEGAL SimpleList operation\n");
    return -1;
  }

private:
  friend class Simple_List_Iterator<T>;
  List_Element<T> **end()
  {
    List_Element<T> **e = &contents;
    while (*e)
      e = &((*e)->tail);
    return e;
  }

  List_Element<T> *contents;
};


template<class T> class Simple_List_Iterator : public List_Element_Iterator<T> {
public:
  Simple_List_Iterator(Simple_List<T> &l);
  Simple_List_Iterator(const Simple_List<T> &l);
  Simple_List_Iterator();
private:
  List_Element<T> &element() { return *this->i; } ;
  friend class Simple_List<T>;
};

} // namespace

#define instantiate_Simple_List(T)	template class Simple_List<T>;  \
  template class Simple_List_Iterator<T>;                           \
  instantiate_Only_List_Element(T)                                  \
  instantiate_Sequence(T)

namespace omega {
  
template<class T> Simple_List_Iterator<T>::Simple_List_Iterator(Simple_List<T> &l) 
: List_Element_Iterator<T>(l.contents) {}

template<class T> Simple_List_Iterator<T>::Simple_List_Iterator(const Simple_List<T> &l) 
: List_Element_Iterator<T>(l.contents) {}

template<class T> Simple_List_Iterator<T>::Simple_List_Iterator()
: List_Element_Iterator<T>(0) {}

template<class T> Iterator<T> *Simple_List<T>::new_iterator()
{
    return new Simple_List_Iterator<T>(*this);
}

template<class T> const T &Simple_List<T>::operator[](int i) const
{
    Simple_List_Iterator<T> p(*this);

    while(--i > 0 && p)
	p++;

    if (p)
	return *p;
    else
	return *((T *)0);
}

template<class T>       T &Simple_List<T>::operator[](int i)
{
    Simple_List_Iterator<T> p(*this);

    while(--i > 0 && p)
	p++;

    if (p)
	return *p;
    else
	return *((T *)0);
}


template<class T> int Simple_List<T>::size() const
    {
    int i = 0;
    List_Element<T> * p = contents;
    while (p)
	{
	p = p->tail;
	i++;
	}
    return i;
    }

template<class T> T &Simple_List<T>::front() const
    {
    return contents->head;
    }

template<class T> T Simple_List<T>::remove_front()
    {
    List_Element<T> *frunt = contents;
    contents = contents->tail;
    T fruntT = frunt->head;
    frunt->tail = 0;
    delete frunt;
    return fruntT;
    }

template<class T> void Simple_List<T>::prepend(const T &item)
    {
    contents = new List_Element<T>(item, contents);
    }


template<class T> void Simple_List<T>::append(const T &item)
    {
    *(end()) = new List_Element<T>(item, 0);
    }


template<class T> void Simple_List<T>::del_front()
    {
    List_Element<T> *e = contents;
    contents = contents->tail;
    e->tail = 0;
    delete e;
    }


template<class T> void Simple_List<T>::clear()
    {
    delete contents;
    contents = 0;
    }

template<class T> void Simple_List<T>::join(Simple_List<T> &consumed)
    {
    List_Element<T> *e = consumed.contents;
    consumed.contents = 0;
    *(end()) = e;
    }

} // namespace
#endif
