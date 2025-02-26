#pragma once
#include <string>
#include "pch.h"
#include "framework.h"
class CTool
{
public:
	static void Dump(BYTE* pData, size_t nSize) {
		std::string strOut;
		for (size_t i = 0; i < nSize; i++) {
			char buf[8] = "";
			if (i > 0 && (i % 16 == 0)) strOut += "\n";
			snprintf(buf, sizeof(buf), "%02X ", pData[i] & 0xFF);
			strOut += buf;
		}
		strOut += "\n";
		//std::cout << strOut.c_str() << std::endl;
		OutputDebugStringA(strOut.c_str());
	}
};

