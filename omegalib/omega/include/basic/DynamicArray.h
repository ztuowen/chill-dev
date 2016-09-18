#ifndef Already_Included_DynamicArray
#define Already_Included_DynamicArray

namespace omega {

template <class T> class DynamicArray2;
template <class T> class DynamicArray3;
template <class T> class DynamicArray4;

template <class T, int d> class DynamicArray
    {
    public:
	DynamicArray(DynamicArray<T,d> &D);
        ~DynamicArray();

    protected:
	DynamicArray();
	bool partial;
	int *bounds;
	T *elements;

	void do_constr();
	void do_destruct();
    };


template <class T> class DynamicArray1 : public DynamicArray<T,1>
    {
    public:
	DynamicArray1(const char *s0 = 0);
	DynamicArray1(int d0);
	void resize(int d0);
        T& operator[](int d);

	friend class DynamicArray2<T>;

    private:
	void do_construct(int d0);
    };


template <class T> class DynamicArray2 : public DynamicArray<T,2>
    {
    public:
	DynamicArray2(const char *s0 = 0, const char *s1 = 0);
	DynamicArray2(int d0, int d1);
	void resize(int d0, int d1);
  	DynamicArray1<T> operator[](int d);

	friend class DynamicArray3<T>;

    private:
	void do_construct(int d0, int d1);
    };


template <class T> class DynamicArray3 : public DynamicArray<T,3>
    {
    public:
	DynamicArray3(const char *s0 = 0, const char *s1 = 0, const char *s2 = 0);
	DynamicArray3(int d0, int d1, int d2);
	void resize(int d0, int d1, int d2);
  	DynamicArray2<T> operator[](int d);

	friend class DynamicArray4<T>;

    private:
	void do_construct(int d0, int d1, int d2);
    };

template <class T> class DynamicArray4 : public DynamicArray<T,4>
    {
    public:
	DynamicArray4(const char *s0 = 0, const char *s1 = 0, const char *s2 = 0, const char *s3 = 0);
	DynamicArray4(int d0, int d1, int d2, int d3);
	void resize(int d0, int d1, int d2, int d3);
  	DynamicArray3<T> operator[](int d);

    private:
	void do_construct(int d0, int d1, int d2, int d3);
    };

} // namespace

#if ! defined DONT_INCLUDE_TEMPLATE_CODE
#include <basic/DynamicArray.c>
#endif

#define instantiate_DynamicArray1(T)	template class DynamicArray1<T>; \
					template class DynamicArray<T,1>;

#define instantiate_DynamicArray2(T)	template class DynamicArray2<T>;  \
					template class DynamicArray<T,2>; \
					instantiate_DynamicArray1(T);

#define instantiate_DynamicArray3(T)	template class DynamicArray3<T>;  \
					template class DynamicArray<T,3>; \
					instantiate_DynamicArray2(T);

#define instantiate_DynamicArray4(T)	template class DynamicArray4<T>;  \
					template class DynamicArray<T,4>; \
					instantiate_DynamicArray3(T);
#endif
