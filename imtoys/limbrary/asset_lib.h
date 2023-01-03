//
//	2022-12-30 / im dong ye
//

#ifndef ASSET_LIB_H
#define ASSET_LIB_H

namespace lim
{
	class Program;
	class AssetLib
	{
	public:
		//****** property ******//
		Program* to_quad_prog;
		GLuint quad_vao = 0;
	private:
		AssetLib();
		~AssetLib();
		/* detete copy & copy asignment singleton obj */
		AssetLib(const AssetLib&)=delete;
		AssetLib &operator=(const AssetLib&)=delete;
	public:
		static AssetLib& get()
		{
			static AssetLib assetLib;
			return assetLib;
		}
	};
}

#endif
