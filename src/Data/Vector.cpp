#include "Data/Vector.h"


Data::Vector::Vector()
{
	_v[0]=0.0f; _v[1]=0.0f; _v[2]=0.0f;
}

Data::Vector::Vector(value_type x,value_type y,value_type z)
{ 
	_v[0]=x; _v[1]=y; _v[2]=z; 
}