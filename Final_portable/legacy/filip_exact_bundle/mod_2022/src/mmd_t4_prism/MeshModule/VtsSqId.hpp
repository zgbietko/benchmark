#ifndef VTS_SQ_ID_HPP
#define VTS_SQ_ID_HPP

#include "../Common.h"

#include <algorithm>
#include <ostream>

/// Vertices Sequence Identyfier
/// assure that vertices are correcly ordered (sorted)
/** \addtogroup MMM_HYBRID Hybrid Mesh Module
 *  \ingroup MM
 *  @{
 */


#ifdef _MSC_VER

template<int TLength>
class VtsSqId;

template<>
class VtsSqId<0> {};

#endif

template<int TLength>
class VtsSqId
{
  uTind v[TLength];
public:
  static const int length=TLength;
  VtsSqId(const uTind vts[])
  {
	memcpy(v,vts,sizeof(uTind)*TLength);
	std::sort(v,v+TLength);
  }
  
  VtsSqId(const VtsSqId & other)
  {
	memcpy(v,other.v,sizeof(uTind)*TLength);
  }

  /// OPERATORS  
  VtsSqId& operator=(const VtsSqId & other)
  {
	memcpy(v,other.v, sizeof(uTind)*TLength);
	return *this;
  }

  bool operator==(const VtsSqId & other) const
  {
	bool eq(true);
	for(int i(0); (i < TLength) && eq; ++i) {
	  if(v[i] != other.v[i]){
		eq=false;
	  }
	}
	return eq;
  }

  bool operator!=(const VtsSqId & other) const
  {
	return !(*this==other);
  }

  bool operator<(const VtsSqId & other) const
  {
	register int i(0);
	while((v[i]==other.v[i]) && (i < TLength-1) ) {
	  ++i;
	}
	return v[i] < other.v[i];
  }

  bool operator>=(const VtsSqId & other) const
  {
	return ! ( this->operator<(other) );
  }

  bool operator>(const VtsSqId & other) const
  {
	register int i(0);
	while((v[i]==other.v[i]) && (i < TLength-1) ) {
	  ++i;
	}
	return v[i] > other.v[i];
  }
  
  bool operator<=(const VtsSqId & other) const
  {
	return ! ( this->operator>(other) );
  }

  
  
  template<int T>
  friend std::ostream &  operator<<(std::ostream & os,const VtsSqId<T> & id);
  
};
template<int T>
std::ostream &  operator<<(std::ostream & os,const VtsSqId<T> & id)
{
  for(int i(0); i < VtsSqId<T>::length; ++i) {
	os << id.v[i] << " ";
  }
  return os;
}

/**  @} */

#endif //guard header
