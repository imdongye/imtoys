//
//	2023-01-02 / im dong ye
//	
//	
//	reference:
//		SCMS.h
//		JGLTest
//		Created by Hyun Joon Shin on 2022/07/25.
//


#ifndef __color_aware_image_h_
#define __color_aware_image_h_

#include <glm/glm.hpp>
#include <glad/glad.h>
#include <string>
#include <vector>
#include <stdio.h>
#include <limbrary/texture.h>
#include <limbrary/framebuffer.h>
#include <limbrary/program.h>
#include <limbrary/asset_lib.h>


namespace ICC
{
	// White point chromaticity
	// From: https://en.wikipedia.org/wiki/Template:Color_temperature_white_points
	const glm::vec2 WHTPT_A		={0.44757f, 0.40745f};
	const glm::vec2 WHTPT_B		={0.34842f, 0.35161f};
	const glm::vec2 WHTPT_C		={0.31006f, 0.31616f};
	const glm::vec2 WHTPT_D50	={0.34567f, 0.35850f};
	const glm::vec2 WHTPT_D55	={0.33242f, 0.34743f};
	const glm::vec2 WHTPT_D60	={0.32168f, 0.33767f};
	const glm::vec2 WHTPT_DCI	={0.31400f, 0.35100f};
	const glm::vec2 WHTPT_D65	={0.31271f, 0.32902f};
	const glm::vec2 WHTPT_D75	={0.29902f, 0.31485f};
	const glm::vec2 WHTPT_E		={0.33333f, 0.33333f};
	const glm::vec2 WHTPT_F1	={0.31310f, 0.33727f};
	const glm::vec2 WHTPT_F2	={0.37208f, 0.37529f};
	const glm::vec2 WHTPT_F3	={0.40910f, 0.39430f};
	const glm::vec2 WHTPT_F4	={0.44018f, 0.40329f};
	const glm::vec2 WHTPT_F5	={0.31379f, 0.34531f};
	const glm::vec2 WHTPT_F6	={0.37790f, 0.38835f};
	const glm::vec2 WHTPT_F7	={0.31292f, 0.32933f};
	const glm::vec2 WHTPT_F8	={0.34588f, 0.35875f};
	const glm::vec2 WHTPT_F9	={0.37417f, 0.37281f};
	const glm::vec2 WHTPT_F10	={0.34609f, 0.35986f};
	const glm::vec2 WHTPT_F11	={0.38052f, 0.37713f};
	const glm::vec2 WHTPT_F12	={0.43695f, 0.40441f};

	// Primary chromaticity
	// From: https://en.wikipedia.org/wiki/SRGB
	const glm::vec2 sRGB_R_xy         ={0.6400f,0.3300f};
	const glm::vec2 sRGB_G_xy         ={0.3000f,0.6000f};
	const glm::vec2 sRGB_B_xy         ={0.1500f,0.0600f};

	// From: https://en.wikipedia.org/wiki/DCI-P3
	// also same Display P3 apple
	const glm::vec2 DCI_P3_R_xy    ={0.680f, 0.320f};
	const glm::vec2 DCI_P3_G_xy    ={0.265f, 0.690f};
	const glm::vec2 DCI_P3_B_xy    ={0.150f, 0.060f};
	
	// From: https://en.wikipedia.org/wiki/Adobe_RGB_color_space
	const glm::vec2 AdobeRGB_R_xy     ={0.6400f,0.3300f};
	const glm::vec2 AdobeRGB_G_xy     ={0.2100f,0.7100f};
	const glm::vec2 AdobeRGB_B_xy     ={0.1500f,0.0600f};

	// From: https://en.wikipedia.org/wiki/ProPhoto_RGB_color_space
	const glm::vec2 ProPhoto_R_xy     ={0.734699f,0.265301f};
	const glm::vec2 ProPhoto_G_xy     ={0.159597f,0.840403f};
	const glm::vec2 ProPhoto_B_xy     ={0.036598f,0.000105f};

	inline glm::vec3 XYZToxyY(float X, float Y, float Z)
	{
		return glm::vec3(X/(X+Y+Z), Y/(X+Y+Z), Y);
	}
	inline glm::vec3 XYZToxyY(const glm::vec3& v) { return XYZToxyY(v.x, v.y, v.z); }

	// From: http://www.brucelindbloom.com/index.html?Eqn_xyY_to_XYZ.html
	inline glm::vec3 xyYToXYZ(float x, float y, float Y)
	{
		//return glm::vec3(x/y, 1, (1-x-y)/y)*Y;
		return glm::vec3(x*Y/y, Y, (1-x-y)*Y/y);
	}
	inline glm::vec3 xyYToXYZ(const glm::vec3& v) { return xyYToXYZ(v.x, v.y, v.z); }
}


namespace ICC
{
	struct FOURC
	{
		char c[4];
		char operator[](int i)const { return c[i]; }
		char& operator[](int i) { return c[i]; }
		bool compare(const char* d) { return c[0]==d[0] && c[1]==d[1] && c[2]==d[2] && c[3]==d[3]; }
	};
	struct TIME
	{
		uint32_t	year, month, day, hour, minute, second;
	};
	struct EIGHTC
	{
		char c[8];
		char operator[](int i)const { return c[i]; }
		char& operator[](int i) { return c[i]; }
	};
	inline FOURC getFourC(uint8_t*& data)
	{
		FOURC val;
		val[0]=data[0]; val[1]=data[1]; val[2]=data[2]; val[3]=data[3];
		data+=4;
		return val;
	}
	inline uint32_t getUint(uint8_t*& data)
	{
		uint32_t val;
		uint8_t* pt = (uint8_t*)(&val);
		pt[0]=data[3]; pt[1]=data[2]; pt[2]=data[1]; pt[3]=data[0];
		data+=4;
		return val;
	}
	inline EIGHTC getEightC(uint8_t*& data)
	{
		EIGHTC val;
		val[0]=data[0];	val[1]=data[1];	val[2]=data[2];	val[3]=data[3];
		val[4]=data[4];	val[5]=data[5];	val[6]=data[6];	val[7]=data[7];
		data+=8;
		return val;
	}
	inline float getFloat(uint8_t*& data)
	{
		int16_t upper;
		uint16_t lower;
		float val;
		uint8_t* pt = (uint8_t*)(&upper);
		pt[0]=data[1]; pt[1]=data[0];
		pt = (uint8_t*)(&lower);
		pt[0]=data[3]; pt[1]=data[2];
		val = float(upper)+float(lower)/65536.f;
		data+=4;
		return val;
	}
	inline glm::vec3 getXYZ(uint8_t*& data)
	{
		glm::vec3 c;
		c.r = getFloat(data);
		c.g = getFloat(data);
		c.b = getFloat(data);
		return c;
	}
	inline float getFloat16(uint8_t*& data)
	{
		int8_t upper;
		uint8_t lower;
		float val;
		uint8_t* pt = (uint8_t*)(&upper);
		pt[0]=data[0];
		pt = (uint8_t*)(&lower);
		pt[0]=data[1];
		val = float(upper)+float(lower)/256.f;
		data+=2;
		return val;
	}
	inline uint16_t getUint16(uint8_t*& data)
	{
		uint16_t val;
		uint8_t* pt = (uint8_t*)(&val);
		pt[0]=data[1]; pt[1]=data[0];
		data+=2;
		return val;
	}
	inline void print_(const char* fm, const FOURC d)
	{
		std::string fmt = fm;
		size_t pos = fmt.find("%F");
		if( pos>fmt.size() ) printf("%s", fm);
		else {
			fmt.replace(pos, 2, "%c%c%c%c");
			printf(fmt.c_str(), d[0], d[1], d[2], d[3]);
		}
	}
	inline void print_(const char* fm, const EIGHTC d)
	{
		std::string fmt = fm;
		size_t pos = fmt.find("%E");
		if( pos>fmt.size() ) printf("%s", fm);
		else {
			fmt.replace(pos, 2, "%c%c%c%c%c%c%c%c");
			printf(fmt.c_str(), d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7]);
		}
	}
	inline void print_(const char* fm, const glm::vec3& c)
	{
		std::string fmt = fm;
		size_t pos = fmt.find("%C");
		if( pos>fmt.size() ) printf("%s", fm);
		else {
			fmt.replace(pos, 2, "%f %f %f");
			printf(fmt.c_str(), c.r, c.g, c.b);
		}
	}
	inline float recognizeTonemap(const std::vector<uint16_t>& gammaTable)
	{
		int count = int(gammaTable.size());
		// Check if it is sRGB
		{
			int offset = int(0.3*(count-1)+0.999999999);
			int n = 0;
			float sum1=0, sum2=0, sum3=0, sum4=0;
			for( int i = offset; i<count; i+=4 ) {
				n++;
				float x = log(0.948f*i/(count-1.f)+0.052f);
				float y = log(gammaTable[i]/65535.f);
				sum1+= x*y;
				sum2+=x;
				sum3+=y;
				sum4+=x*x;
			}
			float gamma = (n*sum1-sum2*sum3)/(n*sum4-sum2*sum2);
			if( abs(gamma-2.4f)<0.01f ) return 2.4f;
		}
		// Check if it is Power curve of linear..
		{
			int n = 0;
			float sum1=0, sum2=0, sum3=0, sum4=0;
			for( int i = 0; i<count; i+=8 ) {
				n++;
				float x = log(i/(count-1.f));
				float y = log(gammaTable[i]/65535.f);
				sum1+= x*y;
				sum2+=x;
				sum3+=y;
				sum4+=x*x;
			}
			float gamma = (n*sum1-sum2*sum3)/(n*sum4-sum2*sum2);
			if( abs(gamma-2.2f)<0.01f ) return 2.2f;
			if( abs(gamma-2.6f)<0.01f ) return 2.6f;
			if( abs(gamma-1.8f)<0.01f ) return 1.8f;
			if( abs(gamma-1.0f)<0.01f ) return 1.0f;
			// Unknown curve.. here we simply approximate the curve as Power function
			return gamma;
		}
	}

	struct Header
	{
	public:
		uint32_t	profileSize;
		FOURC		CMMType;
		uint32_t	version;
		FOURC		profileDeviceClass;
		FOURC		colorSpace;
		FOURC		connectionSpace;
		TIME		creationTime;
		FOURC		signature; 			// acsp
		FOURC		primPlatformTarget;
		uint32_t	flags;
		FOURC		manufacturer;
		FOURC		deviceModel;
		EIGHTC		deviceAttributes;
		uint32_t	renderIntent;
		glm::vec3	illuminantXYZ;	// Connection Space
		FOURC		creator;
	public:
		void read(uint8_t*& data)
		{
			profileSize			= getUint(data);
			CMMType				= getFourC(data);
			version				= getUint(data);
			profileDeviceClass	= getFourC(data);
			colorSpace			= getFourC(data);
			connectionSpace		= getFourC(data);

			FOURC creationData1 = getFourC(data);
			FOURC creationData2 = getFourC(data);
			FOURC creationData3 = getFourC(data);

			creationTime.year   = (uint32_t(creationData1[0]))*256+(uint32_t(creationData1[1]));
			creationTime.month  = (uint32_t(creationData1[2]))*256+(uint32_t(creationData1[3]));
			creationTime.day    = (uint32_t(creationData2[0]))*256+(uint32_t(creationData2[1]));
			creationTime.hour   = (uint32_t(creationData2[2]))*256+(uint32_t(creationData2[3]));
			creationTime.minute = (uint32_t(creationData3[0]))*256+(uint32_t(creationData3[1]));
			creationTime.second = (uint32_t(creationData3[2]))*256+(uint32_t(creationData3[3]));

			signature			= getFourC(data);
			primPlatformTarget	= getFourC(data);
			flags				= getUint(data);


			manufacturer		= getFourC(data);
			deviceModel			= getFourC(data);
			deviceAttributes	= getEightC(data);
			renderIntent		= getUint(data);
			illuminantXYZ		= getXYZ(data);
			creator				= getFourC(data);
		}
		void print()
		{
			printf("< HEADER >\n");
			printf("Profile Size: %X\n", profileSize);
			print_("CMM Type: %F\n", CMMType);
			printf("Profile Version: %d\n", version);
			print_("DeviceClass: %F\n", profileDeviceClass);
			print_("Color Space Sig: %F\n", colorSpace);
			print_("Prof Conn Space: %F\n", connectionSpace);
			printf("Date: %d/%d/%d %d:%d:%d\n", creationTime.year, creationTime.month, creationTime.day, creationTime.hour, creationTime.minute, creationTime.second);
			print_("Profile file Sig: %F\n", signature);
			print_("Platform Target: %F\n", primPlatformTarget);
			printf("Profile Flags: %d\n", flags);

			print_("Device Manuf: %F\n", manufacturer);
			print_("Device Model: %F\n", deviceModel);
			print_("DeviceAttributes: %E\n", deviceAttributes);
			printf("Rendering Intent: %d\n", renderIntent);
			print_("Illuminant XYZ: %C\n", illuminantXYZ);
			print_("Illuminant xyY: %C\n", XYZToxyY(illuminantXYZ));
			print_("Creator: %F\n", creator);
		}

	};
	struct ColorProfile
	{
	public:
		glm::vec3 whtPt, r, g, b;
		glm::vec3 gamma;
		glm::vec3 blkPt;
		Header header;
		std::string copyright;
		std::string name;
	public:
		static float parseGamma(uint8_t*& data, std::string name, bool verbose)
		{
			float gamma = 2.4f;
			uint32_t count = getUint(data);
			if( count==0 ) {
				gamma = 1.;
				if( verbose ) printf("%s: Linear\n", name.c_str());
			} else if( count==1 ) {
				gamma = getFloat16(data);
				if( verbose ) printf("%s: Gamma: %f\n", name.c_str(), gamma);
			} else if( count == 0x00030000 ) { // Type3
				gamma = getFloat(data);
				float a = getFloat(data), b = getFloat(data), c = getFloat(data), d = getFloat(data);
				if( verbose ) printf("%s: Type3: %f <%f %f %f %f>\n", name.c_str(), gamma, a, b, c, d);
			} else {
				std::vector<uint16_t> gammaTable;
				for( uint32_t i=0; i<count; i++ ) gammaTable.push_back(getUint16(data));
				gamma = recognizeTonemap(gammaTable);
				if( verbose ) printf("%s: LUT with gamma: %f\n", name.c_str(), gamma);
			}
			return gamma;
		}
	public:
		bool initWithJPEG(std::string_view path, bool verbose=false)
		{
			printf("< Read profile in JPEG >\n");
			FILE *file = fopen(path.data(), "rb");
			size_t iccSize;
			uint8_t *iccData;
			{ // find icc start
				int off=0;
				int state = 0;
				while( !feof(file) ) {
					uint8_t c=fgetc(file);
					off++;
					if( c==0xFF ) state = 1;
					else if( state==1 && c==0xE2 ) break;
					else state = 0;
				}
				printf("STATE: %d.. EOF: %d\n", state, feof(file));
				if( state==1&&!feof(file) ) printf("finded 0xFFE2\n");
				else {
					printf("error not finded 0xFFE2\n\n");
					fclose(file);
					return false;
				}
			}
			{ // get icc size and copy data
				uint8_t C1=fgetc(file), C2=fgetc(file);
				iccSize = C1<<8|C2;// little endian
				printf("iccSize :  %d\n", (int)iccSize);

				char temp[100];
				fread(temp, 14, 1, file);

				iccData = new uint8_t[iccSize+5];
				fread((char*)iccData, iccSize, 1, file);
			}
			{ // init profile
				uint8_t *pd = iccData; // data pointer
				header.read(pd);
				if( verbose ) header.print();

				printf("< Tagged Data >\n");
				pd+=44;
				int numTags = getUint(pd);

				for( int i=0; i<numTags; i++ ) {
					FOURC tagSig = getFourC(pd);
					uint32_t offset = getUint(pd);
					uint32_t dataSize = getUint(pd)-8;
					uint8_t* data = iccData+offset+8;

					if( tagSig.compare("cprt") ) {
						char temp[200];
						memcpy(temp, data, dataSize);
						for( uint32_t k=0; k<dataSize; k++ ) if( temp[k]=='\0' ) temp[k]=32;
						temp[dataSize]='\0';
						copyright = temp;
						if( verbose ) printf("Copyright: %s\n", temp);
					}
					if( tagSig.compare("wtpt") ) {
						whtPt = getXYZ(data);
						if( verbose ) print_("not used WTPT<xyY>: %C\n", XYZToxyY(whtPt));
						if( verbose ) print_("not used WTPT<XYZ>: %C\n", whtPt);
					}
					if( tagSig.compare("bkpt") ) {
						blkPt = getXYZ(data);
						if( verbose ) print_("BKPT<xyY>: %C\n", XYZToxyY(blkPt));
					}
					if( tagSig.compare("rTRC") ) {
						gamma.r = parseGamma(data, "rTRC", verbose);
					}
					if( tagSig.compare("gTRC") ) {
						gamma.g = parseGamma(data, "gTRC", verbose);
					}
					if( tagSig.compare("bTRC") ) {
						gamma.b = parseGamma(data, "bTRC", verbose);
					}
					if( tagSig.compare("rXYZ") ) {
						r = getXYZ(data);
						if( verbose ) {
							print_("r<xyY>: %C\n", XYZToxyY(r));
							print_("r<XYZ>: %C\n", r);
						}
					}
					if( tagSig.compare("gXYZ") ) {
						g = getXYZ(data);
						if( verbose ) {
							print_("g<xyY>: %C\n", g);
							print_("g<XYZ>: %C\n", g);
						}
					}
					if( tagSig.compare("bXYZ") ) {
						b = getXYZ(data);
						if( verbose ) {
							print_("b<xyY>: %C\n", XYZToxyY(b));
							print_("b<XYZ>: %C\n", b);
						}
					}
				}
				whtPt = header.illuminantXYZ;
			}
			{ // init profile name
				glm::vec3 rXYZ = XYZToxyY(r);
				glm::vec3 gXYZ = XYZToxyY(g);
				glm::vec3 bXYZ = XYZToxyY(b);
				glm::vec2 xyR = {rXYZ.x, rXYZ.y};
				glm::vec2 xyG = {gXYZ.x, gXYZ.y};
				glm::vec2 xyB = {bXYZ.x, bXYZ.y};

				float min;
				float error = 0;
				error += glm::length(sRGB_R_xy-xyR);
				error += glm::length(sRGB_G_xy-xyG);
				error += glm::length(sRGB_B_xy-xyB);
				min=error;
				name = "sRGB";

				error = 0;
				error += glm::length(DCI_P3_R_xy-xyR);
				error += glm::length(DCI_P3_G_xy-xyG);
				error += glm::length(DCI_P3_B_xy-xyB);
				if( error<min ) {
					min=error;
					name = "DisplayP3";
				}

				error = 0;
				error += glm::length(AdobeRGB_R_xy-xyR);
				error += glm::length(AdobeRGB_G_xy-xyG);
				error += glm::length(AdobeRGB_B_xy-xyB);
				if( error<min ) {
					min=error;
					name = "AdobeRGB";
				}

				error = 0;
				error += glm::length(ProPhoto_R_xy-xyR);
				error += glm::length(ProPhoto_G_xy-xyG);
				error += glm::length(ProPhoto_B_xy-xyB);
				if( error<min ) {
					min=error;
					name = "ProPhoto";
				}
			}
			printf("Done parce profile\n\n");
			delete iccData;
			fclose(file);
			return true;
		}
		// From: http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
		glm::mat3 getRGB2XYZ()
		{
			glm::vec3 S = glm::inverse(glm::mat3(r, g, b)) * whtPt;
			// 여기서 rgb가 white point가 맞춰져있어서 S는 1이 나올것이다.
			glm::mat3 M = glm::mat3(r*S.r, g*S.g, b*S.b);
			return M;
		}
		glm::mat3 getXYZ2RGB(glm::vec2 dr_xy, glm::vec2 dg_xy, glm::vec2 db_xy, glm::vec2 dWhtPt_xy) // display primary xy(chromaticity)
		{
			// display rgb XYZ
			glm::vec3 dr = xyYToXYZ(glm::vec3(dr_xy, 1.0f));
			glm::vec3 dg = xyYToXYZ(glm::vec3(dg_xy, 1.0f));
			glm::vec3 db = xyYToXYZ(glm::vec3(db_xy, 1.0f));
			glm::vec3 dWP = xyYToXYZ(glm::vec3(dWhtPt_xy, 1.0f));

			glm::vec3 S = glm::inverse(glm::mat3(dr, dg, db)) * dWP;

			glm::vec3 WW = glm::mat3(dr, dg, db)*glm::vec3(1);
			//S = dWP/WW;

			glm::mat3 M = glm::mat3(dr*S.r, dg*S.g, db*S.b);
			/*
			glm::mat3 MM ={{3.2404542, -0.9692660, 0.0556434},
						   {-1.5371385, 1.8760108, -0.2040259},
						   {-0.4985314, 0.0415560, 1.0572252}};
			return MM;*/
			return glm::inverse(M);
		}
		// From: http://www.brucelindbloom.com/index.html?Eqn_ChromAdapt.html
		// 0. scaling     1. VonKries      2.Bradford
		glm::mat3 chromaticAdaptationTo(const glm::vec2& destWP_xy, int method = 2)
		{
			glm::mat3 ret(1);
			glm::vec3 srcWP, destWP;

			glm::mat3 forward, inverse;

			const glm::mat3 XYZ_Scaling_forward = glm::mat3(1);
			const glm::mat3 XYZ_Scaling_inverse = glm::mat3(1);
			// From : https://en.wikipedia.org/wiki/LMS_color_space
			// Hunt and RLAB color appearance model
			const glm::mat3 VonKries_forward ={{0.4002, -0.2263, 0.0},{0.7076, 1.1653, 0.0}, {-0.0808, 0.0457, 0.9182}};
			const glm::mat3 VonKries_inverse ={{1.8599, 0.3612, 0.0}, {-1.1294, 0.6388, 0.0}, {0.2199, 0.0000, 1.0891}};
			// Bradford's spectrally sharpened mat
			const glm::mat3 Bradford_forward = {{0.8951, -0.7502, 0.0389}, {0.2664, 1.7135, -0.0685}, {-0.1614, 0.0367, 1.0296}};
			const glm::mat3 Bradford_inverse = {{0.9870, 0.4323, -0.0085}, {-0.1471, 0.5184, 0.0400}, {0.1600, 0.0493, 0.9695}};

			switch( method ) {
				case 0:
					forward = XYZ_Scaling_forward;
					inverse = XYZ_Scaling_inverse;
					break;
				case 1:
					forward = VonKries_forward;
					inverse = VonKries_inverse;
					break;
				case 2:
					forward = Bradford_forward;
					inverse = Bradford_inverse;
					break;
			}

			srcWP = whtPt;
			srcWP = forward * srcWP;

			destWP = xyYToXYZ({destWP_xy.x, destWP_xy.y, 1.f});
			destWP = forward * destWP;

			ret[0][0] *= destWP.r/srcWP.r;
			ret[1][1] *= destWP.g/srcWP.g;
			ret[2][2] *= destWP.b/srcWP.b;

			ret = inverse*ret*forward;

			return ret;
		}
	};
}

namespace lim
{
	class ColorAwareImage: public Texture
	{
	public:
		inline static GLuint refCount = 0;
		inline static Program *colorAwareDisplayProg = nullptr;
		ICC::ColorProfile profile;
		glm::mat3 RGB2PCS;
		glm::mat3 PCS2RGB;
		glm::mat3 chromatic_adaptation;
		glm::vec3 output_gamma;
	public:
		ColorAwareImage(const std::string_view _path, glm::vec3 outputGamma = glm::vec3(2.4))
			: Texture(_path, GL_RGB32F), output_gamma(outputGamma)
		{
			if( refCount++==0 ) {
				colorAwareDisplayProg = new Program("color aware display program");
				colorAwareDisplayProg->attatch("tex_to_quad.vs").attatch("imhdr/shaders/rgb_to_pcs_to_display.fs").link();
			}
			/* read meta data Exif
			LibRaw raw;
			raw.open_file(path.c_str());
			raw.unpack();
			Log::get() << "< Meta data >" << Log::endl;
			Log::get() << "ISO : " << raw.imgdata.other.iso_speed << Log::endl;
			Log::get() << "Exposure Time : " << raw.imgdata.other.shutter << Log::endl;
			Log::get() << "Aperture : " << raw.imgdata.other.aperture << Log::endl;
			Log::get() << "Focal Lenth : " << raw.imgdata.other.focal_len << Log::endl;
			Log::get() << "Black Level : " << raw.imgdata.color.black << Log::endl;
			Log::get() << "Max Value : " << raw.imgdata.color.maximum << Log::endl;
			Log::get() << "RAW bit: " << raw.imgdata.color.raw_bps << Log::endl;
			*/

			// is JPEG
			if( (strcmp(format, "jpg")==0||strcmp(format, "jpeg")==0) && profile.initWithJPEG(path, true) ) {
	
			} 
			// todo png
			else { // default : srgb
				profile.name = "sRGB";
				profile.whtPt = ICC::xyYToXYZ({0.31271f, 0.32902f,1.0f});
				profile.r ={0.436066, 0.222488, 0.013916}; //{ 0.4124565, 0.2126729, 0.0193339 };
				profile.g ={0.385147, 0.716873, 0.097076}; //{ 0.3575761, 0.7151522, 0.1191920 };
				profile.b ={0.143066, 0.060608, 0.714096}; //{ 0.1804375, 0.0721750, 0.9503041 };
				profile.gamma = glm::vec3(2.4);
			}

			RGB2PCS = profile.getRGB2XYZ();
			chromatic_adaptation = profile.chromaticAdaptationTo(ICC::WHTPT_D65);
			PCS2RGB = profile.getXYZ2RGB(ICC::sRGB_R_xy, ICC::sRGB_G_xy, ICC::sRGB_B_xy, ICC::WHTPT_D65);
		}
		virtual ~ColorAwareImage()
		{
			if( --refCount==0 ) {
				delete colorAwareDisplayProg;
			}
		}
		void toFramebuffer(const Framebuffer& fb)
		{
			fb.bind();

			GLuint pid = colorAwareDisplayProg->use();

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, tex_id);

			setUniform(pid, "tex", 0);
			setUniform(pid, "nrChannels", nr_channels);
			
			setUniform(pid, "inputGamma", profile.gamma);
			setUniform(pid, "outputGamma", output_gamma);

			setUniform(pid, "RGB2PCS", RGB2PCS);
			setUniform(pid, "chromaticAdaptation", chromatic_adaptation);
			setUniform(pid, "PCS2RGB", PCS2RGB);

			glBindVertexArray(AssetLib::get().quad_vao);
			glDrawArrays(GL_TRIANGLES, 0, 6);

			fb.unbind();
		}
	};
}

#endif