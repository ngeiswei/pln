#ifndef _EDA_REPLACE_H
#define _EDA_REPLACE_H

#include <LADSUtil/exceptions.h>
#include <LADSUtil/lazy_random_selector.h>

#include "MosesEda/eda/using.h"
#include "MosesEda/eda/field_set.h"
#include "MosesEda/eda/scoring.h"

namespace eda {

  //note that NewInst only models InputIterator

  struct replace_the_worst {
    template<typename NewInst,typename Dst>
    void operator()(NewInst from,NewInst to,Dst from_dst,Dst to_dst) const {
      LADSUtil::cassert(TRACE_INFO,
			distance(from,to)<=distance(from_dst,to_dst), 
			"Distance from -> to greater than distance from_dst -> to_dst.");
      nth_element(from_dst,from_dst+distance(from,to),to_dst);
      copy(from,to,from_dst);
    }
  };

  
  struct rtr_replacement {
    rtr_replacement(const field_set& fs,int ws, LADSUtil::RandGen& _rng) : window_size(ws),_fields(&fs),rng(_rng) { }

    template<typename NewInst,typename Dst>
    void operator()(NewInst from,NewInst to,Dst from_dst,Dst to_dst) const {
      LADSUtil::cassert(TRACE_INFO, window_size<=distance(from_dst,to_dst),
			"windows size greater than distance from_dst -> to_dst.");

      for (;from!=to;++from)
	operator()(*from,from_dst,to_dst);
    }

    template<typename Dst,typename ScoreT>
    void operator()(const scored_instance<ScoreT>& inst,
		    Dst from_dst,Dst to_dst) const {
      LADSUtil::lazy_random_selector select(distance(from_dst,to_dst), rng);
      Dst closest=from_dst+select();
      int closest_distance=_fields->hamming_distance(inst,*closest);
      for (int i=1;i<window_size;++i) {
	Dst candidate=from_dst+select();
	int distance=_fields->hamming_distance(inst,*candidate);
	if (distance<closest_distance) {
	  closest_distance=distance;
	  closest=candidate;
	}
      }
      if (*closest<inst)
	*closest=inst;
    }

    int window_size;
  protected:
    const field_set* _fields;
    LADSUtil::RandGen& rng;
  };
  
} //~namespace eda

#endif
