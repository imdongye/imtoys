//
//  SCMS.h
//  JGLTest
//
//  Created by Hyun Joon Shin on 2022/07/25.
//

#ifndef ICC_H
#define ICC_H

namespace ICC
{
	const glm::vec2 sRGB_R_xy         ={0.6400f,0.3300f};
	const glm::vec2 sRGB_G_xy         ={0.3000f,0.6000f};
	const glm::vec2 sRGB_B_xy         ={0.1500f,0.0600f};

	const glm::vec2 DisplayP3_R_xy    ={0.680f, 0.320f};
	const glm::vec2 DisplayP3_G_xy    ={0.265f, 0.690f};
	const glm::vec2 DisplayP3_B_xy    ={0.150f, 0.060f};

	const glm::vec2 AdobeRGB_R_xy     ={0.6400f,0.3300f};
	const glm::vec2 AdobeRGB_G_xy     ={0.2100f,0.7100f};
	const glm::vec2 AdobeRGB_B_xy     ={0.1500f,0.0600f};

	const glm::vec2 ProPhoto_R_xy     ={0.734699f,0.265301f};
	const glm::vec2 ProPhoto_G_xy     ={0.159597f,0.840403f};
	const glm::vec2 ProPhoto_B_xy     ={0.036598f,0.000105f};

	inline glm::vec3 XYZToxyY(float X, float Y, float Z) { return glm::vec3(X/(X+Y+Z), Y/(X+Y+Z), Y); }
	inline glm::vec3 XYZToxyY(const glm::vec3& v) { return XYZToxyY(v.x, v.y, v.z); }
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
		void initWithJPEG(std::string_view path, bool verbose=false)
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
					return;
				}
			}
			{ // get icc size and copy data
				uint8_t C1=fgetc(file), C2=fgetc(file);
				iccSize = C1<<8|C2;
				printf("iccSize :  %d\n", iccSize);

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
						if( verbose ) print_("r<xyY>: %C\n", XYZToxyY(r));
					}
					if( tagSig.compare("gXYZ") ) {
						g = getXYZ(data);
						if( verbose ) print_("g<xyY>: %C\n", g);
					}
					if( tagSig.compare("bXYZ") ) {
						b = getXYZ(data);
						if( verbose ) print_("b<xyY>: %C\n", XYZToxyY(b));
					}
				}
				whtPt = header.illuminantXYZ; 
			}
			{ // init profile name
				glm::vec2 xyR = XYZToxyY(r).xy();
				glm::vec2 xyG = XYZToxyY(g).xy();
				glm::vec2 xyB = XYZToxyY(b).xy();
				
				float min;
				float error = 0;
				error += glm::length(sRGB_R_xy-xyR);
				error += glm::length(sRGB_G_xy-xyG);
				error += glm::length(sRGB_B_xy-xyB);
				min=error;
				name = "sRGB";

				error = 0;
				error += glm::length(DisplayP3_R_xy-xyR);
				error += glm::length(DisplayP3_G_xy-xyG);
				error += glm::length(DisplayP3_B_xy-xyB);
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
		}
	};
}

#endif /* ICC_H */