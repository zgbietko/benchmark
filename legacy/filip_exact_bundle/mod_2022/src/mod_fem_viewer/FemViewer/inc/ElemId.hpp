/*
 * ElemId.hpp
 *
 *  Created on: 24-09-2011
 *      Author: Paweł Macioł
 */

#ifndef ELEMID_HPP_
#define ELEMID_HPP_

#include"defs.h"
#include"fv_compiler.h"
#include<string.h>//memset

namespace FemViewer {

#define EL_ID_ATTRIB(Tid) struct{ Tid eid : 24, etype : 1, faces : 5, active : 1, bound : 1; }
#define FA_ID_ATTRIB(Tid) struct{ Tid fid : 31, ftype : 1; }

template<typename T>
struct BaseId {
	union {
		T id;
		union {
			EL_ID_ATTRIB(T);
			FA_ID_ATTRIB(T);
		};
	};
};

template<typename T>
class Id : public BaseId<T> {
public:
	Id(const T id_ = T(0)) { this->id = id_; }
	Id(const BaseId<T>& ref) { this->id = ref.id; }
	Id& operator=(const BaseId<T>& rhs) { this->id = rhs.id; return *this; }
	//virtual ~Id(){} // +8 bytes for vtable
	inline bool is_bit(T bit) const { return IS_BIT_SET(this->id,bit); }
	inline bool is_bit_not(T bit) const {return !is_bit(bit); }
	//inline virtual int type() const = 0;
};

template<typename T>
class ElemId : public Id<T> {
  public:
	ElemId(const T id_ = T(0)) : Id<T>(id_) {}
	ElemId(const BaseId<T>& ref) : Id<T>(ref) {}
	ElemId& operator=(const BaseId<T>& rhs) { this->id = rhs.id; return *this; }
	bool operator < (const Id<T>& rhs) const {
		return (this->eid < rhs.eid);
	}

	// 0 - tetrahedron
    // 1 - prism
	bool is_tetra() const { return this->is_bit(ELTYPE_BIT_POS(this->id)); }
	bool is_prism() const { return !this->is_tetra(); }
};

template<typename T>
struct FaceId : public Id<T> {
public:
	FaceId(const T id_ = T(0)) : Id<T>(id_) {}
	FaceId(const BaseId<T>& ref) : Id<T>(ref) {}
	FaceId& operator=(const BaseId<T>& rhs) { this->id = rhs.id; return *this; }
	bool operator < (const Id<T>& rhs) const {
		return (this->fid < rhs.fid);
	}
	// 0 - traingle face
	// 1 - quad face
	bool is_tetriangle() const { return this->is_bit(FACE_TYPE_POS(this->id)); }
	bool is_quad() 		 const { return !this->is_triangle(); }
};

#undef EL_ID_ATTRIB
#undef FA_ID_ATTRIB

template <typename T>
struct CompareIdElem {
	bool operator() (T* el1, T* el2) { return (ELEM_ID(el1->id) < ELEM_ID(el2->id)); }
};

template<typename T>
bool compare_func(T* it1, T* it2) { return (ELEM_ID(it1->el_id) < ELEM_ID(it2->el_id)); }

struct Elem_Info {
	ElemId<id_t> elemId;
	int nodes[7];

	Elem_Info() : elemId(0) { memset(nodes,0x0,sizeof(int)*7); }
	bool operator==(const id_t& rh) { return (elemId.eid == rh); };
};

}// end namespace

#endif /* ELEMID_HPP_ */
