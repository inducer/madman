/* -*- C++ -*-
 *
 * refcnt_ptr.h Copyright 2002 Shawn Willden <shawn@willden.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#ifndef refcnt_ptr_h
#define refcnt_ptr_h

#include <assert.h>
#include <memory>

/**
 * refcnt_ptr implements a reference-counting smart pointer class.
 *
 * In general, refcnt_ptrs do the Right Thing.  You can use them as
 * though they were pointers, copy them, assign them, whatever, and
 * the object they point to will continue to live as long as there is
 * at least one refcnt_ptr pointing at it.  There are some caveats,
 * some obvious some less obvious.  They are:
 *
 * 1.  Don't create multiple refcnt_ptr objects from the same raw
 * pointer.  Each one of them will delete it.  The best usage is to
 * put the raw pointer into a refcnt_ptr as soon as you get it, and
 * then make copies of the refcnt_ptr as needed.
 *
 * 2.  Don't create auto_ptrs from refcnt_ptrs, or do other things
 * that might cause the object to be deleted.  Yes, this is obvious,
 * but it's worth hammering home anyway: The *only* safe way to delete
 * an object once you've pointed a refcnt_ptr at it is to destroy all
 * of the relevent refcnt_ptrs.  There is no analog to
 * auto_ptr::release(). Be extremely careful with refcnt_ptr::get().
 *
 * 3.  Don't share refcnt_ptrs across threads, unless you know what
 * you're doing.  If you do it, you will need to synchronize all
 * actions that might update the reference count, but be careful
 * because C++ sometimes creates temporary copies where you might not
 * expect, so knowing what needs to be synchronized requires good C++
 * skillz.  Oh, and you obviously need to synchronize uses of the
 * referenced object as well
 *
 * 4.  refcnt_ptr supports copying and assignment across compatible
 * types.  That is, if T1* is convertible to T2*, then you can copy or
 * assign a refcnt_ptr<T1> to a refcnt_ptr<T2>.  This is very
 * convenient, particularly in cases where T2 is const T1, or where T1
 * is derived from T2.  Be careful, however about pointer upcasts to
 * types that don't have a virtual destructor.  If you create a
 * Derived and make a refcnt_ptr<Base> point to it, it will get
 * destroyed as a Base, not as a Derived, unless Base has a virtual
 * dtor.  IOW, don't slice pointers.
 *
 * 5.  Keep in mind that reference counting does have run-time
 * overhead, and it can get to be significant if you use refcnt_ptr
 * mindlessly, because every copy, assignment and destruction of a
 * refcnt_ptr involves updating the reference count -- just a
 * dereference plus an increment or decrement but there are cases
 * where it matters.  Space overhead may also be an issue if you have
 * lots and lots of pointers.  n pointers to T consume
 *
 *     (n * sizeof(T*)) bytes,
 *
 * whereas n refcnt_ptr<T>s consume
 *
 *     (n * sizeof(T*) + n * sizeof(int*) + sizeof(int)) bytes.
 *
 * Plus some structure padding, maybe, depending on the platform.  In
 * general, just assume that a refcnt_ptr uses three times as much
 * space as a regular pointer.
 */
template <typename T> class refcnt_ptr
    {
public:
    explicit refcnt_ptr(T* obj = 0) throw()
	{
	object = obj;
	if (object)
	    refCnt = new int(1);
#ifndef NDEBUG
	else
	    refCnt = 0;
#endif // NDEBUG
	}

    // Copy ctor.
    refcnt_ptr(const refcnt_ptr<T>& rhs) throw()
	{
	object = rhs.object;
	refCnt = rhs.refCnt;
	increaseCount();
	}

    // Allow creation of a refcnt_ptr from a compatible but different
    // refcnt_ptr type.
    template<typename T2> 
    refcnt_ptr(const refcnt_ptr<T2>& rhs) throw()
	{
	object = rhs.object;
	refCnt = rhs.refCnt;
	increaseCount();
	}

    // Allow creation of a refcnt_ptr from a compatible but different
    // auto_ptr type.
    template<typename T2> 
    refcnt_ptr(std::auto_ptr<T2>& rhs) throw ()
	{
	object = rhs.release();
	refCnt = new int(1);
	}

    // Dtor.  May throw if the T dtor throws.
    ~refcnt_ptr()
	{
	reduceCount();
	}

    // Assignment operator.  May throw if the T dtor throws.
    refcnt_ptr& operator=(const refcnt_ptr& rhs)
	{
	reduceCount();
	object = rhs.object;
	refCnt = rhs.refCnt;
	increaseCount();

	return *this;
	}

    // Assignment operator for assigning from a compatible but
    // different refcnt_ptr type. May throw if the T dtor throws.
    template <typename T2> refcnt_ptr& operator=(const refcnt_ptr<T2> rhs)
	{
	reduceCount();
	object = rhs.object;
	refCnt = rhs.refCnt;
	increaseCount();

	return *this;
	}

    // Assignment operator for assigning from a compatible but
    // different auto_ptr type.  May throw if the T dtor throws.
    template <typename T2> refcnt_ptr& operator=(std::auto_ptr<T2> rhs)
	{
	reduceCount();
	object = rhs.release();
	refCnt = new int(1);
	
	return *this;
	}
       
    // Pointer dereference.  Don't do this if the refcnt_ptr is null
    // (i.e. 'object' is null).
    T* operator->() const throw() { return object; }

    // Pointer dereference.  Don't do this if the refcnt_ptr is null
    // (i.e. 'object' is null).
    T& operator*() const throw() { return *object; }

    // Return the current reference count.
    int count() const throw() { if (!object) return 0; return *refCnt; }

    // Test the refcnt_ptr for validity.
    operator bool() { return object; }

    // Return the underlying pointer.  This does not affect the
    // reference counting at all, which means that the returned naked
    // pointer may dangle if all of the relevant refcnt_ptr objects
    // are destroyed.
    T* get() const throw() { return object; }

private:
    void reduceCount()
	{
	if (object)
	    {
	    assert(*refCnt > 0);
	    if ((--*refCnt) == 0)
		{
		delete object;
		delete refCnt;
		}
	    }
	}
    
    void increaseCount()
	{
	if (object)
	    ++*refCnt;
	}

    T*   object;
    int* refCnt;
    };





#endif // refcnt_ptr_h




// EMACS-FORMAT-TAG
//
// Local Variables:
// mode: C++
// eval: (c-set-style "stroustrup")
// eval: (c-set-offset 'access-label -2)
// eval: (c-set-offset 'inclass '++)
// c-basic-offset: 2
// tab-width: 8
// End:
