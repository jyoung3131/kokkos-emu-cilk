/*
//@HEADER
// ************************************************************************
//
//                        Kokkos v. 2.0
//              Copyright (2014) Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact Christian R. Trott (crtrott@sandia.gov)
//
// ************************************************************************
//@HEADER
*/

#ifndef KOKKOS_CILKPLUSTEAM_HPP
#define KOKKOS_CILKPLUSTEAM_HPP

#include <Kokkos_Macros.hpp>
#if defined( KOKKOS_ENABLE_CILKPLUS )

#include <cstdio>

#include <utility>
#include <impl/Kokkos_Spinwait.hpp>
#include <impl/Kokkos_FunctorAdapter.hpp>
#include <impl/Kokkos_HostThreadTeam.hpp>

#include <Kokkos_Atomic.hpp>

//----------------------------------------------------------------------------

namespace Kokkos {
namespace Impl {


//----------------------------------------------------------------------------

class CilkPlusExecTeamMember {
private:

  enum { TEAM_REDUCE_SIZE = 512 };

  typedef Kokkos::Experimental::CilkPlus execution_space ;
  typedef execution_space::scratch_memory_space space ;

  space                 m_team_shared ;
  int                   m_team_shared_size ;
  int                   m_team_size ;
  int                   m_team_rank ;
  int                   m_team_rank_rev ;
  int                   m_league_size ;
  int                   m_league_end ;
  int                   m_league_rank ;

  int                   m_chunk_size;
  int                   m_league_chunk_end;
  int                   m_team_alloc;

  inline
  void set_team_shared()
    { new( & m_team_shared ) space( ((char *) (*m_team_base)->scratch_memory()) + TEAM_REDUCE_SIZE , m_team_shared_size ); }

public:

  KOKKOS_INLINE_FUNCTION static int team_reduce_size() { return TEAM_REDUCE_SIZE ; }

  KOKKOS_INLINE_FUNCTION
  const execution_space::scratch_memory_space & team_shmem() const
    { return m_team_shared.set_team_thread_mode(0,1,0) ; }

  KOKKOS_INLINE_FUNCTION
  const execution_space::scratch_memory_space & team_scratch(int) const
    { return m_team_shared.set_team_thread_mode(0,1,0) ; }

  KOKKOS_INLINE_FUNCTION
  const execution_space::scratch_memory_space & thread_scratch(int) const
    { return m_team_shared.set_team_thread_mode(0,team_size(),team_rank()) ; }

  KOKKOS_INLINE_FUNCTION int league_rank() const { return m_league_rank ; }
  KOKKOS_INLINE_FUNCTION int league_size() const { return m_league_size ; }
  KOKKOS_INLINE_FUNCTION int team_rank() const { return m_team_rank ; }
  KOKKOS_INLINE_FUNCTION int team_size() const { return m_team_size ; }

  KOKKOS_INLINE_FUNCTION void team_barrier() const
    {
      // need cilkplus barrier here
    }

  template<class ValueType>
  KOKKOS_INLINE_FUNCTION
  void team_broadcast(ValueType& value, const int& thread_id) const
  {
	  // need cilk broadcast here.
  }

  template<class Closure, class ValueType>
  KOKKOS_INLINE_FUNCTION
  void team_broadcast(Closure const & f, ValueType& value, const int& thread_id) const
  {
	  // need cilk broadcast here.
  }
  
  template< typename Type >
  KOKKOS_INLINE_FUNCTION
  typename std::enable_if< !Kokkos::is_reducer< Type >::value , Type>::type
  team_reduce( const Type & value ) const
    { return Type(); }

    template< typename ReducerType >
    KOKKOS_INLINE_FUNCTION
    typename std::enable_if< Kokkos::is_reducer< ReducerType >::value >::type
    team_reduce( const ReducerType & reducer ) const
    {
    }

  template< class ValueType, class JoinOp >
  KOKKOS_INLINE_FUNCTION ValueType
    team_reduce( const ValueType & value
               , const JoinOp & op_in ) const
    { return ValueType(); }

  /** \brief  Intra-team exclusive prefix sum with team_rank() ordering
   *          with intra-team non-deterministic ordering accumulation.
   *
   *  The global inter-team accumulation value will, at the end of the
   *  league's parallel execution, be the scan's total.
   *  Parallel execution ordering of the league's teams is non-deterministic.
   *  As such the base value for each team's scan operation is similarly
   *  non-deterministic.
   */
  template< typename ArgType >
  KOKKOS_INLINE_FUNCTION ArgType team_scan( const ArgType & value , ArgType * const global_accum ) const
    { return ArgType(); }

  /** \brief  Intra-team exclusive prefix sum with team_rank() ordering.
   *
   *  The highest rank thread can compute the reduction total as
   *    reduction_total = dev.team_scan( value ) + value ;
   */
  template< typename ArgType >
  KOKKOS_INLINE_FUNCTION ArgType team_scan( const ArgType & value ) const
    { return this-> template team_scan<ArgType>( value , 0 ); }


  //----------------------------------------
  // Private for the driver

  template< class ... Properties >
  CilkPlusExecTeamMember( const TeamPolicyInternal< Kokkos::Experimental::CilkPlus , Properties ... > & team
                        , const int shared_size )
    : m_team_shared(0,0)
    , m_team_shared_size( shared_size )
    , m_team_size(team.team_size())
    , m_team_rank(0)
    , m_team_rank_rev(0)
    , m_league_size(0)
    , m_league_end(0)
    , m_league_rank(0)
    , m_chunk_size( team.chunk_size() )
    , m_league_chunk_end(0)
    , m_team_alloc( team.team_alloc())
   {
      if ( team.league_size() ) {
        // Execution is using device-team interface:
        Kokkos::Experimental::CilkPlus::thread_pool_size(0)
        Kokkos::Experimental::EmuReplicatedSpace::memory_zones()

        const int pool_rank_rev = m_exec->pool_size() - ( m_exec->pool_rank() + 1 );
        const int team_rank_rev = pool_rank_rev % team.team_alloc();
        const size_t pool_league_size     = m_exec->pool_size() / team.team_alloc() ;
        const size_t pool_league_rank_rev = pool_rank_rev / team.team_alloc() ;
        if(pool_league_rank_rev >= pool_league_size) {
          m_invalid_thread = 1;
          return;
        }
        const size_t pool_league_rank     = pool_league_size - ( pool_league_rank_rev + 1 );

        const int pool_num_teams       = ;
        const int chunk_size           = team.chunk_size()>0?team.chunk_size():team.team_iter();
        const int chunks_per_team      = ( team.league_size() + chunk_size*pool_num_teams-1 ) / (chunk_size*pool_num_teams);
              int league_iter_end      = team.league_size() - pool_league_rank_rev * chunks_per_team * chunk_size;
              int league_iter_begin    = league_iter_end - chunks_per_team * chunk_size;
        if (league_iter_begin < 0)     league_iter_begin = 0;
        if (league_iter_end>team.league_size()) league_iter_end = team.league_size();

        if ( team_rank_rev < team.team_size() ) {

          m_team_size        = team.team_size() ;
          m_team_rank        = team.team_size() - ( team_rank_rev + 1 );
          m_team_rank_rev    = team_rank_rev ;
          m_league_size      = team.league_size();

          m_league_rank      = ( team.league_size() *  pool_league_rank    ) / pool_league_size ;
          m_league_end       = ( team.league_size() * (pool_league_rank+1) ) / pool_league_size ;

          set_team_shared();
        }

        
      }
      else
      { m_invalid_thread = 1; }
    }

  CilkPlusExecTeamMember()
    : m_team_shared(0,0)
    , m_team_shared_size(0)
    , m_team_size(1)
    , m_team_rank(0)
    , m_team_rank_rev(0)
    , m_league_size(1)
    , m_league_end(0)
    , m_league_rank(0)
    , m_chunk_size(0)
    , m_league_chunk_end(0)
    , m_invalid_thread(0)
    , m_team_alloc(0)
    {}


  bool valid_static() const
    { return m_league_rank < m_league_end ; }

  void next_static()
    {
      if ( m_league_rank < m_league_end ) {
        // Make sure all stores are complete before entering the barrier
        memory_fence();
        team_barrier();
        set_team_shared();
      }
      m_league_rank++;
    }

  bool valid_dynamic() {

    if(m_invalid_thread)
      return false;
    if ((m_league_rank < m_league_chunk_end) && (m_league_rank < m_league_size)) {
      return true;
    }

    if (  m_team_rank_rev == 0 ) {
      m_team_base[0]->get_work_index(m_team_alloc);
    }
    team_barrier();

    long work_index = m_team_base[0]->team_work_index();

    m_league_rank = work_index * m_chunk_size;
    m_league_chunk_end = (work_index +1 ) * m_chunk_size;

    if(m_league_chunk_end > m_league_size) m_league_chunk_end = m_league_size;

    if((m_league_rank>=0) && (m_league_rank < m_league_chunk_end))
      return true;
    return false;
  }

  void next_dynamic() {
    if(m_invalid_thread)
      return;

    if ( m_league_rank < m_league_chunk_end ) {
      // Make sure all stores are complete before entering the barrier
      memory_fence();
      team_barrier();
      set_team_shared();
    }
    m_league_rank++;
  }

  void set_league_shmem( const int arg_league_rank
                       , const int arg_league_size
                       , const int arg_shmem_size
                       )
    {
      m_league_rank = arg_league_rank ;
      m_league_size = arg_league_size ;
      m_team_shared_size = arg_shmem_size ;
      set_team_shared();
    }
};

} /* namespace Impl */
} /* namespace Kokkos */

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

namespace Kokkos {
namespace Impl {
template< class ... Properties >
class TeamPolicyInternal< Kokkos::Experimental::CilkPlus , Properties ... >: public PolicyTraits<Properties ...>
{
private:

  int m_league_size ;
  int m_team_size ;
  int m_team_alloc ;
  int m_team_iter ;

  size_t m_team_scratch_size[2];
  size_t m_thread_scratch_size[2];

  int m_chunk_size;

  inline
  void init( const int league_size_request
           , const int team_size_request )
   {
      const int pool_size  = traits::execution_space::thread_pool_size(0);
      const int max_host_team_size =  Impl::HostThreadTeamData::max_team_members;
      const int team_max   = pool_size<max_host_team_size?pool_size:max_host_team_size;
      const int team_grain = pool_size / Kokkos::Experimental::EmuReplicatedSpace::memory_zones();

      m_league_size = league_size_request;

      m_team_size = team_size_request < team_max ?
                    team_size_request : team_max ;

      // Round team size up to a multiple of 'team_gain'
      const int team_size_grain = team_grain * ( ( m_team_size + team_grain - 1 ) / team_grain );
      const int team_count      = pool_size / team_size_grain ;

      // Constraint : pool_size = m_team_alloc * team_count
      m_team_alloc = pool_size / team_count ;

      // Maxumum number of iterations each team will take:
      m_team_iter  = ( m_league_size + team_count - 1 ) / team_count ;

      set_auto_chunk_size();
   }


public:

  //! Tag this class as a kokkos execution policy
  //! Tag this class as a kokkos execution policy
  typedef TeamPolicyInternal      execution_policy ;

  typedef PolicyTraits<Properties ... > traits;

  TeamPolicyInternal& operator = (const TeamPolicyInternal& p) {
    m_league_size = p.m_league_size;
    m_team_size = p.m_team_size;
    m_team_alloc = p.m_team_alloc;
    m_team_iter = p.m_team_iter;
    m_team_scratch_size[0] = p.m_team_scratch_size[0];
    m_thread_scratch_size[0] = p.m_thread_scratch_size[0];
    m_team_scratch_size[1] = p.m_team_scratch_size[1];
    m_thread_scratch_size[1] = p.m_thread_scratch_size[1];
    m_chunk_size = p.m_chunk_size;
    return *this;
  }

  template<class ExecSpace, class ... OtherProperties >
  friend class TeamPolicyInternal;

  template< class ... OtherProperties >
  TeamPolicyInternal(const TeamPolicyInternal<Kokkos::Threads,OtherProperties...>& p) {
    m_league_size = p.m_league_size;
    m_team_size = p.m_team_size;
    m_team_alloc = p.m_team_alloc;
    m_team_iter = p.m_team_iter;
    m_team_scratch_size[0] = p.m_team_scratch_size[0];
    m_thread_scratch_size[0] = p.m_thread_scratch_size[0];
    m_team_scratch_size[1] = p.m_team_scratch_size[1];
    m_thread_scratch_size[1] = p.m_thread_scratch_size[1];
    m_chunk_size = p.m_chunk_size;
  }

  //----------------------------------------

#ifdef KOKKOS_ENABLE_DEPRECATED_CODE
  template< class FunctorType >
  inline static
  int team_size_max( const FunctorType & ) {
     return traits::execution_space::thread_pool_size(0) / 
		     Kokkos::Experimental::EmuReplicatedSpace::memory_zones();
  }

  template< class FunctorType >
  inline static
  int team_size_recommended( const FunctorType & )
  {
     return traits::execution_space::thread_pool_size(0) / 
		     Kokkos::Experimental::EmuReplicatedSpace::memory_zones();
  }

  template< class FunctorType >
  inline static
  int team_size_recommended( const FunctorType &, const int& )
  {
     return traits::execution_space::thread_pool_size(0) / 
		     Kokkos::Experimental::EmuReplicatedSpace::memory_zones();
  }
#endif

  template<class FunctorType>
  int team_size_max( const FunctorType&, const ParallelForTag& ) const {
     return traits::execution_space::thread_pool_size(0) / 
		     Kokkos::Experimental::EmuReplicatedSpace::memory_zones();
  }
  template<class FunctorType>
  int team_size_max( const FunctorType&, const ParallelReduceTag& ) const {
    return traits::execution_space::thread_pool_size(0) / 
		     Kokkos::Experimental::EmuReplicatedSpace::memory_zones();
  }
  template<class FunctorType>
  int team_size_recommended( const FunctorType&, const ParallelForTag& ) const {
     return traits::execution_space::thread_pool_size(0) / 
		     Kokkos::Experimental::EmuReplicatedSpace::memory_zones();
  }
  template<class FunctorType>
  int team_size_recommended( const FunctorType&, const ParallelReduceTag& ) const {
     return traits::execution_space::thread_pool_size(0) / 
		     Kokkos::Experimental::EmuReplicatedSpace::memory_zones();
  }


  inline static
  int vector_length_max()
    { return 1024; } // Use arbitrary large number, is meant as a vectorizable length

  inline static
  int scratch_size_max(int level)
    { return (level==0?
        1024*32: // Roughly L1 size
        20*1024*1024); // Limit to keep compatibility with CUDA
    }

  //----------------------------------------

  inline int team_size() const { return m_team_size ; }
  inline int team_alloc() const { return m_team_alloc ; }
  inline int league_size() const { return m_league_size ; }
  inline size_t scratch_size(const int& level, int team_size_ = -1 ) const {
    if(team_size_ < 0)
      team_size_ = m_team_size;
    return m_team_scratch_size[level] + team_size_*m_thread_scratch_size[level] ;
  }

  inline int team_iter() const { return m_team_iter ; }

  /** \brief  Specify league size, request team size */
  TeamPolicyInternal( const typename traits::execution_space &
            , int league_size_request
            , int team_size_request
            , int vector_length_request = 1 )
    : m_league_size(0)
    , m_team_size(0)
    , m_team_alloc(0)
    , m_team_scratch_size { 0 , 0 }
    , m_thread_scratch_size { 0 , 0 }
    , m_chunk_size(0)
    { init(league_size_request,team_size_request); (void) vector_length_request; }

  /** \brief  Specify league size, request team size */
  TeamPolicyInternal( const typename traits::execution_space &
            , int league_size_request
            , const Kokkos::AUTO_t & /* team_size_request */
            , int /* vector_length_request */ = 1 )
    : m_league_size(0)
    , m_team_size(0)
    , m_team_alloc(0)
    , m_team_scratch_size { 0 , 0 }
    , m_thread_scratch_size { 0 , 0 }
    , m_chunk_size(0)
    { init(league_size_request,traits::execution_space::thread_pool_size(0) / 
		                       Kokkos::Experimental::EmuReplicatedSpace::memory_zones() ); }

  TeamPolicyInternal( int league_size_request
            , int team_size_request
            , int /* vector_length_request */ = 1 )
    : m_league_size(0)
    , m_team_size(0)
    , m_team_alloc(0)
    , m_team_scratch_size { 0 , 0 }
    , m_thread_scratch_size { 0 , 0 }
    , m_chunk_size(0)
    { init(league_size_request,team_size_request); }

  TeamPolicyInternal( int league_size_request
            , const Kokkos::AUTO_t & /* team_size_request */
            , int /* vector_length_request */ = 1 )
    : m_league_size(0)
    , m_team_size(0)
    , m_team_alloc(0)
    , m_team_scratch_size { 0 , 0 }
    , m_thread_scratch_size { 0 , 0 }
    , m_chunk_size(0)
    { init(league_size_request, traits::execution_space::thread_pool_size(0) / 
		                        Kokkos::Experimental::EmuReplicatedSpace::memory_zones() ); }

  inline int chunk_size() const { return m_chunk_size ; }

#ifdef KOKKOS_ENABLE_DEPRECATED_CODE
  /** \brief set chunk_size to a discrete value*/
  inline TeamPolicyInternal set_chunk_size(typename traits::index_type chunk_size_) const {
    TeamPolicyInternal p = *this;
    p.m_chunk_size = chunk_size_;
    return p;
  }

  /** \brief set per team scratch size for a specific level of the scratch hierarchy */
  inline TeamPolicyInternal set_scratch_size(const int& level, const PerTeamValue& per_team) const {
    TeamPolicyInternal p = *this;
    p.m_team_scratch_size[level] = per_team.value;
    return p;
  }

  /** \brief set per thread scratch size for a specific level of the scratch hierarchy */
  inline TeamPolicyInternal set_scratch_size(const int& level, const PerThreadValue& per_thread) const {
    TeamPolicyInternal p = *this;
    p.m_thread_scratch_size[level] = per_thread.value;
    return p;
  }

  /** \brief set per thread and per team scratch size for a specific level of the scratch hierarchy */
  inline TeamPolicyInternal set_scratch_size(const int& level, const PerTeamValue& per_team, const PerThreadValue& per_thread) const {
    TeamPolicyInternal p = *this;
    p.m_team_scratch_size[level] = per_team.value;
    p.m_thread_scratch_size[level] = per_thread.value;
    return p;
  }
#else
  /** \brief set chunk_size to a discrete value*/
  inline TeamPolicyInternal& set_chunk_size(typename traits::index_type chunk_size_) {
    m_chunk_size = chunk_size_;
    return *this;
  }

  /** \brief set per team scratch size for a specific level of the scratch hierarchy */
  inline TeamPolicyInternal& set_scratch_size(const int& level, const PerTeamValue& per_team) {
    m_team_scratch_size[level] = per_team.value;
    return *this;
  }

  /** \brief set per thread scratch size for a specific level of the scratch hierarchy */
  inline TeamPolicyInternal& set_scratch_size(const int& level, const PerThreadValue& per_thread) {
    m_thread_scratch_size[level] = per_thread.value;
    return *this;
  }

  /** \brief set per thread and per team scratch size for a specific level of the scratch hierarchy */
  inline TeamPolicyInternal& set_scratch_size(const int& level, const PerTeamValue& per_team, const PerThreadValue& per_thread) {
    m_team_scratch_size[level] = per_team.value;
    m_thread_scratch_size[level] = per_thread.value;
    return *this;
  }
#endif

protected:
#ifdef KOKKOS_ENABLE_DEPRECATED_CODE
  /** \brief set chunk_size to a discrete value*/
  inline TeamPolicyInternal internal_set_chunk_size(typename traits::index_type chunk_size_) {
    m_chunk_size = chunk_size_;
    return *this;
  }

  /** \brief set per team scratch size for a specific level of the scratch hierarchy */
  inline TeamPolicyInternal internal_set_scratch_size(const int& level, const PerTeamValue& per_team) {
    m_team_scratch_size[level] = per_team.value;
    return *this;
  }

  /** \brief set per thread scratch size for a specific level of the scratch hierarchy */
  inline TeamPolicyInternal internal_set_scratch_size(const int& level, const PerThreadValue& per_thread) {
    m_thread_scratch_size[level] = per_thread.value;
    return *this;
  }

  /** \brief set per thread and per team scratch size for a specific level of the scratch hierarchy */
  inline TeamPolicyInternal internal_set_scratch_size(const int& level, const PerTeamValue& per_team, const PerThreadValue& per_thread) {
    m_team_scratch_size[level] = per_team.value;
    m_thread_scratch_size[level] = per_thread.value;
    return *this;
  }
#endif

private:
  /** \brief finalize chunk_size if it was set to AUTO*/
  inline void set_auto_chunk_size() {

    int concurrency = traits::execution_space::concurrency()/m_team_alloc;
    if( concurrency==0 ) concurrency=1;

    if(m_chunk_size > 0) {
      if(!Impl::is_integral_power_of_two( m_chunk_size ))
        Kokkos::abort("TeamPolicy blocking granularity must be power of two" );
    }

    int new_chunk_size = 1;
    while(new_chunk_size*100*concurrency < m_league_size)
      new_chunk_size *= 2;
    if(new_chunk_size < 128) {
      new_chunk_size = 1;
      while( (new_chunk_size*40*concurrency < m_league_size ) && (new_chunk_size<128) )
        new_chunk_size*=2;
    }
    m_chunk_size = new_chunk_size;
  }

public:

  typedef Impl::CilkPlusExecTeamMember member_type ;

  friend class Impl::CilkPlusExecTeamMember ;
};

} /*namespace Impl */
} /* namespace Kokkos */


namespace Kokkos {

template< typename iType >
KOKKOS_INLINE_FUNCTION
Impl::TeamThreadRangeBoundariesStruct< iType, Impl::CilkPlusExecTeamMember >
TeamThreadRange( const Impl::CilkPlusExecTeamMember& thread, const iType& count )
{
  return Impl::TeamThreadRangeBoundariesStruct< iType, Impl::CilkPlusExecTeamMember >( thread, count );
}

template< typename iType1, typename iType2 >
KOKKOS_INLINE_FUNCTION
Impl::TeamThreadRangeBoundariesStruct< typename std::common_type< iType1, iType2 >::type,
                                       Impl::CilkPlusExecTeamMember>
TeamThreadRange( const Impl::CilkPlusExecTeamMember& thread, const iType1 & begin, const iType2 & end )
{
  typedef typename std::common_type< iType1, iType2 >::type iType;
  return Impl::TeamThreadRangeBoundariesStruct< iType, Impl::CilkPlusExecTeamMember >( thread, iType(begin), iType(end) );
}


template<typename iType>
KOKKOS_INLINE_FUNCTION
Impl::ThreadVectorRangeBoundariesStruct<iType,Impl::CilkPlusExecTeamMember >
  ThreadVectorRange(const Impl::CilkPlusExecTeamMember& thread, const iType& count) {
  return Impl::ThreadVectorRangeBoundariesStruct<iType,Impl::CilkPlusExecTeamMember >(thread,count);
}

template<typename iType>
KOKKOS_INLINE_FUNCTION
Impl::ThreadVectorRangeBoundariesStruct<iType,Impl::CilkPlusExecTeamMember >
  ThreadVectorRange(const Impl::CilkPlusExecTeamMember& thread, const iType& arg_begin, const iType& arg_end) {
  return Impl::ThreadVectorRangeBoundariesStruct<iType,Impl::CilkPlusExecTeamMember >(thread,arg_begin,arg_end);
}


KOKKOS_INLINE_FUNCTION
Impl::ThreadSingleStruct<Impl::CilkPlusExecTeamMember> PerTeam(const Impl::CilkPlusExecTeamMember& thread) {
  return Impl::ThreadSingleStruct<Impl::CilkPlusExecTeamMember>(thread);
}

KOKKOS_INLINE_FUNCTION
Impl::VectorSingleStruct<Impl::CilkPlusExecTeamMember> PerThread(const Impl::CilkPlusExecTeamMember& thread) {
  return Impl::VectorSingleStruct<Impl::CilkPlusExecTeamMember>(thread);
}
} // namespace Kokkos

namespace Kokkos {

  /** \brief  Inter-thread parallel_for. Executes lambda(iType i) for each i=0..N-1.
   *
   * The range i=0..N-1 is mapped to all threads of the the calling thread team.
   * This functionality requires C++11 support.*/
template<typename iType, class Lambda>
KOKKOS_INLINE_FUNCTION
void parallel_for(const Impl::TeamThreadRangeBoundariesStruct<iType,Impl::CilkPlusExecTeamMember>& loop_boundaries, const Lambda& lambda) {
  for( iType i = loop_boundaries.start; i < loop_boundaries.end; i+=loop_boundaries.increment)
    lambda(i);
}

/** \brief  Inter-thread vector parallel_reduce. Executes lambda(iType i, ValueType & val) for each i=0..N-1.
 *
 * The range i=0..N-1 is mapped to all threads of the the calling thread team and a summation of
 * val is performed and put into result. This functionality requires C++11 support.*/
template< typename iType, class Lambda, typename ValueType >
KOKKOS_INLINE_FUNCTION
typename std::enable_if< !Kokkos::is_reducer< ValueType >::value >::type
parallel_reduce(const Impl::TeamThreadRangeBoundariesStruct<iType,Impl::CilkPlusExecTeamMember>& loop_boundaries,
                     const Lambda & lambda, ValueType& result) {

  result = ValueType();

  for( iType i = loop_boundaries.start; i < loop_boundaries.end; i+=loop_boundaries.increment) {
    ValueType tmp = ValueType();
    lambda(i,tmp);
    result+=tmp;
  }

  result = loop_boundaries.thread.team_reduce(result,Impl::JoinAdd<ValueType>());
}

template< typename iType, class Lambda, typename ReducerType >
KOKKOS_INLINE_FUNCTION
typename std::enable_if< Kokkos::is_reducer< ReducerType >::value >::type
parallel_reduce(const Impl::TeamThreadRangeBoundariesStruct<iType,Impl::CilkPlusExecTeamMember>& loop_boundaries,
                     const Lambda & lambda, const ReducerType& reducer) {

  reducer.init(reducer.reference());

  for( iType i = loop_boundaries.start; i < loop_boundaries.end; i+=loop_boundaries.increment) {
    lambda(i,reducer.reference());
  }

  loop_boundaries.thread.team_reduce(reducer);
}

/** \brief  Intra-thread vector parallel_reduce. Executes lambda(iType i, ValueType & val) for each i=0..N-1.
 *
 * The range i=0..N-1 is mapped to all vector lanes of the the calling thread and a reduction of
 * val is performed using JoinType(ValueType& val, const ValueType& update) and put into init_result.
 * The input value of init_result is used as initializer for temporary variables of ValueType. Therefore
 * the input value should be the neutral element with respect to the join operation (e.g. '0 for +-' or
 * '1 for *'). This functionality requires C++11 support.*/
template< typename iType, class Lambda, typename ValueType, class JoinType >
KOKKOS_INLINE_FUNCTION
void parallel_reduce(const Impl::TeamThreadRangeBoundariesStruct<iType,Impl::CilkPlusExecTeamMember>& loop_boundaries,
                     const Lambda & lambda, const JoinType& join, ValueType& init_result) {

  ValueType result = init_result;

  for( iType i = loop_boundaries.start; i < loop_boundaries.end; i+=loop_boundaries.increment) {
    ValueType tmp = ValueType();
    lambda(i,tmp);
    join(result,tmp);
  }

  init_result = loop_boundaries.thread.team_reduce(result,Impl::JoinLambdaAdapter<ValueType,JoinType>(join));
}

} //namespace Kokkos


namespace Kokkos {
/** \brief  Intra-thread vector parallel_for. Executes lambda(iType i) for each i=0..N-1.
 *
 * The range i=0..N-1 is mapped to all vector lanes of the the calling thread.
 * This functionality requires C++11 support.*/
template<typename iType, class Lambda>
KOKKOS_INLINE_FUNCTION
void parallel_for(const Impl::ThreadVectorRangeBoundariesStruct<iType,Impl::CilkPlusExecTeamMember >&
    loop_boundaries, const Lambda& lambda) {
  #ifdef KOKKOS_ENABLE_PRAGMA_IVDEP
  #pragma ivdep
  #endif
  for( iType i = loop_boundaries.start; i < loop_boundaries.end; i+=loop_boundaries.increment)
    lambda(i);
}

/** \brief  Intra-thread vector parallel_reduce. Executes lambda(iType i, ValueType & val) for each i=0..N-1.
 *
 * The range i=0..N-1 is mapped to all vector lanes of the the calling thread and a summation of
 * val is performed and put into result. This functionality requires C++11 support.*/
template< typename iType, class Lambda, typename ValueType >
KOKKOS_INLINE_FUNCTION
typename std::enable_if< !Kokkos::is_reducer< ValueType >::value >::type
parallel_reduce(const Impl::ThreadVectorRangeBoundariesStruct<iType,Impl::CilkPlusExecTeamMember >&
      loop_boundaries, const Lambda & lambda, ValueType& result) {
  result = ValueType();
  for( iType i = loop_boundaries.start; i < loop_boundaries.end; i+=loop_boundaries.increment) {
    lambda(i,result);
  }
}

template< typename iType, class Lambda, typename ReducerType >
KOKKOS_INLINE_FUNCTION
typename std::enable_if< Kokkos::is_reducer< ReducerType >::value >::type
parallel_reduce(const Impl::ThreadVectorRangeBoundariesStruct<iType,Impl::CilkPlusExecTeamMember >&
      loop_boundaries, const Lambda & lambda, const ReducerType& reducer) {
  reducer.init(reducer.reference());
  for( iType i = loop_boundaries.start; i < loop_boundaries.end; i+=loop_boundaries.increment) {
    lambda(i,reducer.reference());
  }
}

/** \brief  Intra-thread vector parallel_reduce. Executes lambda(iType i, ValueType & val) for each i=0..N-1.
 *
 * The range i=0..N-1 is mapped to all vector lanes of the the calling thread and a reduction of
 * val is performed using JoinType(ValueType& val, const ValueType& update) and put into init_result.
 * The input value of init_result is used as initializer for temporary variables of ValueType. Therefore
 * the input value should be the neutral element with respect to the join operation (e.g. '0 for +-' or
 * '1 for *'). This functionality requires C++11 support.*/
template< typename iType, class Lambda, typename ValueType, class JoinType >
KOKKOS_INLINE_FUNCTION
void parallel_reduce(const Impl::ThreadVectorRangeBoundariesStruct<iType,Impl::CilkPlusExecTeamMember >&
      loop_boundaries, const Lambda & lambda, const JoinType& join, ValueType& result ) {

#ifdef KOKKOS_ENABLE_PRAGMA_IVDEP
#pragma ivdep
#endif
  for( iType i = loop_boundaries.start; i < loop_boundaries.end; i+=loop_boundaries.increment) {
    lambda(i,result);
  }
}

/** \brief  Intra-thread vector parallel exclusive prefix sum. Executes lambda(iType i, ValueType & val, bool final)
 *          for each i=0..N-1.
 *
 * The range i=0..N-1 is mapped to all vector lanes in the thread and a scan operation is performed.
 * Depending on the target execution space the operator might be called twice: once with final=false
 * and once with final=true. When final==true val contains the prefix sum value. The contribution of this
 * "i" needs to be added to val no matter whether final==true or not. In a serial execution
 * (i.e. team_size==1) the operator is only called once with final==true. Scan_val will be set
 * to the final sum value over all vector lanes.
 * This functionality requires C++11 support.*/
template< typename iType, class FunctorType >
KOKKOS_INLINE_FUNCTION
void parallel_scan(const Impl::ThreadVectorRangeBoundariesStruct<iType,Impl::CilkPlusExecTeamMember >&
      loop_boundaries, const FunctorType & lambda) {

  typedef Kokkos::Impl::FunctorValueTraits< FunctorType , void > ValueTraits ;
  typedef typename ValueTraits::value_type value_type ;

  value_type scan_val = value_type();

#ifdef KOKKOS_ENABLE_PRAGMA_IVDEP
#pragma ivdep
#endif
  for( iType i = loop_boundaries.start; i < loop_boundaries.end; i+=loop_boundaries.increment) {
    lambda(i,scan_val,true);
  }
}

} // namespace Kokkos

namespace Kokkos {

template<class FunctorType>
KOKKOS_INLINE_FUNCTION
void single(const Impl::VectorSingleStruct<Impl::CilkPlusExecTeamMember>& single_struct, const FunctorType& lambda) {
  lambda();
}

template<class FunctorType>
KOKKOS_INLINE_FUNCTION
void single(const Impl::ThreadSingleStruct<Impl::CilkPlusExecTeamMember>& single_struct, const FunctorType& lambda) {
  if(single_struct.team_member.team_rank()==0) lambda();
}

template<class FunctorType, class ValueType>
KOKKOS_INLINE_FUNCTION
void single(const Impl::VectorSingleStruct<Impl::CilkPlusExecTeamMember>& single_struct, const FunctorType& lambda, ValueType& val) {
  lambda(val);
}

template<class FunctorType, class ValueType>
KOKKOS_INLINE_FUNCTION
void single(const Impl::ThreadSingleStruct<Impl::CilkPlusExecTeamMember>& single_struct, const FunctorType& lambda, ValueType& val) {
  if(single_struct.team_member.team_rank()==0) {
    lambda(val);
  }
  single_struct.team_member.team_broadcast(val,0);
}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#endif
#endif /* #define KOKKOS_CILKPLUSTEAM_HPP */

