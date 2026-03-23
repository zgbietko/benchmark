#ifndef _HOBJ_H_
#define _HOBJ_H_

#include <iostream>

#include "EntityAttributes.hpp"

/** \addtogroup MMM_HYBRID Hybrid Mesh Module
 *  \ingroup MM
 *  @{
 */


class hHybridMesh;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Vertical vtables (credit: Ed Smith)
// Before change to this, answer yourself:
// what this will change?
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class hBase {
 typedef void (*FP)();
 typedef int (*FPGet)(const hBase&);
 typedef void (*FPSet)(hBase&,int);
 static const uint8_t totalClasses=1;
 static const uint8_t totalMethods=2;
 static FP vtbl[totalMethods][totalClasses];
 uint8_t class_tag;
 public:
 int get() const {
    return ((FPGet)(vtbl[0][class_tag]))(*this);
 }
 void set(int x) {
    ((FPSet)vtbl[1][class_tag])(*this,x);
 }
};

class hEdge : public hBase {

};

class hFace : public hBase {

};

class hElem : public hBase {

};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class hObj
{
public:
  //////////////////// STATICs  ////////////////////
  static const int nTypes = 8;
	static const EntityAttributes eTable[nTypes];

	static hHybridMesh * myMesh;    /// Parent mesh
	static ID			makeId(const uTind posId,const uTind typeId) { return ID(posId + (typeId<<29)); }
	static uTind		posFromId(const ID id) {return ((id<<3)>>3); }
	static uTind		typeFromId(const ID id)  { return (id>>29); }
	static ID			maxId() { return std::numeric_limits<ID>::max(); }
	static ID			maxPos() { return  posFromId(maxId());  }
	static ID			maxType(){ return  typeFromId(maxId()); }

	static const int8_t delMark=-4,
		derefMark = -3,
		nothingMark=0,
		partialRefMark = -1,
		fullRefMark = -2;

	////////////////// FUCNTIONS //////////////////
protected:	hObj(const uTind posID,const EntityAttributes & typeSpecyfic);
public:	~hObj();
	
	bool		operator==(const hObj & other) const { return id_ == other.id_; }
	operator const EntityAttributes() const;
	
	hObj*		next()			{ return reinterpret_cast<hObj*>((reinterpret_cast<BYTE*>(this)+this->mySize_)); }
	const hObj* next()			const { return reinterpret_cast<const hObj*>((reinterpret_cast<const BYTE*>(this)+this->mySize_)); }
	int			nCurrentSons()  const { return nMyClassSons_ + nOtherSons_; }

	bool		isBroken()		const { return nMyClassSons_ > 0;}
	bool		isMarkedBreak() const { return nMyClassSons_ == fullRefMark;}
	bool		isMarkedDref()  const { return nMyClassSons_ == derefMark;}
	bool		isSliceBroken() const { return (isBroken() && nMyClassSons_ < 8); }
	bool		isNull()		const { return (mySize_==0); }

	void		mark2Ref(const int refType=8);
	void		mark2Dref();
	void		mark2Del();
	int			hBreak(const Tind sonsCount=8);
	void		derefine();
	bool		test()			const;
	double* geoCenter(double center[3]) const;
	bool	equals(const hObj& other,bool compareBreak=false) const ;

	//hObj*		getMyClassChild(const int number=0);
	//const hObj* getMyClassChild(const int number=0) const;
	template < class T >
	T*			getMyClassChild(const int number)
	{
	    assert( static_cast<int>(nMyClassSons_) > number);
	    T* child(reinterpret_cast<T*>(next()));
    	    assert(child->type_ == T::myType);
    	    for(int i(number-1);i>=0;--i) {
        	do {
            	    child = reinterpret_cast<T*>(child->next());
            	    assert(child->type_ == T::myType);
        	} while (child->level_!= this->level_+1);
    	    }
    	    assert(child->parent_ == this->id_);
    	    assert(child->level_ == this->level_+1);
    	    return child;
	}

	template < class T >
	const T*   getMyClassChild(const int number) const
	{
	    assert( static_cast<int>(nMyClassSons_) > number);
	    const T* child(reinterpret_cast<const T*>(next()));
    	    assert(child->type_ == T::myType);
    	    
    	    for(int i(number-1);i>=0;--i) {
        	do {
            	    child = reinterpret_cast<const T*>(child->next());
            	    assert(child->type_ == T::myType);
        	} while (child->level_!= this->level_+1);
    	    }
    	    assert(child->parent_ == this->id_);
    	    assert(child->level_ == this->level_+1);
    	    return child;
	}

	bool	isAtBoundary() const
	{
		if(type_ == 0)
		{
			return (nOtherSons_==1);
		}
		else throw "setAtBoundary: This function works only for vertex!";
	}
	void	setAtBoundary(const bool atBoundary)
	{
		if(type_== 0)
		{
			nOtherSons_ = ( atBoundary ? 1 : 0 );
		}
		else throw "setAtBoundary: This function works only for vertex!";
	}
	bool	operator< (const hObj & other) const
	{
		register uTind i(0);
		while((components(i) == other.components(i)) && (i<typeSpecyfic_.nComponents_))
			++i;

		return (components(i) < other.components(i));
	}
	
	friend std::ostream& operator<<(std::ostream& os,const hObj & obj);

    void print() const;
	
	static bool	comparePtrs(const hObj * me, const hObj * other)
	{
		return me->operator<(*other);
	}

	void	updatePointers()
	{
		nodes_ = reinterpret_cast<ID*>( this+1 );
	}

//HIDE		  ID&	verts(const uTind i)		{ assert(i>=0 && i < typeSpecyfic_.nVerts_); return nodes_[i];}
	const ID&	verts(const uTind i) const	{ assert(i < typeSpecyfic_.nVerts_); return nodes_[i];}
//HIDE		  ID*	verts()						{ assert( typeSpecyfic_.nVerts_ > 0); return nodes_;}
	const ID*	verts() const				{ assert( typeSpecyfic_.nVerts_ > 0); return nodes_;}
	void		swapVerts(const int v1,const int v2) ;
//HIDE	void		setVerts(const uTind* newVerts){ assert(newVerts != NULL); memcpy(nodes_,newVerts,sizeof(uTind)*typeSpecyfic_.nVerts_); updateHash();  }
		  ID&	components(const uTind i)	{ assert(i < typeSpecyfic_.nComponents_); return nodes_[typeSpecyfic_.compOffset_+i];}
	const ID&	components(const uTind i)const	{ assert(i < typeSpecyfic_.nComponents_); return nodes_[typeSpecyfic_.compOffset_+i];}
		  ID*	components()				{ assert( typeSpecyfic_.nComponents_ > 0); return nodes_+typeSpecyfic_.compOffset_;}
	const ID*	components() const			{ assert( typeSpecyfic_.nComponents_ > 0); return nodes_+typeSpecyfic_.compOffset_;}
		  int&	flags(const uTind i)		{ assert(i < typeSpecyfic_.nFlags_); return reinterpret_cast<int*>(nodes_+typeSpecyfic_.flagOffset_)[i];}
	const int&	flags(const uTind i) const	{ assert(i < typeSpecyfic_.nFlags_); return reinterpret_cast<const int*>(nodes_+typeSpecyfic_.flagOffset_)[i];}
		  int*  flags()						{ assert( typeSpecyfic_.nFlags_ > 0); return reinterpret_cast<int*>(nodes_+typeSpecyfic_.flagOffset_);}
	const int*  flags() const				{ assert( typeSpecyfic_.nFlags_ > 0); return reinterpret_cast<const int*>(nodes_+typeSpecyfic_.flagOffset_);}
	ID&	neighs(const uTind i)				{ assert(i < typeSpecyfic_.nNeighs_); return nodes_[typeSpecyfic_.neighOffset_+i]; }
	const ID&	neighs(const uTind i) const { assert(i < typeSpecyfic_.nNeighs_); return nodes_[typeSpecyfic_.neighOffset_+i]; }
		  ID*	neighs()					{ assert( typeSpecyfic_.nNeighs_ > 0); return nodes_+typeSpecyfic_.neighOffset_;}
	const ID*	neighs() const				{ assert( typeSpecyfic_.nNeighs_ > 0); return nodes_+typeSpecyfic_.neighOffset_;}
		  ID&	sons(const uTind i)			{ assert(i < typeSpecyfic_.nSons_); return nodes_[typeSpecyfic_.sonsOffset_+i]; }
	const ID&	sons(const uTind i) const	{ assert(i < typeSpecyfic_.nSons_); return nodes_[typeSpecyfic_.sonsOffset_+i]; }
		  ID*	sons()						{ assert( typeSpecyfic_.nSons_ > 0); return nodes_+typeSpecyfic_.sonsOffset_;}
	const ID*	sons() const				{ assert( typeSpecyfic_.nSons_ > 0); return nodes_+typeSpecyfic_.sonsOffset_;}
//	int			status()const				{ return status_; }
//	void		status(const int newStatus) { status_=static_cast<int8_t>(newStatus); }

    void    incRef() { ++nRefs; /*mfp_debug("inc(%d) %d|%d",nRefs,type_,pos_);*/ }
    void    decRef() {
        --nRefs;
//        mfp_debug("dec(%d) %d|%d",nRefs,type_,pos_);
        if(nRefs < 1) {
            mark2Del();
        }
    }

	////////////////// FIELDS ////////////////////////
	union {
		ID	id_;
		struct{ ID pos_:29,type_:3;};
	};
	ID		parent_;
	uint8_t  	mySize_;
	int8_t		level_,
			nMyClassSons_,    // when hObj is unrefined keeps mark to refine
            nOtherSons_,      // when hObj is mark to refine stores number of myclass children needed
            nRefs;
    //	  status_;
#ifdef TURBULENTFLOW
	double dist2bound_;
	ID nearstFace_;
	double planeCoords_[4];
#endif
	//	Field<nCoords,Tval>			coords_;
	EntityAttributes const & typeSpecyfic_;
protected:
	void	updateHash() const;

	ID	* nodes_;	// this will points array with verts, components etc. in

private:	// forbidden operations
	hObj(const hObj & other);
	hObj & operator=(const hObj & other);
};

std::ostream& operator<<(std::ostream & os, const hObj & obj);

typedef hObj Face;
typedef hObj Elem;

/**  @} */
#endif // _HOBJ_H_
