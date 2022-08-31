// 

#pragma once
#ifndef FQMS_H
#define FQMS_H

/////////////////////////////////////////////
//
// Mesh Simplification Tutorial
//
// (C) by Sven Forstmann in 2014
//
// License : MIT
// http://opensource.org/licenses/MIT
//
//https://github.com/sp4cerat/Fast-Quadric-Mesh-Simplification
//
// 5/2016: Chris Rorden created minimal version for OSX/Linux/Windows compile


//#include <iostream>
//#include <stddef.h>
//#include <functional>
//#include <sys/stat.h>
//#include <stdbool.h>
#include <string.h>
//#include <ctype.h>
//#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <vector>
#include <string>
#include <math.h>
#include <float.h> //FLT_EPSILON, DBL_EPSILON

#include "lim/model.h"
#include <GLFW/glfw3.h>

#define loopi(start_l,end_l) for ( int i=start_l;i<end_l;++i )
#define loopi(start_l,end_l) for ( int i=start_l;i<end_l;++i )
#define loopj(start_l,end_l) for ( int j=start_l;j<end_l;++j )
#define loopk(start_l,end_l) for ( int k=start_l;k<end_l;++k )

namespace fqms
{

	struct vector3
	{
		double x, y, z;
	};

	struct vec3f
	{
		double x, y, z;

		inline vec3f(void) {}

		//inline vec3f operator =( vector3 a )
		// { vec3f b ; b.x = a.x; b.y = a.y; b.z = a.z; return b;}

		inline vec3f(vector3 a)
		{
			x = a.x; y = a.y; z = a.z;
		}

		inline vec3f(const double X, const double Y, const double Z)
		{
			x = X; y = Y; z = Z;
		}

		inline vec3f operator + (const vec3f& a) const
		{
			return vec3f(x + a.x, y + a.y, z + a.z);
		}

		inline vec3f operator += (const vec3f& a) const
		{
			return vec3f(x + a.x, y + a.y, z + a.z);
		}

		inline vec3f operator * (const double a) const
		{
			return vec3f(x * a, y * a, z * a);
		}

		inline vec3f operator * (const vec3f a) const
		{
			return vec3f(x * a.x, y * a.y, z * a.z);
		}

		inline vec3f v3 () const
		{
			return vec3f(x, y, z);
		}

		inline vec3f operator = (const vector3 a)
		{
			x=a.x; y=a.y; z=a.z; return *this;
		}

		inline vec3f operator = (const vec3f a)
		{
			x=a.x; y=a.y; z=a.z; return *this;
		}

		inline vec3f operator / (const vec3f a) const
		{
			return vec3f(x / a.x, y / a.y, z / a.z);
		}

		inline vec3f operator - (const vec3f& a) const
		{
			return vec3f(x - a.x, y - a.y, z - a.z);
		}

		inline vec3f operator / (const double a) const
		{
			return vec3f(x / a, y / a, z / a);
		}

		inline double dot(const vec3f& a) const
		{
			return a.x*x + a.y*y + a.z*z;
		}

		inline vec3f cross(const vec3f& a, const vec3f& b)
		{
			x = a.y * b.z - a.z * b.y;
			y = a.z * b.x - a.x * b.z;
			z = a.x * b.y - a.y * b.x;
			return *this;
		}

		inline double angle(const vec3f& v)
		{
			vec3f a = v, b = *this;
			double dot = v.x*x + v.y*y + v.z*z;
			double len = a.length() * b.length();
			if( len==0 )len=0.00001f;
			double input = dot  / len;
			if( input<-1 ) input=-1;
			if( input>1 ) input=1;
			return (double)acos (input);
		}

		inline double angle2(const vec3f& v, const vec3f& w)
		{
			vec3f a = v, b= *this;
			double dot = a.x*b.x + a.y*b.y + a.z*b.z;
			double len = a.length() * b.length();
			if( len==0 )len=1;

			vec3f plane; plane.cross(b, w);

			if( plane.x * a.x + plane.y * a.y + plane.z * a.z > 0 )
				return (double)-acos (dot  / len);

			return (double)acos (dot  / len);
		}

		inline vec3f rot_x(double a)
		{
			double yy = cos (a) * y + sin (a) * z;
			double zz = cos (a) * z - sin (a) * y;
			y = yy; z = zz;
			return *this;
		}
		inline vec3f rot_y(double a)
		{
			double xx = cos (-a) * x + sin (-a) * z;
			double zz = cos (-a) * z - sin (-a) * x;
			x = xx; z = zz;
			return *this;
		}
		inline void clamp(double min, double max)
		{
			if( x<min ) x=min;
			if( y<min ) y=min;
			if( z<min ) z=min;
			if( x>max ) x=max;
			if( y>max ) y=max;
			if( z>max ) z=max;
		}
		inline vec3f rot_z(double a)
		{
			double yy = cos (a) * y + sin (a) * x;
			double xx = cos (a) * x - sin (a) * y;
			y = yy; x = xx;
			return *this;
		}
		inline vec3f invert()
		{
			x=-x; y=-y; z=-z; return *this;
		}
		inline vec3f frac()
		{
			return vec3f(
				x-double(int(x)),
				y-double(int(y)),
				z-double(int(z))
			);
		}

		inline vec3f integer()
		{
			return vec3f(
				double(int(x)),
				double(int(y)),
				double(int(z))
			);
		}

		inline double length() const
		{
			return (double)sqrt(x*x + y*y + z*z);
		}

		inline vec3f normalize(double desired_length = 1)
		{
			double square = sqrt(x*x + y*y + z*z);
			/*
			if (square <= 0.00001f )
			{
				x=1;y=0;z=0;
				return *this;
			}*/
			//double len = desired_length / square;
			x/=square; y/=square; z/=square;

			return *this;
		}
		static vec3f normalize(vec3f a);

		static void random_init();
		static double random_double();
		static vec3f random();

		static int random_number;

		double random_double_01(double a)
		{
			double rnf=a*14.434252+a*364.2343+a*4213.45352+a*2341.43255+a*254341.43535+a*223454341.3523534245+23453.423412;
			int rni=((int)rnf)%100000;
			return double(rni)/(100000.0f-1.0f);
		}

		vec3f random01_fxyz()
		{
			x=(double)random_double_01(x);
			y=(double)random_double_01(y);
			z=(double)random_double_01(z);
			return *this;
		}

	};

	vec3f barycentric(const vec3f& p, const vec3f& a, const vec3f& b, const vec3f& c)
	{
		vec3f v0 = b-a;
		vec3f v1 = c-a;
		vec3f v2 = p-a;
		double d00 = v0.dot(v0);
		double d01 = v0.dot(v1);
		double d11 = v1.dot(v1);
		double d20 = v2.dot(v0);
		double d21 = v2.dot(v1);
		double denom = d00*d11-d01*d01;
		double v = (d11 * d20 - d01 * d21) / denom;
		double w = (d00 * d21 - d01 * d20) / denom;
		double u = 1.0 - v - w;
		return vec3f(u, v, w);
	}

	vec3f interpolate(const vec3f& p, const vec3f& a, const vec3f& b, const vec3f& c, const vec3f attrs[3])
	{
		vec3f bary = barycentric(p, a, b, c);
		vec3f out = vec3f(0, 0, 0);
		out = out + attrs[0] * bary.x;
		out = out + attrs[1] * bary.y;
		out = out + attrs[2] * bary.z;
		return out;
	}

	double min(double v1, double v2)
	{
		return fmin(v1, v2);
	}


	class SymetricMatrix
	{

	public:

		// Constructor

		SymetricMatrix(double c=0) { loopi(0, 10) m[i] = c; }

		SymetricMatrix(double m11, double m12, double m13, double m14,
					   double m22, double m23, double m24,
					   double m33, double m34,
					   double m44)
		{
			m[0] = m11;  m[1] = m12;  m[2] = m13;  m[3] = m14;
			m[4] = m22;  m[5] = m23;  m[6] = m24;
			m[7] = m33;  m[8] = m34;
			m[9] = m44;
		}

		// Make plane

		SymetricMatrix(double a, double b, double c, double d)
		{
			m[0] = a*a;  m[1] = a*b;  m[2] = a*c;  m[3] = a*d;
			m[4] = b*b;  m[5] = b*c;  m[6] = b*d;
			m[7] =c*c; m[8] = c*d;
			m[9] = d*d;
		}

		double operator[](int c) const { return m[c]; }

		// Determinant

		double det(int a11, int a12, int a13,
				   int a21, int a22, int a23,
				   int a31, int a32, int a33)
		{
			double det =  m[a11]*m[a22]*m[a33] + m[a13]*m[a21]*m[a32] + m[a12]*m[a23]*m[a31]
				- m[a13]*m[a22]*m[a31] - m[a11]*m[a23]*m[a32]- m[a12]*m[a21]*m[a33];
			return det;
		}

		const SymetricMatrix operator+(const SymetricMatrix& n) const
		{
			return SymetricMatrix(m[0]+n[0], m[1]+n[1], m[2]+n[2], m[3]+n[3],
								  m[4]+n[4], m[5]+n[5], m[6]+n[6],
								  m[7]+n[7], m[8]+n[8],
								  m[9]+n[9]);
		}

		SymetricMatrix& operator+=(const SymetricMatrix& n)
		{
			m[0]+=n[0];   m[1]+=n[1];   m[2]+=n[2];   m[3]+=n[3];
			m[4]+=n[4];   m[5]+=n[5];   m[6]+=n[6];   m[7]+=n[7];
			m[8]+=n[8];   m[9]+=n[9];
			return *this;
		}

		double m[10];
	};

	// Global Variables & Strctures
	enum Attributes
	{
		NONE,
		NORMAL = 2,
		TEXCOORD = 4,
		COLOR = 8
	};
	struct Triangle { int v[3]; double err[4]; int deleted, dirty, attr; vec3f n; vec3f uvs[3]; int material; };
	struct Vertex { vec3f p; int tstart, tcount; SymetricMatrix q; int border; };
	struct Ref { int tid, tvertex; };
	std::vector<Triangle> triangles;
	std::vector<Vertex> vertices;
	std::vector<Ref> refs;
	std::string mtllib;
	std::vector<std::string> materials;

	// Helper functions

	double vertex_error(SymetricMatrix q, double x, double y, double z);
	double calculate_error(int id_v1, int id_v2, vec3f& p_result);
	bool flipped(vec3f p, int i0, int i1, Vertex& v0, Vertex& v1, std::vector<int>& deleted);
	void update_uvs(int i0, const Vertex& v, const vec3f& p, std::vector<int>& deleted);
	void update_triangles(int i0, Vertex& v, std::vector<int>& deleted, int& deleted_triangles);
	void update_mesh(int iteration);
	void compact_mesh();
	//
	// Main simplification function
	//
	// target_count  : target nr. of triangles
	// agressiveness : sharpness to increase the threshold.
	//                 5..8 are good numbers
	//                 more iterations yield higher quality
	//

	void simplify_mesh(int target_count, double agressiveness=7, bool verbose=false)
	{
		// init
		loopi(0, triangles.size())
		{
			triangles[i].deleted=0;
		}

		// main iteration loop
		int deleted_triangles=0;
		std::vector<int> deleted0, deleted1;
		int triangle_count=triangles.size();
		//int iteration = 0;
		//loop(iteration,0,100)
		for( int iteration = 0; iteration < 100; iteration++ )
		{
			if( triangle_count-deleted_triangles<=target_count )break;

			// update mesh once in a while
			if( iteration%5==0 )
			{
				update_mesh(iteration);
			}

			// clear dirty flag
			loopi(0, triangles.size()) triangles[i].dirty=0;

			//
			// All triangles with edges below the threshold will be removed
			//
			// The following numbers works well for most models.
			// If it does not, try to adjust the 3 parameters
			//
			double threshold = 0.000000001*pow(double(iteration+3), agressiveness);

			// target number of triangles reached ? Then break
			if( (verbose) && (iteration%5==0) )
			{
				printf("iteration %d - triangles %d threshold %g\n", iteration, triangle_count-deleted_triangles, threshold);
			}

			// remove vertices & mark deleted triangles
			loopi(0, triangles.size())
			{
				Triangle& t=triangles[i];
				if( t.err[3]>threshold ) continue;
				if( t.deleted ) continue;
				if( t.dirty ) continue;

				loopj(0, 3)if( t.err[j]<threshold )
				{

					int i0=t.v[j]; Vertex& v0 = vertices[i0];
					int i1=t.v[(j+1)%3]; Vertex& v1 = vertices[i1];
					// Border check
					if( v0.border != v1.border )  continue;

					// Compute vertex to collapse to
					vec3f p;
					calculate_error(i0, i1, p);
					deleted0.resize(v0.tcount); // normals temporarily
					deleted1.resize(v1.tcount); // normals temporarily
					// don't remove if flipped
					if( flipped(p, i0, i1, v0, v1, deleted0) ) continue;

					if( flipped(p, i1, i0, v1, v0, deleted1) ) continue;

					if( (t.attr & TEXCOORD) == TEXCOORD )
					{
						update_uvs(i0, v0, p, deleted0);
						update_uvs(i0, v1, p, deleted1);
					}

					// not flipped, so remove edge
					v0.p=p;
					v0.q=v1.q+v0.q;
					int tstart=refs.size();

					update_triangles(i0, v0, deleted0, deleted_triangles);
					update_triangles(i0, v1, deleted1, deleted_triangles);

					int tcount=refs.size()-tstart;

					if( tcount<=v0.tcount )
					{
						// save ram
						if( tcount )memcpy(&refs[v0.tstart], &refs[tstart], tcount*sizeof(Ref));
					}
					else
						// append
						v0.tstart=tstart;

					v0.tcount=tcount;
					break;
				}
				// done?
				if( triangle_count-deleted_triangles<=target_count )break;
			}
		}
		// clean up mesh
		compact_mesh();
	} //simplify_mesh()

	void simplify_mesh_lossless(bool verbose=false)
	{
		// init
		loopi(0, triangles.size()) triangles[i].deleted=0;

		// main iteration loop
		int deleted_triangles=0;
		std::vector<int> deleted0, deleted1;
		int triangle_count=triangles.size();
		//int iteration = 0;
		//loop(iteration,0,100)
		for( int iteration = 0; iteration < 9999; iteration++ )
		{
			// update mesh constantly
			update_mesh(iteration);
			// clear dirty flag
			loopi(0, triangles.size()) triangles[i].dirty=0;
			//
			// All triangles with edges below the threshold will be removed
			//
			// The following numbers works well for most models.
			// If it does not, try to adjust the 3 parameters
			//
			double threshold = DBL_EPSILON; //1.0E-3 EPS;
			if( verbose )
			{
				printf("lossless iteration %d\n", iteration);
			}

			// remove vertices & mark deleted triangles
			loopi(0, triangles.size())
			{
				Triangle& t=triangles[i];
				if( t.err[3]>threshold ) continue;
				if( t.deleted ) continue;
				if( t.dirty ) continue;

				loopj(0, 3)if( t.err[j]<threshold )
				{
					int i0=t.v[j]; Vertex& v0 = vertices[i0];
					int i1=t.v[(j+1)%3]; Vertex& v1 = vertices[i1];

					// Border check
					if( v0.border != v1.border )  continue;

					// Compute vertex to collapse to
					vec3f p;
					calculate_error(i0, i1, p);

					deleted0.resize(v0.tcount); // normals temporarily
					deleted1.resize(v1.tcount); // normals temporarily

					// don't remove if flipped
					if( flipped(p, i0, i1, v0, v1, deleted0) ) continue;
					if( flipped(p, i1, i0, v1, v0, deleted1) ) continue;

					if( (t.attr & TEXCOORD) == TEXCOORD )
					{
						update_uvs(i0, v0, p, deleted0);
						update_uvs(i0, v1, p, deleted1);
					}

					// not flipped, so remove edge
					v0.p=p;
					v0.q=v1.q+v0.q;
					int tstart=refs.size();

					update_triangles(i0, v0, deleted0, deleted_triangles);
					update_triangles(i0, v1, deleted1, deleted_triangles);

					int tcount=refs.size()-tstart;

					if( tcount<=v0.tcount )
					{
						// save ram
						if( tcount )memcpy(&refs[v0.tstart], &refs[tstart], tcount*sizeof(Ref));
					}
					else
						// append
						v0.tstart=tstart;

					v0.tcount=tcount;
					break;
				}
			}
			if( deleted_triangles<=0 )break;
			deleted_triangles=0;
		} //for each iteration
		// clean up mesh
		compact_mesh();
	} //simplify_mesh_lossless()


	// Check if a triangle flips when this edge is removed

	bool flipped(vec3f p, int i0, int i1, Vertex& v0, Vertex& v1, std::vector<int>& deleted)
	{

		loopk(0, v0.tcount)
		{
			Triangle& t=triangles[refs[v0.tstart+k].tid];
			if( t.deleted )continue;

			int s=refs[v0.tstart+k].tvertex;
			int id1=t.v[(s+1)%3];
			int id2=t.v[(s+2)%3];

			if( id1==i1 || id2==i1 ) // delete ?
			{

				deleted[k]=1;
				continue;
			}
			vec3f d1 = vertices[id1].p-p; d1.normalize();
			vec3f d2 = vertices[id2].p-p; d2.normalize();
			if( fabs(d1.dot(d2))>0.999 ) return true;
			vec3f n;
			n.cross(d1, d2);
			n.normalize();
			deleted[k]=0;
			if( n.dot(t.n)<0.2 ) return true;
		}
		return false;
	}

	// update_uvs

	void update_uvs(int i0, const Vertex& v, const vec3f& p, std::vector<int>& deleted)
	{
		loopk(0, v.tcount)
		{
			Ref& r=refs[v.tstart+k];
			Triangle& t=triangles[r.tid];
			if( t.deleted )continue;
			if( deleted[k] )continue;
			vec3f p1=vertices[t.v[0]].p;
			vec3f p2=vertices[t.v[1]].p;
			vec3f p3=vertices[t.v[2]].p;
			t.uvs[r.tvertex] = interpolate(p, p1, p2, p3, t.uvs);
		}
	}

	// Update triangle connections and edge error after a edge is collapsed

	void update_triangles(int i0, Vertex& v, std::vector<int>& deleted, int& deleted_triangles)
	{
		vec3f p;
		loopk(0, v.tcount)
		{
			Ref& r=refs[v.tstart+k];
			Triangle& t=triangles[r.tid];
			if( t.deleted )continue;
			if( deleted[k] )
			{
				t.deleted=1;
				deleted_triangles++;
				continue;
			}
			t.v[r.tvertex]=i0;
			t.dirty=1;
			t.err[0]=calculate_error(t.v[0], t.v[1], p);
			t.err[1]=calculate_error(t.v[1], t.v[2], p);
			t.err[2]=calculate_error(t.v[2], t.v[0], p);
			t.err[3]=min(t.err[0], min(t.err[1], t.err[2]));
			refs.push_back(r);
		}
	}

	// compact triangles, compute edge error and build reference list

	void update_mesh(int iteration)
	{
		if( iteration>0 ) // compact triangles
		{
			int dst=0;
			loopi(0, triangles.size())
				if( !triangles[i].deleted )
				{
					triangles[dst++]=triangles[i];
				}
			triangles.resize(dst);
		}
		//

		// Init Reference ID list
		loopi(0, vertices.size())
		{
			vertices[i].tstart=0;
			vertices[i].tcount=0;
		}
		loopi(0, triangles.size())
		{
			Triangle& t=triangles[i];
			loopj(0, 3) vertices[t.v[j]].tcount++;
		}
		int tstart=0;
		loopi(0, vertices.size())
		{
			Vertex& v=vertices[i];
			v.tstart=tstart;
			tstart+=v.tcount;
			v.tcount=0;
		}

		// Write References
		refs.resize(triangles.size()*3);
		loopi(0, triangles.size())
		{
			Triangle& t=triangles[i];
			loopj(0, 3)
			{
				Vertex& v=vertices[t.v[j]];
				refs[v.tstart+v.tcount].tid=i;
				refs[v.tstart+v.tcount].tvertex=j;
				v.tcount++;
			}
		}

		// Init Quadrics by Plane & Edge Errors
		//
		// required at the beginning ( iteration == 0 )
		// recomputing during the simplification is not required,
		// but mostly improves the result for closed meshes
		//
		if( iteration == 0 )
		{
			// Identify boundary : vertices[].border=0,1

			std::vector<int> vcount, vids;

			loopi(0, vertices.size())
				vertices[i].border=0;

			loopi(0, vertices.size())
			{
				Vertex& v=vertices[i];
				vcount.clear();
				vids.clear();
				loopj(0, v.tcount)
				{
					int k=refs[v.tstart+j].tid;
					Triangle& t=triangles[k];
					loopk(0, 3)
					{
						int ofs=0, id=t.v[k];
						while( ofs<vcount.size() )
						{
							if( vids[ofs]==id )break;
							ofs++;
						}
						if( ofs==vcount.size() )
						{
							vcount.push_back(1);
							vids.push_back(id);
						}
						else
							vcount[ofs]++;
					}
				}
				loopj(0, vcount.size()) if( vcount[j]==1 )
					vertices[vids[j]].border=1;
			}
			//initialize errors
			loopi(0, vertices.size())
				vertices[i].q=SymetricMatrix(0.0);

			loopi(0, triangles.size())
			{
				Triangle& t=triangles[i];
				vec3f n, p[3];
				loopj(0, 3) p[j]=vertices[t.v[j]].p;
				n.cross(p[1]-p[0], p[2]-p[0]);
				n.normalize();
				t.n=n;
				loopj(0, 3) vertices[t.v[j]].q =
					vertices[t.v[j]].q+SymetricMatrix(n.x, n.y, n.z, -n.dot(p[0]));
			}
			loopi(0, triangles.size())
			{
				// Calc Edge Error
				Triangle& t=triangles[i]; vec3f p;
				loopj(0, 3) t.err[j]=calculate_error(t.v[j], t.v[(j+1)%3], p);
				t.err[3]=min(t.err[0], min(t.err[1], t.err[2]));
			}
		}
	}

	// Finally compact mesh before exiting

	void compact_mesh()
	{
		int dst=0;
		loopi(0, vertices.size())
		{
			vertices[i].tcount=0;
		}
		loopi(0, triangles.size())
			if( !triangles[i].deleted )
			{
				Triangle& t=triangles[i];
				triangles[dst++]=t;
				loopj(0, 3)vertices[t.v[j]].tcount=1;
			}
		triangles.resize(dst);
		dst=0;
		loopi(0, vertices.size())
			if( vertices[i].tcount )
			{
				vertices[i].tstart=dst;
				vertices[dst].p=vertices[i].p;
				dst++;
			}
		loopi(0, triangles.size())
		{
			Triangle& t=triangles[i];
			loopj(0, 3)t.v[j]=vertices[t.v[j]].tstart;
		}
		vertices.resize(dst);
	}

	// Error between vertex and Quadric

	double vertex_error(SymetricMatrix q, double x, double y, double z)
	{
		return   q[0]*x*x + 2*q[1]*x*y + 2*q[2]*x*z + 2*q[3]*x + q[4]*y*y
			+ 2*q[5]*y*z + 2*q[6]*y + q[7]*z*z + 2*q[8]*z + q[9];
	}

	// Error for one edge

	double calculate_error(int id_v1, int id_v2, vec3f& p_result)
	{
		// compute interpolated vertex

		SymetricMatrix q = vertices[id_v1].q + vertices[id_v2].q;
		bool   border = vertices[id_v1].border & vertices[id_v2].border;
		double error=0;
		double det = q.det(0, 1, 2, 1, 4, 5, 2, 5, 7);
		if( det != 0 && !border )
		{

			// q_delta is invertible
			p_result.x = -1/det*(q.det(1, 2, 3, 4, 5, 6, 5, 7, 8));	// vx = A41/det(q_delta)
			p_result.y =  1/det*(q.det(0, 2, 3, 1, 5, 6, 2, 7, 8));	// vy = A42/det(q_delta)
			p_result.z = -1/det*(q.det(0, 1, 3, 1, 4, 6, 2, 5, 8));	// vz = A43/det(q_delta)

			error = vertex_error(q, p_result.x, p_result.y, p_result.z);
		}
		else
		{
			// det = 0 -> try to find best result
			vec3f p1=vertices[id_v1].p;
			vec3f p2=vertices[id_v2].p;
			vec3f p3=(p1+p2)/2;
			double error1 = vertex_error(q, p1.x, p1.y, p1.z);
			double error2 = vertex_error(q, p2.x, p2.y, p2.z);
			double error3 = vertex_error(q, p3.x, p3.y, p3.z);
			error = min(error1, min(error2, error3));
			if( error1 == error ) p_result=p1;
			if( error2 == error ) p_result=p2;
			if( error3 == error ) p_result=p3;
		}
		return error;
	}
}

namespace fqms
{
	void updateGlobal(lim::Mesh* mesh)
	{
		fqms::vertices.clear();
		fqms::triangles.clear();

		for( lim::n_mesh::Vertex& v : mesh->vertices )
		{
			fqms::Vertex temp;
			temp.p.x = v.p.x;
			temp.p.y = v.p.y;
			temp.p.z = v.p.z;
			fqms::vertices.push_back(temp);
		}

		// make triangles & edge
		fqms::triangles.resize(mesh->indices.size()/3);
		int triCount=0;
		int vertCount=0;
		for( GLuint idx: mesh->indices )
		{
			fqms::triangles[triCount].v[vertCount] = idx;
			fqms::triangles[triCount].uvs[vertCount] = vec3f(mesh->vertices[idx].uv.x,
															 mesh->vertices[idx].uv.y, 1);
			vertCount++;
			if( vertCount == 3 )
			{
				vertCount = 0;
				triCount++;
			}
		}
		if( triCount*3!=mesh->indices.size()
		   || mesh->indices.size()%3!=0 )
		{
			fprintf(stderr, "simplify failed : mesh is not triangle mesh\n");
		}
	}
	void updateMesh(lim::Mesh* mesh)
	{
		mesh->vertices.clear();
		mesh->indices.clear();

		for( fqms::Vertex& v : fqms::vertices )
		{
			lim::n_mesh::Vertex temp;
			temp.p.x = v.p.x;
			temp.p.y = v.p.y;
			temp.p.z = v.p.z;
			mesh->vertices.push_back(temp);
		}

		// make triangles & edge
		mesh->indices.resize(fqms::triangles.size()*3);

		int idxCount=0;
		for( fqms::Triangle& tri: fqms::triangles )
		{
			for( int i=0; i<3; i++ )
			{
				mesh->indices[idxCount++] = tri.v[i];
				mesh->vertices[tri.v[i]].uv = glm::vec2(tri.uvs[i].x, tri.uvs[i].y);
			}
		}
		mesh->setupMesh();
	}

	lim::Mesh* simplifyMesh(lim::Mesh* mesh, float lived_pct, int agressiveness=7)
	{
		if( lived_pct>1||lived_pct<0 )
		{
			printf("error : simplify percent range error");
			return nullptr;
		}
		updateGlobal(mesh);

		printf("\nsimplify mesh : %s, %d tris\n", mesh->name.c_str(), fqms::triangles.size());

		int target_count = (int)(fqms::triangles.size()*lived_pct);
		double start = glfwGetTime();
		fqms::simplify_mesh(target_count, 7.0, true);

		printf("\nsimplify mesh : %s, %d tris, %lfsec\n", mesh->name.c_str(), fqms::triangles.size(), glfwGetTime()-start);

		lim::Mesh* result = new lim::Mesh(mesh->name.c_str());
		updateMesh(result);
		return result;
	}

	lim::Model* simplifyModel(lim::Model* model, float lived_pct = 0.8f)
	{
		printf("\nsimplify model : %s, %d vertices\n", model->name.c_str(), model->verticesNum);
		std::vector<lim::Mesh*> new_meshes;
		for( lim::Mesh* mesh : model->meshes )
		{
			lim::Mesh* simp_mesh = simplifyMesh(mesh, lived_pct);
			if( simp_mesh != nullptr )
				new_meshes.push_back(simp_mesh);
		}
		lim::Model* result = new lim::Model(new_meshes, model->textures_loaded, model->program, model->name);
		return result;
	}
};
#endif
