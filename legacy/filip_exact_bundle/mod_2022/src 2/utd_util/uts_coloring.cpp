#ifndef _uts_coloring_
#define _uts_coloring_

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/property_map/shared_array_property_map.hpp>
#include <boost/graph/smallest_last_ordering.hpp>
#include "mmh_intf.h"
#include "uth_log.h"


int utr_generate_mesh_coloring( // return number of colors
        const int Mesh_id, // IN: mesh id to color
        int *Elem_colors)  // OUT: array of size number of active elems containing colors (numbers)
{

    mf_check_mem(Elem_colors);

    // Constructing and filling the graph.
    typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS> Graph;
    const int nElems = mmr_get_nr_elem(Mesh_id);
    Graph g(nElems);

    int nel=0;
    while( (nel=mmr_get_next_act_elem(Mesh_id,nel)) != 0) {
        int neigs[MMC_MAXELFAC+1]={0};
        mmr_el_eq_neig(Mesh_id,nel,neigs,NULL);
        for(int i=1; i < neigs[0]; ++i) {
            add_edge(nel,neigs[i],g);
        }
    }

    std::fill(Elem_colors,Elem_colors+nElems,0);
    boost::smallest_last_vertex_ordering(g,Elem_colors);

    std::vector<int> color_counts;
    int cur_color=-1;
    do {
        ++cur_color;
        color_counts.push_back( std::count(Elem_colors,Elem_colors+nElems,cur_color) );
    }
    while(color_counts[cur_color] > 0);

    mf_check_mem(Elem_colors);

    return cur_color;
}

#endif // uts_coloring_
