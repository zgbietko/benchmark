#ifndef _GRAPHIC_ELEM_H_
#define _GRAPHIC_ELEM_H_

#include "Point3D.h"
#include "Color.h"
#include "Fire.h"

#include <sstream>
#include <vector>
#include <string>

namespace FemViewer {

class Fire;


template <typename T> class GraphElement2 {
	struct Vertex {
		Point3D<T> vertex;
		ColorRGB   color;
		T	   	   value;
	};

    public:
		int type;
        std::vector< Point3D<T> > points;
        std::vector<T> 			  values;
        std::vector<ColorRGB> 	  colors;
		
        GraphElement2() : type(3), points(3), values(3), colors(3)
		{}

        GraphElement2(std::vector< Point3D<T> > p, std::vector<T> v)
        {
			type = p.size();
            points = p;
            values = v;
        }

        ~GraphElement2() {
        	points.clear();
        	values.clear();
        	colors.clear();
        }

		void SortVertex(Fire& fire)
		{
			if(values.size() == 3)
			{
				if(values[0] <= values[1] && values[1] <= values[2]) 
					return;
				
				T tabValues[3];
				tabValues[0] = values[0];
				tabValues[1] = values[1];
				tabValues[2] = values[2];
				
				const int *index;
				fire.GetSorted(tabValues,index);

				std::vector<Point3D<T> > vtmp( points);
				std::vector<T>			 vval( values);

				points[0] = vtmp[index[0]];
				points[1] = vtmp[index[1]]; 
				points[2] = vtmp[index[2]];

				values[0] = vval[index[0]];
				values[1] = vval[index[1]];
				values[2] = vval[index[2]];
			}

		};

		bool Check()
		{
			if(points.size() == 3)
			{
				if(points[0].Compare(points[1])
				|| points[1].Compare(points[2])
				|| points[2].Compare(points[0]))
				return true;
				else return false;

			}
			return false;
		}

		// Comper 2 points if equal
		bool Check2() {
			return points[0].Compare(points[1]) ? true : false;
		}



        void Add(const Point3D<T> & p, const double & d)
        {

            //faces.AddPoint(p);
            points.push_back(p);
            values.push_back(d);

        };


        bool IsOk(void)
        {
            if ( (points.size() == colors.size() ) && (points.size() == values.size()))
                return true;

            return false;
        }

        //! Sprawdza czy zgadzaj� si� rozmiary wszytskich sk�adowych (punkt�w, vartosci, kolorow)
        bool IsOkFVC(void)
        {
            if ( (points.size() == colors.size() ) && (points.size() == values.size()) )
                return true;

            return false;
        }

        void Clear(void)
        {

            points.clear();
            values.clear();
            colors.clear();

        };



        void GetValues(const unsigned int & i, T& v1, T&v2) const
        {

            if ( i == values.size()-1)
            {
                //points.size()-1, 0
                //return Segment3D<T>(points[points.size()-1], points[0]);
                v1 = values[values.size()-1];
                v2 = values[0];
            }
            else
            {
                v1 = values[i];
                v2 = values[i+1];
                //i, i+1
                //return Segment3D<T>(points[i], points[i+1]);
            }
        };

//        Segment3D<T> GetSegment(const unsigned int & i)
//        {
//            Point3D<T> p1, p2;
//            this->GetPoints(i, p1, p2);
//            return Segment3D<T>(p1, p2);
//        };

        void GetPoints(const unsigned int & i, Point3D<T>& p1, Point3D<T> &p2) const
        {

            if ( i == points.size()-1)
            {
                p1 = points[points.size()-1];
                p2 = points[0];
            }
            else
            {
                p1 = points[i];
                p2 = points[i+1];
            }


        };

        unsigned int Size(void) const
        {
            return static_cast<unsigned int>(points.size());
        };

        std::string ValueAsString(void) const
        {

            std::stringstream oss;

            for (size_t i = 0; i < values.size(); ++i)
            {
                oss << values[i] << ", ";
            }

            return oss.str();

        };

        std::string PointAsString(void) const
        {

            std::stringstream oss;

            for (size_t i = 0; i < values.size(); ++i)
            {
                oss << points[i].x << ", " << points[i].y << ", " << points[i].z << std::endl;
            }

            return oss.str();

        };

        std::string AsString(void) const
        {

            std::stringstream oss;

            for (size_t i = 0; i < values.size(); ++i)
            {
                /*oss << values[i] << "( " << points[i].AsString() << ")" << "(" << colors[i].AsString() << ")" << std::endl;*/
				oss << values[i] << "( " << points[i].AsString() << ")" << std::endl;
            }

            return oss.str();

        };

};

typedef GraphElement2<double> GEL2d;
typedef std::vector< GEL2d > vGEL2d;

} // end namespace FemViewer

#endif /* _GAPHIC_ELEM_H_
  */
