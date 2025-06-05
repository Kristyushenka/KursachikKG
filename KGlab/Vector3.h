//Гаврилов А.Г.
//27.12.2024
#ifndef  MYVECTOR3D_H
#define  MYVECTOR3D_H

//а кто найдет этот фйлик тот молодец.
//тут как раз описывается тип вектор, которые можно сказдывать, умножать
// искать... ВЕКТОРНОЕ произведение итд)

#include	<cmath>
#include	<cstring>
#include	<initializer_list>
#include	<utility>

//Обычковенный 3хкомпонентный вектор
class Vector3
{
	double* coords;

public:

	double x() const
	{
		return coords[0];
	}
	double y() const
	{
		return coords[1];
	}
	double z() const
	{
		return coords[2];
	}

	//==========обычные конструкторы==============

	Vector3()
	{
		coords = new double[3];
	}

	Vector3(double x, double y, double z) : Vector3()
	{
		coords[0] = x;
		coords[1] = y;
		coords[2] = z;
	}


	Vector3(std::initializer_list<double> &list)
	{
		coords[0] = *(list.begin());
		coords[1] = *(list.begin()+1);
		coords[2] = *(list.begin()+2);
	}
	//==========конструктор копий==============
	Vector3(const Vector3& vec)
	{
		std::memcpy(coords, vec.coords, 3 * sizeof(double));
	}

	//==========конструктор перемещения========
	Vector3(Vector3&& vec) noexcept
	{
		coords = vec.coords;
		vec.coords = nullptr;
	}
	//==========смерть пришла.....========
	~Vector3()
	{
		delete[] coords;
	}

	//==============математика=====================

	void setCoords(double x, double y, double z)
	{
		coords[0] = x;
		coords[1] = y;
		coords[2] = z;
	}

	Vector3 operator + (const Vector3& vec) const
	{
		Vector3 newV;
		newV.coords[0] = coords[0] + vec.coords[0];
		newV.coords[1] = coords[1] + vec.coords[1];
		newV.coords[2] = coords[2] + vec.coords[2];
		return newV;
	}

	Vector3 operator - (const Vector3& vec) const
	{
		Vector3 newV;
		newV.coords[0] = coords[0] - vec.coords[0];
		newV.coords[1] = coords[1] - vec.coords[1];
		newV.coords[2] = coords[2] - vec.coords[2];
		return newV;
	}

	Vector3 operator -() const
	{
		return {-coords[0],-coords[1] ,-coords[2]};
	}
	Vector3 operator +() const
	{
		return  *this;
	}

	template<typename T>
	Vector3 operator * (const T k) const
	{
		Vector3 newV;
		newV.coords[0] = coords[0] * k;
		newV.coords[1] = coords[1] * k;
		newV.coords[2] = coords[2] * k;
		return newV;
	}

	template<typename T>
	Vector3 operator / (const T k) const
	{
		Vector3 newV;
		newV.coords[0] = coords[0] / k;
		newV.coords[1] = coords[1] / k;
		newV.coords[2] = coords[2] / k;
		return newV;
	}

	Vector3& operator=(const Vector3& vec)
	{
		std::memcpy(coords, vec.coords, 3 * sizeof(double));
		return *this;
	}

	Vector3& operator=(Vector3&& vec)
	{
		coords = vec.coords;
		vec.coords = nullptr;
		return *this;
	}

	double length() const
	{
		return (sqrt(coords[0] * coords[0] + coords[1] * coords[1] + coords[2] * coords[2]));
	}

	Vector3 normalize()
	{
		Vector3 newV;
		double l = length();
		newV.setCoords(coords[0] / l, coords[1] / l, coords[2] / l);
		return newV;
	}


	Vector3 operator^(const Vector3& v) const
	{
		Vector3 V;
		V.setCoords(coords[1] * v.coords[2] - coords[2] * v.coords[1],
			coords[2] * v.coords[0] - coords[0] * v.coords[2],
			coords[0] * v.coords[1] - coords[1] * v.coords[0]);
		return V;
	}

	double operator&(const Vector3& v) const
	{
		return coords[0]*v.coords[0]+ coords[1] * v.coords[1]+ coords[2] * v.coords[2];
	}

	const double* operator()() const
	{
		return (coords);
	}
	template<typename T>
	friend Vector3 operator*(const T arg, const Vector3& v);

	template<typename T>
	friend Vector3 operator/(const T arg, const Vector3& v);

	static Vector3 Z() { return { 0,0,1 }; }
	static Vector3 X() { return { 0,1,0 }; }
	static Vector3 Y() { return { 1,0,0 }; }

};


//перегрузка для стандартных типов
template<typename T>
Vector3 operator*(T arg, const Vector3& v)
{
	return Vector3(v.coords[0] * arg, v.coords[1] * arg, v.coords[2] * arg);
}

template<typename T>
Vector3 operator/(T arg, const Vector3& v)
{
	return Vector3(v.coords[0] / arg, v.coords[1] / arg, v.coords[2] / arg);
}

#endif