//
//	2022-11-15 / im dong ye
//  edit GLUK code
//
//
#ifndef ASSET_LIB_H
#define ASSET_LIB_H

class Program;

namespace lim
{
	class AssetLib
	{
	private:
		AssetLib();	// Constructor
		//! Private constructors, which cannot be used
		AssetLib(const AssetLib&) {}
		//! Private copy operator, which cannot be used
		AssetLib& operator=(const AssetLib&) { return *this; }
	public:
		Program* toQuadProg;
		inline static GLuint quadVAO = 0;

	public:
		static AssetLib& get()
		{
			static AssetLib lib;
			return lib;
		}
	};
}

#endif