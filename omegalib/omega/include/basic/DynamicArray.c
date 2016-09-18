#include <assert.h>
#include <basic/DynamicArray.h>

namespace omega {
  
template<class T, int d> void DynamicArray<T,d>::do_constr()
    {
// #if ! defined SHUT_UP_ABOUT_STATEMENT_WITH_NO_EFFECT_IN_DYNAMIC_ARRAY_CREATION
//     assert(d > 0);
// #endif
    bounds = 0;
    elements = 0;
    partial = false;
    }


template<class T> void DynamicArray1<T>::do_construct(int d0)
    {
    this->bounds = new int[1];
    this->bounds[0] = d0;
    this->elements = new T [d0];
    this->partial = false;
    }

template<class T> void DynamicArray2<T>::do_construct(int d0, int d1)
    {
    this->bounds = new int[2];
    this->bounds[0] = d0;
    this->bounds[1] = d1;
    this->elements = new T [d0 * d1];
    this->partial = false;
    }

template<class T> void DynamicArray3<T>::do_construct(int d0,int d1,int d2)
    {
    this->bounds = new int[3];
    this->bounds[0] = d0;
    this->bounds[1] = d1;
    this->bounds[2] = d2;
    this->elements = new T [d0 * d1 * d2];
    this->partial = false;
    }

template<class T> void DynamicArray4<T>::do_construct(int d0,int d1,int d2,int d3)
    {
    this->bounds = new int[4];
    this->bounds[0] = d0;
    this->bounds[1] = d1;
    this->bounds[2] = d2;
    this->bounds[3] = d3;
    this->elements = new T [d0 * d1 * d2 * d3];
    this->partial = false;
    }

template<class T, int d> DynamicArray<T,d>::DynamicArray()
    {
    do_constr();
    }

template<class T> DynamicArray1<T>::DynamicArray1(const char *)
    {
    this->do_constr();
    }

template<class T> DynamicArray2<T>::DynamicArray2(const char *,const char *)
    {
    this->do_constr();
    }

template<class T> DynamicArray3<T>::DynamicArray3(const char *,const char *,const char *)
    {
    this->do_constr();
    }

template<class T> DynamicArray4<T>::DynamicArray4(const char *,const char *,const char *,const char *)
    {
    this->do_constr();
    }

template<class T> DynamicArray1<T>::DynamicArray1(int d0)
    {
    do_construct(d0);
    } 

template<class T> DynamicArray2<T>::DynamicArray2(int d0, int d1)
    {
    do_construct(d0, d1);
    }

template<class T> DynamicArray3<T>::DynamicArray3(int d0,int d1,int d2)
    {
    do_construct(d0, d1, d2);
    }

template<class T> DynamicArray4<T>::DynamicArray4(int d0,int d1,int d2,int d3)
    {
    do_construct(d0, d1, d2, d3);
    }


template<class T, int d> void DynamicArray<T,d>::do_destruct()
    {
    if (! partial)
	{
        delete [] bounds;
        delete [] elements;
	}
    }


template<class T, int d> DynamicArray<T,d>::~DynamicArray()
    {
    do_destruct();
    }


template<class T> void DynamicArray1<T>::resize(int d0)
    {
    assert(!this->partial);
    this->do_destruct();
    if (d0 == 0)
        this->do_constr();
    else
        do_construct(d0);
    } 

template<class T> void DynamicArray2<T>::resize(int d0, int d1)
    {
    assert(!this->partial);
    this->do_destruct();
    if (d0 == 0 && d1 == 0)
        this->do_constr();
    else
        do_construct(d0, d1);
    }

template<class T> void DynamicArray3<T>::resize(int d0, int d1, int d2)
    {
    assert(!this->partial);
    this->do_destruct();
    if (d0 == 0 && d1 == 0 && d2 == 0)
        this->do_constr();
    else
        do_construct(d0, d1, d2);
    }

template<class T> void DynamicArray4<T>::resize(int d0, int d1, int d2, int d3)
    {
    assert(!this->partial);
    this->do_destruct();
    if (d0 == 0 && d1 == 0 && d2 == 0 && d3 == 0)
        this->do_constr();
    else
        do_construct(d0, d1, d2, d3);
    }


template<class T> T& DynamicArray1<T>::operator[](int d0)
    { 
#if !defined (NDEBUG)
    assert(this->elements != 0 && "Trying to dereference undefined array");
    assert(0 <= d0 && d0 < this->bounds[0] && "Array subscript out of bounds");
#endif

    return this->elements[d0];
    }

template<class T>  DynamicArray1<T> DynamicArray2<T>::operator[](int d0)
    { 
#if !defined (NDEBUG)
    assert(this->elements != 0 && "Trying to dereference undefined array");
    assert(0 <= d0 && d0 < this->bounds[0] && "Array subscript out of bounds");
#endif

    DynamicArray1<T> result;
    result.bounds = this->bounds+1;
    result.elements = this->elements + this->bounds[1] * d0;
    result.partial = true;
    return result;
    }

template<class T>  DynamicArray2<T> DynamicArray3<T>::operator[](int d0)
    { 
#if !defined (NDEBUG)
    assert(this->elements != 0 && "Trying to dereference undefined array");
    assert(0 <= d0 && d0 < this->bounds[0] && "Array subscript out of bounds");
#endif
    DynamicArray2<T> result;
    result.bounds = this->bounds+1;
    result.elements = this->elements + this->bounds[1] * this->bounds[2] * d0;
    result.partial = true;
    return result;
    } 

template<class T>  DynamicArray3<T> DynamicArray4<T>::operator[](int d0)
    { 
#if !defined (NDEBUG)
    assert(this->elements != 0 && "Trying to dereference undefined array");
    assert(0 <= d0 && d0 < this->bounds[0] && "Array subscript out of bounds");
#endif

    DynamicArray3<T> result;
    result.bounds = this->bounds+1;
    result.elements = this->elements + this->bounds[1] * this->bounds[2] * this->bounds[3] * d0;
    result.partial = true;
    return result;
    } 


template<class T, int d> 
    DynamicArray<T,d>::DynamicArray(DynamicArray<T,d> &D)
    {
    assert(D.elements != 0 && "Trying to copy an undefined array");
    partial = true;
    bounds = D.bounds;
    elements = D.elements;
    }

} // namespace
