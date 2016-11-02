/*
*  Stack Allocator for C++ STL Container
* _P0W!
*/

#include <cstddef>
#include <cassert>

template< std::size_t N, size_t alignment = alignof(std::max_align_t) >
class Pool
{
   alignas(alignment)  char m_buffer[N];
   char *m_ptr;

public:
   ~Pool() 
   {
      m_ptr = nullptr;
   }

   Pool() noexcept 
      : m_ptr(m_buffer) 
   {

   }
   static std::size_t align_up(std::size_t n) noexcept
   {
     return ( n + (alignment-1) ) & ~( alignment-1 );
   }

   bool pointer_within_buffer(char* p) const noexcept
   {
     return m_buffer <= p && p <= m_buffer + N;
   }

   template <std::size_t reqAlign> 
   char* allocate( size_t size )
   {
      static_assert( reqAlign <= alignment, "alignment is too small for this pool" );
      assert( pointer_within_buffer(m_ptr) && "allocator outside allotated space" );
     
      const std::size_t aligned_n = align_up(size);
      if (static_cast<std::size_t>(m_buffer + N - m_ptr) >= aligned_n)
      {
         char* r = m_ptr;
         m_ptr += aligned_n;
         return r;
      }
     
      static_assert( alignment <= alignof(std::max_align_t), 
      "alignment that is larger than alignof(std::max_align_t), and "
      "cannot be guaranteed by normal operator new"
      );
     
      // Disallow any allocation outside of m_buffer
      assert( false && "Need more space" );
     
      return nullptr;
     
     /* return static_cast<char*>(::operator new(size));    */
   }

   void deallocate(char* p, std::size_t size ) noexcept
   {
      if (pointer_within_buffer(p))
      {
         size = align_up(size);
         if (p + size == m_ptr)
         {
            m_ptr = p;
         }
      }
      /*
      Not possible
      else
      ::operator delete(p);    
      */
   }

   // Not Copyable and Assignable
   Pool( const Pool& ) = delete;
   Pool& operator=( const Pool& ) = delete;
};

template <class T, std::size_t N, std::size_t Align = alignof(std::max_align_t)>
class MyAllocator
{
public:
   using value_type = T;
   static auto constexpr alignment = Align;
   static auto constexpr size = N;
   using pool_type = Pool<size, alignment>;

private:
   pool_type& m_pool;

public:
   MyAllocator(const MyAllocator&) = default;
   MyAllocator& operator=(const MyAllocator&) = delete;

   MyAllocator(pool_type& a) noexcept : m_pool(a)
   {
      static_assert(size % alignment == 0,
         "size N needs to be a multiple of alignment 'Align'");
   }
   template <class U>
   MyAllocator(const MyAllocator<U, N, alignment>& a) noexcept
      : m_pool(a.m_pool) {}

   template <class _Up> 
   struct rebind 
   {
     using other = MyAllocator<_Up, N, alignment>;
   };

   T* allocate(std::size_t n)
   {
      return reinterpret_cast<T*>(m_pool.template allocate<alignof(T)>(n*sizeof(T)));
   }
   void deallocate(T* p, std::size_t n) noexcept
   {
      m_pool.deallocate(reinterpret_cast<char*>(p), n*sizeof(T));
   }
    
  // For containers like List, Unordered Set
   template <class U, std::size_t M, std::size_t A> friend class MyAllocator;

};

/*
template <class T, std::size_t N, std::size_t A1, class U, std::size_t M, std::size_t A2>
inline
bool
operator==(const short_alloc<T, N, A1>& x, const short_alloc<U, M, A2>& y) noexcept
{
return N == M && A1 == A2 && &x.a_ == &y.a_;
}

template <class T, std::size_t N, std::size_t A1, class U, std::size_t M, std::size_t A2>
inline
bool
operator!=(const short_alloc<T, N, A1>& x, const short_alloc<U, M, A2>& y) noexcept
{
return !(x == y);
}
*/

#include <list>
#include <vector>
#include <unordered_set>

template <class T, std::size_t BufSize = 20000u >
using CustomVector = std::vector<T, MyAllocator<T, BufSize, alignof(T)>>;

template <class T, std::size_t BufSize = 200000u >
using CustomList = std::list<T, MyAllocator<T, BufSize >>;

template <class T, std::size_t BufSize = 200000u >
using CustomSet = std::unordered_set<T, std::hash<T>, std::equal_to<T>,
                      MyAllocator<T, BufSize, alignof(T) < 8 ? 8 : alignof(T)>>;


int main()
{

   CustomList<int>::allocator_type::pool_type customListAllocator;
   CustomList<int> customList{customListAllocator};

   CustomVector<int>::allocator_type::pool_type customVectorAllocator;
   CustomVector<int> customVector{customVectorAllocator};
  
   CustomSet<int>::allocator_type::pool_type customSetAllocator;
   CustomSet<int> customSet{customSetAllocator};
  
   customVector.reserve( 5000 );

   for( int i =0;i <2048; ++i )
   {
      customList.push_back(i) ;
      customVector.push_back( i );
      customSet.insert( i );
   }

   return 0;
}
