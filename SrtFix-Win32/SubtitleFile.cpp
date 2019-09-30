#include "stdafx.h"
#include "SubtitleFile.h"

SubtitleFile::SubtitleFile(std::wstring file_fullpath)
{
}


SubtitleFile::~SubtitleFile()
{
}

///analize for ascii,utf8,utf16
SubtitleFile::Encoding SubtitleFile::GetEncoding() {

	char header[4]; //no usar wchar asi leo de a un byte, lo mismo con ifstream en vez de wifstream
	std::ifstream file(file_fullpath, std::ios::in | std::ios::binary); //@@Can this read unicode filenames??
	if (!file) {
		return SubtitleFile::Encoding::UTF8;//parcheo rapido de momento
	}
	else {
		file.read(header, 3);
		file.close();

		if (header[0] == (char)0XEF && header[1] == (char)0XBB && header[2] == (char)0XBF)	
			return SubtitleFile::Encoding::UTF8;
		else if (header[0] == (char)0XEF && header[1] == (char)0XBB)							
			return SubtitleFile::Encoding::UTF8;
		else if (header[0] == (char)0xFF && header[1] == (char)0xFE)							
			return SubtitleFile::Encoding::UTF16;//little-endian
		else if (header[0] == (char)0xFE && header[1] == (char)0xFF)							
			return SubtitleFile::Encoding::UTF16;//big-ending
		else {//do manual checking
			std::wifstream file(file_fullpath, std::ios::in | std::ios::binary);//@@no se si wifstream o ifstream (afecta a stringstream)
			std::wstringstream buffer;
			buffer << file.rdbuf();
			file.close();
			buffer.seekg(0, std::ios::end);
			int length = buffer.tellg();//long enough length
			buffer.clear();
			buffer.seekg(0, std::ios::beg);
			//https://unicodebook.readthedocs.io/guess_encoding.html
			//NOTE: creo q esto testea UTF16 asi que probablemente haya errores, y toma ascii como utf
			int test;
			test = IS_TEXT_UNICODE_ILLEGAL_CHARS;
			if (IsTextUnicode(buffer.str().c_str(), length, &test)) return SubtitleFile::Encoding::ANSI;
			test = IS_TEXT_UNICODE_STATISTICS;
			if (IsTextUnicode(buffer.str().c_str(), length, &test)) return SubtitleFile::Encoding::UTF8;
			test = IS_TEXT_UNICODE_REVERSE_STATISTICS;
			if (IsTextUnicode(buffer.str().c_str(), length, &test)) return SubtitleFile::Encoding::UTF8;
			test = IS_TEXT_UNICODE_ASCII16;
			if (IsTextUnicode(buffer.str().c_str(), length, &test)) return SubtitleFile::Encoding::UTF8;
			return SubtitleFile::Encoding::ANSI;
		}
	}
}

///Every line separation(\r or \n or \r\n) will be transformed into \r\n
void SubtitleFile::ApplyProperLineSeparation() {
	unsigned int pos = 0;

	while (pos <= text.length()) {//length doesnt include \0
		if (text[pos] == L'\r') {
			if (text[pos + 1] != L'\n') {
				text.insert(pos + 1, L"\n");//este nunca se pasa del rango
			}
			pos += 2;
		}
		else if (text[pos] == L'\n') {
			//if(text[pos-1] == L'\r')//ya se que no tiene \r xq sino hubiera entrado por el otro
			text.insert(pos, L"\r");
			pos += 2;
		}
		else {
			pos++;
		}
	}
}